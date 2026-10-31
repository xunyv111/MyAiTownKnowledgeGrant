#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AiNpcProviderTypes.h"
#include "KnowledgeResolverLibrary.h"
#include "AiNpcDialogueComponent.generated.h"

class UAiNpcProviderSubsystem;
class UKnowledgeContextCollectorComponent;
class UKnowledgeDialogueGuardPolicy;
class UKnowledgeGrantComponent;
class UKnowledgePack;
class UKnowledgeSemanticReviewComponent;
class UKnowledgeSemanticReviewPolicy;

/**
 * Add one component to each NPC. It automatically owns an isolated local
 * provider session, but all NPCs share the GameInstance provider gateway and
 * its process-memory credentials.
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class MYAITOWNKNOWLEDGEGRANT_API UAiNpcDialogueComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC") FAiNpcPersona Persona;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Knowledge") TObjectPtr<UKnowledgeGrantComponent> PlayerKnowledge;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Knowledge") TArray<TObjectPtr<UKnowledgePack>> CandidatePacks;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Knowledge") TArray<TObjectPtr<UKnowledgeContextCollectorComponent>> ContextCollectors;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Guard") TObjectPtr<UKnowledgeDialogueGuardPolicy> GuardPolicy;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Guard") TObjectPtr<UKnowledgeSemanticReviewComponent> SemanticReviewComponent;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Guard") TObjectPtr<UKnowledgeSemanticReviewPolicy> SemanticReviewPolicy;
    /** If true and no host listener is bound, this component sends the review request to the built-in provider. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Guard") bool bUseBuiltInSemanticReviewProvider = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Privacy", meta=(ClampMin="1", ClampMax="200")) int32 MaxLocalTranscriptTurns = 24;
    /** Maximum completed client-request records retained for idempotent replay. Oldest completed records are pruned. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Request", meta=(ClampMin="1", ClampMax="4096")) int32 MaxClientRequestRecords = 128;

    UPROPERTY(BlueprintAssignable, Category="AI NPC") FAiNpcFinalReplySignature OnFinalReply;

    UFUNCTION(BlueprintCallable, Category="AI NPC") FGuid SendPlayerMessage(const FString& PlayerText, FString& OutError);
    /**
     * Idempotent request entry point. A pending duplicate returns the original request id and waits for its callback;
     * a completed duplicate replays the cached OnFinalReply. Conflicting input for the same id fails closed.
     */
    UFUNCTION(BlueprintCallable, Category="AI NPC|Request")
    FGuid SendPlayerMessageWithClientRequestId(
        const FString& PlayerText, FGuid ClientRequestId, FString& OutError);
    UFUNCTION(BlueprintPure, Category="AI NPC|Request")
    bool IsClientRequestPending(FGuid ClientRequestId) const;
    UFUNCTION(BlueprintPure, Category="AI NPC|Request")
    bool TryGetClientRequestResult(FGuid ClientRequestId, FAiNpcFinalReply& OutReply) const;
    /** Broadcasts a cached completed reply with bDuplicateRequest=true. */
    UFUNCTION(BlueprintCallable, Category="AI NPC|Request")
    bool ReplayClientRequestResult(FGuid ClientRequestId, FString& OutError);
    UFUNCTION(BlueprintPure, Category="AI NPC|Request")
    bool HasPendingRequest() const { return !PendingTurns.IsEmpty(); }
    /** Atomically replaces the persona captured by the provider session. Pending replies from the old session are ignored. */
    UFUNCTION(BlueprintCallable, Category="AI NPC")
    bool ApplyPersona(const FAiNpcPersona& NewPersona, bool bClearTranscript, FString& OutError);
    UFUNCTION(BlueprintPure, Category="AI NPC") FGuid GetLocalSessionId() const { return SessionId; }
    UFUNCTION(BlueprintPure, Category="AI NPC") int32 GetPersonaRevision() const { return PersonaRevision; }
    UFUNCTION(BlueprintPure, Category="AI NPC|Privacy") TArray<FAiNpcLocalTurn> GetLocalTranscript() const { return LocalTranscript; }
    UFUNCTION(BlueprintCallable, Category="AI NPC|Privacy") void ClearLocalTranscript();
    /** Selects one current-view FactId deterministically; it never invents or rewrites fact text. */
    static FString BuildAuthorizedFactFallback(
        const FString& PlayerText, const FDialogueKnowledgeView& KnowledgeView,
        const FText& DefaultFallback, TArray<FName>& OutEvidenceFactIds);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    struct FPendingTurn
    {
        FString PlayerText;
        FString AuthorizedContext;
        FDialogueKnowledgeView KnowledgeView;
        FGuid OriginalRequestId;
        FGuid ClientRequestId;
        int32 PersonaRevision = 0;
        bool bProviderRetryAttempted = false;
        bool bFormatRepairAttempted = false;
    };

    struct FClientRequestRecord
    {
        FString NormalizedInput;
        FGuid AuthorizationEpoch;
        int64 ScopeFingerprint = 0;
        FGuid OriginalRequestId;
        bool bCompleted = false;
        double CompletedAtSeconds = 0.0;
        FAiNpcFinalReply CompletedReply;
    };

    UPROPERTY(Transient) FGuid SessionId;
    UPROPERTY(Transient) int32 PersonaRevision = 1;
    UPROPERTY(Transient) TArray<FAiNpcLocalTurn> LocalTranscript;
    TMap<FGuid, FPendingTurn> PendingTurns;
    TMap<FGuid, FClientRequestRecord> ClientRequests;
    TMap<FString, FGuid> SemanticToProviderRequest;

    UFUNCTION() void HandleProviderCompleted(FAiNpcProviderResult Result);
    UFUNCTION() void HandleReviewProviderCompleted(FKnowledgeSemanticReviewResponse Response);
    UFUNCTION() void HandleSemanticReviewCompleted(FKnowledgeSemanticReviewResult Result);

    UAiNpcProviderSubsystem* GetProviderSubsystem() const;
    FString BuildAuthorizedContext(const FDialogueKnowledgeView& View) const;
    FString BuildDeterministicAuthorizedFallback(
        const FPendingTurn* Turn, const FText& DefaultFallback, TArray<FName>& OutEvidenceFactIds) const;
    void BroadcastFallback(FGuid RequestId, const FString& Error, const FText& Fallback,
        const FPendingTurn* Turn = nullptr);
    void StampReplyFromTurn(FAiNpcFinalReply& Reply, const FPendingTurn* Turn) const;
    void CacheCompletedReply(const FPendingTurn* Turn, const FAiNpcFinalReply& Reply);
    void PruneCompletedClientRequests(int32 MaxRecords);
    bool TryResolveExistingClientRequest(
        FGuid ClientRequestId, const FString& NormalizedInput,
        const FDialogueKnowledgeView& View, FGuid& OutRequestId, FString& OutError) const;
    bool IsPendingTurnCurrent(const FPendingTurn& Turn, FString& OutError) const;
    bool CorrelateSemanticRequest(const FString& ReviewRequestId, FGuid DialogueRequestId);
    void DiscardStaleTurn(FGuid ProviderRequestId, const FPendingTurn& Turn, const FString& Error);
    void StoreApprovedTurn(const FString& PlayerText, const FString& NpcText);

#if WITH_DEV_AUTOMATION_TESTS
    friend class FKnowledgeRequestIdempotencyTest;
    friend class FKnowledgeStaleResponseRejectionTest;
    friend class FKnowledgeSemanticRequestCorrelationTest;
#endif
};
