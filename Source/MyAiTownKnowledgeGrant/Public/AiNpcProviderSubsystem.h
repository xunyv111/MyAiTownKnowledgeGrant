#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AiNpcProviderTypes.h"
#include "AiNpcProviderSubsystem.generated.h"

/**
 * Player-owned, local provider gateway. Secrets live only in this subsystem's
 * process memory. NPCs receive isolated session IDs but share HTTP connections
 * and never receive or duplicate the API key.
 */
UCLASS()
class MYAITOWNKNOWLEDGEGRANT_API UAiNpcProviderSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category="AI NPC|Provider") FAiNpcProviderCompletedSignature OnDialogueProviderCompleted;
    UPROPERTY(BlueprintAssignable, Category="AI NPC|Provider") FAiNpcReviewProviderCompletedSignature OnReviewProviderCompleted;

    UFUNCTION(BlueprintCallable, Category="AI NPC|Provider")
    bool ConfigureProvider(EAiNpcProviderChannel Channel,
        const FAiNpcProviderConfiguration& Configuration,
        const FString& ApiKey, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="AI NPC|Provider") void ClearCredentials();
    UFUNCTION(BlueprintPure, Category="AI NPC|Provider") bool IsProviderReady(EAiNpcProviderChannel Channel) const;

    UFUNCTION(BlueprintCallable, Category="AI NPC|Session")
    FGuid OpenNpcSession(const FAiNpcPersona& Persona);
    UFUNCTION(BlueprintCallable, Category="AI NPC|Session") void CloseNpcSession(FGuid SessionId);

    /** Sends no conversation history. Only role instructions, authorized turn facts and current player text leave the device. */
    UFUNCTION(BlueprintCallable, Category="AI NPC|Dialogue")
    FGuid RequestNpcDialogue(FGuid SessionId, const FString& PlayerText,
        const FString& AuthorizedTurnContext, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="AI NPC|Semantic Review")
    bool RequestSemanticReview(const FKnowledgeSemanticReviewRequest& ReviewRequest, FString& OutError);

    UFUNCTION(BlueprintPure, Category="AI NPC|Privacy")
    static bool ValidateConfiguration(const FAiNpcProviderConfiguration& Configuration, FString& OutError);

    /** Deterministic helpers used by adapters and automation; neither accepts or returns API keys. */
    static bool BuildDialogueRequestBody(const FAiNpcProviderConfiguration& Configuration,
        const FAiNpcPersona& Persona, const FString& PlayerText,
        const FString& AuthorizedTurnContext, FString& OutJson, FString& OutError);
    static bool ParseDialogueResponse(const FString& ResponseJson, FString& OutPayload, FString& OutError);
    static bool BuildReviewRequestBody(const FAiNpcProviderConfiguration& Configuration,
        const FKnowledgeSemanticReviewRequest& ReviewRequest, FString& OutJson, FString& OutError);
    static bool ParseReviewResponse(const FString& ResponseJson,
        const FKnowledgeSemanticReviewRequest& OriginalRequest,
        const FAiNpcProviderConfiguration& Configuration,
        FKnowledgeSemanticReviewResponse& OutResponse, FString& OutError);

private:
    struct FRuntimeProvider
    {
        FAiNpcProviderConfiguration Configuration;
        FString ApiKey;
        bool bConfigured = false;
    };

    struct FNpcSession
    {
        FAiNpcPersona Persona;
        TSet<FGuid> PendingRequests;
    };

    FRuntimeProvider DialogueProvider;
    FRuntimeProvider ReviewProvider;
    TMap<FGuid, FNpcSession> Sessions;
    TMap<FGuid, FGuid> RequestToSession;
    TMap<FString, FKnowledgeSemanticReviewRequest> PendingReviews;

    FRuntimeProvider& ProviderFor(EAiNpcProviderChannel Channel);
    const FRuntimeProvider& ProviderFor(EAiNpcProviderChannel Channel) const;
    void HandleDialogueHttpComplete(TSharedPtr<class IHttpRequest> Request,
        TSharedPtr<class IHttpResponse> Response, bool bConnectedSuccessfully, FGuid RequestId);
    void HandleReviewHttpComplete(TSharedPtr<class IHttpRequest> Request,
        TSharedPtr<class IHttpResponse> Response, bool bConnectedSuccessfully, FString ReviewRequestId);
};
