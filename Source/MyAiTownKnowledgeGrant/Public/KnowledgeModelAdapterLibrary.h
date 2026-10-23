#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KnowledgeDialogueGuardLibrary.h"
#include "KnowledgeModelAdapterLibrary.generated.h"

class UKnowledgeGrantComponent;
class UKnowledgeContextCollectorComponent;

/**
 * Strict JSON entrance for untrusted model output. It never returns parsed raw
 * claims to Blueprint: success and failure both leave through mechanically
 * validated SafeText. Free-form dialogue may require semantic review before
 * any formal outlet consumes text.
 */
UCLASS()
class MYAITOWNKNOWLEDGEGRANT_API UKnowledgeModelAdapterLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Knowledge|Model Adapter")
    static FKnowledgeDialogueGuardResult GuardKnowledgeModelJsonResponse(
        const UKnowledgeGrantComponent* PlayerKnowledge,
        const FDialogueKnowledgeView& KnowledgeView,
        const FGameplayTagContainer& CurrentActiveContextTags,
        const FString& JsonPayload,
        const UKnowledgeDialogueGuardPolicy* GuardPolicy);

    /** Preferred entry point: live context comes from collectors, not the caller. */
    UFUNCTION(BlueprintPure, Category = "Knowledge|Model Adapter")
    static FKnowledgeDialogueGuardResult GuardKnowledgeModelJsonResponseForCollectors(
        const UKnowledgeGrantComponent* PlayerKnowledge,
        const FDialogueKnowledgeView& KnowledgeView,
        const TArray<UKnowledgeContextCollectorComponent*>& ContextCollectors,
        const FString& JsonPayload,
        const UKnowledgeDialogueGuardPolicy* GuardPolicy);
};
