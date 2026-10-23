#include "VRMainMenuWidget.h"
#include "VRMainMenuPawn.h"
#include "CTLLevelList.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UVRMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (StartButton) StartButton->OnClicked.AddUniqueDynamic(this, &UVRMainMenuWidget::StartClicked);
    if (ExitButton) ExitButton->OnClicked.AddUniqueDynamic(this, &UVRMainMenuWidget::ExitClicked);
    if (TutorialButton) TutorialButton->OnClicked.AddUniqueDynamic(this, &UVRMainMenuWidget::TutorialClicked);
    if (LevelOneButton) LevelOneButton->OnClicked.AddUniqueDynamic(this, &UVRMainMenuWidget::LevelOneClicked);
    if (BackButton) BackButton->OnClicked.AddUniqueDynamic(this, &UVRMainMenuWidget::BackClicked);
}

void UVRMainMenuWidget::NativeDestruct()
{
    if (StartButton) StartButton->OnClicked.RemoveDynamic(this, &UVRMainMenuWidget::StartClicked);
    if (ExitButton) ExitButton->OnClicked.RemoveDynamic(this, &UVRMainMenuWidget::ExitClicked);
    if (TutorialButton) TutorialButton->OnClicked.RemoveDynamic(this, &UVRMainMenuWidget::TutorialClicked);
    if (LevelOneButton) LevelOneButton->OnClicked.RemoveDynamic(this, &UVRMainMenuWidget::LevelOneClicked);
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
    if (BackButton) BackButton->SetIsEnabled(!bBusy);
}

void UVRMainMenuWidget::StartClicked()
{
    if (MenuPawn.IsValid()) MenuPawn->StartGame();
}

void UVRMainMenuWidget::ExitClicked()
{
    if (MenuPawn.IsValid()) MenuPawn->ExitGame();
}

void UVRMainMenuWidget::TutorialClicked() { if (MenuPawn.IsValid()) MenuPawn->SelectLevel(TEXT("Tutorial")); }
void UVRMainMenuWidget::LevelOneClicked() { if (MenuPawn.IsValid()) MenuPawn->SelectLevel(TEXT("Level1")); }
void UVRMainMenuWidget::BackClicked() { if (MenuPawn.IsValid()) MenuPawn->ReturnToMainMenu(); }
