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

    /** Configures one NPC dialogue route. Secrets remain in this subsystem and are never copied to the NPC. */
    UFUNCTION(BlueprintCallable, Category="AI NPC|Provider")
    bool ConfigureNpcProvider(FName ProviderProfileId,
        const FAiNpcProviderConfiguration& Configuration,
        const FString& ApiKey, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="AI NPC|Provider") void ClearCredentials();
    /** Clears one shared channel without disconnecting per-NPC dialogue routes. */
    UFUNCTION(BlueprintCallable, Category="AI NPC|Provider")
    bool ClearProviderCredentials(EAiNpcProviderChannel Channel);
    /** Clears one NPC route without disconnecting other group members or the reviewer. */
    UFUNCTION(BlueprintCallable, Category="AI NPC|Provider")
    bool ClearNpcProviderCredentials(FName ProviderProfileId);
    UFUNCTION(BlueprintPure, Category="AI NPC|Provider") bool IsProviderReady(EAiNpcProviderChannel Channel) const;
    UFUNCTION(BlueprintPure, Category="AI NPC|Provider") bool IsNpcProviderReady(FName ProviderProfileId) const;

    UFUNCTION(BlueprintCallable, Category="AI NPC|Session")
    FGuid OpenNpcSession(const FAiNpcPersona& Persona);
    UFUNCTION(BlueprintCallable, Category="AI NPC|Session") void CloseNpcSession(FGuid SessionId);

    /** Sends no conversation history. Only role instructions, authorized turn facts and current player text leave the device. */
    UFUNCTION(BlueprintCallable, Category="AI NPC|Dialogue")
    FGuid RequestNpcDialogue(FGuid SessionId, const FString& PlayerText,
        const FString& AuthorizedTurnContext, FString& OutError);

    /** One bounded format-repair attempt for a model response that failed the strict JSON contract. */
    UFUNCTION(BlueprintCallable, Category="AI NPC|Dialogue")
    FGuid RequestNpcDialogueFormatRepair(FGuid SessionId, const FString& MalformedPayload,
        const FString& AuthorizedTurnContext, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="AI NPC|Semantic Review")
    bool RequestSemanticReview(const FKnowledgeSemanticReviewRequest& ReviewRequest, FString& OutError);

    UFUNCTION(BlueprintPure, Category="AI NPC|Privacy")
    static bool ValidateConfiguration(const FAiNpcProviderConfiguration& Configuration, FString& OutError);

    /** Deterministic helpers used by adapters and automation; neither accepts or returns API keys. */
    static bool BuildDialogueRequestBody(const FAiNpcProviderConfiguration& Configuration,
        const FAiNpcPersona& Persona, const FString& PlayerText,
        const FString& AuthorizedTurnContext, FString& OutJson, FString& OutError);
    static bool BuildDialogueRepairRequestBody(const FAiNpcProviderConfiguration& Configuration,
        const FAiNpcPersona& Persona, const FString& MalformedPayload,
        const FString& AuthorizedTurnContext, FString& OutJson, FString& OutError);
    static bool ParseDialogueResponse(const FString& ResponseJson, FString& OutPayload, FString& OutError);
    static bool BuildReviewRequestBody(const FAiNpcProviderConfiguration& Configuration,
        const FKnowledgeSemanticReviewRequest& ReviewRequest, FString& OutJson, FString& OutError,
        const FString& PreviousValidationError = FString());
    static bool ParseReviewResponse(const FString& ResponseJson,
        const FKnowledgeSemanticReviewRequest& OriginalRequest,
        const FAiNpcProviderConfiguration& Configuration,
        FKnowledgeSemanticReviewResponse& OutResponse, FString& OutError);
    /** Deterministic retry policy used by runtime transport and fault-injection tests. */
    static bool IsRetryableReviewErrorCode(const FString& Error);

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

    struct FPendingReviewRequest
    {
        FKnowledgeSemanticReviewRequest Request;
        int32 RetryCount = 0;
        double StartedAtSeconds = 0.0;
    };

    FRuntimeProvider DialogueProvider;
    FRuntimeProvider ReviewProvider;
    TMap<FName, FRuntimeProvider> NpcDialogueProviders;
    TMap<FGuid, FNpcSession> Sessions;
    TMap<FGuid, FGuid> RequestToSession;
    TMap<FString, FPendingReviewRequest> PendingReviews;

    FRuntimeProvider& ProviderFor(EAiNpcProviderChannel Channel);
    const FRuntimeProvider& ProviderFor(EAiNpcProviderChannel Channel) const;
    const FRuntimeProvider* DialogueProviderForSession(const FNpcSession& Session) const;
    bool ValidateIndependentPair(EAiNpcProviderChannel Channel,
        const FAiNpcProviderConfiguration& Configuration, FString& OutError) const;
    FGuid DispatchDialogueRequest(FGuid SessionId, const FString& Body,
        const FRuntimeProvider& Provider, FString& OutError);
    void HandleDialogueHttpComplete(TSharedPtr<class IHttpRequest> Request,
        TSharedPtr<class IHttpResponse> Response, bool bConnectedSuccessfully, FGuid RequestId);
    void HandleReviewHttpComplete(TSharedPtr<class IHttpRequest> Request,
        TSharedPtr<class IHttpResponse> Response, bool bConnectedSuccessfully, FString ReviewRequestId);

#if WITH_DEV_AUTOMATION_TESTS
    friend class FKnowledgeMultiProviderRoutingTest;
#endif
};
