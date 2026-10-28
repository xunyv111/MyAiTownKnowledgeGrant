// Copyright 2026 xunyv.

#include "KnowledgeGrantComponent.h"

#include "GameFramework/Actor.h"
#include "KnowledgePack.h"
#include "Net/UnrealNetwork.h"

UKnowledgeGrantComponent::UKnowledgeGrantComponent()
{
    SetIsReplicatedByDefault(true);
}

FKnowledgeGrantResult UKnowledgeGrantComponent::GrantKnowledgeFromQuest(
    const FString& GrantEventId, FName CompletedQuestId, UKnowledgePack* KnowledgePack)
{
    if (!HasAuthorityForMutation()) return Failure(TEXT("authority_required"), State.Revision);
    const FString NormalizedGrantEventId = GrantEventId.TrimStartAndEnd();
    if (NormalizedGrantEventId.IsEmpty()) return Failure(TEXT("grant_event_id_invalid"), State.Revision);
    if (CompletedQuestId.IsNone() || !IsValid(KnowledgePack)) return Failure(TEXT("quest_or_pack_invalid"), State.Revision);

    FString DefinitionError;
    if (!KnowledgePack->IsDefinitionValid(DefinitionError)) return Failure(DefinitionError, State.Revision);
    if (KnowledgePack->SourceQuestId != CompletedQuestId) return Failure(TEXT("quest_pack_mismatch"), State.Revision);
    return ApplyGrant(NormalizedGrantEventId, KnowledgePack);
}

FKnowledgeGrantResult UKnowledgeGrantComponent::GrantKnowledgeFromWorldEvent(
    const FString& GrantEventId, FName OccurredWorldEventId, UKnowledgePack* KnowledgePack)
{
    if (!HasAuthorityForMutation()) return Failure(TEXT("authority_required"), State.Revision);
    const FString NormalizedGrantEventId = GrantEventId.TrimStartAndEnd();
    if (NormalizedGrantEventId.IsEmpty()) return Failure(TEXT("grant_event_id_invalid"), State.Revision);
    if (OccurredWorldEventId.IsNone() || !IsValid(KnowledgePack)) return Failure(TEXT("world_event_or_pack_invalid"), State.Revision);

    FString DefinitionError;
    if (!KnowledgePack->IsDefinitionValid(DefinitionError)) return Failure(DefinitionError, State.Revision);
    if (KnowledgePack->SourceWorldEventId != OccurredWorldEventId) return Failure(TEXT("world_event_pack_mismatch"), State.Revision);
    return ApplyGrant(NormalizedGrantEventId, KnowledgePack);
}

bool UKnowledgeGrantComponent::ArePrerequisitesSatisfied(const UKnowledgePack* KnowledgePack) const
{
    if (!IsValid(KnowledgePack)) return false;
    for (const FName RequiredPackId : KnowledgePack->RequiredPackIds)
    {
        if (!State.GrantedPackIds.Contains(RequiredPackId)) return false;
    }
    return true;
}

