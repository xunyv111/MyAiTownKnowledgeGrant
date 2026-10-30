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
#include "Misc/SecureHash.h"

namespace
{
bool IsTransientHttpError(const FString& Error, const TCHAR* Prefix)
{
    if (!Error.StartsWith(Prefix)) return false;
    const int32 Status = FCString::Atoi(*Error.Mid(FCString::Strlen(Prefix)));
    return Status == 408 || Status == 409 || Status == 425 || Status == 429
        || (Status >= 500 && Status <= 599);
}

bool IsRetryableDialogueProviderError(const FString& Error)
{
    return Error == TEXT("provider_response_not_json")
        || Error == TEXT("provider_choices_invalid")
        || Error == TEXT("provider_content_missing")
        || Error == TEXT("provider_connection_failed")
        || IsTransientHttpError(Error, TEXT("provider_http_error_"));
}
}

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
        SemanticReviewComponent->OnSemanticReviewCompleted.RemoveDynamic(
            this, &UAiNpcDialogueComponent::HandleSemanticReviewCompleted);
    }
    PendingTurns.Reset();
    ClientRequests.Reset();
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
            Context += FString::Printf(
                TEXT("FactId=%s | AllowedEntities=[%s] | AllowedNumbers=[%s] | Negated=%s | Statement=%s\n"),
                *Fact.FactId.ToString(),
                *FString::Join(Fact.Entities, TEXT(", ")),
                *FString::Join(Fact.Numbers, TEXT(", ")),
                Fact.bNegated ? TEXT("true") : TEXT("false"),
                *Fact.Statement);
        }
    }
    return Context;
}

FGuid UAiNpcDialogueComponent::SendPlayerMessage(const FString& PlayerText, FString& OutError)
{
    return SendPlayerMessageWithClientRequestId(PlayerText, FGuid::NewGuid(), OutError);
}

FGuid UAiNpcDialogueComponent::SendPlayerMessageWithClientRequestId(
    const FString& PlayerText, FGuid ClientRequestId, FString& OutError)
{
    OutError.Reset();
    if (!ClientRequestId.IsValid()) { OutError = TEXT("client_request_id_invalid"); return FGuid(); }
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
    const FString NormalizedInput = PlayerText.TrimStartAndEnd();
    const FString InputHash = FMD5::HashAnsiString(*NormalizedInput);
    FGuid ExistingRequestId;
    if (TryResolveExistingClientRequest(
        ClientRequestId, InputHash, View, ExistingRequestId, OutError))
    {
        return OutError == TEXT("client_request_conflict") ? FGuid() : ExistingRequestId;
    }
    if (!PendingTurns.IsEmpty()) { OutError = TEXT("npc_request_pending"); return FGuid(); }
    const FString AuthorizedContext = BuildAuthorizedContext(View);
    const FGuid RequestId = Provider->RequestNpcDialogue(
        SessionId, NormalizedInput, AuthorizedContext, OutError);
    if (RequestId.IsValid())
    {
        FPendingTurn Pending;
        Pending.PlayerText = NormalizedInput;
        Pending.InputHash = InputHash;
        Pending.AuthorizedContext = AuthorizedContext;
        Pending.KnowledgeView = View;
        Pending.OriginalRequestId = RequestId;
        Pending.ClientRequestId = ClientRequestId;
        Pending.PersonaRevision = PersonaRevision;
        PendingTurns.Add(RequestId, MoveTemp(Pending));

        FClientRequestRecord Record;
        Record.InputHash = InputHash;
        Record.AuthorizationEpoch = View.AuthorizationEpoch;
        Record.ScopeFingerprint = View.ScopeFingerprint;
        Record.OriginalRequestId = RequestId;
        ClientRequests.Add(ClientRequestId, MoveTemp(Record));
    }
    return RequestId;
}

bool UAiNpcDialogueComponent::IsClientRequestPending(FGuid ClientRequestId) const
{
    const FClientRequestRecord* Record = ClientRequests.Find(ClientRequestId);
    return Record != nullptr && !Record->bCompleted;
}

