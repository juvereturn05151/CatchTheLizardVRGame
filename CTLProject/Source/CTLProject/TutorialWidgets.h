#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialWidgets.generated.h"
class UButton;
class UTextBlock;
class ATrainingDirector;

UCLASS(Blueprintable)
class CTLPROJECT_API UTutorialDialogueWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void InitializeDirector(ATrainingDirector* InDirector);
    void Present(const FText& Dialogue, const FText& Label, bool bCanContinue);
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> DialogueText;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> ActionLabel;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UButton> ContinueButton;
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    TWeakObjectPtr<ATrainingDirector> Director;
    UFUNCTION() void ContinueClicked();
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
