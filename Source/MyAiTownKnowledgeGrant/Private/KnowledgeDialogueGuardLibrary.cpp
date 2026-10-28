// Copyright 2026 xunyv.

#include "KnowledgeDialogueGuardLibrary.h"

#include "KnowledgeContextCollectorComponent.h"
#include "KnowledgeGrantComponent.h"

namespace
{
FString PolicyText(const UKnowledgeDialogueGuardPolicy* Policy, const FString& Error)
{
    if (!IsValid(Policy)) return FString();
    if (Error == TEXT("dialogue_claim_malformed")) return Policy->MalformedOutputText.ToString();
    if (Error.StartsWith(TEXT("dialogue_claim_"))) return Policy->InsufficientEvidenceText.ToString();
    return Policy->InvalidViewText.ToString();
}

bool BuildTokenSet(const TArray<FString>& Tokens, TSet<FString>& OutTokens)
{
    for (const FString& Token : Tokens)
    {
        const FString Canonical = Token.TrimStartAndEnd().ToLower();
        if (Canonical.IsEmpty() || OutTokens.Contains(Canonical)) return false;
        OutTokens.Add(Canonical);
    }
    return true;
}

bool SetsEqual(const TSet<FString>& A, const TSet<FString>& B)
{
    if (A.Num() != B.Num()) return false;
    for (const FString& Value : A) if (!B.Contains(Value)) return false;
    return true;
}

bool TextContainsAll(const FString& Text, const TSet<FString>& Tokens)
{
    for (const FString& Token : Tokens)
    {
        if (!Text.Contains(Token, ESearchCase::IgnoreCase)) return false;
    }
    return true;
}

bool IsAsciiDigit(TCHAR Character)
{
    return Character >= TEXT('0') && Character <= TEXT('9');
}

TSet<FString> ExtractNumericTokens(const FString& Text)
{
    TSet<FString> Result;
    for (int32 Index = 0; Index < Text.Len(); ++Index)
    {
        const bool bHasSign = (Text[Index] == TEXT('-') || Text[Index] == TEXT('+'))
            && Index + 1 < Text.Len() && IsAsciiDigit(Text[Index + 1])
            && (Index == 0 || !IsAsciiDigit(Text[Index - 1]));
        if (!IsAsciiDigit(Text[Index]) && !bHasSign) continue;

        const int32 Start = Index;
        if (bHasSign) ++Index;
        while (Index + 1 < Text.Len() && IsAsciiDigit(Text[Index + 1])) ++Index;
        if (Index + 2 < Text.Len() && Text[Index + 1] == TEXT('.') && IsAsciiDigit(Text[Index + 2]))
        {
            Index += 2;
            while (Index + 1 < Text.Len() && IsAsciiDigit(Text[Index + 1])) ++Index;
        }
        Result.Add(Text.Mid(Start, Index - Start + 1));
    }
    return Result;
}

TSet<FString> FilterMachineNumericTokens(const TSet<FString>& DeclaredTokens)
{
    TSet<FString> Result;
    for (const FString& Token : DeclaredTokens)
    {
        const TSet<FString> Parsed = ExtractNumericTokens(Token);
        if (Parsed.Num() == 1 && Parsed.Contains(Token)) Result.Add(Token);
    }
    return Result;
}

bool TextIsNegated(const FString& Text)
{
    static const TCHAR* Markers[] = {TEXT("没有"), TEXT("并非"), TEXT("不是"), TEXT("从未"), TEXT("未曾"),
        TEXT("并未"), TEXT("尚未"), TEXT("绝非"), TEXT("无一"), TEXT(" never "), TEXT(" not "), TEXT(" no ")};
    const FString Padded = TEXT(" ") + Text.ToLower() + TEXT(" ");
    for (const TCHAR* Marker : Markers)
    {
        if (Padded.Contains(Marker)) return true;
    }
    return false;
}

bool ContainsUndeclaredProtectedToken(const FString& Text, const TArray<FString>& Lexicon,
    const TSet<FString>& DeclaredTokens)
{
    const FString CanonicalText = Text.ToLower();
    for (const FString& Token : Lexicon)
    {
        const FString Canonical = Token.TrimStartAndEnd().ToLower();
        if (Canonical.IsEmpty() || DeclaredTokens.Contains(Canonical)) continue;

        int32 TokenStart = CanonicalText.Find(Canonical, ESearchCase::CaseSensitive);
        while (TokenStart != INDEX_NONE)
        {
            bool bOccurrenceCovered = false;
            for (const FString& Declared : DeclaredTokens)
            {
                if (Declared.Len() <= Canonical.Len() || !Declared.Contains(Canonical)) continue;
                int32 DeclaredStart = CanonicalText.Find(Declared, ESearchCase::CaseSensitive);
                while (DeclaredStart != INDEX_NONE)
                {
                    const int32 DeclaredEnd = DeclaredStart + Declared.Len();
                    if (TokenStart >= DeclaredStart && TokenStart + Canonical.Len() <= DeclaredEnd)
                    {
                        bOccurrenceCovered = true;
                        break;
                    }
                    DeclaredStart = CanonicalText.Find(
                        Declared, ESearchCase::CaseSensitive, ESearchDir::FromStart, DeclaredStart + 1);
                }
                if (bOccurrenceCovered) break;
            }
            if (!bOccurrenceCovered) return true;
            TokenStart = CanonicalText.Find(
                Canonical, ESearchCase::CaseSensitive, ESearchDir::FromStart, TokenStart + 1);
        }
    }
    return false;
}
}

