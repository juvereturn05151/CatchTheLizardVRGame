#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "LizardAnimInstance.generated.h"
class UAnimSequence;
class UAnimBlueprint;

/** Animation presentation only. World movement stays in ALizardGameplayActor. */
UCLASS(Blueprintable, BlueprintType)
class CTLPROJECT_API ULizardAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion") TObjectPtr<UAnimSequence> IdleClip;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion") TObjectPtr<UAnimSequence> MovementClip;
    UPROPERTY(BlueprintReadOnly, Category="Locomotion") float SurfaceSpeed = 0;
    UPROPERTY(BlueprintReadOnly, Category="Locomotion") float MovementBlend = 0;
    UPROPERTY(BlueprintReadOnly, Category="Locomotion") float MovementPlayRate = 1;
    UPROPERTY(BlueprintReadOnly, Category="Locomotion") bool bLocomotionAllowed = false;
    UFUNCTION(BlueprintCallable, Category="Lizard|Editor") static bool AuthorAnimationGraph(UAnimBlueprint* Blueprint, UAnimSequence* Idle, UAnimSequence* Movement);
};
