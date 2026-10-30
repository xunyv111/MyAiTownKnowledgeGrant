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

bool IsFullWidthDigit(TCHAR Character)
{
    return Character >= 0xFF10 && Character <= 0xFF19;
}

bool IsAnyDigit(TCHAR Character)
{
    return (Character >= TEXT('0') && Character <= TEXT('9')) || IsFullWidthDigit(Character);
}

FString NormalizeCompatibilityText(const FString& Input)
{
    FString Result;
    Result.Reserve(Input.Len());
    for (int32 Index = 0; Index < Input.Len(); ++Index)
    {
        TCHAR Character = Input[Index];
        if (Character == 0x3000) Character = TEXT(' ');
        else if (Character >= 0xFF01 && Character <= 0xFF5E) Character -= 0xFEE0;
        else if (Character == 0x2013 || Character == 0x2014 || Character == 0xFF0D) Character = TEXT('-');
        else if (Character == 0xFF0C) Character = TEXT(',');

        if (Character == TEXT(',') && !Result.IsEmpty() && IsAnyDigit(Result[Result.Len() - 1])
            && Index + 1 < Input.Len() && IsAnyDigit(Input[Index + 1]))
        {
            continue;
        }
        Result.AppendChar(FChar::ToLower(Character));
    }
    return Result.TrimStartAndEnd();
}

bool TryParseSurfaceInteger(const FString& Input, int64& OutValue)
{
    const FString Token = NormalizeCompatibilityText(Input);
    if (Token.IsEmpty()) return false;
    bool bAsciiOnly = true;
    for (const TCHAR Character : Token)
        bAsciiOnly = bAsciiOnly && Character >= TEXT('0') && Character <= TEXT('9');
    if (bAsciiOnly)
    {
        OutValue = FCString::Atoi64(*Token);
        return true;
    }

    const auto DigitValue = [](TCHAR Character) -> int32
    {
        const int32 Index = FString(TEXT("零一二三四五六七八九")).Find(FString::Chr(Character));
        if (Index != INDEX_NONE) return Index;
        if (Character == TEXT('〇')) return 0;
        if (Character == TEXT('两')) return 2;
        return INDEX_NONE;
    };
    const auto UnitValue = [](TCHAR Character) -> int32
    {
        if (Character == TEXT('十')) return 10;
        if (Character == TEXT('百')) return 100;
        if (Character == TEXT('千')) return 1000;
        if (Character == TEXT('万')) return 10000;
        return 0;
    };

    bool bHasUnit = false;
    for (const TCHAR Character : Token)
    {
        if (DigitValue(Character) == INDEX_NONE && UnitValue(Character) == 0) return false;
        bHasUnit = bHasUnit || UnitValue(Character) != 0;
    }
    if (!bHasUnit)
    {
        int64 Value = 0;
        for (const TCHAR Character : Token) Value = Value * 10 + DigitValue(Character);
        OutValue = Value;
        return true;
    }

    int64 Total = 0;
    int64 Section = 0;
    int64 Number = 0;
    for (const TCHAR Character : Token)
    {
        const int32 Digit = DigitValue(Character);
        if (Digit != INDEX_NONE)
        {
            Number = Digit;
            continue;
        }
        const int32 Unit = UnitValue(Character);
        if (Unit == 10000)
        {
            Total += (Section + Number) * Unit;
            Section = 0;
            Number = 0;
        }
        else
        {
            if (Number == 0) Number = 1;
            Section += Number * Unit;
            Number = 0;
        }
    }
    OutValue = Total + Section + Number;
    return true;
}

FString NormalizeNumberToken(const FString& Input)
{
    int64 Value = 0;
    return TryParseSurfaceInteger(Input, Value)
        ? FString::Printf(TEXT("%lld"), Value)
        : NormalizeCompatibilityText(Input);
}

bool ContainsNumberToken(const FString& CanonicalText, const FString& CanonicalToken);