FKnowledgeDialogueGuardResult UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeView(
    const UKnowledgeGrantComponent* PlayerKnowledge,
    const FDialogueKnowledgeView& KnowledgeView,
    const FGameplayTagContainer& CurrentActiveContextTags,
    const TArray<FKnowledgeDialogueClaim>& Claims,
    const UKnowledgeDialogueGuardPolicy* Policy)
{
    FKnowledgeDialogueGuardResult Result;
    FString ViewError;
    if (!UKnowledgeResolverLibrary::ValidateDialogueKnowledgeView(
        PlayerKnowledge, KnowledgeView, CurrentActiveContextTags, ViewError))
    {
        Result.bBlocked = true;
        Result.Outcome = EKnowledgeDialogueGuardOutcome::Invalid;
        Result.Error = ViewError;
        Result.SafeText = PolicyText(Policy, ViewError);
        return Result;
    }
    Result.ValidatedScopeFingerprint = KnowledgeView.ScopeFingerprint;

    TMap<FName, const FKnowledgeFact*> AllowedFacts;
    for (const FKnowledgeDocument& Document : KnowledgeView.AllowedDocuments)
    {
        for (const FKnowledgeFact& Fact : Document.Facts) AllowedFacts.Add(Fact.FactId, &Fact);
    }

    FString FirstBlockError;
    TSet<FName> SeenClaimIds;
    TArray<FString> AllowedTexts;
    for (const FKnowledgeDialogueClaim& Claim : Claims)
    {
        TSet<FString> ClaimedEntities;
        TSet<FString> ClaimedNumbers;
        const bool bMalformed = Claim.ClaimId.IsNone() || Claim.Text.TrimStartAndEnd().IsEmpty()
            || SeenClaimIds.Contains(Claim.ClaimId) || Claim.EvidenceFactIds.IsEmpty()
            || !BuildTokenSet(Claim.ClaimedEntities, ClaimedEntities)
            || !BuildTokenSet(Claim.ClaimedNumbers, ClaimedNumbers);
        SeenClaimIds.Add(Claim.ClaimId);

        bool bGrounded = !bMalformed;
        FString ClaimError = bMalformed ? TEXT("dialogue_claim_malformed") : FString();
        TSet<FName> SeenEvidenceIds;
        TSet<FString> FactEntities;
        TSet<FString> FactNumbers;
        TOptional<bool> FactNegated;
        for (const FName EvidenceId : Claim.EvidenceFactIds)
        {
            const FKnowledgeFact* const* FactPtr = AllowedFacts.Find(EvidenceId);
            if (EvidenceId.IsNone() || SeenEvidenceIds.Contains(EvidenceId) || FactPtr == nullptr)
            {
                bGrounded = false;
                ClaimError = TEXT("dialogue_claim_fact_not_grounded");
                break;
            }
            SeenEvidenceIds.Add(EvidenceId);
            const FKnowledgeFact& Fact = **FactPtr;
            for (const FString& Entity : Fact.Entities) FactEntities.Add(Entity.TrimStartAndEnd().ToLower());
            for (const FString& Number : Fact.Numbers) FactNumbers.Add(Number.TrimStartAndEnd().ToLower());
            if (FactNegated.IsSet() && FactNegated.GetValue() != Fact.bNegated)
            {
                bGrounded = false;
                ClaimError = TEXT("dialogue_claim_negation_mismatch");
                break;
            }
            FactNegated = Fact.bNegated;
        }

        if (bGrounded && (!SetsEqual(ClaimedEntities, FactEntities)
            || !TextContainsAll(Claim.Text, ClaimedEntities)
            || (IsValid(Policy) && ContainsUndeclaredProtectedToken(
                Claim.Text, Policy->ProtectedEntityLexicon, ClaimedEntities))))
        {
            bGrounded = false;
            ClaimError = TEXT("dialogue_claim_entity_mismatch");
        }
        if (bGrounded && (!SetsEqual(ClaimedNumbers, FactNumbers)
            || !TextContainsAll(Claim.Text, ClaimedNumbers)
            || !SetsEqual(ExtractNumericTokens(Claim.Text), FilterMachineNumericTokens(ClaimedNumbers))
            || (IsValid(Policy) && ContainsUndeclaredProtectedToken(
                Claim.Text, Policy->ProtectedNumberLexicon, ClaimedNumbers))))
        {
            bGrounded = false;
            ClaimError = TEXT("dialogue_claim_number_mismatch");
        }
        if (bGrounded && (Claim.bNegated != FactNegated.Get(false)
            || TextIsNegated(Claim.Text) != Claim.bNegated))
        {
            bGrounded = false;
            ClaimError = TEXT("dialogue_claim_negation_mismatch");
        }

        if (bGrounded)
        {
            Result.AllowedClaimIds.Add(Claim.ClaimId);
            AllowedTexts.Add(Claim.Text);
        }
        else
        {
            Result.BlockedClaimIds.Add(Claim.ClaimId);
            if (FirstBlockError.IsEmpty()) FirstBlockError = ClaimError;
        }
    }

    Result.bBlocked = !Result.BlockedClaimIds.IsEmpty();
    const bool bRejectWholeResponse = Result.bBlocked
        && (!IsValid(Policy) || Policy->Mode == EKnowledgeDialogueGuardMode::AllOrNothing);
    if (bRejectWholeResponse)
    {
        Result.AllowedClaimIds.Reset();
        AllowedTexts.Reset();
    }

    if (!AllowedTexts.IsEmpty())
    {
        Result.SafeText = FString::Join(AllowedTexts, TEXT("\n"));
        Result.bOk = true;
        Result.Outcome = Result.bBlocked
            ? EKnowledgeDialogueGuardOutcome::PartiallyAllowed
            : EKnowledgeDialogueGuardOutcome::Allowed;
    }
    else if (Result.bBlocked)
    {
        Result.SafeText = PolicyText(Policy, FirstBlockError);
        Result.Outcome = EKnowledgeDialogueGuardOutcome::Blocked;
    }
    else
    {
        Result.bBlocked = true;
        Result.Error = TEXT("dialogue_claims_empty");
        Result.SafeText = PolicyText(Policy, Result.Error);
        Result.Outcome = EKnowledgeDialogueGuardOutcome::Blocked;
        return Result;
    }
    Result.Error = FirstBlockError;
    return Result;
}

FKnowledgeDialogueGuardResult UKnowledgeDialogueGuardLibrary::GuardDialogueKnowledgeViewForCollectors(
    const UKnowledgeGrantComponent* PlayerKnowledge,
    const FDialogueKnowledgeView& KnowledgeView,
    const TArray<UKnowledgeContextCollectorComponent*>& ContextCollectors,
    const TArray<FKnowledgeDialogueClaim>& Claims,
    const UKnowledgeDialogueGuardPolicy* Policy)
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
        FKnowledgeDialogueGuardResult Result;
        Result.bBlocked = true;
        Result.Outcome = EKnowledgeDialogueGuardOutcome::Invalid;
        Result.Error = TEXT("context_collector_missing");
        Result.SafeText = PolicyText(Policy, Result.Error);
        return Result;
    }

    return GuardDialogueKnowledgeView(
        PlayerKnowledge, KnowledgeView, ActiveContextTags, Claims, Policy);
}
