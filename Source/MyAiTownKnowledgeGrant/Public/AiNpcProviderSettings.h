// Copyright 2026 xunyv.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AiNpcProviderTypes.h"
#include "AiNpcProviderSettings.generated.h"

/** Project defaults visible under Project Settings > Plugins > AI NPC Roleplay. No secrets are stored here. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="AI NPC Roleplay"))
class MYAITOWNKNOWLEDGEGRANT_API UAiNpcProviderSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UAiNpcProviderSettings();

    UPROPERTY(Config, EditAnywhere, Category="Dialogue Provider") FAiNpcProviderConfiguration DialogueDefaults;
    UPROPERTY(Config, EditAnywhere, Category="Semantic Reviewer") FAiNpcProviderConfiguration ReviewerDefaults;
    UPROPERTY(Config, EditAnywhere, Category="Privacy", meta=(MultiLine=true)) FText PlayerDisclosure;
    UPROPERTY(Config, EditAnywhere, Category="Privacy") bool bRequireIndependentSemanticReview = true;

    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
