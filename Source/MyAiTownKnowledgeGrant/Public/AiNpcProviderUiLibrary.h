// Copyright 2026 xunyv.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AiNpcProviderUiLibrary.generated.h"

class APlayerController;
class UAiNpcProviderSettingsWidget;

UCLASS()
class MYAITOWNKNOWLEDGEGRANT_API UAiNpcProviderUiLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Creates the plugin-native BYOK panel and adds it to the local player's viewport. */
    UFUNCTION(BlueprintCallable, Category="AI NPC|Provider UI")
    static UAiNpcProviderSettingsWidget* ShowAiNpcProviderSettings(
        APlayerController* PlayerController, int32 ZOrder = 100);
};
