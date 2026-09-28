#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LizardStatusWidget.generated.h"

class UProgressBar;
class UTextBlock;

/** Data presenter for the editable WBP_LizardSprayStatus layout. No gameplay state lives in the widget. */
UCLASS(Blueprintable)
class CTLPROJECT_API ULizardStatusWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetStatus(float Percent, bool Stunned, bool Falling, float RemainingSeconds);
    UPROPERTY(BlueprintReadOnly, Category="Lizard Status") float ExposurePercent = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Lizard Status") bool bStunned = false;
    UPROPERTY(BlueprintReadOnly, Category="Lizard Status") bool bFalling = false;
    UPROPERTY(BlueprintReadOnly, Category="Lizard Status") float StunSecondsRemaining = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Lizard Status|Visuals", meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> ExposureBar;
    UPROPERTY(BlueprintReadOnly, Category="Lizard Status|Visuals", meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> StatusText;
protected:
    virtual void NativeConstruct() override;
    UFUNCTION(BlueprintImplementableEvent, Category="Lizard Status", meta=(ToolTip="Optional visual-only styling/animation when the displayed data changes.")) void OnStatusChanged();
private:
    void RefreshDisplay();
};
