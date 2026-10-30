// Copyright 2026 xunyv.

#pragma once

#include "CoreMinimal.h"
#include "ArpgKnowledgeGrantAdapterComponent.h"
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
