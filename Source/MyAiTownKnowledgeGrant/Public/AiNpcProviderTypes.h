#pragma once

#include "CoreMinimal.h"
#include "KnowledgeSemanticReviewComponent.h"
#include "AiNpcProviderTypes.generated.h"

UENUM(BlueprintType)
enum class EAiNpcProviderChannel : uint8
{
    DialogueGeneration,
    SemanticReview
};

/** Non-secret runtime connection settings. API keys are held only in process memory. */
USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FAiNpcProviderConfiguration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Provider") FString ProviderName;
    /** Full OpenAI-compatible chat-completions URL. HTTPS is required except for loopback. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Provider") FString ChatCompletionsUrl;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Provider") FString Model;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Provider", meta=(ClampMin="1.0", ClampMax="120.0")) float TimeoutSeconds = 30.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Provider", meta=(ClampMin="16", ClampMax="8192")) int32 MaxOutputTokens = 512;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Provider", meta=(ClampMin="0.0", ClampMax="2.0")) float Temperature = 0.7f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Provider") bool bAllowInsecureLoopback = true;
    /** Must be set by the player after seeing the disclosure. Never inferred or persisted. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Privacy") bool bPlayerConsentedToProviderTransmission = false;

    /** Non-reflected request hint for providers that support JSON object response mode. */
    bool bRequestJsonObjectResponse = false;

    /**
     * Provider compatibility hint for strict JSON contracts. DeepSeek V4 enables
     * thinking by default, which can consume the output budget before content.
     */
    bool bDisableThinkingForJsonContract = false;
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FAiNpcPersona
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC") FName NpcId;
    /** Selects an in-memory dialogue provider profile. None uses the shared default profile. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Provider") FName ProviderProfileId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC") FString DisplayName;
    /** Public role instructions sent to the selected provider on each request. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC", meta=(MultiLine=true)) FString RoleInstructions;

    /**
     * Opt-in for non-factual in-character dialogue without fact citations.
     * World-state assertions still require authorized evidence and the guard
     * policy must independently opt in before such dialogue can be released.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI NPC|Guard")
    bool bAllowUncitedRoleplay = false;
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FAiNpcProviderResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="AI NPC") bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC") FGuid RequestId;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC") FGuid SessionId;
    /** Untrusted strict-JSON candidate. It must pass the mechanical and semantic gates. */
    UPROPERTY(BlueprintReadOnly, Category="AI NPC") FString RawProviderPayload;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC") FString Error;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC") int32 HttpStatus = 0;
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FAiNpcFinalReply
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="AI NPC") bool bCompleted = false;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC") bool bApproved = false;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC") bool bUsedTrustedFallback = false;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC") FGuid RequestId;
    /** Stable caller-owned idempotency key. Never contains prompt or credential data. */
    UPROPERTY(BlueprintReadOnly, Category="AI NPC|Request") FGuid ClientRequestId;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC|Request") bool bDuplicateRequest = false;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC|Request") bool bProviderRetryAttempted = false;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC|Request") bool bFormatRepairAttempted = false;
    /** The only field intended for subtitles, voice and behavior outlets. */
    UPROPERTY(BlueprintReadOnly, Category="AI NPC") FString FinalApprovedText;
    /** Development diagnostics only; a semantic reviewer rejected this mechanically validated candidate. */
    UPROPERTY(BlueprintReadOnly, Category="AI NPC") FString RejectedCandidateText;
    /** Trusted FactIds used verbatim by the deterministic fallback. Empty when no authorized fact was available. */
    UPROPERTY(BlueprintReadOnly, Category="AI NPC|Diagnostics") TArray<FName> FallbackEvidenceFactIds;
    /** Locally validated semantic assessments for diagnostics; never use these as dialogue output. */
    UPROPERTY(BlueprintReadOnly, Category="AI NPC")
    TArray<FKnowledgeSemanticClaimAssessment> SemanticClaimAssessments;
    /** Mechanically validated claim texts corresponding to SemanticClaimAssessments. Diagnostics only. */
    UPROPERTY(BlueprintReadOnly, Category="AI NPC")
    TArray<FKnowledgeDialogueClaim> SemanticReviewedClaims;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC|Diagnostics") FString SemanticReviewRequestId;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC|Diagnostics") FString ReviewerName;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC|Diagnostics") FString ReviewerVersion;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC|Diagnostics") FName ReviewPolicyVersion;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC|Diagnostics") FGuid AuthorizationEpoch;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC|Diagnostics") int64 ScopeFingerprint = 0;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC|Diagnostics") int32 ReviewDurationMs = 0;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC") FString Error;
};

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FAiNpcLocalTurn
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="AI NPC") FString PlayerText;
    UPROPERTY(BlueprintReadOnly, Category="AI NPC") FString NpcText;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAiNpcProviderCompletedSignature, FAiNpcProviderResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAiNpcReviewProviderCompletedSignature, FKnowledgeSemanticReviewResponse, Response);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAiNpcFinalReplySignature, FAiNpcFinalReply, Reply);
