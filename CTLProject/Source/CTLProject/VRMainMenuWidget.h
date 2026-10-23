#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VRMainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class AVRMainMenuPawn;

/** Behavior only; WBP_MainMenu owns the complete editable designer tree and button styles. */
UCLASS(Blueprintable)
class CTLPROJECT_API UVRMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void InitializeMenu(AVRMainMenuPawn* InPawn);
    void SetStatus(const FText& Message, bool bBusy);

    UPROPERTY(BlueprintReadOnly, Category="Menu", meta=(BindWidgetOptional)) TObjectPtr<UButton> StartButton;
    UPROPERTY(BlueprintReadOnly, Category="Menu", meta=(BindWidgetOptional)) TObjectPtr<UButton> ExitButton;
    UPROPERTY(BlueprintReadOnly, Category="Menu", meta=(BindWidgetOptional)) TObjectPtr<UButton> TutorialButton;
    UPROPERTY(BlueprintReadOnly, Category="Menu", meta=(BindWidgetOptional)) TObjectPtr<UButton> LevelOneButton;
    UPROPERTY(BlueprintReadOnly, Category="Menu", meta=(BindWidgetOptional)) TObjectPtr<UButton> BackButton;
    UPROPERTY(BlueprintReadOnly, Category="Menu", meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TutorialLabel;
    UPROPERTY(BlueprintReadOnly, Category="Menu", meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> LevelOneLabel;
    UPROPERTY(BlueprintReadOnly, Category="Menu", meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> StatusText;
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    TWeakObjectPtr<AVRMainMenuPawn> MenuPawn;
    UFUNCTION() void StartClicked();
    UFUNCTION() void ExitClicked();
    UFUNCTION() void TutorialClicked();
    UFUNCTION() void LevelOneClicked();
    UFUNCTION() void BackClicked();
};
