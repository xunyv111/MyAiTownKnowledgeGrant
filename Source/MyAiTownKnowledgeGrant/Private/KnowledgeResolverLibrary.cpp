// Copyright 2026 xunyv.

#include "KnowledgeResolverLibrary.h"

#include "GameFramework/Actor.h"
#include "KnowledgeGrantComponent.h"
#include "Misc/Crc.h"

namespace
{
int64 ComputeScopeFingerprint(const FDialogueKnowledgeView& View)
{
    FString Canonical = FString::Printf(TEXT("revision=%d|epoch=%s|tags=%s"),
        View.PlayerRevision, *View.AuthorizationEpoch.ToString(), *View.ActiveTags.ToStringSimple());
    for (const FName PackId : View.IncludedPackIds)
    {
        Canonical += TEXT("|pack=") + PackId.ToString();
    }
    for (const FKnowledgeDocument& Document : View.AllowedDocuments)
    {
        Canonical += TEXT("|doc=") + Document.DocumentId.ToString() + TEXT(":") + Document.Content;
        for (const FKnowledgeFact& Fact : Document.Facts)
        {
            Canonical += TEXT("|fact=") + Fact.FactId.ToString() + TEXT(":") + Fact.Statement
                + FString::Printf(TEXT(":negated=%d"), Fact.bNegated ? 1 : 0);
            for (const FString& Entity : Fact.Entities) Canonical += TEXT(":entity=") + Entity;
            for (const FString& Number : Fact.Numbers) Canonical += TEXT(":number=") + Number;
        }
    }
    return static_cast<int64>(FCrc::StrCrc32(*Canonical));
}

void AddExclusion(FDialogueKnowledgeView& View, const UKnowledgePack* Pack,
    EKnowledgeExclusionReason Reason, const FString& Detail)
{
    FKnowledgePackExclusion Exclusion;
    Exclusion.PackId = IsValid(Pack) ? Pack->PackId : NAME_None;
    Exclusion.Reason = Reason;
    Exclusion.Detail = Detail;
    View.ExcludedReasons.Add(Exclusion);
}
}

FDialogueKnowledgeView UKnowledgeResolverLibrary::BuildDialogueKnowledgeView(
    const UKnowledgeGrantComponent* PlayerKnowledge,
    const TArray<UKnowledgePack*>& CandidatePacks,
    const FGameplayTagContainer& ActiveContextTags)
{
    FDialogueKnowledgeView View;
    View.ActiveTags = ActiveContextTags;
    if (!IsValid(PlayerKnowledge))
    {
        View.Error = TEXT("player_knowledge_missing");
        return View;
    }
    const AActor* Owner = PlayerKnowledge->GetOwner();
    if (Owner == nullptr || !Owner->HasAuthority())
    {
        View.Error = TEXT("authority_required");
        return View;
    }
    View.PlayerRevision = PlayerKnowledge->GetState().Revision;
    View.AuthorizationEpoch = PlayerKnowledge->GetState().AuthorizationEpoch;

    TSet<FName> DocumentIds;
    TSet<FName> FactIds;
    for (const UKnowledgePack* Pack : CandidatePacks)
    {
        if (!IsValid(Pack))
        {
            AddExclusion(View, Pack, EKnowledgeExclusionReason::InvalidPack, TEXT("knowledge_pack_invalid"));
            continue;
        }

        if (Pack->PackId.IsNone())
        {
            AddExclusion(View, Pack, EKnowledgeExclusionReason::InvalidPack, TEXT("knowledge_pack_id_invalid"));
            continue;
        }
        if (!PlayerKnowledge->HasKnowledgePack(Pack->PackId))
        {
            AddExclusion(View, Pack, EKnowledgeExclusionReason::NotGranted, TEXT("knowledge_pack_locked"));
            continue;
        }

        FString DefinitionError;
        if (!Pack->IsDefinitionValid(DefinitionError))
        {
            AddExclusion(View, Pack, EKnowledgeExclusionReason::InvalidPack, DefinitionError);
            View.Error = DefinitionError;
            return View;
        }
        if (!Pack->ActivationTags.IsEmpty() && !ActiveContextTags.HasAny(Pack->ActivationTags))
        {
            AddExclusion(View, Pack, EKnowledgeExclusionReason::ContextMismatch, TEXT("context_tags_mismatch"));
            continue;
        }

        View.IncludedPackIds.Add(Pack->PackId);
        for (const FKnowledgeDocument& Document : Pack->Documents)
        {
            if (DocumentIds.Contains(Document.DocumentId))
            {
                View.IncludedPackIds.Reset();
                View.AllowedDocuments.Reset();
                View.Error = TEXT("document_id_conflict");
                return View;
            }
            for (const FKnowledgeFact& Fact : Document.Facts)
            {
                if (FactIds.Contains(Fact.FactId))
                {
                    View.IncludedPackIds.Reset();
                    View.AllowedDocuments.Reset();
                    View.Error = TEXT("fact_id_conflict");
                    return View;
                }
                FactIds.Add(Fact.FactId);
            }
            DocumentIds.Add(Document.DocumentId);
            View.AllowedDocuments.Add(Document);
        }
    }

    View.bOk = true;
    View.ScopeFingerprint = ComputeScopeFingerprint(View);
    return View;
}

bool UKnowledgeResolverLibrary::ValidateDialogueKnowledgeView(
    const UKnowledgeGrantComponent* PlayerKnowledge,
    const FDialogueKnowledgeView& View,
    const FGameplayTagContainer& CurrentActiveContextTags,
    FString& OutError)
{
    if (!IsValid(PlayerKnowledge) || !View.bOk)
    {
        OutError = TEXT("dialogue_view_invalid");
        return false;
    }
    const AActor* Owner = PlayerKnowledge->GetOwner();
    if (Owner == nullptr || !Owner->HasAuthority())
    {
        OutError = TEXT("authority_required");
        return false;
    }
    if (View.PlayerRevision != PlayerKnowledge->GetState().Revision)
    {
        OutError = TEXT("dialogue_view_stale");
        return false;
    }
    if (View.AuthorizationEpoch != PlayerKnowledge->GetState().AuthorizationEpoch)
    {
        OutError = TEXT("dialogue_view_authorization_changed");
        return false;
    }
    if (View.ActiveTags.Num() != CurrentActiveContextTags.Num()
        || !View.ActiveTags.HasAllExact(CurrentActiveContextTags))
    {
        OutError = TEXT("dialogue_view_context_changed");
        return false;
    }
    if (View.ScopeFingerprint == 0 || View.ScopeFingerprint != ComputeScopeFingerprint(View))
    {
        OutError = TEXT("dialogue_view_tampered");
        return false;
    }
    OutError.Empty();
    return true;
}
