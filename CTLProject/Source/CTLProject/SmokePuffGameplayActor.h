#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SmokePuffGameplayActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ALizardGameplayActor;

/** One lightweight bubble with swept gameplay hits and independent visual expiry. */
UCLASS(Blueprintable)
class CTLPROJECT_API ASmokePuffGameplayActor : public AActor
{
    GENERATED_BODY()
public:
    ASmokePuffGameplayActor();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Smoke|Appearance", meta=(ToolTip="Existing smoke material opacity parameter.")) FName OpacityParameter = TEXT("Opacity");
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Smoke|Appearance", meta=(ClampMin="0", ClampMax="1", ToolTip="Opacity at the start of the existing linear fade.")) float InitialOpacity = .3f;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Smoke|Runtime", meta=(Units="cm")) float TravelRangeCm = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Smoke|Runtime", meta=(Units="cm")) float RadiusCm = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Smoke|Runtime", meta=(Units="s")) float LifetimeSeconds = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Smoke|Runtime", meta=(Units="s")) float AgeSeconds = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Smoke|Runtime", meta=(Units="cm")) float TravelledCm = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Smoke|Runtime") bool bBlocked = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Smoke|Runtime", meta=(Units="cm")) float MoveStepCm = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Smoke|Runtime") FVector SpawnPosition = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Smoke|Runtime") TObjectPtr<UMaterialInstanceDynamic> SmokeMaterial;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Smoke|Runtime") TArray<TObjectPtr<AActor>> TraceIgnoredActors;

    UFUNCTION(BlueprintCallable, Category="Smoke|Gameplay") void InitializeSmoke(float RangeCm, float SmokeRadiusCm, float LifeSeconds, AActor* SourceTool);
    UFUNCTION(BlueprintCallable, Category="Smoke|Gameplay") void UpdateSmoke(float DeltaSeconds);
protected:
    virtual void BeginPlay() override;
private:
    FCollisionQueryParams TraceParameters() const;
    void ApplyGameplayHits(const FVector& Start, const FVector& End, const FCollisionQueryParams& OcclusionParameters);
    TSet<TWeakObjectPtr<ALizardGameplayActor>> HitLizards;
    TWeakObjectPtr<UStaticMeshComponent> VisualMesh;
};
