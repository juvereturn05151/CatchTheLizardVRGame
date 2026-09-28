#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BroomGameplayActor.generated.h"

class UPrimitiveComponent;
class USceneComponent;

/** Behavior for BP_Broom. Blueprint still owns its separate handle, tip, mesh and BP_GrabComponent. */
UCLASS(Blueprintable)
class CTLPROJECT_API ABroomGameplayActor : public AActor
{
    GENERATED_BODY()
public:
    ABroomGameplayActor();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Broom|Swing", meta=(Units="cm/s", ClampMin="0", ToolTip="Minimum world-space tip speed for a qualifying rake. Sustained contact is latched until separation."))
    float RakeVelocityThreshold = 150.f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Broom|Runtime") FVector PreviousHeadPosition = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Broom|Runtime") FVector HeadVelocity = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Broom|Runtime") bool bSampleValid = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Broom|Runtime") TArray<TObjectPtr<AActor>> ContactActors;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Broom|Runtime") TArray<TObjectPtr<AActor>> NextContacts;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Broom|Runtime") TObjectPtr<USceneComponent> LastHolder;

    UFUNCTION(BlueprintCallable, Category="Broom|Gameplay") void UpdateRake(float DeltaSeconds);
    UFUNCTION(BlueprintCallable, Category="Broom|Gameplay") void ProcessRakeContact(AActor* Target, bool AtTip);
    UFUNCTION(BlueprintCallable, Category="Broom|Integration") void GetHeldThreat(bool& Held, FVector& Position);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    UFUNCTION() void OnGrabStateChanged();
    void ResolveComponents();
    void ResetSample(USceneComponent* Holder);
    TWeakObjectPtr<USceneComponent> GrabState;
    TWeakObjectPtr<UPrimitiveComponent> TipVolume;
    TWeakObjectPtr<UClass> RakeInterface;
    TSet<TWeakObjectPtr<AActor>> AttemptedThisSample;
    FTransform PreviousPawnTransform;
    TWeakObjectPtr<AActor> PreviousHoldingPawn;
};
