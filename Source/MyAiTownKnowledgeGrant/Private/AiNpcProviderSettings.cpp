#include "AiNpcProviderSettings.h"

UAiNpcProviderSettings::UAiNpcProviderSettings()
{
    DialogueDefaults.ProviderName = TEXT("OpenAI-compatible dialogue provider");
    DialogueDefaults.Model = TEXT("model-name");
    DialogueDefaults.bPlayerConsentedToProviderTransmission = false;
    static const TCHAR* ProfileIds[] = {
        TEXT("npc.slot.1"), TEXT("npc.slot.2"), TEXT("npc.slot.3"), TEXT("npc.slot.4"), TEXT("npc.slot.5")};
    for (int32 Index = 0; Index < 5; ++Index)
    {
        FAiNpcProviderProfileDefaults Profile;
        Profile.ProfileId = ProfileIds[Index];
        Profile.DisplayLabel = FString::Printf(TEXT("NPC %d"), Index + 1);
        Profile.Defaults = DialogueDefaults;
        Profile.Defaults.ProviderName = FString::Printf(TEXT("NPC %d dialogue provider"), Index + 1);
        NpcDialogueProfiles.Add(Profile);
    }
    ReviewerDefaults.ProviderName = TEXT("OpenAI-compatible semantic reviewer");
    ReviewerDefaults.Model = TEXT("reviewer-model-name");
    ReviewerDefaults.Temperature = 0.0f;
    ReviewerDefaults.bPlayerConsentedToProviderTransmission = false;
    PlayerDisclosure = FText::FromString(TEXT(
        "The selected provider receives this NPC's public role instructions, the authorized facts selected for this turn, and your current message. "
        "The API key and the local conversation transcript are kept only in this running game and are not written to disk by the plugin."));
}