FKnowledgeGrantResult UKnowledgeGrantComponent::ApplyGrant(const FString& GrantEventId, UKnowledgePack* KnowledgePack)
{
    for (const FKnowledgeGrantRecord& Existing : State.ProcessedGrants)
    {
        if (Existing.GrantEventId == GrantEventId)
        {
            if (Existing.PackId != KnowledgePack->PackId)
            {
                return Failure(TEXT("grant_event_id_conflict"), State.Revision);
            }
            FKnowledgeGrantResult Result;
            Result.bOk = true;
            Result.bDuplicate = true;
            Result.Revision = State.Revision;
            return Result;
        }
    }

    if (!ArePrerequisitesSatisfied(KnowledgePack)) return Failure(TEXT("knowledge_prerequisite_missing"), State.Revision);
    if (MaxProcessedGrantRecords > 0 && State.ProcessedGrants.Num() >= MaxProcessedGrantRecords)
    {
        return Failure(TEXT("grant_history_full"), State.Revision);
    }

    const bool bAccessChanged = !State.GrantedPackIds.Contains(KnowledgePack->PackId);
    if (bAccessChanged) State.GrantedPackIds.Add(KnowledgePack->PackId);
    if (KnowledgePack->UnlockTag.IsValid()) State.GrantedTags.AddTag(KnowledgePack->UnlockTag);
    FKnowledgeGrantRecord Record;
    Record.GrantEventId = GrantEventId;
    Record.PackId = KnowledgePack->PackId;
    State.ProcessedGrants.Add(Record);
    ++State.Revision;
    State.AuthorizationEpoch = FGuid::NewGuid();

    FKnowledgeGrantResult Result;
    Result.bOk = true;
    Result.bApplied = bAccessChanged;
    Result.Revision = State.Revision;
    OnKnowledgeChanged.Broadcast(State.Revision);
    return Result;
}

bool UKnowledgeGrantComponent::HasKnowledgePack(FName PackId) const
{
    return !PackId.IsNone() && State.GrantedPackIds.Contains(PackId);
}

bool UKnowledgeGrantComponent::HasKnowledgeTag(FGameplayTag KnowledgeTag) const
{
    return KnowledgeTag.IsValid() && State.GrantedTags.HasTagExact(KnowledgeTag);
}

FKnowledgeGrantResult UKnowledgeGrantComponent::RevokeKnowledgePack(
    FName PackId, const TArray<UKnowledgePack*>& RegisteredPacks)
{
    if (!HasAuthorityForMutation()) return Failure(TEXT("authority_required"), State.Revision);
    if (PackId.IsNone() || !State.GrantedPackIds.Contains(PackId))
    {
        return Failure(TEXT("pack_not_granted"), State.Revision);
    }

    TMap<FName, const UKnowledgePack*> Registry;
    for (const UKnowledgePack* Pack : RegisteredPacks)
    {
        if (!IsValid(Pack) || Pack->PackId.IsNone() || !State.GrantedPackIds.Contains(Pack->PackId)) continue;

        FString DefinitionError;
        if (!Pack->IsDefinitionValid(DefinitionError) || Registry.Contains(Pack->PackId))
        {
            return Failure(TEXT("revoke_registry_invalid"), State.Revision);
        }
        Registry.Add(Pack->PackId, Pack);
    }
    for (const FName GrantedPackId : State.GrantedPackIds)
    {
        if (GrantedPackId != PackId && !Registry.Contains(GrantedPackId))
        {
            return Failure(TEXT("revoke_registry_incomplete"), State.Revision);
        }
    }

    for (const FName GrantedPackId : State.GrantedPackIds)
    {
        if (GrantedPackId == PackId) continue;
        const UKnowledgePack* const* Pack = Registry.Find(GrantedPackId);
        if (Pack != nullptr && (*Pack)->RequiredPackIds.Contains(PackId))
        {
            return Failure(TEXT("pack_has_dependents"), State.Revision);
        }
    }

    FGameplayTagContainer RemainingTags;
    for (const FName GrantedPackId : State.GrantedPackIds)
    {
        if (GrantedPackId == PackId) continue;
        const UKnowledgePack* const* Pack = Registry.Find(GrantedPackId);
        if (Pack != nullptr && (*Pack)->UnlockTag.IsValid())
        {
            RemainingTags.AddTag((*Pack)->UnlockTag);
        }
    }

    State.GrantedPackIds.Remove(PackId);
    State.ProcessedGrants.RemoveAll(
        [PackId](const FKnowledgeGrantRecord& Record) { return Record.PackId == PackId; });
    State.GrantedTags = MoveTemp(RemainingTags);
    ++State.Revision;
    State.AuthorizationEpoch = FGuid::NewGuid();

    FKnowledgeGrantResult Result;
    Result.bOk = true;
    Result.bApplied = true;
    Result.Revision = State.Revision;
    OnKnowledgeChanged.Broadcast(State.Revision);
    return Result;
}

