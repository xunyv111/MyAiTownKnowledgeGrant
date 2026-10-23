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
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") TArray<FKnowledgeFact> AllowedFacts;
};

/** A reviewer may approve or block only. It cannot inject replacement dialogue. */
USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgeSemanticReviewResponse
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") FString RequestId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") EKnowledgeSemanticReviewDecision Decision = EKnowledgeSemanticReviewDecision::Error;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") TArray<FName> ReviewedClaimIds;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") TArray<FName> UnsupportedClaimIds;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") TArray<FString> ReasonCodes;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") FString ReviewerName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge|Semantic Review") FString ReviewerVersion;
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
    /** The only text that formal dialogue outlets may consume after semantic review is enabled. */
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") FString FinalApprovedText;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|Semantic Review") FString Error;
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
