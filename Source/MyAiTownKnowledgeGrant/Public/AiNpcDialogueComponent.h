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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Privacy", meta=(ClampMin="1", ClampMax="200")) int32 MaxLocalTranscriptTurns = 24;

    UPROPERTY(BlueprintAssignable, Category="AI NPC") FAiNpcFinalReplySignature OnFinalReply;

    UFUNCTION(BlueprintCallable, Category="AI NPC") FGuid SendPlayerMessage(const FString& PlayerText, FString& OutError);
    UFUNCTION(BlueprintPure, Category="AI NPC") FGuid GetLocalSessionId() const { return SessionId; }
    UFUNCTION(BlueprintPure, Category="AI NPC|Privacy") TArray<FAiNpcLocalTurn> GetLocalTranscript() const { return LocalTranscript; }
    UFUNCTION(BlueprintCallable, Category="AI NPC|Privacy") void ClearLocalTranscript();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    struct FPendingTurn
    {
        FString PlayerText;
        FDialogueKnowledgeView KnowledgeView;
    };

    UPROPERTY(Transient) FGuid SessionId;
    UPROPERTY(Transient) TArray<FAiNpcLocalTurn> LocalTranscript;
    TMap<FGuid, FPendingTurn> PendingTurns;
    TMap<FString, FGuid> SemanticToProviderRequest;
    FGuid StartingSemanticForProviderRequest;

    UFUNCTION() void HandleProviderCompleted(FAiNpcProviderResult Result);
    UFUNCTION() void HandleSemanticReviewRequested(FKnowledgeSemanticReviewRequest Request);
    UFUNCTION() void HandleReviewProviderCompleted(FKnowledgeSemanticReviewResponse Response);
    UFUNCTION() void HandleSemanticReviewCompleted(FKnowledgeSemanticReviewResult Result);

    UAiNpcProviderSubsystem* GetProviderSubsystem() const;
    FString BuildAuthorizedContext(const FDialogueKnowledgeView& View) const;
    void BroadcastFallback(FGuid RequestId, const FString& Error, const FText& Fallback);
    void StoreApprovedTurn(const FString& PlayerText, const FString& NpcText);
};