FKnowledgeGrantSnapshot UKnowledgeGrantComponent::CreateSnapshot(FName PackRegistryVersion) const
{
    FKnowledgeGrantSnapshot Snapshot;
    Snapshot.PackRegistryVersion = PackRegistryVersion;
    Snapshot.State = State;
    return Snapshot;
}

FKnowledgeGrantResult UKnowledgeGrantComponent::RestoreSnapshot(
    const FKnowledgeGrantSnapshot& Snapshot, const TArray<UKnowledgePack*>& RegisteredPacks,
    FName ExpectedPackRegistryVersion)
{
    if (!HasAuthorityForMutation()) return Failure(TEXT("authority_required"), State.Revision);
    FKnowledgeGrantState ValidatedState;
    FString Error;
    if (!ValidateSnapshotAgainstRegistry(
        Snapshot, RegisteredPacks, ExpectedPackRegistryVersion, ValidatedState, Error))
    {
        return Failure(Error, State.Revision);
    }

    if (MaxProcessedGrantRecords > 0 && ValidatedState.ProcessedGrants.Num() > MaxProcessedGrantRecords)
    {
        return Failure(TEXT("snapshot_grant_history_limit_exceeded"), State.Revision);
    }
    if (State.Revision == MAX_int32 || ValidatedState.Revision == MAX_int32)
    {
        return Failure(TEXT("snapshot_revision_overflow"), State.Revision);
    }
    ValidatedState.Revision = FMath::Max(State.Revision, ValidatedState.Revision) + 1;
    ValidatedState.AuthorizationEpoch = FGuid::NewGuid();
    State = MoveTemp(ValidatedState);
    FKnowledgeGrantResult Result;
    Result.bOk = true;
    Result.bApplied = true;
    Result.Revision = State.Revision;
    OnKnowledgeChanged.Broadcast(State.Revision);
    return Result;
}

void UKnowledgeGrantComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    // Owner-only: ProcessedGrants embeds stable player ids and quest progress,
    // so other clients must never receive it. The server stays the single
    // resolution authority for every dialogue view.
    DOREPLIFETIME_CONDITION(UKnowledgeGrantComponent, State, COND_OwnerOnly);
}

void UKnowledgeGrantComponent::OnRep_State()
{
    OnKnowledgeChanged.Broadcast(State.Revision);
}

bool UKnowledgeGrantComponent::HasAuthorityForMutation() const
{
    const AActor* Owner = GetOwner();
    return Owner != nullptr && Owner->HasAuthority();
}

