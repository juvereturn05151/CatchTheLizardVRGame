#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ToolPalmGripComponent.generated.h"

class USceneComponent;

/** Aligns authored tool grip frames after the existing XRFramework accepts a grab. */
UCLASS(ClassGroup=VR, meta=(BlueprintSpawnableComponent))
class CTLPROJECT_API UToolPalmGripComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UToolPalmGripComponent();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grip|Object", meta=(ToolTip="Scene/Arrow component on this tool marking the left palm contact point and orientation.")) FName LeftGripPoint = TEXT("GripPointLeft");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grip|Object") FName RightGripPoint = TEXT("GripPointRight");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grip|Hand", meta=(ToolTip="Palm target on the holding pawn, under its left Grip motion controller.")) FName LeftPalmTarget = TEXT("PalmGripLeft");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grip|Hand") FName RightPalmTarget = TEXT("PalmGripRight");
    UFUNCTION(BlueprintCallable, Category="Grip") void AlignToPalm();
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    TWeakObjectPtr<USceneComponent> GrabState;
};
