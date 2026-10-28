// Copyright 2026 xunyv.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KnowledgePack.h"
#include "KnowledgeReaction.generated.h"

/** Stable authored personality. Dynamic mood belongs in FNpcAffectContext. */
UCLASS(BlueprintType)
class MYAITOWNKNOWLEDGEGRANT_API UNpcPersonalityProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality", meta = (ClampMin = "0", ClampMax = "100"))
    int32 Empathy = 50;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality", meta = (ClampMin = "0", ClampMax = "100"))
    int32 Curiosity = 50;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality", meta = (ClampMin = "0", ClampMax = "100"))
    int32 Caution = 50;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality", meta = (ClampMin = "0", ClampMax = "100"))
    int32 OrderLoyalty = 50;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personality", meta = (ClampMin = "0", ClampMax = "100"))
    int32 Aggression = 50;

    bool IsDefinitionValid(FString& OutError) const;
};

/** Optional semantic affect input. It can be populated from Affect Core or another system. */
USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FNpcAffectContext
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
    FName Mood = TEXT("neutral");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reaction")
    FName Activation = TEXT("alert");
};

/** Prompt-ready reaction cues plus bounded deltas for an optional affect system. */
USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FNpcKnowledgeReaction
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Reaction") bool bOk = false;
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") bool bDuplicate = false;
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") FString Error;
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") FName DocumentId;
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") int32 Interest = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") int32 Approval = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") int32 PerceivedThreat = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") int32 Intensity = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") FName DialogueTone = TEXT("neutral");
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") FName BehaviorIntent = TEXT("observe");
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") int32 SuggestedValenceDelta = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") int32 SuggestedArousalDelta = 0;
};

/** Deterministic and model-independent personality appraisal. */
UCLASS()
class MYAITOWNKNOWLEDGEGRANT_API UKnowledgeReactionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Knowledge|Reaction")
    static FNpcKnowledgeReaction EvaluateKnowledgeReaction(
        const UNpcPersonalityProfile* Personality,
        const FKnowledgeDocument& Document,
        const FNpcAffectContext& AffectContext);
};