bool UAiNpcDialogueComponent::TryGetClientRequestResult(
    FGuid ClientRequestId, FAiNpcFinalReply& OutReply) const
{
    const FClientRequestRecord* Record = ClientRequests.Find(ClientRequestId);
    if (Record == nullptr || !Record->bCompleted) return false;
    OutReply = Record->CompletedReply;
    OutReply.bDuplicateRequest = true;
    return true;
}

bool UAiNpcDialogueComponent::ApplyPersona(
    const FAiNpcPersona& NewPersona, bool bClearTranscript, FString& OutError)
{
    OutError.Reset();
    UAiNpcProviderSubsystem* Provider = GetProviderSubsystem();
    if (!Provider)
    {
        OutError = TEXT("provider_subsystem_missing");
        return false;
    }
    const FGuid NewSessionId = Provider->OpenNpcSession(NewPersona);
    if (!NewSessionId.IsValid())
    {
        OutError = TEXT("npc_persona_invalid");
        return false;
    }

    const FGuid OldSessionId = SessionId;
    SessionId = NewSessionId;
    Persona = NewPersona;
    ++PersonaRevision;
    PendingTurns.Reset();
    ClientRequests.Reset();
    SemanticToProviderRequest.Reset();
    if (SemanticReviewComponent) SemanticReviewComponent->CancelAllPendingReviews();
    if (OldSessionId.IsValid()) Provider->CloseNpcSession(OldSessionId);
    if (bClearTranscript) LocalTranscript.Reset();
    return true;
}

void UAiNpcDialogueComponent::BroadcastFallback(
    FGuid RequestId, const FString& Error, const FText& Fallback, const FPendingTurn* Turn)
{
    FAiNpcFinalReply Reply;
    Reply.bCompleted = true;
    Reply.bApproved = false;
    Reply.bUsedTrustedFallback = true;
    Reply.RequestId = RequestId;
    Reply.FinalApprovedText = BuildDeterministicAuthorizedFallback(
        Turn, Fallback, Reply.FallbackEvidenceFactIds);
    Reply.Error = Error;
    StampReplyFromTurn(Reply, Turn);
    CacheCompletedReply(Turn, Reply);
    OnFinalReply.Broadcast(Reply);
}

FString UAiNpcDialogueComponent::BuildDeterministicAuthorizedFallback(
    const FPendingTurn* Turn, const FText& DefaultFallback,
    TArray<FName>& OutEvidenceFactIds) const
{
    OutEvidenceFactIds.Reset();
    if (Turn == nullptr) return DefaultFallback.ToString();
    FString ValidationError;
    if (!IsPendingTurnCurrent(*Turn, ValidationError)) return DefaultFallback.ToString();
    return BuildAuthorizedFactFallback(
        Turn->PlayerText, Turn->KnowledgeView, DefaultFallback, OutEvidenceFactIds);
}

FString UAiNpcDialogueComponent::BuildAuthorizedFactFallback(
    const FString& PlayerText, const FDialogueKnowledgeView& KnowledgeView,
    const FText& DefaultFallback, TArray<FName>& OutEvidenceFactIds)
{
    OutEvidenceFactIds.Reset();
    const FKnowledgeFact* Selected = nullptr;
    int32 SelectedScore = INDEX_NONE;
    for (const FKnowledgeDocument& Document : KnowledgeView.AllowedDocuments)
    {
        for (const FKnowledgeFact& Fact : Document.Facts)
        {
            if (Fact.FactId.IsNone() || Fact.Statement.TrimStartAndEnd().IsEmpty()) continue;
            int32 Score = 0;
            for (const FString& Entity : Fact.Entities)
                if (!Entity.IsEmpty() && PlayerText.Contains(Entity, ESearchCase::IgnoreCase)) ++Score;
            if (Selected == nullptr || Score > SelectedScore
                || (Score == SelectedScore
                    && Fact.FactId.ToString() < Selected->FactId.ToString()))
            {
                Selected = &Fact;
                SelectedScore = Score;
            }
        }
    }
    if (Selected == nullptr) return DefaultFallback.ToString();

    OutEvidenceFactIds.Add(Selected->FactId);
    return Selected->Statement.TrimStartAndEnd()
        + TEXT("\n如果你愿意，可以把看到的具体痕迹或方向告诉我。");
}

