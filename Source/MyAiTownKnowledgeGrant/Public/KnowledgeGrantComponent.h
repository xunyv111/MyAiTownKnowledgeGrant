// Copyright 2026 xunyv.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "KnowledgeGrantComponent.generated.h"

class UKnowledgePack;

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgeGrantRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Knowledge") FString GrantEventId;
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Knowledge") FName PackId;
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgeGrantState
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Knowledge")
    TArray<FName> GrantedPackIds;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Knowledge")
    FGameplayTagContainer GrantedTags;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Knowledge")
    TArray<FKnowledgeGrantRecord> ProcessedGrants;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Knowledge")
    int32 Revision = 0;

    /** Changes whenever the effective authorization state is replaced or mutated. */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Knowledge")
    FGuid AuthorizationEpoch;
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgeGrantSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Knowledge")
    int32 SchemaVersion = 3;

    /** Version of the exact pack registry used when this snapshot was created. */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Knowledge")
    FName PackRegistryVersion;

    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Knowledge")
    FKnowledgeGrantState State;
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FKnowledgeGrantResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") bool bOk = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") bool bApplied = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") bool bDuplicate = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") FString Error;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge") int32 Revision = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FKnowledgeChangedSignature, int32, NewRevision);

/**
 * Attach to a persistent player-owned actor in standalone games, or to the
 * authoritative PlayerState in networked games. Quest systems call
 * GrantKnowledgeFromQuest on completion. World simulation systems call
 * GrantKnowledgeFromWorldEvent only after an event has actually happened.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class MYAITOWNKNOWLEDGEGRANT_API UKnowledgeGrantComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKnowledgeGrantComponent();

    UPROPERTY(BlueprintAssignable, Category = "Knowledge")
    FKnowledgeChangedSignature OnKnowledgeChanged;

    /** 0 = unlimited. When full, new grants fail closed instead of silently dropping history. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|Retention", meta = (ClampMin = "0"))
    int32 MaxProcessedGrantRecords = 0;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Knowledge")
    FKnowledgeGrantResult GrantKnowledgeFromQuest(const FString& GrantEventId, FName CompletedQuestId, UKnowledgePack* KnowledgePack);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Knowledge")
    FKnowledgeGrantResult GrantKnowledgeFromWorldEvent(const FString& GrantEventId, FName OccurredWorldEventId, UKnowledgePack* KnowledgePack);

    UFUNCTION(BlueprintPure, Category = "Knowledge")
    bool HasKnowledgePack(FName PackId) const;

    UFUNCTION(BlueprintPure, Category = "Knowledge")
    bool HasKnowledgeTag(FGameplayTag KnowledgeTag) const;

    /**
     * Removes one granted pack and its grant history. Fails if another granted
     * pack still depends on it. RegisteredPacks is used to re-derive tags.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Knowledge")
    FKnowledgeGrantResult RevokeKnowledgePack(FName PackId, const TArray<UKnowledgePack*>& RegisteredPacks);

    UFUNCTION(BlueprintPure, Category = "Knowledge")
    FKnowledgeGrantSnapshot CreateSnapshot(FName PackRegistryVersion) const;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Knowledge")
    FKnowledgeGrantResult RestoreSnapshot(const FKnowledgeGrantSnapshot& Snapshot,
        const TArray<UKnowledgePack*>& RegisteredPacks, FName ExpectedPackRegistryVersion);

    const FKnowledgeGrantState& GetState() const { return State; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY(ReplicatedUsing = OnRep_State, SaveGame)
    FKnowledgeGrantState State;

    UFUNCTION()
    void OnRep_State();

    bool HasAuthorityForMutation() const;
    bool ArePrerequisitesSatisfied(const UKnowledgePack* KnowledgePack) const;
    FKnowledgeGrantResult ApplyGrant(const FString& GrantEventId, UKnowledgePack* KnowledgePack);
    static bool ValidateSnapshotAgainstRegistry(const FKnowledgeGrantSnapshot& Snapshot,
        const TArray<UKnowledgePack*>& RegisteredPacks, FName ExpectedPackRegistryVersion,
        FKnowledgeGrantState& OutValidatedState, FString& OutError);
    static FKnowledgeGrantResult Failure(const FString& Error, int32 Revision);
};
