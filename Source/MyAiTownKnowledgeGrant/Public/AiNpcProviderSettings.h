#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AiNpcProviderTypes.h"
#include "AiNpcProviderSettings.generated.h"

USTRUCT(BlueprintType)
struct MYAITOWNKNOWLEDGEGRANT_API FAiNpcProviderProfileDefaults
{
    GENERATED_BODY()

    UPROPERTY(Config, EditAnywhere, Category="AI NPC|Profile") FName ProfileId;
    UPROPERTY(Config, EditAnywhere, Category="AI NPC|Profile") FString DisplayLabel;
    UPROPERTY(Config, EditAnywhere, Category="AI NPC|Profile") FAiNpcProviderConfiguration Defaults;
};

/** Project defaults visible under Project Settings > Plugins > AI NPC Roleplay. No secrets are stored here. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="AI NPC Roleplay"))
class MYAITOWNKNOWLEDGEGRANT_API UAiNpcProviderSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UAiNpcProviderSettings();

    UPROPERTY(Config, EditAnywhere, Category="Dialogue Provider") FAiNpcProviderConfiguration DialogueDefaults;
    /** Optional non-secret NPC routes shown by the runtime panel. Keys are still entered only at runtime. */
    UPROPERTY(Config, EditAnywhere, Category="Dialogue Provider") TArray<FAiNpcProviderProfileDefaults> NpcDialogueProfiles;
    UPROPERTY(Config, EditAnywhere, Category="Semantic Reviewer") FAiNpcProviderConfiguration ReviewerDefaults;
    UPROPERTY(Config, EditAnywhere, Category="Privacy", meta=(MultiLine=true)) FText PlayerDisclosure;
    UPROPERTY(Config, EditAnywhere, Category="Privacy") bool bRequireIndependentSemanticReview = true;

    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