void UAiNpcDialogueComponent::StampReplyFromTurn(
    FAiNpcFinalReply& Reply, const FPendingTurn* Turn) const
{
    if (Turn == nullptr) return;
    Reply.ClientRequestId = Turn->ClientRequestId;
    Reply.bProviderRetryAttempted = Turn->bProviderRetryAttempted;
    Reply.bFormatRepairAttempted = Turn->bFormatRepairAttempted;
    Reply.AuthorizationEpoch = Turn->KnowledgeView.AuthorizationEpoch;
    Reply.ScopeFingerprint = Turn->KnowledgeView.ScopeFingerprint;
}

void UAiNpcDialogueComponent::CacheCompletedReply(
    const FPendingTurn* Turn, const FAiNpcFinalReply& Reply)
{
    if (Turn == nullptr || !Turn->ClientRequestId.IsValid()) return;
    FClientRequestRecord* Record = ClientRequests.Find(Turn->ClientRequestId);
    if (Record == nullptr) return;
    Record->bCompleted = true;
    Record->CompletedReply = Reply;
}

bool UAiNpcDialogueComponent::TryResolveExistingClientRequest(
    FGuid ClientRequestId, const FString& InputHash,
    const FDialogueKnowledgeView& View, FGuid& OutRequestId, FString& OutError) const
{
    OutRequestId.Invalidate();
    const FClientRequestRecord* Existing = ClientRequests.Find(ClientRequestId);
    if (Existing == nullptr) return false;

    if (Existing->InputHash != InputHash
        || Existing->AuthorizationEpoch != View.AuthorizationEpoch
        || Existing->ScopeFingerprint != View.ScopeFingerprint)
    {
        OutError = TEXT("client_request_conflict");
        return true;
    }

    OutRequestId = Existing->OriginalRequestId;
    OutError = Existing->bCompleted
        ? TEXT("request_duplicate_completed") : TEXT("request_duplicate_pending");
    return true;
}

bool UAiNpcDialogueComponent::IsPendingTurnCurrent(
    const FPendingTurn& Turn, FString& OutError) const
{
    if (Turn.PersonaRevision != PersonaRevision)
    {
        OutError = TEXT("dialogue_persona_changed");
        return false;
    }

    FGameplayTagContainer ActiveTags;
    for (const UKnowledgeContextCollectorComponent* Collector : ContextCollectors)
    {
        if (IsValid(Collector)) ActiveTags.AppendTags(Collector->GetActiveContextTags());
    }
    return UKnowledgeResolverLibrary::ValidateDialogueKnowledgeView(
        PlayerKnowledge, Turn.KnowledgeView, ActiveTags, OutError);
}

bool UAiNpcDialogueComponent::CorrelateSemanticRequest(
    const FString& ReviewRequestId, FGuid DialogueRequestId)
{
    if (ReviewRequestId.IsEmpty() || !DialogueRequestId.IsValid()
        || SemanticToProviderRequest.Contains(ReviewRequestId))
    {
        return false;
    }
    SemanticToProviderRequest.Add(ReviewRequestId, DialogueRequestId);
    return true;
}

void UAiNpcDialogueComponent::DiscardStaleTurn(
    FGuid ProviderRequestId, const FPendingTurn& Turn, const FString& Error)
{
    FAiNpcFinalReply Reply;
    Reply.bCompleted = true;
    Reply.RequestId = Turn.OriginalRequestId.IsValid()
        ? Turn.OriginalRequestId : ProviderRequestId;
    Reply.Error = Error.IsEmpty() ? TEXT("dialogue_request_stale") : Error;
    StampReplyFromTurn(Reply, &Turn);
    CacheCompletedReply(&Turn, Reply);
    PendingTurns.Remove(ProviderRequestId);
    OnFinalReply.Broadcast(Reply);
}

