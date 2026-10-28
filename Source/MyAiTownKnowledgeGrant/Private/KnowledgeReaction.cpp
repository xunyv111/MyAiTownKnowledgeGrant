// Copyright 2026 xunyv.

#include "KnowledgeReaction.h"

namespace
{
bool IsTraitValid(int32 Value)
{
    return Value >= 0 && Value <= 100;
}

int32 MoodBias(FName Mood)
{
    if (Mood == TEXT("distressed")) return -15;
    if (Mood == TEXT("unhappy")) return -8;
    if (Mood == TEXT("positive")) return 5;
    if (Mood == TEXT("delighted")) return 10;
    return 0;
}

int32 ActivationBoost(FName Activation)
{
    if (Activation == TEXT("tense")) return 20;
    if (Activation == TEXT("alert")) return 8;
    return 0;
}
}

bool UNpcPersonalityProfile::IsDefinitionValid(FString& OutError) const
{
    if (!IsTraitValid(Empathy) || !IsTraitValid(Curiosity) || !IsTraitValid(Caution)
        || !IsTraitValid(OrderLoyalty) || !IsTraitValid(Aggression))
    {
        OutError = TEXT("personality_trait_invalid");
        return false;
    }
    OutError.Empty();
    return true;
}

FNpcKnowledgeReaction UKnowledgeReactionLibrary::EvaluateKnowledgeReaction(
    const UNpcPersonalityProfile* Personality,
    const FKnowledgeDocument& Document,
    const FNpcAffectContext& AffectContext)
{
    FNpcKnowledgeReaction Reaction;
    if (!IsValid(Personality))
    {
        Reaction.Error = TEXT("personality_missing");
        return Reaction;
    }

    FString PersonalityError;
    if (!Personality->IsDefinitionValid(PersonalityError))
    {
        Reaction.Error = PersonalityError;
        return Reaction;
    }
    if (Document.DocumentId.IsNone() || Document.Content.TrimStartAndEnd().IsEmpty()
        || Document.ThreatLevel < 0 || Document.ThreatLevel > 100
        || Document.Novelty < 0 || Document.Novelty > 100
        || Document.MoralViolation < 0 || Document.MoralViolation > 100
        || Document.OrderImpact < -100 || Document.OrderImpact > 100)
    {
        Reaction.Error = TEXT("document_reaction_metadata_invalid");
        return Reaction;
    }

    const int32 ArousalBoost = ActivationBoost(AffectContext.Activation);
    Reaction.DocumentId = Document.DocumentId;
    Reaction.Interest = FMath::Clamp(
        (Document.Novelty * (50 + Personality->Curiosity)) / 100
            + (Document.ThreatLevel * Personality->Caution) / 200,
        0, 100);
    Reaction.Approval = FMath::Clamp(
        (Document.OrderImpact * (50 + Personality->OrderLoyalty)) / 100
            - (Document.MoralViolation * Personality->Empathy) / 100
            + MoodBias(AffectContext.Mood),
        -100, 100);
    Reaction.PerceivedThreat = FMath::Clamp(
        (Document.ThreatLevel * (50 + Personality->Caution)) / 100 + ArousalBoost,
        0, 100);
    Reaction.Intensity = FMath::Clamp(
        (FMath::Abs(Reaction.Approval) + Reaction.PerceivedThreat + Reaction.Interest) / 3
            + ArousalBoost / 2,
        0, 100);

    if (Reaction.PerceivedThreat >= 70 && Personality->Caution > Personality->Aggression) Reaction.DialogueTone = TEXT("fearful");
    else if (Reaction.Approval <= -45 && Personality->Aggression >= 60) Reaction.DialogueTone = TEXT("hostile");
    else if (Reaction.Approval <= -35) Reaction.DialogueTone = TEXT("disapproving");
    else if (Reaction.Intensity >= 70) Reaction.DialogueTone = TEXT("urgent");
    else if (Reaction.Interest >= 65) Reaction.DialogueTone = TEXT("curious");
    else if (Reaction.Approval >= 40) Reaction.DialogueTone = TEXT("approving");
    else if (AffectContext.Mood == TEXT("distressed")) Reaction.DialogueTone = TEXT("withdrawn");
    else Reaction.DialogueTone = TEXT("neutral");

    if (Reaction.PerceivedThreat >= 70 && Personality->Aggression >= 65) Reaction.BehaviorIntent = TEXT("confront");
    else if (Reaction.PerceivedThreat >= 70 && Personality->Caution >= 60) Reaction.BehaviorIntent = TEXT("seek_safety");
    else if (Reaction.Interest >= 70) Reaction.BehaviorIntent = TEXT("investigate");
    else if (Reaction.Approval <= -45 && Personality->Empathy >= 60) Reaction.BehaviorIntent = TEXT("protect_others");
    else if (Reaction.Approval >= 45) Reaction.BehaviorIntent = TEXT("support");
    else Reaction.BehaviorIntent = TEXT("observe");

    Reaction.SuggestedValenceDelta = FMath::Clamp(Reaction.Approval / 10, -15, 15);
    Reaction.SuggestedArousalDelta = FMath::Clamp((Reaction.Intensity - 40) / 4, -10, 15);
    Reaction.bOk = true;
    return Reaction;
}
