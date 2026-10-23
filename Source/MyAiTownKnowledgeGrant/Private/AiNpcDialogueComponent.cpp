#include "AiNpcDialogueComponent.h"

#include "AiNpcProviderSubsystem.h"
#include "KnowledgeContextCollectorComponent.h"
#include "KnowledgeDialogueGuardLibrary.h"
#include "KnowledgeGrantComponent.h"
#include "KnowledgeModelAdapterLibrary.h"
#include "KnowledgePack.h"
#include "KnowledgeSemanticReviewComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

void UAiNpcDialogueComponent::BeginPlay()
{
    Super::BeginPlay();
    if (Persona.NpcId.IsNone() && GetOwner()) Persona.NpcId = FName(*GetOwner()->GetName());
    if (Persona.DisplayName.IsEmpty() && GetOwner()) Persona.DisplayName = GetOwner()->GetName();
    if (UAiNpcProviderSubsystem* Provider = GetProviderSubsystem())
    {
        SessionId = Provider->OpenNpcSession(Persona);
        Provider->OnDialogueProviderCompleted.AddDynamic(this, &UAiNpcDialogueComponent::HandleProviderCompleted);
        Provider->OnReviewProviderCompleted.AddDynamic(this, &UAiNpcDialogueComponent::HandleReviewProviderCompleted);
    }
    if (SemanticReviewComponent)
    {
        SemanticReviewComponent->OnSemanticReviewRequested.AddDynamic(
            this, &UAiNpcDialogueComponent::HandleSemanticReviewRequested);
        SemanticReviewComponent->OnSemanticReviewCompleted.AddDynamic(
            this, &UAiNpcDialogueComponent::HandleSemanticReviewCompleted);
    }
}

void UAiNpcDialogueComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UAiNpcProviderSubsystem* Provider = GetProviderSubsystem())
    {
        Provider->OnDialogueProviderCompleted.RemoveDynamic(this, &UAiNpcDialogueComponent::HandleProviderCompleted);
        Provider->OnReviewProviderCompleted.RemoveDynamic(this, &UAiNpcDialogueComponent::HandleReviewProviderCompleted);
        Provider->CloseNpcSession(SessionId);
    }
    if (SemanticReviewComponent)
    {
        SemanticReviewComponent->OnSemanticReviewRequested.RemoveDynamic(
            this, &UAiNpcDialogueComponent::HandleSemanticReviewRequested);
        SemanticReviewComponent->OnSemanticReviewCompleted.RemoveDynamic(
            this, &UAiNpcDialogueComponent::HandleSemanticReviewCompleted);
    }
    PendingTurns.Reset();
    SemanticToProviderRequest.Reset();
    Super::EndPlay(EndPlayReason);
}

UAiNpcProviderSubsystem* UAiNpcDialogueComponent::GetProviderSubsystem() const
{
    const UWorld* World = GetWorld();
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UAiNpcProviderSubsystem>() : nullptr;
}

FString UAiNpcDialogueComponent::BuildAuthorizedContext(const FDialogueKnowledgeView& View) const
{
    FString Context;
    for (const FKnowledgeDocument& Document : View.AllowedDocuments)
    {
        for (const FKnowledgeFact& Fact : Document.Facts)
        {
            Context += FString::Printf(TEXT("FactId=%s | %s\n"), *Fact.FactId.ToString(), *Fact.Statement);
        }
    }
    return Context;
}

FGuid UAiNpcDialogueComponent::SendPlayerMessage(const FString& PlayerText, FString& OutError)
{
    OutError.Reset();
    UAiNpcProviderSubsystem* Provider = GetProviderSubsystem();
    if (!Provider || !SessionId.IsValid()) { OutError = TEXT("npc_session_not_ready"); return FGuid(); }
    if (!IsValid(PlayerKnowledge)) { OutError = TEXT("player_knowledge_missing"); return FGuid(); }
    TArray<UKnowledgeContextCollectorComponent*> Collectors;
    FGameplayTagContainer ActiveTags;
    for (UKnowledgeContextCollectorComponent* Collector : ContextCollectors)
    {
        if (!IsValid(Collector)) continue;
        Collectors.Add(Collector);
        ActiveTags.AppendTags(Collector->GetActiveContextTags());
    }
    if (Collectors.IsEmpty()) { OutError = TEXT("context_collector_missing"); return FGuid(); }
    TArray<UKnowledgePack*> Packs;
    for (UKnowledgePack* Pack : CandidatePacks) if (IsValid(Pack)) Packs.Add(Pack);
    const FDialogueKnowledgeView View = UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(
        PlayerKnowledge, Packs, ActiveTags);
    if (!View.bOk) { OutError = View.Error; return FGuid(); }
    const FGuid RequestId = Provider->RequestNpcDialogue(
        SessionId, PlayerText, BuildAuthorizedContext(View), OutError);
    if (RequestId.IsValid()) PendingTurns.Add(RequestId, FPendingTurn{PlayerText, View});
    return RequestId;
}

void UAiNpcDialogueComponent::BroadcastFallback(
    FGuid RequestId, const FString& Error, const FText& Fallback)
{
    FAiNpcFinalReply Reply;
    Reply.bCompleted = true;
    Reply.bApproved = false;
    Reply.bUsedTrustedFallback = true;
    Reply.RequestId = RequestId;
    Reply.FinalApprovedText = Fallback.ToString();
    Reply.Error = Error;
    OnFinalReply.Broadcast(Reply);
}

