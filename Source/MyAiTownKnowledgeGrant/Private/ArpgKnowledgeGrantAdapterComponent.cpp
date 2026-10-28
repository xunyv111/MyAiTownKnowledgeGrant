// Copyright 2026 xunyv.

#include "ArpgKnowledgeGrantAdapterComponent.h"

#include "GameFramework/Actor.h"
#include "KnowledgePack.h"

bool UArpgKnowledgeGrantRegistry::IsDefinitionValid(FString& OutError) const
{
    if (RegistryVersion.IsNone() || Entries.IsEmpty())
    {
        OutError = TEXT("arpg_registry_invalid");
        return false;
    }
    TSet<FName> QuestIds;
    TSet<FName> PackIds;
    for (const FArpgKnowledgeRegistryEntry& Entry : Entries)
    {
        if (Entry.QuestId.IsNone() || !IsValid(Entry.KnowledgePack)
            || QuestIds.Contains(Entry.QuestId) || PackIds.Contains(Entry.KnowledgePack->PackId))
        {
            OutError = TEXT("arpg_registry_entry_invalid");
            return false;
        }
        FString PackError;
        if (!Entry.KnowledgePack->IsDefinitionValid(PackError)
            || Entry.KnowledgePack->SourceQuestId != Entry.QuestId)
        {
            OutError = TEXT("arpg_registry_pack_mismatch");
            return false;
        }
        QuestIds.Add(Entry.QuestId);
        PackIds.Add(Entry.KnowledgePack->PackId);
    }
    OutError.Empty();
    return true;
}

const UKnowledgePack* UArpgKnowledgeGrantRegistry::FindPack(FName QuestId) const
{
    for (const FArpgKnowledgeRegistryEntry& Entry : Entries)
    {
        if (Entry.QuestId == QuestId) return Entry.KnowledgePack;
    }
    return nullptr;
}

FArpgKnowledgeGrantAdapterResult UArpgKnowledgeGrantAdapterComponent::GrantCompletedQuestKnowledge(FName QuestId)
{
    UKnowledgeGrantComponent* PlayerKnowledge = ResolveKnowledgeComponent();
    const int32 CurrentRevision = IsValid(PlayerKnowledge) ? PlayerKnowledge->GetState().Revision : 0;
    const AActor* Owner = GetOwner();
    if (Owner == nullptr || !Owner->HasAuthority())
    {
        return Failure(QuestId, NAME_None, FString(), FString(), TEXT("authority_required"), CurrentRevision);
    }
    if (QuestId.IsNone())
    {
        return Failure(QuestId, NAME_None, FString(), FString(), TEXT("quest_id_invalid"), CurrentRevision);
    }
    if (!IsValid(PlayerKnowledge))
    {
        return Failure(QuestId, NAME_None, FString(), FString(), TEXT("knowledge_component_missing"), CurrentRevision);
    }
    FString RegistryError;
    if (!IsValid(Registry) || !Registry->IsDefinitionValid(RegistryError))
    {
        return Failure(QuestId, NAME_None, FString(), FString(),
            RegistryError.IsEmpty() ? TEXT("arpg_registry_missing") : RegistryError, CurrentRevision);
    }
    const UKnowledgePack* Pack = Registry->FindPack(QuestId);
    if (!IsValid(Pack))
    {
        return Failure(QuestId, NAME_None, FString(), FString(), TEXT("quest_not_registered"), CurrentRevision);
    }
    if (!IsValid(QuestAuthorityProvider)
        || !QuestAuthorityProvider->GetClass()->ImplementsInterface(UArpgQuestAuthorityProvider::StaticClass()))
    {
        return Failure(QuestId, Pack->PackId, FString(), FString(),
            TEXT("quest_authority_provider_missing"), CurrentRevision);
    }

    FString StablePlayerId;
    int32 CompletionRevision = 0;
    bool bDebugEvent = false;
    // Always dispatch through the interface thunk: calling ResolveCompletedQuest_Implementation
    // directly on a natively-cast pointer would skip Blueprint overrides of this BlueprintNativeEvent.
    const bool bCompleted = IArpgQuestAuthorityProvider::Execute_ResolveCompletedQuest(
        QuestAuthorityProvider, Owner, QuestId, StablePlayerId, CompletionRevision, bDebugEvent);
    StablePlayerId = StablePlayerId.TrimStartAndEnd();
    if (bDebugEvent)
    {
        return Failure(QuestId, Pack->PackId, StablePlayerId, FString(),
            TEXT("debug_quest_event_rejected"), CurrentRevision);
    }
    if (!bCompleted)
    {
        return Failure(QuestId, Pack->PackId, StablePlayerId, FString(),
            TEXT("quest_not_completed"), CurrentRevision);
    }
    if (StablePlayerId.IsEmpty() || StablePlayerId.Len() > 128 || StablePlayerId.Contains(TEXT("|")))
    {
        return Failure(QuestId, Pack->PackId, StablePlayerId, FString(),
            TEXT("stable_player_id_invalid"), CurrentRevision);
    }
    if (CompletionRevision <= 0)
    {
        return Failure(QuestId, Pack->PackId, StablePlayerId, FString(),
            TEXT("quest_completion_revision_invalid"), CurrentRevision);
    }

    const FString GrantEventId = BuildGrantEventId(StablePlayerId, QuestId, CompletionRevision);
    const int32 HighestRecordedRevision = FindHighestRecordedCompletionRevision(StablePlayerId, QuestId);
    if (HighestRecordedRevision > CompletionRevision)
    {
        return Failure(QuestId, Pack->PackId, StablePlayerId, GrantEventId,
            TEXT("quest_completion_revision_stale"), CurrentRevision);
    }

    const FKnowledgeGrantResult Grant = PlayerKnowledge->GrantKnowledgeFromQuest(
        GrantEventId, QuestId, const_cast<UKnowledgePack*>(Pack));
    FArpgKnowledgeGrantAdapterResult Result;
    Result.bOk = Grant.bOk;
    Result.bDuplicate = Grant.bDuplicate;
    Result.bPersistenceRequired = Grant.bOk && Grant.bApplied;
    Result.Error = Grant.Error;
    Result.GrantEventId = GrantEventId;
    Result.PackId = Pack->PackId;
    Result.Revision = Grant.Revision;
    RecordAudit(QuestId, StablePlayerId, Result);
    return Result;
}

