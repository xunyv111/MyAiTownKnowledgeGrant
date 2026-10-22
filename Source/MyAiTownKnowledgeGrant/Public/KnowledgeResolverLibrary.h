#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KnowledgePack.h"
#include "KnowledgeResolverLibrary.generated.h"

class UKnowledgeGrantComponent;

UENUM(BlueprintType)
enum class EKnowledgeExclusionReason : uint8
{
    NotGranted,
    ContextMismatch,
    InvalidPack
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgePackExclusion
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") FName PackId;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") EKnowledgeExclusionReason Reason = EKnowledgeExclusionReason::NotGranted;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") FString Detail;
};

/** Binds an authorization epoch, exact context tags and allowed documents for one dialogue turn. */
USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FDialogueKnowledgeView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") bool bOk = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") FString Error;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") int32 PlayerRevision = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") FGuid AuthorizationEpoch;
    /** CRC consistency checksum. It detects accidental mutation; it is not a security signature. */
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") int64 ScopeFingerprint = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") FGameplayTagContainer ActiveTags;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") TArray<FName> IncludedPackIds;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") TArray<FKnowledgeDocument> AllowedDocuments;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") TArray<FKnowledgePackExclusion> ExcludedReasons;
};

/** Builds the only supported prompt knowledge view for an NPC dialogue turn. */
UCLASS()
class MYAITOWNKNOWLEDGEGRANT_API UKnowledgeResolverLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Knowledge")
    static FDialogueKnowledgeView BuildDialogueKnowledgeView(
        const UKnowledgeGrantComponent* PlayerKnowledge,
        const TArray<UKnowledgePack*>& CandidatePacks,
        const FGameplayTagContainer& ActiveContextTags);

    static bool ValidateDialogueKnowledgeView(
        const UKnowledgeGrantComponent* PlayerKnowledge,
        const FDialogueKnowledgeView& View,
        const FGameplayTagContainer& CurrentActiveContextTags,
        FString& OutError);
};