bool ContainsEquivalentNumberToken(const FString& Text, const FString& Number)
{
    const FString CanonicalText = NormalizeCompatibilityText(Text);
    const FString CanonicalNumber = NormalizeNumberToken(Number);
    if (ContainsNumberToken(CanonicalText, NormalizeCompatibilityText(Number))) return true;
    int64 Expected = 0;
    if (!TryParseSurfaceInteger(CanonicalNumber, Expected)) return false;
    const FString ChineseIntegerCharacters = TEXT("零〇一二两三四五六七八九十百千万");
    for (int32 Start = 0; Start < CanonicalText.Len(); ++Start)
    {
        if (!IsAnyDigit(CanonicalText[Start])
            && !ChineseIntegerCharacters.Contains(FString::Chr(CanonicalText[Start]))) continue;
        int32 End = Start + 1;
        while (End < CanonicalText.Len() && (IsAnyDigit(CanonicalText[End])
            || ChineseIntegerCharacters.Contains(FString::Chr(CanonicalText[End])))) ++End;
        int64 Actual = 0;
        if (TryParseSurfaceInteger(CanonicalText.Mid(Start, End - Start), Actual) && Actual == Expected)
            return true;
        Start = End - 1;
    }
    return false;
}

bool BuildTokenSet(const TArray<FString>& Tokens, TSet<FString>& OutTokens)
{
    for (const FString& Token : Tokens)
    {
        const FString Canonical = NormalizeCompatibilityText(Token);
        if (Canonical.IsEmpty() || OutTokens.Contains(Canonical)) return false;
        OutTokens.Add(Canonical);
    }
    return true;
}

TSet<FString> FindTokensPresentInText(const FString& Text, const TSet<FString>& Tokens)
{
    TSet<FString> Present;
    const FString CanonicalText = NormalizeCompatibilityText(Text);
    for (const FString& Token : Tokens)
    {
        const FString Canonical = NormalizeCompatibilityText(Token);
        if (!Canonical.IsEmpty() && CanonicalText.Contains(Canonical)) Present.Add(Canonical);
    }
    return Present;
}

bool IsChineseNumberCharacter(TCHAR Character)
{
    static const FString Characters = TEXT("零〇一二两三四五六七八九十百千万亿半数几多余约");
    return Characters.Contains(FString::Chr(Character));
}

bool IsNumericContinuation(TCHAR Character)
{
    return IsAnyDigit(Character) || IsChineseNumberCharacter(Character)
        || Character == TEXT('.') || Character == TEXT('+') || Character == TEXT('-')
        || Character == TEXT('~') || Character == TEXT('至') || Character == TEXT('到');
}

bool HasNumericBoundary(const FString& Text, int32 Start, int32 Length)
{
    const bool bLeftClear = Start <= 0 || !IsNumericContinuation(Text[Start - 1]);
    const int32 End = Start + Length;
    const bool bRightClear = End >= Text.Len() || !IsNumericContinuation(Text[End]);
    return bLeftClear && bRightClear;
}

bool ContainsNumberToken(const FString& CanonicalText, const FString& CanonicalToken)
{
    int32 Start = CanonicalText.Find(CanonicalToken, ESearchCase::CaseSensitive);
    while (Start != INDEX_NONE)
    {
        if (HasNumericBoundary(CanonicalText, Start, CanonicalToken.Len())) return true;
        Start = CanonicalText.Find(CanonicalToken, ESearchCase::CaseSensitive,
            ESearchDir::FromStart, Start + 1);
    }
    return false;
}

TSet<FString> FindProtectedNumbersPresentInText(
    const FString& Text, const TSet<FString>& AllowedNumbers)
{
    TSet<FString> Present;
    const FString CanonicalText = NormalizeCompatibilityText(Text);
    for (const FString& Number : AllowedNumbers)
    {
        const FString Canonical = NormalizeNumberToken(Number);
        if (!Canonical.IsEmpty() && ContainsEquivalentNumberToken(CanonicalText, Canonical))
            Present.Add(Canonical);
    }
    return Present;
}

