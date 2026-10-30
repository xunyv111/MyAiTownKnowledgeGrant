#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AiNpcProviderSettingsWidget.generated.h"

class SCheckBox;
class SEditableTextBox;
class STextBlock;

/** Native runtime BYOK panel. Keys are password-masked and discarded from the widget after apply. */
UCLASS(BlueprintType)
class MYAITOWNKNOWLEDGEGRANT_API UAiNpcProviderSettingsWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    struct FProfileFields
    {
        FName ProfileId;
        TSharedPtr<SEditableTextBox> ProviderName;
        TSharedPtr<SEditableTextBox> Url;
        TSharedPtr<SEditableTextBox> Model;
        TSharedPtr<SEditableTextBox> ApiKey;
    };

    TArray<FProfileFields> ProfileFields;
    TSharedPtr<SEditableTextBox> DialogueProviderName;
    TSharedPtr<SEditableTextBox> DialogueUrl;
    TSharedPtr<SEditableTextBox> DialogueModel;
    TSharedPtr<SEditableTextBox> DialogueApiKey;
    TSharedPtr<SEditableTextBox> ReviewProviderName;
    TSharedPtr<SEditableTextBox> ReviewUrl;
    TSharedPtr<SEditableTextBox> ReviewModel;
    TSharedPtr<SEditableTextBox> ReviewApiKey;
    TSharedPtr<SCheckBox> ConsentCheckBox;
    TSharedPtr<SCheckBox> SameProviderCheckBox;
    TSharedPtr<STextBlock> StatusText;

    FReply ApplyConfiguration();
    FReply ClearCredentials();
    FReply ClosePanel();
};
