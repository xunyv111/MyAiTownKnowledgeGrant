#include "KnowledgeSemanticReviewComponent.h"

#include "GameFramework/Actor.h"
#include "KnowledgeContextCollectorComponent.h"
#include "KnowledgeGrantComponent.h"
#include "KnowledgeResolverLibrary.h"

namespace
{
bool NameSetsEqual(const TArray<FName>& A, const TArray<FName>& B)
{
    if (A.Num() != B.Num()) return false;
    TSet<FName> ASet;
    TSet<FName> BSet;
    for (const FName Value : A) ASet.Add(Value);
    for (const FName Value : B) BSet.Add(Value);
    if (ASet.Num() != A.Num() || BSet.Num() != B.Num()) return false;
    for (const FName Value : ASet) if (!BSet.Contains(Value)) return false;
    return true;
}
}

UKnowledgeSemanticReviewComponent::UKnowledgeSemanticReviewComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

FKnowledgeSemanticReviewBeginResult UKnowledgeSemanticReviewComponent::BeginSemanticReview(
    const UKnowledgeGrantComponent* PlayerKnowledge,
    const FDialogueKnowledgeView& KnowledgeView,
    const TArray<UKnowledgeContextCollectorComponent*>& ContextCollectors,
    const FKnowledgeDialogueGuardResult& MechanicalResult,
    const UKnowledgeSemanticReviewPolicy* Policy)
{
    FKnowledgeSemanticReviewBeginResult Begin;
    if (!HasAuthorityForMutation())
    {
        Begin.Error = TEXT("authority_required");
        return Begin;
    }
    if (!IsValid(Policy) || Policy->PolicyVersion.IsNone()
        || Policy->TimeoutSeconds < 0.1f || Policy->TimeoutSeconds > 60.0f)
    {
        Begin.Error = TEXT("semantic_review_policy_invalid");
        return Begin;
    }
    if (!MechanicalResult.bOk || MechanicalResult.SafeText.TrimStartAndEnd().IsEmpty()
        || MechanicalResult.AllowedClaimIds.IsEmpty()
        || MechanicalResult.ValidatedScopeFingerprint != KnowledgeView.ScopeFingerprint
        || (MechanicalResult.Outcome != EKnowledgeDialogueGuardOutcome::Allowed
            && MechanicalResult.Outcome != EKnowledgeDialogueGuardOutcome::PartiallyAllowed))
    {
        Begin.Error = TEXT("semantic_review_mechanical_result_invalid");
        return Begin;
    }

    FGameplayTagContainer LiveTags;
    TArray<TWeakObjectPtr<UKnowledgeContextCollectorComponent>> WeakCollectors;
    for (UKnowledgeContextCollectorComponent* Collector : ContextCollectors)
    {
        if (!IsValid(Collector)) continue;
        WeakCollectors.Add(Collector);
        LiveTags.AppendTags(Collector->GetActiveContextTags());
    }
    if (WeakCollectors.IsEmpty())
    {
        Begin.Error = TEXT("context_collector_missing");
        return Begin;
    }
    FString ViewError;
    if (!UKnowledgeResolverLibrary::ValidateDialogueKnowledgeView(
        PlayerKnowledge, KnowledgeView, LiveTags, ViewError))
    {
        Begin.Error = ViewError;
        return Begin;
    }

    FKnowledgeSemanticReviewRequest Request;
    Request.RequestId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
    Request.PlayerRevision = KnowledgeView.PlayerRevision;
    Request.AuthorizationEpoch = KnowledgeView.AuthorizationEpoch;
    Request.ScopeFingerprint = KnowledgeView.ScopeFingerprint;
    Request.PolicyVersion = Policy->PolicyVersion;
    Request.ExpiresAtUtc = FDateTime::UtcNow() + FTimespan::FromSeconds(Policy->TimeoutSeconds);
    Request.MechanicallyValidatedText = MechanicalResult.SafeText;
    Request.AllowedClaimIds = MechanicalResult.AllowedClaimIds;
    for (const FKnowledgeDocument& Document : KnowledgeView.AllowedDocuments)
    {
        Request.AllowedFacts.Append(Document.Facts);
    }

    FPendingReview Pending;
    Pending.Request = Request;
    Pending.KnowledgeView = KnowledgeView;
    Pending.PlayerKnowledge = const_cast<UKnowledgeGrantComponent*>(PlayerKnowledge);
    Pending.ContextCollectors = MoveTemp(WeakCollectors);
    Pending.Policy = const_cast<UKnowledgeSemanticReviewPolicy*>(Policy);
    PendingReviews.Add(Request.RequestId, MoveTemp(Pending));

    Begin.bOk = true;
    Begin.Request = Request;
    OnSemanticReviewRequested.Broadcast(Request);
    return Begin;
}

