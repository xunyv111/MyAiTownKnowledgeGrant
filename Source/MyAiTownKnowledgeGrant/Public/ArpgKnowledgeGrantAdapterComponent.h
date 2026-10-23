#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "KnowledgeGrantComponent.h"
#include "UObject/Interface.h"
#include "ArpgKnowledgeGrantAdapterComponent.generated.h"

class UKnowledgePack;

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FArpgKnowledgeRegistryEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|ARPG")
    FName QuestId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|ARPG")
    TObjectPtr<UKnowledgePack> KnowledgePack;
};

/** Fixed server-authored QuestId -> KnowledgePack registry for an ARPG project. */
UCLASS(BlueprintType)
class MYAITOWNKNOWLEDGEGRANT_API UArpgKnowledgeGrantRegistry : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|ARPG")
    FName RegistryVersion;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|ARPG")
    TArray<FArpgKnowledgeRegistryEntry> Entries;

    bool IsDefinitionValid(FString& OutError) const;
    const UKnowledgePack* FindPack(FName QuestId) const;
};

/**
 * Implement this on a trusted server-side quest subsystem or adapter object.
 * The plugin never accepts a caller-supplied player id or completion revision.
 */
UINTERFACE(BlueprintType)
class MYAITOWNKNOWLEDGEGRANT_API UArpgQuestAuthorityProvider : public UInterface
{
    GENERATED_BODY()
};

class MYAITOWNKNOWLEDGEGRANT_API IArpgQuestAuthorityProvider
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Knowledge|ARPG")
    bool ResolveCompletedQuest(
        const AActor* PlayerOwner,
        FName QuestId,
        FString& OutStablePlayerId,
        int32& OutCompletionRevision,
        bool& bOutDebugEvent) const;
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FArpgKnowledgeGrantAuditRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") FDateTime TimestampUtc;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") FString StablePlayerId;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") FName QuestId;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") FName PackId;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") FString GrantEventId;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") bool bOk = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") bool bDuplicate = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") bool bPersistenceRequired = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") int32 Revision = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") FString Error;
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FArpgKnowledgeGrantAdapterResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") bool bOk = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") bool bDuplicate = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") bool bPersistenceRequired = false;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") FString Error;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") FString GrantEventId;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") FName PackId;
    UPROPERTY(BlueprintReadOnly, Category = "Knowledge|ARPG") int32 Revision = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FArpgKnowledgeGrantAuditedSignature, FArpgKnowledgeGrantAuditRecord, Record);

/**
 * Safe front door for quest-derived knowledge in an ARPG. It verifies the live
 * quest state through a trusted provider, resolves a fixed registry entry,
 * generates the idempotency key on the server, rejects debug/stale events, and
 * emits a transient audit/outbox event for project-owned persistence.
 */
UCLASS(ClassGroup = (Knowledge), meta = (BlueprintSpawnableComponent))
class MYAITOWNKNOWLEDGEGRANT_API UArpgKnowledgeGrantAdapterComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|ARPG")
    TObjectPtr<UArpgKnowledgeGrantRegistry> Registry;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|ARPG")
    TObjectPtr<UObject> QuestAuthorityProvider;

    /** Optional explicit component; otherwise the owner is searched. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|ARPG")
    TObjectPtr<UKnowledgeGrantComponent> KnowledgeComponent;

    /** Transient diagnostics only. Durable audit storage belongs to the host. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge|ARPG|Audit", meta = (ClampMin = "1", ClampMax = "4096"))
    int32 MaxAuditRecords = 256;

    UPROPERTY(BlueprintAssignable, Category = "Knowledge|ARPG|Audit")
    FArpgKnowledgeGrantAuditedSignature OnGrantAudited;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Knowledge|ARPG")
    FArpgKnowledgeGrantAdapterResult GrantCompletedQuestKnowledge(FName QuestId);

    UFUNCTION(BlueprintPure, Category = "Knowledge|ARPG|Audit")
    TArray<FArpgKnowledgeGrantAuditRecord> GetAuditRecords() const { return AuditRecords; }

private:
    UPROPERTY(Transient)
    TArray<FArpgKnowledgeGrantAuditRecord> AuditRecords;

    FArpgKnowledgeGrantAdapterResult Failure(
        FName QuestId, FName PackId, const FString& StablePlayerId,
        const FString& GrantEventId, const FString& Error, int32 Revision);
    void RecordAudit(FName QuestId, const FString& StablePlayerId,
        const FArpgKnowledgeGrantAdapterResult& Result);
    UKnowledgeGrantComponent* ResolveKnowledgeComponent() const;
    int32 FindHighestRecordedCompletionRevision(
        const FString& StablePlayerId, FName QuestId) const;
    static FString BuildGrantEventId(
        const FString& StablePlayerId, FName QuestId, int32 CompletionRevision);
};
