#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrainingDirector.generated.h"
class ALizardGameplayActor;
class AMissionDirector;
class UCTLLevelList;
class UWidgetComponent;
class USphereComponent;
class UInputAction;
class UMaterialInterface;
class UVRPanelInteractor;
class UTutorialDialogueWidget;
class UTutorialObjectiveWidget;

UENUM(BlueprintType)
enum class ETrainingState : uint8 { Welcome, Instructions, Capture, Success, BroomExplanation, Finish, Traveling };

/** Per-world state machine. Actor references are assigned in the level; nothing is saved across visits. */
UCLASS(Blueprintable)
class CTLPROJECT_API ATrainingDirector : public AActor
{
    GENERATED_BODY()
public:
    ATrainingDirector();
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Tutorial|References") TObjectPtr<AMissionDirector> ExitMission;
    UFUNCTION(BlueprintCallable, Category="Tutorial") void AdvanceDialogue();
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Tutorial") ETrainingState State = ETrainingState::Welcome;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Tutorial|References") TObjectPtr<ALizardGameplayActor> TrainingDoll;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Tutorial|References") TObjectPtr<AActor> CaptureContainer;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Tutorial|References") TObjectPtr<AActor> TrainingBroom;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Tutorial|References") TObjectPtr<AActor> DollHighlight;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Tutorial|References") TObjectPtr<AActor> ContainerHighlight;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Tutorial|References") TObjectPtr<AActor> BroomHighlight;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Travel") TObjectPtr<UCTLLevelList> LevelList;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Travel") FName NextLevelId = TEXT("Level1");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Input") TObjectPtr<UInputAction> LeftTriggerAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Input") TObjectPtr<UInputAction> RightTriggerAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Input") TObjectPtr<UMaterialInterface> RayMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Recovery") FVector RecoveryBoundsMin = FVector(-400, -400, -30);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Recovery") FVector RecoveryBoundsMax = FVector(400, 400, 350);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Dialogue", meta=(MultiLine=true)) FText WelcomeDialogue;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Dialogue", meta=(MultiLine=true)) FText PracticeDialogue;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Dialogue", meta=(MultiLine=true)) FText SuccessDialogue;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Dialogue", meta=(MultiLine=true)) FText BroomDialogue;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Dialogue", meta=(MultiLine=true)) FText FinishDialogue;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Text", meta=(MultiLine=true)) FText CaptureObjective;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Text", meta=(MultiLine=true)) FText GripInstructions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Text") FText ContinueLabel;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Text") FText BeginLevelLabel;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Text") FText InProgressLabel;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tutorial|Text") FText CompleteLabel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tutorial|Components") TObjectPtr<UWidgetComponent> DialoguePanel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tutorial|Components") TObjectPtr<UWidgetComponent> ObjectivePanel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tutorial|Components") TObjectPtr<UVRPanelInteractor> PanelInteraction;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void InitializeTutorial();
    void EnterState(ETrainingState NewState);
    void StartDepositCheck();
    void EvaluateDeposit();
    void CheckRecovery();
    void BeginAssignment();
    void LoadAssignment();
    UFUNCTION() void DollCaught(ALizardGameplayActor* Lizard);
    UFUNCTION() void DollStored(ALizardGameplayActor* Lizard, AActor* Container);
    UFUNCTION() void MouthEntered(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Hit);
    UPROPERTY(Transient) TObjectPtr<USphereComponent> Mouth;
    UPROPERTY(Transient) TObjectPtr<UTutorialDialogueWidget> Dialogue;
    UPROPERTY(Transient) TObjectPtr<UTutorialObjectiveWidget> Objective;
    FTransform TableDollTransform;
    TSoftObjectPtr<UWorld> PendingMap;
    float NextAdvanceTime = 0.f;
    FTimerHandle DepositTimer;
    FTimerHandle RecoveryTimer;
    FTimerHandle TravelTimer;
    FTimerHandle InitializationTimer;
};
