#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StoredLizardDisplayComponent.generated.h"
class ALizardGameplayActor;

/** Presentation only: existing lizard/mission state remains authoritative. */
UCLASS(ClassGroup=VR, meta=(BlueprintSpawnableComponent))
class CTLPROJECT_API UStoredLizardDisplayComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UStoredLizardDisplayComponent();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contents") TArray<FName> InteriorSlots = {TEXT("StoredSlot1"), TEXT("StoredSlot2"), TEXT("StoredSlot3")};
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contents", meta=(ToolTip="Scale multiplier for the original lizard's visual mesh, not its gameplay/collision root.")) FVector StoredModelScale = FVector(.48f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contents", meta=(Units="cm")) FVector StoredModelOffset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Contents", meta=(ToolTip="Extra visual spacing if a designer allows more deposits than authored interior slots. Does not change capture capacity.")) FVector OverflowSlotStep = FVector(0, 0, 7);
    void ShowStoredLizard(ALizardGameplayActor* Lizard);
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    TArray<TWeakObjectPtr<ALizardGameplayActor>> Occupants;
};