bool ContainsUndeclaredProtectedToken(const FString& Text, const TArray<FString>& Lexicon,
    const TSet<FString>& DeclaredTokens)
{
    const FString CanonicalText = NormalizeCompatibilityText(Text);
    TArray<FString> SortedLexicon;
    for (const FString& Token : Lexicon)
    {
        const FString Canonical = NormalizeCompatibilityText(Token);
        if (!Canonical.IsEmpty()) SortedLexicon.AddUnique(Canonical);
    }
    SortedLexicon.Sort([](const FString& A, const FString& B) { return A.Len() > B.Len(); });
    for (const FString& Canonical : SortedLexicon)
    {
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

bool ContainsUndeclaredProtectedNumber(const FString& Text, const TArray<FString>& Lexicon,
    const TSet<FString>& AllowedNumbersPresent)
{
    const FString CanonicalText = NormalizeCompatibilityText(Text);
    TArray<FString> SortedLexicon;
    for (const FString& Token : Lexicon)
    {
        const FString Canonical = NormalizeNumberToken(Token);
        if (!Canonical.IsEmpty()) SortedLexicon.AddUnique(Canonical);
    }
    SortedLexicon.Sort([](const FString& A, const FString& B) { return A.Len() > B.Len(); });
    for (const FString& Canonical : SortedLexicon)
    {
        if (AllowedNumbersPresent.Contains(Canonical)) continue;
        if (ContainsEquivalentNumberToken(CanonicalText, Canonical)) return true;
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
    TSet<FString> AllAllowedEntities;
    TSet<FString> AllAllowedNumbers;
    for (const FKnowledgeDocument& Document : KnowledgeView.AllowedDocuments)
    {
        for (const FKnowledgeFact& Fact : Document.Facts)
        {
            AllowedFacts.Add(Fact.FactId, &Fact);
            for (const FString& Entity : Fact.Entities)
                AllAllowedEntities.Add(NormalizeCompatibilityText(Entity));
            for (const FString& Number : Fact.Numbers)
                AllAllowedNumbers.Add(NormalizeNumberToken(Number));
        }
    }

    FString FirstBlockError;
    TSet<FName> SeenClaimIds;
    TArray<FString> AllowedTexts;
    for (const FKnowledgeDialogueClaim& Claim : Claims)
    {
        TSet<FString> ClaimedEntities;
        TSet<FString> ClaimedNumbers;
        const bool bUncitedRoleplay = Claim.EvidenceFactIds.IsEmpty()
            && IsValid(Policy) && Policy->bAllowUncitedRoleplay;
        const bool bMalformed = Claim.ClaimId.IsNone() || Claim.Text.TrimStartAndEnd().IsEmpty()
            || SeenClaimIds.Contains(Claim.ClaimId)
            || (Claim.EvidenceFactIds.IsEmpty() && !bUncitedRoleplay)
            || !BuildTokenSet(Claim.ClaimedEntities, ClaimedEntities)
            || !BuildTokenSet(Claim.ClaimedNumbers, ClaimedNumbers);
        SeenClaimIds.Add(Claim.ClaimId);

        bool bGrounded = !bMalformed;
        FString ClaimError = bMalformed ? TEXT("dialogue_claim_malformed") : FString();
        TSet<FName> SeenEvidenceIds;
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
            if (FactNegated.IsSet() && FactNegated.GetValue() != Fact.bNegated)
            {
                bGrounded = false;
                ClaimError = TEXT("dialogue_claim_negation_mismatch");
                break;
            }
            FactNegated = Fact.bNegated;
        }

        // Model-authored citations and metadata are not authorization sources. The mechanical
        // gate limits protected tokens to the complete current authorized view; the independent
        // semantic reviewer decides whether the resulting natural-language claim is entailed.
        const TSet<FString> EffectiveEntities = FindTokensPresentInText(Claim.Text, AllAllowedEntities);
        if (bGrounded && IsValid(Policy) && ContainsUndeclaredProtectedToken(
            Claim.Text, Policy->ProtectedEntityLexicon, EffectiveEntities))
        {
            bGrounded = false;
            ClaimError = bUncitedRoleplay
                ? TEXT("dialogue_claim_uncited_roleplay_unsafe")
                : TEXT("dialogue_claim_entity_mismatch");
        }
        const TSet<FString> EffectiveNumbers =
            FindProtectedNumbersPresentInText(Claim.Text, AllAllowedNumbers);
        if (bGrounded && IsValid(Policy) && ContainsUndeclaredProtectedNumber(
            Claim.Text, Policy->ProtectedNumberLexicon, EffectiveNumbers))
        {
            bGrounded = false;
            ClaimError = bUncitedRoleplay
                ? TEXT("dialogue_claim_uncited_roleplay_unsafe")
                : TEXT("dialogue_claim_number_mismatch");
        }

        if (bGrounded)
        {
            Result.AllowedClaimIds.Add(Claim.ClaimId);
            Result.AllowedClaims.Add(Claim);
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
        Result.AllowedClaims.Reset();
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
