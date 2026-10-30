#include "KnowledgeModelAdapterLibrary.h"

#include "KnowledgeContextCollectorComponent.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
constexpr int32 MaxPayloadCharacters = 32768;
constexpr int32 MaxClaims = 16;
constexpr int32 MaxClaimTextCharacters = 2048;
constexpr int32 MaxIdCharacters = 128;
constexpr int32 MaxEvidenceFacts = 16;
constexpr int32 MaxMetadataTokens = 16;

FKnowledgeDialogueGuardResult BlockedResult(const FString& Error, const FText& Fallback)
{
    FKnowledgeDialogueGuardResult Result;
    Result.bBlocked = true;
    Result.Outcome = EKnowledgeDialogueGuardOutcome::Invalid;
    Result.Error = Error;
    Result.SafeText = Fallback.ToString();
    return Result;
}

bool HasExactFields(const TSharedPtr<FJsonObject>& Object, const TSet<FString>& Required)
{
    if (!Object.IsValid() || Object->Values.Num() != Required.Num()) return false;
    for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : Object->Values)
    {
        if (!Required.Contains(Field.Key)) return false;
    }
    return true;
}

bool ReadStringArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName,
    int32 MaximumCount, TArray<FString>& OutValues, bool bRequireNonEmpty)
{
    const TArray<TSharedPtr<FJsonValue>>* JsonValues = nullptr;
    if (!Object->TryGetArrayField(FieldName, JsonValues) || JsonValues == nullptr
        || JsonValues->Num() > MaximumCount || (bRequireNonEmpty && JsonValues->IsEmpty()))
    {
        return false;
    }

    TSet<FString> Seen;
    for (const TSharedPtr<FJsonValue>& JsonValue : *JsonValues)
    {
        FString Value;
        if (!JsonValue.IsValid() || !JsonValue->TryGetString(Value)) return false;
        Value = Value.TrimStartAndEnd();
        const FString Canonical = Value.ToLower();
        if (Value.IsEmpty() || Value.Len() > MaxIdCharacters || Seen.Contains(Canonical)) return false;
        Seen.Add(Canonical);
        OutValues.Add(Value);
    }
    return true;
}

bool ParseClaims(const FString& JsonPayload, bool bAllowUncitedRoleplay,
    TArray<FKnowledgeDialogueClaim>& OutClaims, FString& OutError)
{
    if (JsonPayload.TrimStartAndEnd().IsEmpty())
    {
        OutError = TEXT("model_json_payload_empty");
        return false;
    }
    if (JsonPayload.Len() > MaxPayloadCharacters)
    {
        OutError = TEXT("model_json_payload_too_large");
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonPayload);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("model_json_malformed");
        return false;
    }
    const TSet<FString> RootFields{TEXT("claims")};
    if (!HasExactFields(Root, RootFields))
    {
        OutError = TEXT("model_json_schema_invalid");
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* JsonClaims = nullptr;
    if (!Root->TryGetArrayField(TEXT("claims"), JsonClaims) || JsonClaims == nullptr || JsonClaims->IsEmpty())
    {
        OutError = TEXT("model_json_claims_empty");
        return false;
    }
    if (JsonClaims->Num() > MaxClaims)
    {
        OutError = TEXT("model_json_claim_limit_exceeded");
        return false;
    }

    const TSet<FString> ClaimFields{
        TEXT("claimId"), TEXT("text"), TEXT("evidenceFactIds"),
        TEXT("claimedEntities"), TEXT("claimedNumbers"), TEXT("negated")};
    for (const TSharedPtr<FJsonValue>& JsonClaimValue : *JsonClaims)
    {
        const TSharedPtr<FJsonObject>* JsonClaimObject = nullptr;
        if (!JsonClaimValue.IsValid() || !JsonClaimValue->TryGetObject(JsonClaimObject)
            || JsonClaimObject == nullptr || !HasExactFields(*JsonClaimObject, ClaimFields))
        {
            OutError = TEXT("model_json_claim_invalid");
            return false;
        }

        FString ClaimIdText;
        FString ClaimText;
        bool bNegated = false;
        if (!(*JsonClaimObject)->TryGetStringField(TEXT("claimId"), ClaimIdText)
            || !(*JsonClaimObject)->TryGetStringField(TEXT("text"), ClaimText)
            || !(*JsonClaimObject)->TryGetBoolField(TEXT("negated"), bNegated))
        {
            OutError = TEXT("model_json_claim_invalid");
            return false;
        }
        ClaimIdText = ClaimIdText.TrimStartAndEnd();
        ClaimText = ClaimText.TrimStartAndEnd();
        if (ClaimIdText.IsEmpty() || ClaimIdText.Len() > MaxIdCharacters)
        {
            OutError = TEXT("model_json_claim_invalid");
            return false;
        }
        if (ClaimText.IsEmpty() || ClaimText.Len() > MaxClaimTextCharacters)
        {
            OutError = TEXT("model_json_claim_text_invalid");
            return false;
        }

        TArray<FString> FactIds;
        TArray<FString> Entities;
        TArray<FString> Numbers;
        if (!ReadStringArray(*JsonClaimObject, TEXT("evidenceFactIds"), MaxEvidenceFacts,
            FactIds, !bAllowUncitedRoleplay))
        {
            OutError = TEXT("model_json_evidence_invalid");
            return false;
        }
        if (!ReadStringArray(*JsonClaimObject, TEXT("claimedEntities"), MaxMetadataTokens, Entities, false)
            || !ReadStringArray(*JsonClaimObject, TEXT("claimedNumbers"), MaxMetadataTokens, Numbers, false))
        {
            OutError = TEXT("model_json_metadata_invalid");
            return false;
        }

        FKnowledgeDialogueClaim Claim;
        Claim.ClaimId = FName(*FString::Printf(TEXT("claim-%04d"), OutClaims.Num() + 1));
        Claim.Text = MoveTemp(ClaimText);
        for (const FString& FactId : FactIds) Claim.EvidenceFactIds.Add(FName(*FactId));
        Claim.ClaimedEntities = MoveTemp(Entities);
        Claim.ClaimedNumbers = MoveTemp(Numbers);
        Claim.bNegated = bNegated;
        OutClaims.Add(MoveTemp(Claim));
    }

    OutError.Empty();
    return true;
}
}

