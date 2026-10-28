// Copyright 2026 xunyv.

#include "KnowledgeContextCollectorComponent.h"

void UKnowledgeContextCollectorComponent::EnterSceneContext(
    UObject* Source, const FGameplayTagContainer& ContextTags)
{
    if (!IsValid(Source)) return;
    if (ContextTags.IsEmpty())
    {
        SceneContexts.Remove(Source);
        return;
    }
    SceneContexts.FindOrAdd(Source) = ContextTags;
}

void UKnowledgeContextCollectorComponent::ExitSceneContext(UObject* Source)
{
    if (Source != nullptr) SceneContexts.Remove(Source);
}

void UKnowledgeContextCollectorComponent::SetInteractionContext(
    UObject* Source, const FGameplayTagContainer& ContextTags)
{
    if (!IsValid(Source) || ContextTags.IsEmpty())
    {
        InteractionSource.Reset();
        InteractionTags.Reset();
        return;
    }
    InteractionSource = Source;
    InteractionTags = ContextTags;
}

void UKnowledgeContextCollectorComponent::ClearInteractionContext(UObject* Source)
{
    if (Source != nullptr && InteractionSource.Get() == Source)
    {
        InteractionSource.Reset();
        InteractionTags.Reset();
    }
}

void UKnowledgeContextCollectorComponent::SetExplicitMentionContext(
    const FGameplayTagContainer& ContextTags)
{
    ExplicitMentionTags = ContextTags;
}

void UKnowledgeContextCollectorComponent::ClearExplicitMentionContext()
{
    ExplicitMentionTags.Reset();
}

FGameplayTagContainer UKnowledgeContextCollectorComponent::GetActiveContextTags() const
{
    FGameplayTagContainer Result;
    for (auto It = SceneContexts.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
            continue;
        }
        Result.AppendTags(It.Value());
    }
    if (InteractionSource.IsValid()) Result.AppendTags(InteractionTags);
    Result.AppendTags(ExplicitMentionTags);
    return Result;
}

void UKnowledgeContextCollectorComponent::ResetAllContexts()
{
    SceneContexts.Reset();
    InteractionSource.Reset();
    InteractionTags.Reset();
    ExplicitMentionTags.Reset();
}