void UAiNpcDialogueComponent::HandleProviderCompleted(FAiNpcProviderResult Result)
{
    FPendingTurn* Turn = PendingTurns.Find(Result.RequestId);
    if (Turn == nullptr || Result.SessionId != SessionId) return;
    FString StaleError;
    if (!IsPendingTurnCurrent(*Turn, StaleError))
    {
        const FPendingTurn StableTurn = *Turn;
        DiscardStaleTurn(Result.RequestId, StableTurn, StaleError);
        return;
    }
    const FGuid OriginalRequestId = Turn->OriginalRequestId.IsValid()
        ? Turn->OriginalRequestId : Result.RequestId;
    if (!Result.bSuccess)
    {
        const bool bRetryableProviderFailure = IsRetryableDialogueProviderError(Result.Error);
        if (!Turn->bProviderRetryAttempted && bRetryableProviderFailure)
        {
            FString RetryError;
            UAiNpcProviderSubsystem* Provider = GetProviderSubsystem();
            const FGuid RetryRequestId = Provider
                ? Provider->RequestNpcDialogue(
                    SessionId, Turn->PlayerText, Turn->AuthorizedContext, RetryError)
                : FGuid();
            if (RetryRequestId.IsValid())
            {
                FPendingTurn RetryTurn = *Turn;
                RetryTurn.OriginalRequestId = OriginalRequestId;
                RetryTurn.bProviderRetryAttempted = true;
                PendingTurns.Remove(Result.RequestId);
                PendingTurns.Add(RetryRequestId, MoveTemp(RetryTurn));
                return;
            }
        }
        const FPendingTurn StableTurn = *Turn;
        PendingTurns.Remove(Result.RequestId);
        BroadcastFallback(OriginalRequestId, Result.Error,
            SemanticReviewPolicy ? SemanticReviewPolicy->ErrorText : FText::GetEmpty(), &StableTurn);
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
        const bool bRepairableGuardFailure = Guarded.Error.StartsWith(TEXT("model_json_"))
            || Guarded.Error.StartsWith(TEXT("dialogue_claim_"));
        if (!Turn->bFormatRepairAttempted && bRepairableGuardFailure)
        {
            FString RepairError;
            UAiNpcProviderSubsystem* Provider = GetProviderSubsystem();
            const FString RepairCandidate = Result.RawProviderPayload.Left(28000);
            const FString RepairPayload = FString::Printf(
                TEXT("VALIDATION ERROR: %s\nCANDIDATE JSON%s:\n%s"),
                *Guarded.Error,
                RepairCandidate.Len() < Result.RawProviderPayload.Len() ? TEXT(" (TRUNCATED)") : TEXT(""),
                *RepairCandidate);
            const FGuid RepairRequestId = Provider
                ? Provider->RequestNpcDialogueFormatRepair(
                    SessionId, RepairPayload, Turn->AuthorizedContext, RepairError)
                : FGuid();
            if (RepairRequestId.IsValid())
            {
                FPendingTurn RepairTurn = *Turn;
                RepairTurn.bFormatRepairAttempted = true;
                PendingTurns.Remove(Result.RequestId);
                PendingTurns.Add(RepairRequestId, MoveTemp(RepairTurn));
                return;
            }
        }
        FAiNpcFinalReply Reply;
        Reply.bCompleted = true;
        Reply.bUsedTrustedFallback = true;
        Reply.RequestId = OriginalRequestId;
        Reply.FinalApprovedText = Guarded.SafeText;
        Reply.Error = Guarded.Error;
        StampReplyFromTurn(Reply, Turn);
        CacheCompletedReply(Turn, Reply);
        OnFinalReply.Broadcast(Reply);
        PendingTurns.Remove(Result.RequestId);
        return;
    }
    if (!IsValid(SemanticReviewComponent) || !IsValid(SemanticReviewPolicy))
    {
        const FPendingTurn StableTurn = *Turn;
        PendingTurns.Remove(Result.RequestId);
        BroadcastFallback(OriginalRequestId, TEXT("semantic_review_not_configured"),
            SemanticReviewPolicy ? SemanticReviewPolicy->ErrorText : FText::GetEmpty(), &StableTurn);
        return;
    }

    if (Result.RequestId != OriginalRequestId)
    {
        const FPendingTurn StableTurn = *Turn;
        PendingTurns.Remove(Result.RequestId);
        PendingTurns.Add(OriginalRequestId, StableTurn);
        Turn = PendingTurns.Find(OriginalRequestId);
    }
    const FKnowledgeSemanticReviewBeginResult Begin = SemanticReviewComponent->BeginSemanticReview(
        PlayerKnowledge, Turn->KnowledgeView, Collectors, Guarded, SemanticReviewPolicy);
    if (!Begin.bOk)
    {
        const FPendingTurn StableTurn = *Turn;
        PendingTurns.Remove(OriginalRequestId);
        BroadcastFallback(OriginalRequestId, Begin.Error, SemanticReviewPolicy->ErrorText, &StableTurn);
        return;
    }

    // Correlate at the call site. A shared review component may synchronously broadcast
    // requests to several dialogue components, so no transient member can identify the owner.
    if (!CorrelateSemanticRequest(Begin.Request.RequestId, OriginalRequestId))
    {
        const FPendingTurn StableTurn = *Turn;
        PendingTurns.Remove(OriginalRequestId);
        BroadcastFallback(OriginalRequestId, TEXT("semantic_request_correlation_failed"),
            SemanticReviewPolicy->ErrorText, &StableTurn);
        return;
    }
    FString Error;
    UAiNpcProviderSubsystem* Provider = GetProviderSubsystem();
    if (!Provider || !Provider->RequestSemanticReview(Begin.Request, Error))
    {
        FKnowledgeSemanticReviewResponse Failure;
        Failure.RequestId = Begin.Request.RequestId;
        Failure.ReviewerName = TEXT("local-provider-gateway");
        Failure.ReviewerVersion = TEXT("unavailable");
        Failure.Error = Error.IsEmpty() ? TEXT("review_provider_not_available") : Error;
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
    if (Turn != nullptr)
    {
        FString StaleError;
        if (!IsPendingTurnCurrent(*Turn, StaleError))
        {
            const FPendingTurn StableTurn = *Turn;
            SemanticToProviderRequest.Remove(Result.RequestId);
            DiscardStaleTurn(RequestId, StableTurn, StaleError);
            return;
        }
    }
    FAiNpcFinalReply Reply;
    Reply.bCompleted = Result.bCompleted;
    Reply.bApproved = Result.bApproved;
    Reply.bUsedTrustedFallback = !Result.bApproved;
    Reply.RequestId = RequestId;
    Reply.FinalApprovedText = Result.bApproved
        ? Result.FinalApprovedText
        : BuildDeterministicAuthorizedFallback(
            Turn, SemanticReviewPolicy ? SemanticReviewPolicy->BlockedText : FText::GetEmpty(),
            Reply.FallbackEvidenceFactIds);
    Reply.RejectedCandidateText = Result.RejectedCandidateText;
    Reply.SemanticClaimAssessments = Result.ClaimAssessments;
    Reply.SemanticReviewedClaims = Result.ReviewedClaims;
    Reply.SemanticReviewRequestId = Result.RequestId;
    Reply.ReviewerName = Result.ReviewerName;
    Reply.ReviewerVersion = Result.ReviewerVersion;
    Reply.ReviewPolicyVersion = Result.PolicyVersion;
    Reply.AuthorizationEpoch = Result.AuthorizationEpoch;
    Reply.ScopeFingerprint = Result.ScopeFingerprint;
    Reply.ReviewDurationMs = Result.ReviewDurationMs;
    Reply.Error = Result.Error;
    StampReplyFromTurn(Reply, Turn);
    if (Result.bCompleted && Result.bApproved && Turn && !Result.FinalApprovedText.IsEmpty())
        StoreApprovedTurn(Turn->PlayerText, Result.FinalApprovedText);
    CacheCompletedReply(Turn, Reply);
    PendingTurns.Remove(RequestId);
    SemanticToProviderRequest.Remove(Result.RequestId);
    OnFinalReply.Broadcast(Reply);
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
