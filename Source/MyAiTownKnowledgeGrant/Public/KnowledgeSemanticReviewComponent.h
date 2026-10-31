#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "KnowledgeDialogueGuardLibrary.h"
#include "KnowledgeSemanticReviewComponent.generated.h"

class UKnowledgeContextCollectorComponent;
class UKnowledgeGrantComponent;

UENUM(BlueprintType)
enum class EKnowledgeSemanticReviewDecision : uint8
{
    Approved,
    Blocked,
    Error
};

UENUM(BlueprintType)
enum class EKnowledgeSemanticReviewOutcome : uint8
{
    Invalid,
    Pending,
    Approved,
    Blocked,
    Error
};

UENUM(BlueprintType)
enum class EKnowledgeClaimType : uint8
{
    Factual,
    NonFactual
};

UENUM(BlueprintType)
enum class EKnowledgeClaimSupport : uint8
{
    Supported,
    Partial,
    Unsupported
};

UENUM(BlueprintType)
enum class EKnowledgeClaimRisk : uint8
{
    None,
    Possible,
    Clear
};

UENUM(BlueprintType)
enum class EKnowledgeRoleFit : uint8
{
    Good,
    Weak,
    Bad
};

UENUM(BlueprintType)
enum class EKnowledgeReviewConfidence : uint8
{
    High,
    Medium,
    Low
};

/** Host-authored fail-closed policy. API credentials and provider settings stay outside this asset. */
UCLASS(BlueprintType)
class MYAITOWNKNOWLEDGEGRANT_API UKnowledgeSemanticReviewPolicy : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Semantic Review")
    FName PolicyVersion;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Semantic Review", meta = (ClampMin = "0.1", ClampMax = "60.0"))
    float TimeoutSeconds = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Semantic Review", meta = (MultiLine = true))
    FText BlockedText;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Semantic Review", meta = (MultiLine = true))
    FText ErrorText;

    /** Quality weights are diagnostic only. They never compensate for a failed safety gate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Semantic Review|Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FactSupportQualityWeight = 0.35f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Semantic Review|Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ContradictionSafetyQualityWeight = 0.20f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Semantic Review|Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float OverreachSafetyQualityWeight = 0.20f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Semantic Review|Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float TaskLeakSafetyQualityWeight = 0.15f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Semantic Review|Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RoleFitQualityWeight = 0.10f;

    /** Weak role fit is a quality warning, not a knowledge-safety failure, unless the host opts into strict role quality. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Semantic Review|Gate")
    bool bBlockWeakRoleFit = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Semantic Review|Gate")
    bool bBlockMediumConfidence = false;
};

/** Provider-neutral payload for an external API, local model, or project-owned reviewer. */
USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgeSemanticReviewRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") FString RequestId;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") int32 PlayerRevision = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") FGuid AuthorizationEpoch;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") int64 ScopeFingerprint = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") FName PolicyVersion;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") FDateTime ExpiresAtUtc;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") FString MechanicallyValidatedText;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") TArray<FName> AllowedClaimIds;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") TArray<FKnowledgeDialogueClaim> AllowedClaims;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") TArray<FKnowledgeFact> AllowedFacts;
};

/** One complete, non-compensating assessment for one plugin-assigned claim ID. */
USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgeSemanticClaimAssessment
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") FName ClaimId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") EKnowledgeClaimType ClaimType = EKnowledgeClaimType::Factual;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") EKnowledgeClaimSupport Support = EKnowledgeClaimSupport::Unsupported;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") EKnowledgeClaimRisk Contradiction = EKnowledgeClaimRisk::Clear;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") EKnowledgeClaimRisk Overreach = EKnowledgeClaimRisk::Clear;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") EKnowledgeClaimRisk TaskLeak = EKnowledgeClaimRisk::Clear;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") EKnowledgeRoleFit RoleFit = EKnowledgeRoleFit::Bad;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") EKnowledgeReviewConfidence Confidence = EKnowledgeReviewConfidence::Low;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") TArray<FName> EvidenceFactIds;

    /** Untrusted reviewer telemetry. Every value must be finite and in [0,1]; gates use the enums above. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review|Telemetry") float FactSupportScore = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review|Telemetry") float ContradictionRiskScore = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review|Telemetry") float OverreachRiskScore = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review|Telemetry") float TaskLeakRiskScore = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review|Telemetry") float RoleFitScore = 0.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review|Telemetry") float ConfidenceScore = 0.0f;
};

/** A reviewer may approve or block only. It cannot inject replacement dialogue. */
USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgeSemanticReviewResponse
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") FString RequestId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") TArray<FKnowledgeSemanticClaimAssessment> ClaimAssessments;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") FString ReviewerName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") FString ReviewerVersion;
    /** Trusted local transport/parser error. Remote response bodies never populate this field directly. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") FString Error;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review", meta = (ClampMin = "0")) int32 ReviewDurationMs = 0;
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgeSemanticReviewResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") bool bCompleted = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") bool bApproved = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") EKnowledgeSemanticReviewOutcome Outcome = EKnowledgeSemanticReviewOutcome::Invalid;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") FString RequestId;
    /** Trusted local review identity and policy metadata. Diagnostics only. */
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review|Diagnostics") FString ReviewerName;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review|Diagnostics") FString ReviewerVersion;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review|Diagnostics") FName PolicyVersion;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review|Diagnostics") FGuid AuthorizationEpoch;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review|Diagnostics") int64 ScopeFingerprint = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review|Diagnostics") int32 ReviewDurationMs = 0;
    /** The only text that formal dialogue outlets may consume after semantic review is enabled. */
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") FString FinalApprovedText;
    /** Development diagnostics only. Never route this rejected candidate to gameplay, voice, or behavior. */
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") FString RejectedCandidateText;
    /** Locally validated per-claim reviewer output, retained for non-Shipping diagnostics. */
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review")
    TArray<FKnowledgeSemanticClaimAssessment> ClaimAssessments;
    /** Mechanically validated claim texts corresponding to ClaimAssessments. Diagnostics only. */
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review")
    TArray<FKnowledgeDialogueClaim> ReviewedClaims;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") FString Error;
    /** Minimum diagnostic quality across all claims; never used to override a failed gate. */
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") float WorstClaimQualityScore = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") FName WorstClaimId;
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgeSemanticReviewBeginResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") bool bOk = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") FString Error;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") FKnowledgeSemanticReviewRequest Request;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FKnowledgeSemanticReviewRequestedSignature, FKnowledgeSemanticReviewRequest, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FKnowledgeSemanticReviewCompletedSignature, FKnowledgeSemanticReviewResult, Result);

