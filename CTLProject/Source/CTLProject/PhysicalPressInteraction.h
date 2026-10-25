#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicalPressInteraction.generated.h"
class UInputAction;
class UEnhancedInputComponent;
class USceneComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPhysicalPressed);

/** A fresh index-trigger press with a tracked hand at the physical control. No proximity activation. */
UCLASS(ClassGroup=(VR), meta=(BlueprintSpawnableComponent))
class CTLPROJECT_API UPhysicalPressInteraction : public UActorComponent
{
    GENERATED_BODY()
public:
    UPhysicalPressInteraction();
    virtual void TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Tick) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction") TObjectPtr<USceneComponent> PressPoint;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="1", Units="cm")) float ReachRadius = 20.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction") TObjectPtr<UInputAction> LeftAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction") TObjectPtr<UInputAction> RightAction;
    UPROPERTY(BlueprintAssignable) FPhysicalPressed OnPressed;
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void Left(const struct FInputActionValue& Value);
    void Right(const struct FInputActionValue& Value);
    void LeftUp();
    void RightUp();
    void Value(int32 Hand, float Amount);
    UPROPERTY(Transient) TObjectPtr<UEnhancedInputComponent> PressInput;
    bool Armed[2] = {false, false};
};
