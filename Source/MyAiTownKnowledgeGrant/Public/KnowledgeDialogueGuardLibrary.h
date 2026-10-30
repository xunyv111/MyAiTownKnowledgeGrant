#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KnowledgeResolverLibrary.h"
#include "KnowledgeDialogueGuardLibrary.generated.h"

class UKnowledgeGrantComponent;
class UKnowledgeContextCollectorComponent;

UENUM(BlueprintType)
enum class EKnowledgeDialogueGuardMode : uint8
{
    AllOrNothing,
    PerClaim
};

UENUM(BlueprintType)
enum class EKnowledgeDialogueGuardOutcome : uint8
{
    Invalid,
    Allowed,
    PartiallyAllowed,
    Blocked
};

/** Trusted host-authored fallback policy. Model output cannot populate these fields. */
UCLASS(BlueprintType)
class MYAITOWNKNOWLEDGEGRANT_API UKnowledgeDialogueGuardPolicy : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    EKnowledgeDialogueGuardMode Mode = EKnowledgeDialogueGuardMode::AllOrNothing;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge", meta = (MultiLine = true))
    FText InsufficientEvidenceText;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge", meta = (MultiLine = true))
    FText InvalidViewText;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge", meta = (MultiLine = true))
    FText MalformedOutputText;

    /**
     * Allows uncited conversational lines only when they contain no protected
     * entities, numbers, negation, or declared fact metadata. Disabled by default.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Roleplay")
    bool bAllowUncitedRoleplay = false;

    /** Host-authored vocabulary used to catch protected proper nouns omitted from claim metadata. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Fact Guard")
    TArray<FString> ProtectedEntityLexicon;

    /** Optional written-number vocabulary; digit sequences are checked automatically. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Fact Guard")
    TArray<FString> ProtectedNumberLexicon;
};

/** Every model-generated atomic statement is untrusted and requires evidence. */
USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgeDialogueClaim
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge") FName ClaimId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge", meta = (MultiLine = true)) FString Text;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge") TArray<FName> EvidenceFactIds;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge") TArray<FString> ClaimedEntities;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge") TArray<FString> ClaimedNumbers;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Knowledge") bool bNegated = false;
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgeDialogueGuardResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") bool bOk = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") bool bBlocked = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") EKnowledgeDialogueGuardOutcome Outcome = EKnowledgeDialogueGuardOutcome::Invalid;
    /** Binds this mechanical result to the exact dialogue view that produced it. */
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") int64 ValidatedScopeFingerprint = 0;
    /** Mechanically validated text. When semantic review is enabled, this is not the final outlet text. */
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") FString SafeText;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") FString Error;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") TArray<FName> AllowedClaimIds;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") TArray<FName> BlockedClaimIds;
    /** Plugin-assigned atomic claims carried into the independent semantic review contract. */
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") TArray<FKnowledgeDialogueClaim> AllowedClaims;
};

/** Mechanical post-generation gate. Raw model text must never bypass SafeText. */
UCLASS()
class MYAITOWNKNOWLEDGEGRANT_API UKnowledgeDialogueGuardLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Knowledge")
    static FKnowledgeDialogueGuardResult GuardDialogueKnowledgeView(
        const UKnowledgeGrantComponent* PlayerKnowledge,
        const FDialogueKnowledgeView& KnowledgeView,
        const FGameplayTagContainer& CurrentActiveContextTags,
        const TArray<FKnowledgeDialogueClaim>& Claims,
        const UKnowledgeDialogueGuardPolicy* Policy);

    /**
     * Preferred entry point: derives the live context from actual collectors
     * instead of trusting a caller-supplied tag container. At least one valid
     * collector is required.
     */
    UFUNCTION(BlueprintPure, Category = "Knowledge")
    static FKnowledgeDialogueGuardResult GuardDialogueKnowledgeViewForCollectors(
        const UKnowledgeGrantComponent* PlayerKnowledge,
        const FDialogueKnowledgeView& KnowledgeView,
        const TArray<UKnowledgeContextCollectorComponent*>& ContextCollectors,
        const TArray<FKnowledgeDialogueClaim>& Claims,
        const UKnowledgeDialogueGuardPolicy* Policy);
};