/**
 * Coordinates an asynchronous, provider-neutral semantic review. The host sends
 * OnSemanticReviewRequested to its API/local model and returns only a structured
 * decision through CompleteSemanticReview. Every response is revalidated against
 * the live authorization epoch and context before text can be released.
 */
UCLASS(ClassGroup = (Knowledge), meta = (BlueprintSpawnableComponent))
class MYAITOWNKNOWLEDGEGRANT_API UKnowledgeSemanticReviewComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKnowledgeSemanticReviewComponent();

    UPROPERTY(BlueprintAssignable, Category = "Knowledge|Semantic Review")
    FKnowledgeSemanticReviewRequestedSignature OnSemanticReviewRequested;

    UPROPERTY(BlueprintAssignable, Category = "Knowledge|Semantic Review")
    FKnowledgeSemanticReviewCompletedSignature OnSemanticReviewCompleted;

    /**
     * Creates the pending review and returns its request without broadcasting. Callers that must
     * correlate the review with a dialogue request should call this, correlate, then dispatch.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Knowledge|Semantic Review")
    FKnowledgeSemanticReviewBeginResult CreateSemanticReview(
        const UKnowledgeGrantComponent* PlayerKnowledge,
        const FDialogueKnowledgeView& KnowledgeView,
        const TArray<UKnowledgeContextCollectorComponent*>& ContextCollectors,
        const FKnowledgeDialogueGuardResult& MechanicalResult,
        const UKnowledgeSemanticReviewPolicy* Policy);

    /** Broadcasts OnSemanticReviewRequested for an existing pending review. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Knowledge|Semantic Review")
    bool DispatchSemanticReviewRequest(const FString& RequestId);

    /**
     * Compatibility convenience: creates and dispatches immediately. Prefer CreateSemanticReview
     * followed by explicit correlation and DispatchSemanticReviewRequest when a sync host callback
     * is possible.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Knowledge|Semantic Review")
    FKnowledgeSemanticReviewBeginResult BeginSemanticReview(
        const UKnowledgeGrantComponent* PlayerKnowledge,
        const FDialogueKnowledgeView& KnowledgeView,
        const TArray<UKnowledgeContextCollectorComponent*>& ContextCollectors,
        const FKnowledgeDialogueGuardResult& MechanicalResult,
        const UKnowledgeSemanticReviewPolicy* Policy);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Knowledge|Semantic Review")
    FKnowledgeSemanticReviewResult CompleteSemanticReview(
        const FKnowledgeSemanticReviewResponse& Response);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Knowledge|Semantic Review")
    void CancelAllPendingReviews();

    UFUNCTION(BlueprintPure, Category = "Knowledge|Semantic Review")
    int32 GetPendingReviewCount() const { return PendingReviews.Num(); }

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    struct FPendingReview
    {
        FKnowledgeSemanticReviewRequest Request;
        FDialogueKnowledgeView KnowledgeView;
        TWeakObjectPtr<UKnowledgeGrantComponent> PlayerKnowledge;
        TArray<TWeakObjectPtr<UKnowledgeContextCollectorComponent>> ContextCollectors;
        TWeakObjectPtr<UKnowledgeSemanticReviewPolicy> Policy;
    };

    TMap<FString, FPendingReview> PendingReviews;

    bool HasAuthorityForMutation() const;
    static bool CollectLiveContext(
        const TArray<TWeakObjectPtr<UKnowledgeContextCollectorComponent>>& Collectors,
        FGameplayTagContainer& OutTags);
    static FKnowledgeSemanticReviewResult TerminalFailure(
        const FString& RequestId, const FString& Error,
        const UKnowledgeSemanticReviewPolicy* Policy,
        EKnowledgeSemanticReviewOutcome Outcome);
};
