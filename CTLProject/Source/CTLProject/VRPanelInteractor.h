#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "VRPanelInteractor.generated.h"
class UWidgetInteractionComponent;
class UUserWidget;
class UInputAction;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UEnhancedInputComponent;
class APawn;
class APlayerController;

/** Reuses the XR Pawn's WidgetInteractions with the main menu's trigger/ray conventions. */
UCLASS(ClassGroup=UI, meta=(BlueprintSpawnableComponent))
class CTLPROJECT_API UVRPanelInteractor : public UActorComponent
{
    GENERATED_BODY()
public:
    UVRPanelInteractor();
    void Initialize(APawn* Pawn, UUserWidget* Widget, UInputAction* Left, UInputAction* Right, UMaterialInterface* RayMaterial);
    void LockForSeconds(float Seconds);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void Release(int32 Hand, bool bCancel);
    void LeftValue(const FInputActionValue& Value);
    void RightValue(const FInputActionValue& Value);
    void LeftReleased();
    void RightReleased();
    void LeftCanceled();
    void RightCanceled();
    void MousePressed();
    void MouseReleased();
    UPROPERTY(Transient) TArray<TObjectPtr<UWidgetInteractionComponent>> Pointers;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Rays;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Dots;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
    UPROPERTY(Transient) TObjectPtr<UEnhancedInputComponent> MenuInput;
    TWeakObjectPtr<UUserWidget> Target;
    TWeakObjectPtr<APlayerController> Player;
    float Values[2] = {0, 0};
    bool Armed[2] = {false, false};
    bool Pressed[2] = {false, false};
    bool bHMD = false;
    int32 Active = INDEX_NONE;
    float LockedUntil = 0.f;
};