FKnowledgeDialogueGuardResult UKnowledgeModelAdapterLibrary::GuardKnowledgeModelJsonResponse(
    const UKnowledgeGrantComponent* PlayerKnowledge,
    const FDialogueKnowledgeView& KnowledgeView,
    const FGameplayTagContainer& CurrentActiveContextTags,
    const FString& JsonPayload,
    const UKnowledgeDialogueGuardPolicy* GuardPolicy)
{
    FString ViewError;
    if (!UKnowledgeResolverLibrary::ValidateDialogueKnowledgeView(
        PlayerKnowledge, KnowledgeView, CurrentActiveContextTags, ViewError))
    {
        return BlockedResult(ViewError, IsValid(GuardPolicy) ? GuardPolicy->InvalidViewText : FText::GetEmpty());
    }

    TArray<FKnowledgeDialogueClaim> Claims;
    FString ParseError;
    const bool bAllowUncitedRoleplay = IsValid(GuardPolicy) && GuardPolicy->bAllowUncitedRoleplay;
    if (!ParseClaims(JsonPayload, bAllowUncitedRoleplay, Claims, ParseError))
    {
        return BlockedResult(ParseError, IsValid(GuardPolicy) ? GuardPolicy->MalformedOutputText : FText::GetEmpty());
    }

    return UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
        PlayerKnowledge, KnowledgeView, CurrentActiveContextTags, Claims, GuardPolicy);
}

FKnowledgeDialogueGuardResult UKnowledgeModelAdapterLibrary::GuardKnowledgeModelJsonResponseForCollectors(
    const UKnowledgeGrantComponent* PlayerKnowledge,
    const FDialogueKnowledgeView& KnowledgeView,
    const TArray<UKnowledgeContextCollectorComponent*>& ContextCollectors,
    const FString& JsonPayload,
    const UKnowledgeDialogueGuardPolicy* GuardPolicy)
{
    FGameplayTagContainer ActiveContextTags;
    int32 ValidCollectorCount = 0;
    for (const UKnowledgeContextCollectorComponent* Collector : ContextCollectors)
    {
        if (!IsValid(Collector)) continue;
        ActiveContextTags.AppendTags(Collector->GetActiveContextTags());
        ++ValidCollectorCount;
    }
    if (ValidCollectorCount == 0)
    {
        return BlockedResult(TEXT("context_collector_missing"),
            IsValid(GuardPolicy) ? GuardPolicy->InvalidViewText : FText::GetEmpty());
    }

    FString ViewError;
    if (!UKnowledgeResolverLibrary::ValidateDialogueKnowledgeView(
        PlayerKnowledge, KnowledgeView, ActiveContextTags, ViewError))
    {
        return BlockedResult(ViewError, IsValid(GuardPolicy) ? GuardPolicy->InvalidViewText : FText::GetEmpty());
    }

    TArray<FKnowledgeDialogueClaim> Claims;
    FString ParseError;
    const bool bAllowUncitedRoleplay = IsValid(GuardPolicy) && GuardPolicy->bAllowUncitedRoleplay;
    if (!ParseClaims(JsonPayload, bAllowUncitedRoleplay, Claims, ParseError))
    {
        return BlockedResult(ParseError, IsValid(GuardPolicy) ? GuardPolicy->MalformedOutputText : FText::GetEmpty());
    }

    return UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
        PlayerKnowledge, KnowledgeView, ActiveContextTags, Claims, GuardPolicy);
}
