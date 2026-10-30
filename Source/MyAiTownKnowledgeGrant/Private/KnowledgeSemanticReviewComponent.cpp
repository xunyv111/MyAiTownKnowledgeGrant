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

bool IsUnitScore(float Value)
{
    return FMath::IsFinite(Value) && Value >= 0.0f && Value <= 1.0f;
}

bool IsPolicyValid(const UKnowledgeSemanticReviewPolicy* Policy)
{
    if (!IsValid(Policy) || Policy->PolicyVersion.IsNone()
        || Policy->TimeoutSeconds < 0.1f || Policy->TimeoutSeconds > 60.0f) return false;
    const float Weights[] = {
        Policy->FactSupportQualityWeight,
        Policy->ContradictionSafetyQualityWeight,
        Policy->OverreachSafetyQualityWeight,
        Policy->TaskLeakSafetyQualityWeight,
        Policy->RoleFitQualityWeight};
    float Sum = 0.0f;
    for (const float Weight : Weights)
    {
        if (!FMath::IsFinite(Weight) || Weight < 0.0f || Weight > 1.0f) return false;
        Sum += Weight;
    }
    return Sum > UE_SMALL_NUMBER;
}

float ClaimQualityScore(const FKnowledgeSemanticClaimAssessment& Assessment,
    const UKnowledgeSemanticReviewPolicy* Policy)
{
    const float WeightSum = Policy->FactSupportQualityWeight
        + Policy->ContradictionSafetyQualityWeight
        + Policy->OverreachSafetyQualityWeight
        + Policy->TaskLeakSafetyQualityWeight
        + Policy->RoleFitQualityWeight;
    // Fact support is not applicable to a question, intention, reaction, or proposal.
    // Treat that quality dimension as neutral/full instead of penalizing safe roleplay.
    const float FactSupportQuality = Assessment.ClaimType == EKnowledgeClaimType::Factual
        ? Assessment.FactSupportScore : 1.0f;
    const float Weighted = FactSupportQuality * Policy->FactSupportQualityWeight
        + (1.0f - Assessment.ContradictionRiskScore) * Policy->ContradictionSafetyQualityWeight
        + (1.0f - Assessment.OverreachRiskScore) * Policy->OverreachSafetyQualityWeight
        + (1.0f - Assessment.TaskLeakRiskScore) * Policy->TaskLeakSafetyQualityWeight
        + Assessment.RoleFitScore * Policy->RoleFitQualityWeight;
    return Weighted / WeightSum;
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
    if (!IsPolicyValid(Policy))
    {
        Begin.Error = TEXT("semantic_review_policy_invalid");
        return Begin;
    }
    if (!MechanicalResult.bOk || MechanicalResult.SafeText.TrimStartAndEnd().IsEmpty()
        || MechanicalResult.AllowedClaimIds.IsEmpty()
        || MechanicalResult.AllowedClaims.IsEmpty()
        || !NameSetsEqual(MechanicalResult.AllowedClaimIds,
            [&MechanicalResult]()
            {
                TArray<FName> Ids;
                for (const FKnowledgeDialogueClaim& Claim : MechanicalResult.AllowedClaims)
                    Ids.Add(Claim.ClaimId);
                return Ids;
            }())
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
    Request.AllowedClaims = MechanicalResult.AllowedClaims;
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
    else if (!Response.Error.IsEmpty())
    {
        Result = TerminalFailure(Response.RequestId, Response.Error, Policy,
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
        else
        {
            TArray<FName> AssessmentIds;
            TSet<FName> AllowedFactIds;
            for (const FKnowledgeFact& Fact : Pending.Request.AllowedFacts)
                AllowedFactIds.Add(Fact.FactId);
            TMap<FName, const FKnowledgeDialogueClaim*> ClaimsById;
            for (const FKnowledgeDialogueClaim& Claim : Pending.Request.AllowedClaims)
                ClaimsById.Add(Claim.ClaimId, &Claim);

            bool bAssessmentInvalid = Response.ClaimAssessments.Num()
                != Pending.Request.AllowedClaimIds.Num();
            bool bGateBlocked = false;
            float WorstQuality = 1.0f;
            FName WorstClaimId = NAME_None;
            for (const FKnowledgeSemanticClaimAssessment& Assessment : Response.ClaimAssessments)
            {
                AssessmentIds.Add(Assessment.ClaimId);
                const FKnowledgeDialogueClaim* const* SourceClaim = ClaimsById.Find(Assessment.ClaimId);
                const bool bExpectedFactual = SourceClaim != nullptr
                    && !(*SourceClaim)->EvidenceFactIds.IsEmpty();
                const bool bClaimTypeMatches = SourceClaim != nullptr
                    && Assessment.ClaimType == (bExpectedFactual
                        ? EKnowledgeClaimType::Factual : EKnowledgeClaimType::NonFactual);
                const bool bScoresValid = IsUnitScore(Assessment.FactSupportScore)
                    && IsUnitScore(Assessment.ContradictionRiskScore)
                    && IsUnitScore(Assessment.OverreachRiskScore)
                    && IsUnitScore(Assessment.TaskLeakRiskScore)
                    && IsUnitScore(Assessment.RoleFitScore)
                    && IsUnitScore(Assessment.ConfidenceScore);
                TSet<FName> SourceEvidence;
                if (SourceClaim != nullptr)
                {
                    for (const FName FactId : (*SourceClaim)->EvidenceFactIds)
                        SourceEvidence.Add(FactId);
                }
                TSet<FName> SeenEvidence;
                bool bEvidenceValid = true;
                for (const FName FactId : Assessment.EvidenceFactIds)
                {
                    if (FactId.IsNone() || SeenEvidence.Contains(FactId)
                        || !AllowedFactIds.Contains(FactId)
                        || !SourceEvidence.Contains(FactId))
                    {
                        bEvidenceValid = false;
                        break;
                    }
                    SeenEvidence.Add(FactId);
                }
                if (Assessment.ClaimType == EKnowledgeClaimType::Factual
                    && (Assessment.EvidenceFactIds.IsEmpty() || SourceEvidence.IsEmpty()))
                    bEvidenceValid = false;
                if (Assessment.ClaimType == EKnowledgeClaimType::NonFactual
                    && !Assessment.EvidenceFactIds.IsEmpty())
                    bEvidenceValid = false;
                if (!bScoresValid || !bEvidenceValid || !bClaimTypeMatches)
                    bAssessmentInvalid = true;

                const bool bFactSupportPasses = !bExpectedFactual
                    || Assessment.Support == EKnowledgeClaimSupport::Supported;
                const bool bClaimPasses = bFactSupportPasses
                    && Assessment.Contradiction == EKnowledgeClaimRisk::None
                    && Assessment.Overreach == EKnowledgeClaimRisk::None
                    && Assessment.TaskLeak == EKnowledgeClaimRisk::None
                    && Assessment.RoleFit != EKnowledgeRoleFit::Bad
                    && (!Policy->bBlockWeakRoleFit || Assessment.RoleFit == EKnowledgeRoleFit::Good)
                    && Assessment.Confidence != EKnowledgeReviewConfidence::Low
                    && (!Policy->bBlockMediumConfidence
                        || Assessment.Confidence == EKnowledgeReviewConfidence::High);
                if (!bClaimPasses) bGateBlocked = true;

                if (bScoresValid)
                {
                    const float Quality = ClaimQualityScore(Assessment, Policy);
                    if (WorstClaimId.IsNone() || Quality < WorstQuality)
                    {
                        WorstQuality = Quality;
                        WorstClaimId = Assessment.ClaimId;
                    }
                }
            }
            if (!NameSetsEqual(AssessmentIds, Pending.Request.AllowedClaimIds))
                bAssessmentInvalid = true;

            if (bAssessmentInvalid)
            {
                Result = TerminalFailure(Response.RequestId,
                    TEXT("semantic_review_assessment_invalid"), Policy,
                    EKnowledgeSemanticReviewOutcome::Error);
            }
            else if (bGateBlocked)
            {
                Result = TerminalFailure(Response.RequestId, TEXT("semantic_review_blocked"), Policy,
                    EKnowledgeSemanticReviewOutcome::Blocked);
#if !UE_BUILD_SHIPPING
                Result.ClaimAssessments = Response.ClaimAssessments;
                Result.ReviewedClaims = Pending.Request.AllowedClaims;
                Result.WorstClaimQualityScore = WorstQuality;
                Result.WorstClaimId = WorstClaimId;
#endif
            }
            else
            {
                Result.bCompleted = true;
                Result.bApproved = true;
                Result.Outcome = EKnowledgeSemanticReviewOutcome::Approved;
                Result.RequestId = Response.RequestId;
                Result.FinalApprovedText = Pending.Request.MechanicallyValidatedText;
#if !UE_BUILD_SHIPPING
                Result.ClaimAssessments = Response.ClaimAssessments;
                Result.ReviewedClaims = Pending.Request.AllowedClaims;
                Result.WorstClaimQualityScore = WorstQuality;
                Result.WorstClaimId = WorstClaimId;
#endif
            }
        }
    }

#if !UE_BUILD_SHIPPING
    Result.ReviewerName = Response.ReviewerName;
    Result.ReviewerVersion = Response.ReviewerVersion;
    Result.PolicyVersion = Pending.Request.PolicyVersion;
    Result.AuthorizationEpoch = Pending.Request.AuthorizationEpoch;
    Result.ScopeFingerprint = Pending.Request.ScopeFingerprint;
    Result.ReviewDurationMs = Response.ReviewDurationMs;

    if (!Result.bApproved)
        Result.RejectedCandidateText = Pending.Request.MechanicallyValidatedText;

    if (!Result.ClaimAssessments.IsEmpty())
    {
        UE_LOG(LogTemp, Display,
            TEXT("Semantic review summary: request=%s approved=%s outcome=%d worstClaim=%s worstQuality=%.3f"),
            *Result.RequestId,
            Result.bApproved ? TEXT("true") : TEXT("false"),
            static_cast<int32>(Result.Outcome),
            *Result.WorstClaimId.ToString(),
            Result.WorstClaimQualityScore);
    }
#endif

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
