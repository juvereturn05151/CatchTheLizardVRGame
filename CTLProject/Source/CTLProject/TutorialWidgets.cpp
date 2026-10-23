#include "TutorialWidgets.h"
#include "TrainingDirector.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
void UTutorialDialogueWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (ContinueButton) ContinueButton->OnClicked.AddUniqueDynamic(this, &UTutorialDialogueWidget::ContinueClicked);
}
void UTutorialDialogueWidget::NativeDestruct()
{
    if (ContinueButton) ContinueButton->OnClicked.RemoveDynamic(this, &UTutorialDialogueWidget::ContinueClicked);
    Director.Reset();
    Super::NativeDestruct();
}
void UTutorialDialogueWidget::InitializeDirector(ATrainingDirector* InDirector) { Director = InDirector; }
void UTutorialDialogueWidget::ContinueClicked() { if (Director.IsValid()) Director->AdvanceDialogue(); }
void UTutorialDialogueWidget::Present(const FText& Dialogue, const FText& Label, bool bCanContinue)
{
    if (DialogueText) DialogueText->SetText(Dialogue);
    if (ActionLabel) ActionLabel->SetText(Label);
    if (ContinueButton)
    {
        ContinueButton->SetIsEnabled(bCanContinue);
        ContinueButton->SetVisibility(bCanContinue ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}
void UTutorialObjectiveWidget::Present(const FText& Objective, const FText& Controls, const FText& Status)
{
    if (ObjectiveText) ObjectiveText->SetText(Objective);
    if (ControlsText) ControlsText->SetText(Controls);
    if (StatusText) StatusText->SetText(Status);
}