FArpgKnowledgeGrantAdapterResult UArpgKnowledgeGrantAdapterComponent::Failure(
    FName QuestId, FName PackId, const FString& StablePlayerId,
    const FString& GrantEventId, const FString& Error, int32 Revision)
{
    FArpgKnowledgeGrantAdapterResult Result;
    Result.Error = Error;
    Result.GrantEventId = GrantEventId;
    Result.PackId = PackId;
    Result.Revision = Revision;
    RecordAudit(QuestId, StablePlayerId, Result);
    return Result;
}

void UArpgKnowledgeGrantAdapterComponent::RecordAudit(
    FName QuestId, const FString& StablePlayerId,
    const FArpgKnowledgeGrantAdapterResult& Result)
{
    FArpgKnowledgeGrantAuditRecord Record;
    Record.TimestampUtc = FDateTime::UtcNow();
    Record.StablePlayerId = StablePlayerId;
    Record.QuestId = QuestId;
    Record.PackId = Result.PackId;
    Record.GrantEventId = Result.GrantEventId;
    Record.bOk = Result.bOk;
    Record.bDuplicate = Result.bDuplicate;
    Record.bPersistenceRequired = Result.bPersistenceRequired;
    Record.Revision = Result.Revision;
    Record.Error = Result.Error;
    AuditRecords.Add(Record);
    const int32 Limit = FMath::Clamp(MaxAuditRecords, 1, 4096);
    if (AuditRecords.Num() > Limit)
    {
        AuditRecords.RemoveAt(0, AuditRecords.Num() - Limit, EAllowShrinking::No);
    }
    OnGrantAudited.Broadcast(Record);
}

UKnowledgeGrantComponent* UArpgKnowledgeGrantAdapterComponent::ResolveKnowledgeComponent() const
{
    if (IsValid(KnowledgeComponent)) return KnowledgeComponent;
    AActor* Owner = GetOwner();
    return Owner != nullptr ? Owner->FindComponentByClass<UKnowledgeGrantComponent>() : nullptr;
}

int32 UArpgKnowledgeGrantAdapterComponent::FindHighestRecordedCompletionRevision(
    const FString& StablePlayerId, FName QuestId) const
{
    const UKnowledgeGrantComponent* PlayerKnowledge = ResolveKnowledgeComponent();
    if (!IsValid(PlayerKnowledge)) return INDEX_NONE;
    const FString Prefix = FString::Printf(TEXT("arpg|%s|%s|"), *StablePlayerId, *QuestId.ToString());
    int32 Highest = INDEX_NONE;
    for (const FKnowledgeGrantRecord& Record : PlayerKnowledge->GetState().ProcessedGrants)
    {
        if (!Record.GrantEventId.StartsWith(Prefix, ESearchCase::CaseSensitive)) continue;
        int32 ParsedRevision = 0;
        if (LexTryParseString(ParsedRevision, *Record.GrantEventId.RightChop(Prefix.Len())))
        {
            Highest = FMath::Max(Highest, ParsedRevision);
        }
    }
    return Highest;
}

FString UArpgKnowledgeGrantAdapterComponent::BuildGrantEventId(
    const FString& StablePlayerId, FName QuestId, int32 CompletionRevision)
{
    return FString::Printf(TEXT("arpg|%s|%s|%d"),
        *StablePlayerId, *QuestId.ToString(), CompletionRevision);
}
