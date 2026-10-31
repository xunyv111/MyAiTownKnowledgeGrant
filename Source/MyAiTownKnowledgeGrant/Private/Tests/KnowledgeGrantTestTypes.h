// Copyright 2026 xunyv.

#pragma once

#if WITH_EDITORONLY_DATA

#include "CoreMinimal.h"
#include "ArpgKnowledgeGrantAdapterComponent.h"
#include "KnowledgeSemanticReviewComponent.h"
#include "KnowledgeGrantTestTypes.generated.h"

/** Native interface probe that records whether the generated Execute_ thunk used ProcessEvent. */
UCLASS()
class UKnowledgeQuestAuthorityDispatchProbe final
    : public UObject, public IArpgQuestAuthorityProvider
{
    GENERATED_BODY()

public:
    int32 ProcessEventCallCount = 0;
    mutable int32 ImplementationCallCount = 0;

    virtual void ProcessEvent(UFunction* Function, void* Parameters) override;
    virtual bool ResolveCompletedQuest_Implementation(
        const AActor* PlayerOwner, FName QuestId,
        FString& OutStablePlayerId, int32& OutCompletionRevision,
        bool& bOutDebugEvent) const override;
};

/** Synchronous host listener used to prove dispatch correlation precedes completion. */
UCLASS()
class UKnowledgeSemanticReviewSyncProbe final : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY() TObjectPtr<UKnowledgeSemanticReviewComponent> Coordinator;
    int32 RequestCount = 0;
    int32 CompletionCount = 0;
    bool bApproved = false;
    FKnowledgeSemanticReviewResult LastResult;

    UFUNCTION() void HandleRequest(FKnowledgeSemanticReviewRequest Request);
    UFUNCTION() void HandleCompleted(FKnowledgeSemanticReviewResult Result);
};

#endif // WITH_EDITORONLY_DATA
