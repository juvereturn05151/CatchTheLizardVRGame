#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "VRMainMenuPawn.generated.h"

class UCameraComponent;
class UMotionControllerComponent;
class UWidgetComponent;
class UWidgetInteractionComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UInputMappingContext;
class UInputAction;
class UEnhancedInputLocalPlayerSubsystem;
class UVRMainMenuWidget;
class UCTLLevelList;

/** Isolated menu pawn: no grab, spray, movement or gameplay input bindings. */
UCLASS(Blueprintable)
class CTLPROJECT_API AVRMainMenuPawn : public APawn
{
    GENERATED_BODY()
public:
    AVRMainMenuPawn();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void PawnClientRestart() override;

    UFUNCTION(BlueprintCallable, Category="Main Menu") void StartGame();
    UFUNCTION(BlueprintCallable, Category="Main Menu") void ExitGame();
    UFUNCTION(BlueprintCallable, Category="Main Menu") void SelectLevel(FName Id);
    UFUNCTION(BlueprintCallable, Category="Main Menu") void ReturnToMainMenu();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Main Menu|Travel") TObjectPtr<UCTLLevelList> LevelList;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu|Travel", meta=(AllowedClasses="/Script/Engine.World"))
    TSoftObjectPtr<UWorld> GameplayMap;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Main Menu|Input") TObjectPtr<UInputMappingContext> MenuInputContext;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Main Menu|Input") TObjectPtr<UInputAction> LeftTriggerAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Main Menu|Input") TObjectPtr<UInputAction> RightTriggerAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu|Placement", meta=(ClampMin="100", ClampMax="400", Units="cm")) float MenuDistanceCm = 180.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu|Placement", meta=(ClampMin="-50", ClampMax="50", Units="cm")) float MenuHeightOffsetCm = -5.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu|Placement", meta=(ClampMin="0.1", ClampMax="3", Units="s")) float TrackingSettleSeconds = .35f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu|Input", meta=(ClampMin="0.3", ClampMax="1")) float TriggerPressThreshold = .65f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu|Input", meta=(ClampMin="0", ClampMax="0.29")) float TriggerReleaseThreshold = .2f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu|Ray", meta=(ClampMin="100", ClampMax="1000", Units="cm")) float RayLengthCm = 500.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu|Ray", meta=(ClampMin="0.05", ClampMax="1", Units="cm")) float RayDiameterCm = .25f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu|Ray") TObjectPtr<UMaterialInterface> RayMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu|Ray") FLinearColor RayColor = FLinearColor(.1f, .65f, 1.f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu|Ray") FLinearColor HoverRayColor = FLinearColor(.25f, 1.f, .45f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Main Menu|Ray") FLinearColor PressedRayColor = FLinearColor(1.f, .65f, .12f);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Main Menu|Components") TObjectPtr<USceneComponent> VROrigin;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Main Menu|Components") TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Main Menu|Components") TObjectPtr<UMotionControllerComponent> LeftAim;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Main Menu|Components") TObjectPtr<UMotionControllerComponent> RightAim;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Main Menu|Components") TObjectPtr<UWidgetComponent> MenuPanel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Main Menu|Components") TObjectPtr<UWidgetInteractionComponent> LeftInteraction;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Main Menu|Components") TObjectPtr<UWidgetInteractionComponent> RightInteraction;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Main Menu|Components") TObjectPtr<UStaticMeshComponent> LeftRay;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Main Menu|Components") TObjectPtr<UStaticMeshComponent> RightRay;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Main Menu|Components") TObjectPtr<UStaticMeshComponent> LeftPointer;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Main Menu|Components") TObjectPtr<UStaticMeshComponent> RightPointer;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Main Menu|State") bool bMenuPlaced = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Main Menu|State") bool bTransitionInProgress = false;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void InstallMenuInput();
    void PlaceMenu();
    void UpdateHand(int32 Hand);
    void UpdateRay(int32 Hand);
    void ReleasePointer(int32 Hand, bool bCancel);
    bool BeginTransition(const FText& Message);
    void FinishStart();
    void TravelTo(const TSoftObjectPtr<UWorld>& Destination);
    void FinishExit();
    void LeftTrigger(const FInputActionValue& Value);
    void RightTrigger(const FInputActionValue& Value);
    void LeftReleased();
    void RightReleased();
    void LeftCanceled();
    void RightCanceled();
    void DesktopPressed();
    void DesktopReleased();
    UWidgetInteractionComponent* Interaction(int32 Hand) const;
    bool HandAvailable(int32 Hand) const;

    UPROPERTY(Transient) TObjectPtr<UVRMainMenuWidget> MenuWidget;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> LeftRayMID;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RightRayMID;
    TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSubsystem;
    float TriggerValues[2] = {0.f, 0.f};
    bool bArmed[2] = {false, false};
    bool bPointerPressed[2] = {false, false};
    int32 ActivePointer = INDEX_NONE;
    bool bUsingHMD = false;
    float StableTrackingTime = 0.f;
    FTimerHandle TransitionTimer;
    TSoftObjectPtr<UWorld> PendingMap;
};
