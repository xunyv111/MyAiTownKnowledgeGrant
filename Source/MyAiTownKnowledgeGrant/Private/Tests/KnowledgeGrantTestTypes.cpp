// Copyright 2026 xunyv.

#if WITH_EDITORONLY_DATA

#include "Tests/KnowledgeGrantTestTypes.h"

void UKnowledgeQuestAuthorityDispatchProbe::ProcessEvent(
    UFunction* Function, void* Parameters)
{
    if (Function != nullptr && Function->GetFName() == TEXT("ResolveCompletedQuest"))
    {
        ++ProcessEventCallCount;
    }
    Super::ProcessEvent(Function, Parameters);
}

bool UKnowledgeQuestAuthorityDispatchProbe::ResolveCompletedQuest_Implementation(
    const AActor* PlayerOwner, FName QuestId,
    FString& OutStablePlayerId, int32& OutCompletionRevision,
    bool& bOutDebugEvent) const
{
    ++ImplementationCallCount;
    OutStablePlayerId = TEXT("dispatch-probe-player");
    OutCompletionRevision = 7;
    bOutDebugEvent = false;
    return PlayerOwner == nullptr && QuestId == TEXT("quest.dispatch.probe");
}

void UKnowledgeSemanticReviewSyncProbe::HandleRequest(FKnowledgeSemanticReviewRequest Request)
{
    ++RequestCount;
    if (!IsValid(Coordinator)) return;

    FKnowledgeSemanticReviewResponse Response;
    Response.RequestId = Request.RequestId;
    Response.ReviewerName = TEXT("sync-probe");
    Response.ReviewerVersion = TEXT("1.0");
    for (const FKnowledgeDialogueClaim& Claim : Request.AllowedClaims)
    {
        FKnowledgeSemanticClaimAssessment Assessment;
        Assessment.ClaimId = Claim.ClaimId;
        Assessment.ClaimType = Claim.EvidenceFactIds.IsEmpty()
            ? EKnowledgeClaimType::NonFactual : EKnowledgeClaimType::Factual;
        Assessment.Support = Claim.EvidenceFactIds.IsEmpty()
            ? EKnowledgeClaimSupport::Unsupported : EKnowledgeClaimSupport::Supported;
        Assessment.Contradiction = EKnowledgeClaimRisk::None;
        Assessment.Overreach = EKnowledgeClaimRisk::None;
        Assessment.TaskLeak = EKnowledgeClaimRisk::None;
        Assessment.RoleFit = EKnowledgeRoleFit::Good;
        Assessment.Confidence = EKnowledgeReviewConfidence::High;
        Assessment.EvidenceFactIds = Claim.EvidenceFactIds;
        Assessment.FactSupportScore = 1.0f;
        Assessment.ContradictionRiskScore = 0.0f;
        Assessment.OverreachRiskScore = 0.0f;
        Assessment.TaskLeakRiskScore = 0.0f;
        Assessment.RoleFitScore = 1.0f;
        Assessment.ConfidenceScore = 1.0f;
        Response.ClaimAssessments.Add(MoveTemp(Assessment));
    }
    bApproved = Coordinator->CompleteSemanticReview(Response).bApproved;
}

#endif // WITH_EDITORONLY_DATA
