// Copyright 2026 xunyv.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "KnowledgeContextCollectorComponent.generated.h"

/**
 * Collects dialogue context from scene overlaps, the current interaction target,
 * and explicit proper-noun mentions. It selects relevant knowledge libraries;
 * it never grants knowledge.
 */
UCLASS(ClassGroup = (Knowledge), meta = (BlueprintSpawnableComponent))
class MYAITOWNKNOWLEDGEGRANT_API UKnowledgeContextCollectorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    /** Call from a Trigger Volume's BeginOverlap. Reusing a source replaces its previous tags. */
    UFUNCTION(BlueprintCallable, Category = "Knowledge|Context")
    void EnterSceneContext(UObject* Source, const FGameplayTagContainer& ContextTags);

    /** Call from the matching Trigger Volume's EndOverlap. */
    UFUNCTION(BlueprintCallable, Category = "Knowledge|Context")
    void ExitSceneContext(UObject* Source);

    /** Replaces the previous interaction target instead of accumulating stale interaction tags. */
    UFUNCTION(BlueprintCallable, Category = "Knowledge|Context")
    void SetInteractionContext(UObject* Source, const FGameplayTagContainer& ContextTags);

    /** Clears only when Source is still the current interaction target. */
    UFUNCTION(BlueprintCallable, Category = "Knowledge|Context")
    void ClearInteractionContext(UObject* Source);

    /** Replaces the proper nouns explicitly recognized for the current dialogue turn. */
    UFUNCTION(BlueprintCallable, Category = "Knowledge|Context")
    void SetExplicitMentionContext(const FGameplayTagContainer& ContextTags);

    UFUNCTION(BlueprintCallable, Category = "Knowledge|Context")
    void ClearExplicitMentionContext();

    /** Union of valid scene sources, current interaction, and explicit mentions. */
    UFUNCTION(BlueprintPure, Category = "Knowledge|Context")
    FGameplayTagContainer GetActiveContextTags() const;

    UFUNCTION(BlueprintCallable, Category = "Knowledge|Context")
    void ResetAllContexts();

private:
    /** Mutable only so the const query can prune destroyed weak sources. */
    mutable TMap<TWeakObjectPtr<UObject>, FGameplayTagContainer> SceneContexts;
    TWeakObjectPtr<UObject> InteractionSource;
    FGameplayTagContainer InteractionTags;
    FGameplayTagContainer ExplicitMentionTags;
};