bool UKnowledgeGrantComponent::ValidateSnapshotAgainstRegistry(
    const FKnowledgeGrantSnapshot& Snapshot, const TArray<UKnowledgePack*>& RegisteredPacks,
    FName ExpectedPackRegistryVersion, FKnowledgeGrantState& OutValidatedState, FString& OutError)
{
    if (Snapshot.SchemaVersion != 3)
    {
        OutError = TEXT("snapshot_schema_unsupported");
        return false;
    }
    if (ExpectedPackRegistryVersion.IsNone() || Snapshot.PackRegistryVersion.IsNone())
    {
        OutError = TEXT("snapshot_registry_version_invalid");
        return false;
    }
    if (Snapshot.PackRegistryVersion != ExpectedPackRegistryVersion)
    {
        OutError = TEXT("snapshot_registry_version_mismatch");
        return false;
    }
    if (Snapshot.State.Revision < 0)
    {
        OutError = TEXT("snapshot_revision_invalid");
        return false;
    }

    TSet<FName> PackIds;
    for (const FName PackId : Snapshot.State.GrantedPackIds)
    {
        if (PackId.IsNone() || PackIds.Contains(PackId))
        {
            OutError = TEXT("snapshot_pack_ids_invalid");
            return false;
        }
        PackIds.Add(PackId);
    }

    TSet<FString> GrantIds;
    TSet<FName> PacksWithGrantRecords;
    for (const FKnowledgeGrantRecord& Record : Snapshot.State.ProcessedGrants)
    {
        const FString NormalizedGrantId = Record.GrantEventId.TrimStartAndEnd();
        if (NormalizedGrantId.IsEmpty() || Record.GrantEventId != NormalizedGrantId
            || Record.PackId.IsNone() || GrantIds.Contains(NormalizedGrantId))
        {
            OutError = TEXT("snapshot_grant_ids_invalid");
            return false;
        }
        if (!PackIds.Contains(Record.PackId))
        {
            OutError = TEXT("snapshot_grant_pack_mismatch");
            return false;
        }
        GrantIds.Add(NormalizedGrantId);
        PacksWithGrantRecords.Add(Record.PackId);
    }
    if (PacksWithGrantRecords.Num() != PackIds.Num())
    {
        OutError = TEXT("snapshot_grant_history_incomplete");
        return false;
    }

    TMap<FName, const UKnowledgePack*> Registry;
    for (const UKnowledgePack* Pack : RegisteredPacks)
    {
        // Only granted packs participate in this restore. A broken but ungranted
        // candidate must not block loading an otherwise valid player save.
        if (!IsValid(Pack) || Pack->PackId.IsNone() || !PackIds.Contains(Pack->PackId)) continue;

        FString DefinitionError;
        if (!Pack->IsDefinitionValid(DefinitionError) || Registry.Contains(Pack->PackId))
        {
            OutError = TEXT("snapshot_registry_invalid");
            return false;
        }
        Registry.Add(Pack->PackId, Pack);
    }
    for (const FName PackId : PackIds)
    {
        if (!Registry.Contains(PackId))
        {
            OutError = TEXT("snapshot_pack_unregistered");
            return false;
        }
    }

    TSet<FName> Visiting;
    TSet<FName> Visited;
    TFunction<bool(FName)> ValidateClosure = [&](FName PackId)
    {
        if (Visited.Contains(PackId)) return true;
        if (Visiting.Contains(PackId))
        {
            OutError = TEXT("snapshot_prerequisite_cycle");
            return false;
        }
        Visiting.Add(PackId);
        const UKnowledgePack* const* Pack = Registry.Find(PackId);
        if (Pack == nullptr)
        {
            OutError = TEXT("snapshot_pack_unregistered");
            return false;
        }
        for (const FName RequiredPackId : (*Pack)->RequiredPackIds)
        {
            if (!PackIds.Contains(RequiredPackId))
            {
                OutError = TEXT("snapshot_prerequisite_missing");
                return false;
            }
            if (!ValidateClosure(RequiredPackId)) return false;
        }
        Visiting.Remove(PackId);
        Visited.Add(PackId);
        return true;
    };
    for (const FName PackId : PackIds)
    {
        if (!ValidateClosure(PackId)) return false;
    }

    FGameplayTagContainer DerivedTags;
    for (const FName PackId : PackIds)
    {
        const UKnowledgePack* Pack = Registry.FindChecked(PackId);
        if (Pack->UnlockTag.IsValid()) DerivedTags.AddTag(Pack->UnlockTag);
    }
    if (Snapshot.State.GrantedTags.Num() != DerivedTags.Num()
        || !Snapshot.State.GrantedTags.HasAllExact(DerivedTags))
    {
        OutError = TEXT("snapshot_granted_tags_mismatch");
        return false;
    }
    OutValidatedState = Snapshot.State;
    OutValidatedState.GrantedTags = MoveTemp(DerivedTags);
    return true;
}

FKnowledgeGrantResult UKnowledgeGrantComponent::Failure(const FString& Error, int32 Revision)
{
    FKnowledgeGrantResult Result;
    Result.Error = Error;
    Result.Revision = Revision;
    return Result;
}