FKnowledgeSemanticReviewResult UKnowledgeSemanticReviewComponent::CompleteSemanticReview(
    const FKnowledgeSemanticReviewResponse& Response)
{
    if (!HasAuthorityForMutation())
    {
        return TerminalFailure(Response.RequestId, TEXT("authority_required"), nullptr,
            EKnowledgeSemanticReviewOutcome::Invalid);
    }

    FPendingReview Pending;
    if (!PendingReviews.RemoveAndCopyValue(Response.RequestId, Pending))
    {
        return TerminalFailure(Response.RequestId, TEXT("semantic_review_request_unknown"), nullptr,
            EKnowledgeSemanticReviewOutcome::Invalid);
    }
    const UKnowledgeSemanticReviewPolicy* Policy = Pending.Policy.Get();
    FKnowledgeSemanticReviewResult Result;
    if (FDateTime::UtcNow() > Pending.Request.ExpiresAtUtc)
    {
        Result = TerminalFailure(Response.RequestId, TEXT("semantic_review_timeout"), Policy,
            EKnowledgeSemanticReviewOutcome::Error);
    }
    else if (Response.ReviewerName.TrimStartAndEnd().IsEmpty()
        || Response.ReviewerVersion.TrimStartAndEnd().IsEmpty())
    {
        Result = TerminalFailure(Response.RequestId, TEXT("semantic_review_identity_invalid"), Policy,
            EKnowledgeSemanticReviewOutcome::Error);
    }
    else
    {
        FGameplayTagContainer LiveTags;
        FString ViewError;
        if (!CollectLiveContext(Pending.ContextCollectors, LiveTags))
        {
            Result = TerminalFailure(Response.RequestId, TEXT("context_collector_missing"), Policy,
                EKnowledgeSemanticReviewOutcome::Invalid);
        }
        else if (!UKnowledgeResolverLibrary::ValidateDialogueKnowledgeView(
            Pending.PlayerKnowledge.Get(), Pending.KnowledgeView, LiveTags, ViewError))
        {
            Result = TerminalFailure(Response.RequestId, ViewError, Policy,
                EKnowledgeSemanticReviewOutcome::Invalid);
        }
        else if (Response.Decision == EKnowledgeSemanticReviewDecision::Approved)
        {
            if (!Response.UnsupportedClaimIds.IsEmpty()
                || !NameSetsEqual(Response.ReviewedClaimIds, Pending.Request.AllowedClaimIds))
            {
                Result = TerminalFailure(Response.RequestId, TEXT("semantic_review_coverage_invalid"), Policy,
                    EKnowledgeSemanticReviewOutcome::Error);
            }
            else
            {
                Result.bCompleted = true;
                Result.bApproved = true;
                Result.Outcome = EKnowledgeSemanticReviewOutcome::Approved;
                Result.RequestId = Response.RequestId;
                Result.FinalApprovedText = Pending.Request.MechanicallyValidatedText;
            }
        }
        else if (Response.Decision == EKnowledgeSemanticReviewDecision::Blocked)
        {
            Result = TerminalFailure(Response.RequestId, TEXT("semantic_review_blocked"), Policy,
                EKnowledgeSemanticReviewOutcome::Blocked);
        }
        else
        {
            Result = TerminalFailure(Response.RequestId, TEXT("semantic_review_provider_error"), Policy,
                EKnowledgeSemanticReviewOutcome::Error);
        }
    }

    OnSemanticReviewCompleted.Broadcast(Result);
    return Result;
}

void UKnowledgeSemanticReviewComponent::CancelAllPendingReviews()
{
    if (!HasAuthorityForMutation()) return;
    TArray<FPendingReview> PendingValues;
    PendingReviews.GenerateValueArray(PendingValues);
    PendingReviews.Reset();
    for (const FPendingReview& Pending : PendingValues)
    {
        OnSemanticReviewCompleted.Broadcast(TerminalFailure(
            Pending.Request.RequestId, TEXT("semantic_review_cancelled"), Pending.Policy.Get(),
            EKnowledgeSemanticReviewOutcome::Error));
    }
}

void UKnowledgeSemanticReviewComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!HasAuthorityForMutation() || PendingReviews.IsEmpty()) return;

    const FDateTime Now = FDateTime::UtcNow();
    TArray<FString> ExpiredIds;
    for (const TPair<FString, FPendingReview>& Pair : PendingReviews)
    {
        if (Now > Pair.Value.Request.ExpiresAtUtc) ExpiredIds.Add(Pair.Key);
    }
    for (const FString& RequestId : ExpiredIds)
    {
        FPendingReview Pending;
        if (!PendingReviews.RemoveAndCopyValue(RequestId, Pending)) continue;
        OnSemanticReviewCompleted.Broadcast(TerminalFailure(
            RequestId, TEXT("semantic_review_timeout"), Pending.Policy.Get(),
            EKnowledgeSemanticReviewOutcome::Error));
    }
}

bool UKnowledgeSemanticReviewComponent::HasAuthorityForMutation() const
{
    const AActor* Owner = GetOwner();
    return Owner != nullptr && Owner->HasAuthority();
}

bool UKnowledgeSemanticReviewComponent::CollectLiveContext(
    const TArray<TWeakObjectPtr<UKnowledgeContextCollectorComponent>>& Collectors,
    FGameplayTagContainer& OutTags)
{
    int32 ValidCount = 0;
    for (const TWeakObjectPtr<UKnowledgeContextCollectorComponent>& WeakCollector : Collectors)
    {
        const UKnowledgeContextCollectorComponent* Collector = WeakCollector.Get();
        if (!IsValid(Collector)) continue;
        OutTags.AppendTags(Collector->GetActiveContextTags());
        ++ValidCount;
    }
    return ValidCount > 0;
}

FKnowledgeSemanticReviewResult UKnowledgeSemanticReviewComponent::TerminalFailure(
    const FString& RequestId, const FString& Error,
    const UKnowledgeSemanticReviewPolicy* Policy,
    EKnowledgeSemanticReviewOutcome Outcome)
{
    FKnowledgeSemanticReviewResult Result;
    Result.bCompleted = true;
    Result.Outcome = Outcome;
    Result.RequestId = RequestId;
    Result.Error = Error;
    if (IsValid(Policy))
    {
        Result.FinalApprovedText = Outcome == EKnowledgeSemanticReviewOutcome::Blocked
            ? Policy->BlockedText.ToString() : Policy->ErrorText.ToString();
    }
    return Result;
}
