#include "AiNpcProviderSettingsWidget.h"

#include "AiNpcProviderSettings.h"
#include "AiNpcProviderSubsystem.h"
#include "Engine/GameInstance.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> UAiNpcProviderSettingsWidget::RebuildWidget()
{
    const UAiNpcProviderSettings* Defaults = GetDefault<UAiNpcProviderSettings>();
    const FText Disclosure = Defaults ? Defaults->PlayerDisclosure : FText::GetEmpty();
    const bool bRequireIndependentReview = Defaults && Defaults->bRequireIndependentSemanticReview;
    const FAiNpcProviderConfiguration Dialogue = Defaults ? Defaults->DialogueDefaults : FAiNpcProviderConfiguration();
    const FAiNpcProviderConfiguration Review = Defaults ? Defaults->ReviewerDefaults : FAiNpcProviderConfiguration();
    ProfileFields.Reset();
    TSharedRef<SVerticalBox> ProfilePanel = SNew(SVerticalBox);
    if (Defaults)
    {
        for (const FAiNpcProviderProfileDefaults& Definition : Defaults->NpcDialogueProfiles)
        {
            if (Definition.ProfileId.IsNone()) continue;
            FProfileFields& Fields = ProfileFields.AddDefaulted_GetRef();
            Fields.ProfileId = Definition.ProfileId;
            ProfilePanel->AddSlot().AutoHeight().Padding(0, 10, 0, 3)
            [ SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%s [%s]"),
                *Definition.DisplayLabel, *Definition.ProfileId.ToString()))) ];
            ProfilePanel->AddSlot().AutoHeight()
            [ SAssignNew(Fields.ProviderName, SEditableTextBox)
                .Text(FText::FromString(Definition.Defaults.ProviderName)) ];
            ProfilePanel->AddSlot().AutoHeight()
            [ SAssignNew(Fields.Url, SEditableTextBox)
                .HintText(FText::FromString(TEXT("Chat-completions URL")))
                .Text(FText::FromString(Definition.Defaults.ChatCompletionsUrl)) ];
            ProfilePanel->AddSlot().AutoHeight()
            [ SAssignNew(Fields.Model, SEditableTextBox)
                .HintText(FText::FromString(TEXT("Model")))
                .Text(FText::FromString(Definition.Defaults.Model)) ];
            ProfilePanel->AddSlot().AutoHeight()
            [ SAssignNew(Fields.ApiKey, SEditableTextBox)
                .HintText(FText::FromString(TEXT("API key (memory only; blank disables this route)")))
                .IsPassword(true) ];
        }
    }

    return SNew(SBorder).Padding(20)
    [
        SNew(SScrollBox)
        + SScrollBox::Slot()
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("AI NPC Provider"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
            [ SNew(STextBlock).Text(Disclosure).AutoWrapText(true) ]
            + SVerticalBox::Slot().AutoHeight()
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Dialogue provider name"))) ]
            + SVerticalBox::Slot().AutoHeight()
            [ SAssignNew(DialogueProviderName, SEditableTextBox).Text(FText::FromString(Dialogue.ProviderName)) ]
            + SVerticalBox::Slot().AutoHeight()
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Dialogue chat-completions URL"))) ]
            + SVerticalBox::Slot().AutoHeight()
            [ SAssignNew(DialogueUrl, SEditableTextBox).Text(FText::FromString(Dialogue.ChatCompletionsUrl)) ]
            + SVerticalBox::Slot().AutoHeight()
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Dialogue model"))) ]
            + SVerticalBox::Slot().AutoHeight()
            [ SAssignNew(DialogueModel, SEditableTextBox).Text(FText::FromString(Dialogue.Model)) ]
            + SVerticalBox::Slot().AutoHeight()
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Dialogue API key (memory only)"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
            [ SAssignNew(DialogueApiKey, SEditableTextBox).IsPassword(true) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 8)
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Per-NPC dialogue routes (optional; override the shared default)"))) ]
            + SVerticalBox::Slot().AutoHeight()
            [ ProfilePanel ]
            + SVerticalBox::Slot().AutoHeight()
            [
                SAssignNew(SameProviderCheckBox, SCheckBox)
                .IsChecked(bRequireIndependentReview ? ECheckBoxState::Unchecked : ECheckBoxState::Checked)
                .IsEnabled(!bRequireIndependentReview)
                [ SNew(STextBlock).Text(FText::FromString(bRequireIndependentReview
                    ? TEXT("Independent semantic reviewer required by project policy")
                    : TEXT("Use the same provider for semantic review"))) ]
            ]
            + SVerticalBox::Slot().AutoHeight()
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Reviewer provider name (used when not sharing)"))) ]
            + SVerticalBox::Slot().AutoHeight()
            [ SAssignNew(ReviewProviderName, SEditableTextBox).Text(FText::FromString(Review.ProviderName)) ]
            + SVerticalBox::Slot().AutoHeight()
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Reviewer chat-completions URL"))) ]
            + SVerticalBox::Slot().AutoHeight()
            [ SAssignNew(ReviewUrl, SEditableTextBox).Text(FText::FromString(Review.ChatCompletionsUrl)) ]
            + SVerticalBox::Slot().AutoHeight()
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Reviewer model"))) ]
            + SVerticalBox::Slot().AutoHeight()
            [ SAssignNew(ReviewModel, SEditableTextBox).Text(FText::FromString(Review.Model)) ]
            + SVerticalBox::Slot().AutoHeight()
            [ SNew(STextBlock).Text(FText::FromString(TEXT("Reviewer API key (memory only)"))) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
            [ SAssignNew(ReviewApiKey, SEditableTextBox).IsPassword(true) ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [
                SAssignNew(ConsentCheckBox, SCheckBox)
                [ SNew(STextBlock).Text(FText::FromString(TEXT("I consent to sending the disclosed turn data to the selected provider(s)."))) ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [
                SNew(SButton).Text(FText::FromString(TEXT("Apply for this game session")))
                .OnClicked_UObject(this, &UAiNpcProviderSettingsWidget::ApplyConfiguration)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [
                SNew(SButton).Text(FText::FromString(TEXT("Clear credentials")))
                .OnClicked_UObject(this, &UAiNpcProviderSettingsWidget::ClearCredentials)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 4)
            [
                SNew(SButton).Text(FText::FromString(TEXT("Close")))
                .OnClicked_UObject(this, &UAiNpcProviderSettingsWidget::ClosePanel)
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(0, 8)
            [ SAssignNew(StatusText, STextBlock).AutoWrapText(true) ]
        ]
    ];
}

FReply UAiNpcProviderSettingsWidget::ApplyConfiguration()
{
    UGameInstance* GameInstance = GetGameInstance();
    UAiNpcProviderSubsystem* Provider = GameInstance ? GameInstance->GetSubsystem<UAiNpcProviderSubsystem>() : nullptr;
    if (!Provider)
    {
        StatusText->SetText(FText::FromString(TEXT("Provider subsystem unavailable.")));
        return FReply::Handled();
    }
    const bool bConsented = ConsentCheckBox.IsValid()
        && ConsentCheckBox->GetCheckedState() == ECheckBoxState::Checked;
    FAiNpcProviderConfiguration Dialogue = GetDefault<UAiNpcProviderSettings>()->DialogueDefaults;
    Dialogue.ProviderName = DialogueProviderName->GetText().ToString();
    Dialogue.ChatCompletionsUrl = DialogueUrl->GetText().ToString();
    Dialogue.Model = DialogueModel->GetText().ToString();
    Dialogue.bPlayerConsentedToProviderTransmission = bConsented;
    FString Error;
    bool bConfiguredDialogue = false;
    const FString DefaultDialogueKey = DialogueApiKey->GetText().ToString();
    if (!DefaultDialogueKey.TrimStartAndEnd().IsEmpty())
    {
        if (!Provider->ConfigureProvider(EAiNpcProviderChannel::DialogueGeneration,
            Dialogue, DefaultDialogueKey, Error))
        {
            StatusText->SetText(FText::FromString(Error));
            return FReply::Handled();
        }
        bConfiguredDialogue = true;
    }
    for (FProfileFields& Fields : ProfileFields)
    {
        const FString Key = Fields.ApiKey->GetText().ToString();
        if (Key.TrimStartAndEnd().IsEmpty()) continue;
        FAiNpcProviderConfiguration ProfileConfiguration = Dialogue;
        ProfileConfiguration.ProviderName = Fields.ProviderName->GetText().ToString();
        ProfileConfiguration.ChatCompletionsUrl = Fields.Url->GetText().ToString();
        ProfileConfiguration.Model = Fields.Model->GetText().ToString();
        if (!Provider->ConfigureNpcProvider(Fields.ProfileId, ProfileConfiguration, Key, Error))
        {
            Provider->ClearCredentials();
            StatusText->SetText(FText::FromString(FString::Printf(TEXT("%s: %s"),
                *Fields.ProfileId.ToString(), *Error)));
            return FReply::Handled();
        }
        bConfiguredDialogue = true;
    }
    if (!bConfiguredDialogue)
    {
        StatusText->SetText(FText::FromString(TEXT("Configure the shared dialogue provider or at least one NPC route.")));
        return FReply::Handled();
    }

    FAiNpcProviderConfiguration Review = GetDefault<UAiNpcProviderSettings>()->ReviewerDefaults;
    FString ReviewKey;
    const UAiNpcProviderSettings* Settings = GetDefault<UAiNpcProviderSettings>();
    const bool bRequireIndependentReview = Settings && Settings->bRequireIndependentSemanticReview;
    if (!bRequireIndependentReview
        && SameProviderCheckBox->GetCheckedState() == ECheckBoxState::Checked)
    {
        Review = Dialogue;
        Review.ProviderName += TEXT(" reviewer");
        Review.Temperature = 0.0f;
        ReviewKey = DialogueApiKey->GetText().ToString();
    }
    else
    {
        Review.ProviderName = ReviewProviderName->GetText().ToString();
        Review.ChatCompletionsUrl = ReviewUrl->GetText().ToString();
        Review.Model = ReviewModel->GetText().ToString();
        Review.bPlayerConsentedToProviderTransmission = bConsented;
        ReviewKey = ReviewApiKey->GetText().ToString();
    }
    if (!Provider->ConfigureProvider(EAiNpcProviderChannel::SemanticReview, Review, ReviewKey, Error))
    {
        Provider->ClearCredentials();
        StatusText->SetText(FText::FromString(Error));
        return FReply::Handled();
    }
    DialogueApiKey->SetText(FText::GetEmpty());
    for (FProfileFields& Fields : ProfileFields) if (Fields.ApiKey.IsValid()) Fields.ApiKey->SetText(FText::GetEmpty());
    ReviewApiKey->SetText(FText::GetEmpty());
    StatusText->SetText(FText::FromString(TEXT("Configured in process memory. Nothing was written to disk.")));
    return FReply::Handled();
}

FReply UAiNpcProviderSettingsWidget::ClearCredentials()
{
    if (UGameInstance* GameInstance = GetGameInstance())
        if (UAiNpcProviderSubsystem* Provider = GameInstance->GetSubsystem<UAiNpcProviderSubsystem>())
            Provider->ClearCredentials();
    if (DialogueApiKey.IsValid()) DialogueApiKey->SetText(FText::GetEmpty());
    for (FProfileFields& Fields : ProfileFields) if (Fields.ApiKey.IsValid()) Fields.ApiKey->SetText(FText::GetEmpty());
    if (ReviewApiKey.IsValid()) ReviewApiKey->SetText(FText::GetEmpty());
    if (StatusText.IsValid()) StatusText->SetText(FText::FromString(TEXT("Credentials cleared.")));
    return FReply::Handled();
}

FReply UAiNpcProviderSettingsWidget::ClosePanel()
{
    RemoveFromParent();
    return FReply::Handled();
}
