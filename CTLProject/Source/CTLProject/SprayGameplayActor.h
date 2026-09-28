#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SprayGameplayActor.generated.h"

class UInputAction;
class USceneComponent;
class ASmokePuffGameplayActor;
struct FInputActionValue;

/** BP_Spray native emission/input. The unchanged XRFramework grab system controls attachment. */
UCLASS(Blueprintable)
class CTLPROJECT_API ASprayGameplayActor : public AActor
{
    GENERATED_BODY()
public:
    ASprayGameplayActor();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spray|Smoke", meta=(Units="cm", ClampMin="1", ClampMax="1000", ToolTip="Maximum puff travel distance along the nozzle's forward (+X) direction.")) float SprayRangeCm = 180.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spray|Smoke", meta=(Units="cm", ClampMin="2", ClampMax="100", ToolTip="Visible puff and gameplay exposure radius; also used for wall sweeps.")) float SmokeRadiusCm = 22.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spray|Smoke", meta=(Units="s", ClampMin="0.1", ClampMax="5", ToolTip="Independent lifetime of each emitted puff. Releasing the tool does not destroy existing puffs.")) float SmokeLifetimeSeconds = 1.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spray|Emission", meta=(Units="s", ClampMin="0.08", ToolTip="Interval between puffs, bounded to at least 0.08 seconds and one puff per frame.")) float EmissionIntervalSeconds = .15f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spray|Input", meta=(ClampMin="0", ClampMax="1", ToolTip="Holding controller's trigger axis threshold (0 to 1).")) float TriggerThreshold = .2f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spray|Integration", meta=(ToolTip="Blueprint puff subclass supplying the mesh and material. Saved here so cooking retains the puff asset.")) TSubclassOf<ASmokePuffGameplayActor> SmokePuffClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spray|Integration") TSoftObjectPtr<UInputAction> LeftTriggerAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spray|Integration") TSoftObjectPtr<UInputAction> RightTriggerAction;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Spray|Runtime", meta=(Units="s")) float EmissionClock = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Spray|Runtime") bool bEmitting = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Spray|Runtime") TObjectPtr<ASmokePuffGameplayActor> NewPuff;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Spray|Runtime") float LeftTriggerValue = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Spray|Runtime") float RightTriggerValue = 0.f;

    UFUNCTION(BlueprintCallable, Category="Spray|Gameplay") void ReadSprayTrigger(bool& Pressed);
    UFUNCTION(BlueprintCallable, Category="Spray|Gameplay") void UpdateEmission(float DeltaSeconds, bool TriggerPressed);
    UFUNCTION(BlueprintCallable, Category="Spray|Gameplay") void EmitSmokePuff();
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    UFUNCTION() void OnGrabStateChanged();
    void ResolveComponents();
    void BindTriggerInput();
    void SetLeftTrigger(const FInputActionValue& Value);
    void SetRightTrigger(const FInputActionValue& Value);
    void ClearLeftTrigger();
    void ClearRightTrigger();
    TWeakObjectPtr<USceneComponent> GrabState;
    TWeakObjectPtr<USceneComponent> NozzleComponent;
};
