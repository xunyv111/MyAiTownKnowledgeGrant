// Copyright 2026 xunyv.

#include "NpcKnowledgeReceiverComponent.h"

#include "GameFramework/Actor.h"
#include "KnowledgeGrantComponent.h"
#include "KnowledgePack.h"

UNpcKnowledgeReceiverComponent::UNpcKnowledgeReceiverComponent()
{
    SetIsReplicatedByDefault(false);
}

FNpcKnowledgeReceiptResult UNpcKnowledgeReceiverComponent::ReceiveUnlockedKnowledgePack(
    const FString& AudienceId, const FString& ReceiptEventId,
    const UKnowledgeGrantComponent* PlayerKnowledge,
    UKnowledgePack* KnowledgePack,
    const FNpcAffectContext& AffectContext)
{
    if (!HasAuthorityForMutation()) return Failure(TEXT("authority_required"), State.Revision);
    const FString NormalizedAudienceId = AudienceId.TrimStartAndEnd();
    const FString NormalizedReceiptEventId = ReceiptEventId.TrimStartAndEnd();
    if (NormalizedAudienceId.IsEmpty()) return Failure(TEXT("audience_id_invalid"), State.Revision);
    if (NormalizedReceiptEventId.IsEmpty()) return Failure(TEXT("receipt_event_id_invalid"), State.Revision);
    if (!IsValid(PersonalityProfile)) return Failure(TEXT("personality_missing"), State.Revision);
    if (!IsValid(PlayerKnowledge) || !IsValid(KnowledgePack)) return Failure(TEXT("knowledge_input_invalid"), State.Revision);

    for (const FNpcKnowledgeReceiptRecord& Existing : State.Receipts)
    {
        if (Existing.AudienceId == NormalizedAudienceId
            && Existing.ReceiptEventId == NormalizedReceiptEventId)
        {
            if (Existing.PackId != KnowledgePack->PackId)
            {
                return Failure(TEXT("receipt_event_id_conflict"), State.Revision);
            }
            FNpcKnowledgeReceiptResult Duplicate;
            Duplicate.bOk = true;
            Duplicate.bDuplicate = true;
            Duplicate.Revision = State.Revision;
            Duplicate.Reactions = Existing.Reactions;
            for (FNpcKnowledgeReaction& Reaction : Duplicate.Reactions) Reaction.bDuplicate = true;
            return Duplicate;
        }
    }

    FString PackError;
    if (!KnowledgePack->IsDefinitionValid(PackError)) return Failure(PackError, State.Revision);
    if (!PlayerKnowledge->HasKnowledgePack(KnowledgePack->PackId)) return Failure(TEXT("knowledge_pack_locked"), State.Revision);
    if (MaxReceiptRecords > 0 && State.Receipts.Num() >= MaxReceiptRecords)
    {
        return Failure(TEXT("receipt_history_full"), State.Revision);
    }

    FNpcKnowledgeReceiptRecord Record;
    Record.AudienceId = NormalizedAudienceId;
    Record.ReceiptEventId = NormalizedReceiptEventId;
    Record.PackId = KnowledgePack->PackId;
    for (const FKnowledgeDocument& Document : KnowledgePack->Documents)
    {
        FNpcKnowledgeReaction Reaction = UKnowledgeReactionLibrary::EvaluateKnowledgeReaction(PersonalityProfile, Document, AffectContext);
        if (!Reaction.bOk) return Failure(Reaction.Error, State.Revision);
        Record.Reactions.Add(Reaction);
    }

    State.Receipts.Add(Record);
    ++State.Revision;

    FNpcKnowledgeReceiptResult Result;
    Result.bOk = true;
    Result.Revision = State.Revision;
    Result.Reactions = Record.Reactions;
    OnKnowledgeReceived.Broadcast(NormalizedAudienceId, NormalizedReceiptEventId, State.Revision);
    return Result;
}

bool UNpcKnowledgeReceiverComponent::HasReceivedEventForAudience(
    const FString& AudienceId, const FString& ReceiptEventId) const
{
    const FString NormalizedAudienceId = AudienceId.TrimStartAndEnd();
    const FString NormalizedReceiptEventId = ReceiptEventId.TrimStartAndEnd();
    return State.Receipts.ContainsByPredicate(
        [&NormalizedAudienceId, &NormalizedReceiptEventId](const FNpcKnowledgeReceiptRecord& Record)
        {
            return Record.AudienceId == NormalizedAudienceId
                && Record.ReceiptEventId == NormalizedReceiptEventId;
        });
}

FNpcKnowledgeReactionSnapshot UNpcKnowledgeReceiverComponent::CreateReactionSnapshot() const
{
    return State;
}

