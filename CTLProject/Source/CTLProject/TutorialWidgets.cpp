#include "TutorialWidgets.h"
#include "CTLSoundEffects.h"
#include "TrainingDirector.h"
#include "MissionFlow.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
void UTutorialDialogueWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (ContinueButton) ContinueButton->OnClicked.AddUniqueDynamic(this, &UTutorialDialogueWidget::ContinueClicked);
    if (ContinueButton) ContinueButton->OnHovered.AddUniqueDynamic(this, &UTutorialDialogueWidget::ContinueHovered);
}
void UTutorialDialogueWidget::NativeDestruct()
{
    if (ContinueButton) ContinueButton->OnClicked.RemoveDynamic(this, &UTutorialDialogueWidget::ContinueClicked);
    if (ContinueButton) ContinueButton->OnHovered.RemoveDynamic(this, &UTutorialDialogueWidget::ContinueHovered);
    Director.Reset();
    Super::NativeDestruct();
}
void UTutorialDialogueWidget::InitializeDirector(ATrainingDirector* InDirector) { Director = InDirector; }
void UTutorialDialogueWidget::InitializeMission(AMissionDirector* InMission) { Mission = InMission; }
void UTutorialDialogueWidget::ContinueClicked()
{
    if (Mission.IsValid()) { Mission->AcknowledgeIntroduction(); return; }
    if (!Director.IsValid()) return;
    const ETrainingState Previous = Director->State;
    Director->AdvanceDialogue();
    if (Director->State != Previous)
        if (auto* Audio = UCTLSoundEffectsSubsystem::Get(this)) Audio->PlayEffect(this, ECTLSoundEffect::UIConfirm);
}
void UTutorialDialogueWidget::ContinueHovered()
{
    if (ContinueButton && ContinueButton->GetIsEnabled())
        if (auto* Audio = UCTLSoundEffectsSubsystem::Get(this)) Audio->PlayEffect(this, ECTLSoundEffect::UIHover);
}
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
