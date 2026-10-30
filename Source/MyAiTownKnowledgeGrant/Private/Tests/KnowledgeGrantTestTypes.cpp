// Copyright 2026 xunyv.

#if WITH_EDITORONLY_DATA

#include "Tests/KnowledgeGrantTestTypes.h"

void UKnowledgeQuestAuthorityDispatchProbe::ProcessEvent(
    UFunction* Function, void* Parameters)
{
    if (Function != nullptr && Function->GetFName() == TEXT("ResolveCompletedQuest"))
    {
        ++ProcessEventCallCount;
    }
    Super::ProcessEvent(Function, Parameters);
}

bool UKnowledgeQuestAuthorityDispatchProbe::ResolveCompletedQuest_Implementation(
    const AActor* PlayerOwner, FName QuestId,
    FString& OutStablePlayerId, int32& OutCompletionRevision,
    bool& bOutDebugEvent) const
{
    ++ImplementationCallCount;
    OutStablePlayerId = TEXT("dispatch-probe-player");
    OutCompletionRevision = 7;
    bOutDebugEvent = false;
    return PlayerOwner == nullptr && QuestId == TEXT("quest.dispatch.probe");
}

#endif // WITH_EDITORONLY_DATA
