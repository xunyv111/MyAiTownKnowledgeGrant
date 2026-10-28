// Copyright 2026 xunyv.

#include "KnowledgePack.h"

namespace
{
bool ValidateTokens(const TArray<FString>& Tokens)
{
    TSet<FString> Seen;
    for (const FString& Token : Tokens)
    {
        const FString Canonical = Token.TrimStartAndEnd().ToLower();
        if (Canonical.IsEmpty() || Seen.Contains(Canonical)) return false;
        Seen.Add(Canonical);
    }
    return true;
}
}

bool UKnowledgePack::IsDefinitionValid(FString& OutError) const
{
    if (PackId.IsNone())
    {
        OutError = TEXT("pack_id_invalid");
        return false;
    }
    const bool bHasQuestSource = !SourceQuestId.IsNone();
    const bool bHasWorldEventSource = !SourceWorldEventId.IsNone();
    if (bHasQuestSource == bHasWorldEventSource)
    {
        OutError = TEXT("exactly_one_knowledge_source_required");
        return false;
    }

    TSet<FName> PrerequisiteIds;
    for (const FName RequiredPackId : RequiredPackIds)
    {
        if (RequiredPackId.IsNone() || RequiredPackId == PackId || PrerequisiteIds.Contains(RequiredPackId))
        {
            OutError = TEXT("required_pack_ids_invalid");
            return false;
        }
        PrerequisiteIds.Add(RequiredPackId);
    }

    TSet<FName> DocumentIds;
    TSet<FName> FactIds;
    for (const FKnowledgeDocument& Document : Documents)
    {
        if (Document.DocumentId.IsNone() || Document.Content.TrimStartAndEnd().IsEmpty()
            || Document.Facts.IsEmpty())
        {
            OutError = TEXT("document_invalid");
            return false;
        }
        if (Document.ThreatLevel < 0 || Document.ThreatLevel > 100
            || Document.Novelty < 0 || Document.Novelty > 100
            || Document.MoralViolation < 0 || Document.MoralViolation > 100
            || Document.OrderImpact < -100 || Document.OrderImpact > 100)
        {
            OutError = TEXT("document_reaction_metadata_invalid");
            return false;
        }
        if (DocumentIds.Contains(Document.DocumentId))
        {
            OutError = TEXT("document_id_duplicate");
            return false;
        }
        DocumentIds.Add(Document.DocumentId);

        for (const FKnowledgeFact& Fact : Document.Facts)
        {
            if (Fact.FactId.IsNone() || Fact.Statement.TrimStartAndEnd().IsEmpty()
                || FactIds.Contains(Fact.FactId)
                || !ValidateTokens(Fact.Entities) || !ValidateTokens(Fact.Numbers))
            {
                OutError = TEXT("fact_invalid");
                return false;
            }
            FactIds.Add(Fact.FactId);
        }
    }

    OutError.Empty();
    return true;
}