FNpcKnowledgeReceiptResult UNpcKnowledgeReceiverComponent::RestoreReactionSnapshot(const FNpcKnowledgeReactionSnapshot& Snapshot)
{
    if (!HasAuthorityForMutation()) return Failure(TEXT("authority_required"), State.Revision);
    FString Error;
    if (!IsSnapshotValid(Snapshot, Error)) return Failure(Error, State.Revision);
    if (MaxReceiptRecords > 0 && Snapshot.Receipts.Num() > MaxReceiptRecords)
    {
        return Failure(TEXT("reaction_snapshot_history_limit_exceeded"), State.Revision);
    }
    if (State.Revision == MAX_int32 || Snapshot.Revision == MAX_int32)
    {
        return Failure(TEXT("reaction_snapshot_revision_overflow"), State.Revision);
    }
    const int32 CurrentRevision = State.Revision;
    State = Snapshot;
    State.Revision = FMath::Max(CurrentRevision, Snapshot.Revision) + 1;
    FNpcKnowledgeReceiptResult Result;
    Result.bOk = true;
    Result.Revision = State.Revision;
    OnReactionMemoryRestored.Broadcast(State.Revision);
    return Result;
}

bool UNpcKnowledgeReceiverComponent::HasAuthorityForMutation() const
{
    const AActor* Owner = GetOwner();
    return Owner != nullptr && Owner->HasAuthority();
}

bool UNpcKnowledgeReceiverComponent::IsSnapshotValid(const FNpcKnowledgeReactionSnapshot& Snapshot, FString& OutError)
{
    if (Snapshot.SchemaVersion != 3)
    {
        OutError = TEXT("reaction_snapshot_schema_unsupported");
        return false;
    }
    if (Snapshot.Revision < 0)
    {
        OutError = TEXT("reaction_snapshot_revision_invalid");
        return false;
    }

    TMap<FString, TSet<FString>> ReceiptIdsByAudience;
    for (const FNpcKnowledgeReceiptRecord& Record : Snapshot.Receipts)
    {
        TSet<FString>& AudienceReceiptIds = ReceiptIdsByAudience.FindOrAdd(Record.AudienceId);
        if (Record.AudienceId.TrimStartAndEnd().IsEmpty()
            || Record.AudienceId != Record.AudienceId.TrimStartAndEnd()
            || Record.ReceiptEventId.TrimStartAndEnd().IsEmpty()
            || Record.ReceiptEventId != Record.ReceiptEventId.TrimStartAndEnd() || Record.PackId.IsNone()
            || AudienceReceiptIds.Contains(Record.ReceiptEventId) || Record.Reactions.IsEmpty())
        {
            OutError = TEXT("reaction_snapshot_record_invalid");
            return false;
        }
        AudienceReceiptIds.Add(Record.ReceiptEventId);
        TSet<FName> DocumentIds;
        for (const FNpcKnowledgeReaction& Reaction : Record.Reactions)
        {
            static const TSet<FName> ValidTones{
                TEXT("fearful"), TEXT("hostile"), TEXT("disapproving"), TEXT("urgent"),
                TEXT("curious"), TEXT("approving"), TEXT("withdrawn"), TEXT("neutral")};
            static const TSet<FName> ValidIntents{
                TEXT("confront"), TEXT("seek_safety"), TEXT("investigate"),
                TEXT("protect_others"), TEXT("support"), TEXT("observe")};
            if (!Reaction.bOk || Reaction.bDuplicate || !Reaction.Error.IsEmpty()
                || Reaction.DocumentId.IsNone() || DocumentIds.Contains(Reaction.DocumentId)
                || Reaction.Interest < 0 || Reaction.Interest > 100
                || Reaction.Approval < -100 || Reaction.Approval > 100
                || Reaction.PerceivedThreat < 0 || Reaction.PerceivedThreat > 100
                || Reaction.Intensity < 0 || Reaction.Intensity > 100
                || !ValidTones.Contains(Reaction.DialogueTone)
                || !ValidIntents.Contains(Reaction.BehaviorIntent)
                || Reaction.SuggestedValenceDelta < -15 || Reaction.SuggestedValenceDelta > 15
                || Reaction.SuggestedArousalDelta < -10 || Reaction.SuggestedArousalDelta > 15)
            {
                OutError = TEXT("reaction_snapshot_reaction_invalid");
                return false;
            }
            DocumentIds.Add(Reaction.DocumentId);
        }
    }
    return true;
}

FNpcKnowledgeReceiptResult UNpcKnowledgeReceiverComponent::Failure(const FString& Error, int32 Revision)
{
    FNpcKnowledgeReceiptResult Result;
    Result.Error = Error;
    Result.Revision = Revision;
    return Result;
}
