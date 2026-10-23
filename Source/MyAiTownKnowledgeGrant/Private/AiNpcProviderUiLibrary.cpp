#include "AiNpcProviderUiLibrary.h"

#include "AiNpcProviderSettingsWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

UAiNpcProviderSettingsWidget* UAiNpcProviderUiLibrary::ShowAiNpcProviderSettings(
    APlayerController* PlayerController, int32 ZOrder)
{
    if (!IsValid(PlayerController) || !PlayerController->IsLocalController()) return nullptr;
    UAiNpcProviderSettingsWidget* Widget = CreateWidget<UAiNpcProviderSettingsWidget>(
        PlayerController, UAiNpcProviderSettingsWidget::StaticClass());
    if (Widget) Widget->AddToViewport(ZOrder);
    return Widget;
}
