#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KnowledgeReaction.h"
#include "NpcKnowledgeReceiverComponent.generated.h"

class UKnowledgeGrantComponent;
class UKnowledgePack;

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FNpcKnowledgeReceiptRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Reaction") FString AudienceId;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Reaction") FString ReceiptEventId;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Reaction") FName PackId;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Reaction") TArray<FNpcKnowledgeReaction> Reactions;
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FNpcKnowledgeReactionSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Reaction") int32 SchemaVersion = 3;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Reaction") int32 Revision = 0;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Reaction") TArray<FNpcKnowledgeReceiptRecord> Receipts;
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FNpcKnowledgeReceiptResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Reaction") bool bOk = false;
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") bool bDuplicate = false;
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") FString Error;
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") int32 Revision = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Reaction") TArray<FNpcKnowledgeReaction> Reactions;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FNpcKnowledgeReceivedSignature, FString, AudienceId, FString, ReceiptEventId, int32, NewRevision);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FNpcReactionMemoryRestoredSignature, int32, NewRevision);

/** Lazily records how one NPC first reacted to an unlocked knowledge pack. */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class MYAITOWNKNOWLEDGEGRANT_API UNpcKnowledgeReceiverComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNpcKnowledgeReceiverComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reaction")
    TObjectPtr<UNpcPersonalityProfile> PersonalityProfile;

    UPROPERTY(BlueprintAssignable, Category = "Reaction")
    FNpcKnowledgeReceivedSignature OnKnowledgeReceived;

    UPROPERTY(BlueprintAssignable, Category = "Reaction")
    FNpcReactionMemoryRestoredSignature OnReactionMemoryRestored;

    /** 0 = unlimited. When full, new receipts fail closed instead of dropping old memory. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reaction|Retention", meta = (ClampMin = "0"))
    int32 MaxReceiptRecords = 0;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Knowledge|Reaction")
    FNpcKnowledgeReceiptResult ReceiveUnlockedKnowledgePack(
        const FString& AudienceId,
        const FString& ReceiptEventId,
        const UKnowledgeGrantComponent* PlayerKnowledge,
        UKnowledgePack* KnowledgePack,
        const FNpcAffectContext& AffectContext);

    UFUNCTION(BlueprintPure, Category = "Knowledge|Reaction")
    bool HasReceivedEventForAudience(const FString& AudienceId, const FString& ReceiptEventId) const;

    UFUNCTION(BlueprintPure, Category = "Knowledge|Reaction")
    FNpcKnowledgeReactionSnapshot CreateReactionSnapshot() const;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Knowledge|Reaction")
    FNpcKnowledgeReceiptResult RestoreReactionSnapshot(const FNpcKnowledgeReactionSnapshot& Snapshot);

private:
    UPROPERTY(SaveGame)
    FNpcKnowledgeReactionSnapshot State;

    bool HasAuthorityForMutation() const;
    static bool IsSnapshotValid(const FNpcKnowledgeReactionSnapshot& Snapshot, FString& OutError);
    static FNpcKnowledgeReceiptResult Failure(const FString& Error, int32 Revision);
};
