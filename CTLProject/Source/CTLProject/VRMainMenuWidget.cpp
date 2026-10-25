#include "VRMainMenuWidget.h"
#include "CTLSoundEffects.h"
#include "VRMainMenuPawn.h"
#include "CTLLevelList.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UVRMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    for (UButton* Button : {StartButton.Get(), ExitButton.Get(), TutorialButton.Get(), LevelOneButton.Get(), LevelTwoButton.Get(), BackButton.Get()})
        if (Button) Button->OnHovered.AddUniqueDynamic(this, &UVRMainMenuWidget::ButtonHovered);
    if (StartButton) StartButton->OnClicked.AddUniqueDynamic(this, &UVRMainMenuWidget::StartClicked);
    if (ExitButton) ExitButton->OnClicked.AddUniqueDynamic(this, &UVRMainMenuWidget::ExitClicked);
    if (TutorialButton) TutorialButton->OnClicked.AddUniqueDynamic(this, &UVRMainMenuWidget::TutorialClicked);
    if (LevelOneButton) LevelOneButton->OnClicked.AddUniqueDynamic(this, &UVRMainMenuWidget::LevelOneClicked);
    if (LevelTwoButton) LevelTwoButton->OnClicked.AddUniqueDynamic(this, &UVRMainMenuWidget::LevelTwoClicked);
    if (BackButton) BackButton->OnClicked.AddUniqueDynamic(this, &UVRMainMenuWidget::BackClicked);
}

void UVRMainMenuWidget::NativeDestruct()
{
    for (UButton* Button : {StartButton.Get(), ExitButton.Get(), TutorialButton.Get(), LevelOneButton.Get(), LevelTwoButton.Get(), BackButton.Get()})
        if (Button) Button->OnHovered.RemoveDynamic(this, &UVRMainMenuWidget::ButtonHovered);
    if (StartButton) StartButton->OnClicked.RemoveDynamic(this, &UVRMainMenuWidget::StartClicked);
    if (ExitButton) ExitButton->OnClicked.RemoveDynamic(this, &UVRMainMenuWidget::ExitClicked);
    if (TutorialButton) TutorialButton->OnClicked.RemoveDynamic(this, &UVRMainMenuWidget::TutorialClicked);
    if (LevelOneButton) LevelOneButton->OnClicked.RemoveDynamic(this, &UVRMainMenuWidget::LevelOneClicked);
    if (LevelTwoButton) LevelTwoButton->OnClicked.RemoveDynamic(this, &UVRMainMenuWidget::LevelTwoClicked);
    if (BackButton) BackButton->OnClicked.RemoveDynamic(this, &UVRMainMenuWidget::BackClicked);
    MenuPawn.Reset();
    Super::NativeDestruct();
}

void UVRMainMenuWidget::InitializeMenu(AVRMainMenuPawn* InPawn)
{
    MenuPawn = InPawn;
    if (InPawn && InPawn->LevelList)
    {
        if (const auto* Entry = InPawn->LevelList->Find(TEXT("Tutorial")))
            if (TutorialLabel) TutorialLabel->SetText(Entry->DisplayName);
        if (const auto* Entry = InPawn->LevelList->Find(TEXT("Level1")))
            if (LevelOneLabel) LevelOneLabel->SetText(Entry->DisplayName);
        if (const auto* Entry = InPawn->LevelList->Find(TEXT("Level2")))
            if (LevelTwoLabel) LevelTwoLabel->SetText(Entry->DisplayName);
    }
    SetStatus(FText::GetEmpty(), false);
}

void UVRMainMenuWidget::SetStatus(const FText& Message, bool bBusy)
{
    if (StatusText) StatusText->SetText(Message);
    if (StartButton) StartButton->SetIsEnabled(!bBusy);
    if (ExitButton) ExitButton->SetIsEnabled(!bBusy);
    if (TutorialButton) TutorialButton->SetIsEnabled(!bBusy);
    if (LevelOneButton) LevelOneButton->SetIsEnabled(!bBusy);
    if (LevelTwoButton) LevelTwoButton->SetIsEnabled(!bBusy);
    if (BackButton) BackButton->SetIsEnabled(!bBusy);
}

void UVRMainMenuWidget::StartClicked()
{
    if (ConfirmClick()) MenuPawn->StartGame();
}

void UVRMainMenuWidget::ExitClicked()
{
    if (ConfirmClick()) MenuPawn->ExitGame();
}

void UVRMainMenuWidget::TutorialClicked() { if (ConfirmClick()) MenuPawn->SelectLevel(TEXT("Tutorial")); }
void UVRMainMenuWidget::LevelOneClicked() { if (ConfirmClick()) MenuPawn->SelectLevel(TEXT("Level1")); }
void UVRMainMenuWidget::LevelTwoClicked() { if (ConfirmClick()) MenuPawn->SelectLevel(TEXT("Level2")); }
void UVRMainMenuWidget::BackClicked() { if (ConfirmClick()) MenuPawn->ReturnToMainMenu(); }

void UVRMainMenuWidget::ButtonHovered()
{
    if (MenuPawn.IsValid() && !MenuPawn->bTransitionInProgress)
        if (auto* Audio = UCTLSoundEffectsSubsystem::Get(this)) Audio->PlayEffect(this, ECTLSoundEffect::UIHover);
}

bool UVRMainMenuWidget::ConfirmClick()
{
    if (!MenuPawn.IsValid() || MenuPawn->bTransitionInProgress) return false;
    if (auto* Audio = UCTLSoundEffectsSubsystem::Get(this)) Audio->PlayEffect(this, ECTLSoundEffect::UIConfirm);
    return true;
}
