#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialWidgets.generated.h"
class UButton;
class UTextBlock;
class ATrainingDirector;
class AMissionDirector;

UCLASS(Blueprintable)
class CTLPROJECT_API UTutorialDialogueWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void InitializeDirector(ATrainingDirector* InDirector);
    void InitializeMission(AMissionDirector* InMission);
    void Present(const FText& Dialogue, const FText& Label, bool bCanContinue);
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> DialogueText;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> ActionLabel;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UButton> ContinueButton;
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    TWeakObjectPtr<ATrainingDirector> Director;
    TWeakObjectPtr<AMissionDirector> Mission;
    UFUNCTION() void ContinueClicked();
    UFUNCTION() void ContinueHovered();
};

UCLASS(Blueprintable)
class CTLPROJECT_API UTutorialObjectiveWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Present(const FText& Objective, const FText& Controls, const FText& Status);
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> ObjectiveText;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> ControlsText;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> StatusText;
};