void UAiNpcDialogueComponent::HandleProviderCompleted(FAiNpcProviderResult Result)
{
    FPendingTurn* Turn = PendingTurns.Find(Result.RequestId);
    if (Turn == nullptr || Result.SessionId != SessionId) return;
    if (!Result.bSuccess)
    {
        BroadcastFallback(Result.RequestId, Result.Error,
            SemanticReviewPolicy ? SemanticReviewPolicy->ErrorText : FText::GetEmpty());
        PendingTurns.Remove(Result.RequestId);
        return;
    }

    TArray<UKnowledgeContextCollectorComponent*> Collectors;
    for (UKnowledgeContextCollectorComponent* Collector : ContextCollectors)
        if (IsValid(Collector)) Collectors.Add(Collector);
    const FKnowledgeDialogueGuardResult Guarded =
        UKnowledgeModelAdapterLibrary::GuardKnowledgeModelJsonResponseForCollectors(
            PlayerKnowledge, Turn->KnowledgeView, Collectors,
            Result.RawProviderPayload, GuardPolicy);
    if (!Guarded.bOk)
    {
        FAiNpcFinalReply Reply;
        Reply.bCompleted = true;
        Reply.bUsedTrustedFallback = true;
        Reply.RequestId = Result.RequestId;
        Reply.FinalApprovedText = Guarded.SafeText;
        Reply.Error = Guarded.Error;
        OnFinalReply.Broadcast(Reply);
        PendingTurns.Remove(Result.RequestId);
        return;
    }
    if (!IsValid(SemanticReviewComponent) || !IsValid(SemanticReviewPolicy))
    {
        BroadcastFallback(Result.RequestId, TEXT("semantic_review_not_configured"),
            SemanticReviewPolicy ? SemanticReviewPolicy->ErrorText : FText::GetEmpty());
        PendingTurns.Remove(Result.RequestId);
        return;
    }

    StartingSemanticForProviderRequest = Result.RequestId;
    const FKnowledgeSemanticReviewBeginResult Begin = SemanticReviewComponent->BeginSemanticReview(
        PlayerKnowledge, Turn->KnowledgeView, Collectors, Guarded, SemanticReviewPolicy);
    StartingSemanticForProviderRequest.Invalidate();
    if (!Begin.bOk)
    {
        BroadcastFallback(Result.RequestId, Begin.Error, SemanticReviewPolicy->ErrorText);
        PendingTurns.Remove(Result.RequestId);
    }
}

void UAiNpcDialogueComponent::HandleSemanticReviewRequested(FKnowledgeSemanticReviewRequest Request)
{
    if (!StartingSemanticForProviderRequest.IsValid()) return;
    SemanticToProviderRequest.Add(Request.RequestId, StartingSemanticForProviderRequest);
    FString Error;
    UAiNpcProviderSubsystem* Provider = GetProviderSubsystem();
    if (!Provider || !Provider->RequestSemanticReview(Request, Error))
    {
        FKnowledgeSemanticReviewResponse Failure;
        Failure.RequestId = Request.RequestId;
        Failure.Decision = EKnowledgeSemanticReviewDecision::Error;
        Failure.ReviewerName = TEXT("local-provider-gateway");
        Failure.ReviewerVersion = TEXT("unavailable");
        Failure.ReasonCodes.Add(Error.IsEmpty() ? TEXT("review_provider_not_available") : Error);
        SemanticReviewComponent->CompleteSemanticReview(Failure);
    }
}

void UAiNpcDialogueComponent::HandleReviewProviderCompleted(FKnowledgeSemanticReviewResponse Response)
{
    if (!SemanticToProviderRequest.Contains(Response.RequestId) || !SemanticReviewComponent) return;
    SemanticReviewComponent->CompleteSemanticReview(Response);
}

void UAiNpcDialogueComponent::HandleSemanticReviewCompleted(FKnowledgeSemanticReviewResult Result)
{
    const FGuid* ProviderRequestId = SemanticToProviderRequest.Find(Result.RequestId);
    if (ProviderRequestId == nullptr) return;
    const FGuid RequestId = *ProviderRequestId;
    const FPendingTurn* Turn = PendingTurns.Find(RequestId);
    FAiNpcFinalReply Reply;
    Reply.bCompleted = Result.bCompleted;
    Reply.bApproved = Result.bApproved;
    Reply.bUsedTrustedFallback = !Result.bApproved;
    Reply.RequestId = RequestId;
    Reply.FinalApprovedText = Result.FinalApprovedText;
    Reply.Error = Result.Error;
    if (Result.bCompleted && Turn && !Result.FinalApprovedText.IsEmpty())
        StoreApprovedTurn(Turn->PlayerText, Result.FinalApprovedText);
    OnFinalReply.Broadcast(Reply);
    PendingTurns.Remove(RequestId);
    SemanticToProviderRequest.Remove(Result.RequestId);
}

void UAiNpcDialogueComponent::StoreApprovedTurn(const FString& PlayerText, const FString& NpcText)
{
    LocalTranscript.Add(FAiNpcLocalTurn{PlayerText, NpcText});
    const int32 Limit = FMath::Clamp(MaxLocalTranscriptTurns, 1, 200);
    if (LocalTranscript.Num() > Limit) LocalTranscript.RemoveAt(0, LocalTranscript.Num() - Limit);
}

void UAiNpcDialogueComponent::ClearLocalTranscript()
{
    LocalTranscript.Reset();
}
