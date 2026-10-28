// Copyright 2026 xunyv.

#include "AiNpcProviderSettings.h"

UAiNpcProviderSettings::UAiNpcProviderSettings()
{
    DialogueDefaults.ProviderName = TEXT("OpenAI-compatible dialogue provider");
    DialogueDefaults.Model = TEXT("model-name");
    DialogueDefaults.bPlayerConsentedToProviderTransmission = false;
    ReviewerDefaults.ProviderName = TEXT("OpenAI-compatible semantic reviewer");
    ReviewerDefaults.Model = TEXT("reviewer-model-name");
    ReviewerDefaults.Temperature = 0.0f;
    ReviewerDefaults.bPlayerConsentedToProviderTransmission = false;
    PlayerDisclosure = FText::FromString(TEXT(
        "The selected provider receives this NPC's public role instructions, the authorized facts selected for this turn, and your current message. "
        "The API key and the local conversation transcript are kept only in this running game and are not written to disk by the plugin."));
}
