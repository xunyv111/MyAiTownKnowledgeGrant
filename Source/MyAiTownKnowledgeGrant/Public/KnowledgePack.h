#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "KnowledgePack.generated.h"

/** One atomic, mechanically checkable fact inside a prompt document. */
USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgeFact
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    FName FactId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge", meta = (MultiLine = true))
    FString Statement;

    /** Proper nouns or protected entity phrases that a claim must declare and contain. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    TArray<FString> Entities;

    /** Canonical number tokens, including project-specific written forms such as "三". */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    TArray<FString> Numbers;

    /** True only when the atomic statement itself is negative. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    bool bNegated = false;
};

/** One prompt-ready document. Locked documents and their facts never enter resolver output. */
USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgeDocument
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    FName DocumentId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge", meta = (MultiLine = true))
    FString Content;

    /** Claims cite FactId values, never the broader DocumentId. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    TArray<FKnowledgeFact> Facts;

    /** 0..100: immediate danger implied by this fact. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reaction", meta = (ClampMin = "0", ClampMax = "100"))
    int32 ThreatLevel = 0;

    /** 0..100: how surprising or information-rich this fact is. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reaction", meta = (ClampMin = "0", ClampMax = "100"))
    int32 Novelty = 50;

    /** 0..100: degree of cruelty, betrayal, or injustice contained in the fact. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reaction", meta = (ClampMin = "0", ClampMax = "100"))
    int32 MoralViolation = 0;

    /** -100..100: negative undermines order, positive reinforces it. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reaction", meta = (ClampMin = "-100", ClampMax = "100"))
    int32 OrderImpact = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reaction")
    FGameplayTagContainer Topics;
};

/**
 * Facts made available by one completed quest or one world event.
 * Put only facts that are already true/observable in a pack. Future script
 * details belong to a later pack whose prerequisite domino has actually fired.
 */
UCLASS(BlueprintType)
class MYAITOWNKNOWLEDGEGRANT_API UKnowledgePack : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    FName PackId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    FName SourceQuestId;

    /** Alternative to SourceQuestId for discoveries and world-state changes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    FName SourceWorldEventId;

    /** Every pack in this list must already be granted before this pack can be granted. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    TArray<FName> RequiredPackIds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    FGameplayTag UnlockTag;

    /**
     * Optional proper-noun or scene markers that make this library relevant.
     * An empty container means globally relevant. Otherwise any matching active
     * context tag activates the pack, but only after the pack is granted.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Context")
    FGameplayTagContainer ActivationTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    TArray<FKnowledgeDocument> Documents;

    bool IsDefinitionValid(FString& OutError) const;
};
