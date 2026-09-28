#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LizardGameplayActor.generated.h"

class UInputAction;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class ASmokePuffGameplayActor;

/** Native behavior for the existing BP_Lizard. Its Blueprint owns the unchanged visual/collision components. */
UCLASS(Blueprintable)
class CTLPROJECT_API ALizardGameplayActor : public AActor
{
    GENERATED_BODY()

public:
    ALizardGameplayActor();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Threat", meta=(Units="cm", ClampMin="0", ToolTip="Hand proximity distance that starts alert/flee behavior."))
    float DetectionRadius = 80.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Movement", meta=(Units="cm/s", ClampMin="0", ToolTip="Speed while wandering along the current surface."))
    float WanderSpeed = 15.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Movement", meta=(Units="cm/s", ClampMin="0", ToolTip="Surface movement speed after the alert delay while fleeing."))
    float FleeSpeed = 70.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Threat", meta=(Units="s", ClampMin="0.01", ToolTip="Interval between hand and held broom threat checks."))
    float FleeReevaluateInterval = .2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Catch", meta=(Units="cm", ClampMin="0", ToolTip="Preserved prototype tuning value. Actual catch overlap is controlled by the Blueprint CatchSphere and Pawn hand volumes."))
    float CatchRadius = 12.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Threat", meta=(Units="s", ClampMin="0", ToolTip="Pause after initially detecting a threat, before fleeing starts."))
    float AlertDuration = .3f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Surface", meta=(ToolTip="Initial supporting surface normal. Runtime traces update it."))
    FVector SurfaceNormal = FVector::ForwardVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Surface", meta=(ToolTip="Initial direction along the supporting surface."))
    FVector MoveDirection = FVector::RightVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Broom", meta=(Units="s", ClampMin="0.01", ToolTip="Minimum interval between accepted relocations. Broom still handles separate-contact swing latching."))
    float RakeCooldown = .5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Broom", meta=(Units="cm", ClampMin="0", ToolTip="Maximum relocated center height above the supporting VR play-space floor."))
    float MaxRakeHeight = 140.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Broom", meta=(Units="s", ClampMin="0.01", ToolTip="Movement pause immediately after successful broom relocation."))
    float DazedDuration = .4f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Broom", meta=(Units="cm", ClampMin="0", ToolTip="Additional detection distance for a held broom tip."))
    float BroomDetectionRadiusBonus = 20.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Broom", meta=(Units="s", ClampMin="0.1", ToolTip="Minimum flee period after broom daze; ordinary threat checks cannot cancel it."))
    float ForcedFleeDuration = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Spray Exposure", meta=(ClampMin="0", ClampMax="100", Units="%", ToolTip="Exposure percentage points gained per distinct gameplay bubble. 100 percent triggers fall/stun."))
    float SmokeExposurePerHitPercent = 25.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Spray Exposure", meta=(ClampMin="0", ToolTip="Exposure percentage points removed per second after the last-hit decay delay (%/s)."))
    float SmokeExposureDecayPercentPerSecond = 50.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Spray Exposure", meta=(Units="s", ClampMin="0", ToolTip="Delay after the last accepted bubble hit before exposure begins decaying."))
    float SmokeExposureDecayDelaySeconds = .75f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Smoke", meta=(Units="s", ClampMin="0.01", ToolTip="Grounded stun duration. Countdown starts on landing; further spray cannot extend it."))
    float SmokeStunDurationSeconds = 3.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Stun Fall", meta=(Units="cm", ClampMin="1", ToolTip="Swept body radius used for gravity/collision during spray stun. Landing stand-off is radius plus 1 cm."))
    float SmokeFallCollisionRadiusCm = 8.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Stun Fall", meta=(ClampMin="0.01", ToolTip="Multiplier on world gravity during stunned falling."))
    float SmokeFallGravityScale = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Spray UI", meta=(Units="cm", ToolTip="World-space offset above the actor, independent of its wall/surface rotation."))
    FVector SprayWidgetWorldOffset = FVector(0, 0, 32.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lizard|Debug", meta=(ToolTip="Log bubble hits, exposure changes, stun activation, landing and recovery. Off by default."))
    bool bDebugSprayState = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lizard|Feedback", meta=(ToolTip="Existing mesh material parameter used for feedback where supported."))
    FName FeedbackColorParameter = TEXT("BodyColor");
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lizard|Feedback")
    FVector NormalBodyScale = FVector(.18);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lizard|Feedback")
    FVector CatchAndDazeBodyScale = FVector(.24);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lizard|Feedback")
    FVector StunnedBodyScale = FVector(.22);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lizard|Feedback")
    FVector NormalFeedbackColor = FVector(0, 1, 0);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lizard|Feedback")
    FVector CatchFeedbackColor = FVector(0, 1, 1);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lizard|Feedback")
    FVector DazeFeedbackColor = FVector(1, .5, .02);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lizard|Feedback")
    FVector StunFeedbackColor = FVector(.35, .08, 1);
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lizard|Feedback", meta=(Units="s", ClampMin="0.01", ToolTip="Existing catch/rake scale pulse duration."))
    float FeedbackPulseSeconds = .2f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lizard|Integration")
    TSoftClassPtr<AActor> BroomClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lizard|Integration")
    TSoftObjectPtr<UInputAction> LeftCatchAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lizard|Integration")
    TSoftObjectPtr<UInputAction> RightCatchAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lizard|Integration")
    TSoftObjectPtr<UInputAction> LeftReleaseAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lizard|Integration")
    TSoftObjectPtr<UInputAction> RightReleaseAction;

    // Keep the existing names so saved Blueprint callers and designer/test inspection remain compatible.
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") bool Caught = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") bool Fleeing = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") bool bStored = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") bool bDropped = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") bool bWasRaked = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") bool bSmokeStunned = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lizard|Runtime") bool bSmokeFalling = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") bool bRakeTargetFound = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") bool LeftGripDown = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") bool RightGripDown = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") float Elapsed = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") float ClosestDistance = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") float ReactionTime = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") float LastWanderTime = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") float RakeReadyTime = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") float DazedUntil = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") float ForceFleeUntil = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") float RakeBestDistance = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") float RakeFloorZ = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Lizard|Runtime", meta=(Units="%")) float SmokeExposurePercent = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") float SmokeStunUntil = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") float DropSpeedCmPerSecond = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") float DropStartedTime = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") FVector ThreatPosition = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") FVector RakeThreatPosition = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") FVector RakeTargetPosition = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") FVector RakeTargetNormal = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") FVector CatchSurfaceLocation = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") FVector CatchSurfaceNormal = FVector::ZeroVector;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") TObjectPtr<USceneComponent> LeftHand;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") TObjectPtr<USceneComponent> RightHand;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") TObjectPtr<UPrimitiveComponent> HandOverlapLeft;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") TObjectPtr<UPrimitiveComponent> HandOverlapRight;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category="Lizard|Runtime") TArray<TObjectPtr<AActor>> RakeIgnoredActors;

    UFUNCTION(BlueprintCallable, Category="Lizard|Threat") void SenseThreat();
    UFUNCTION(BlueprintCallable, Category="Lizard|Threat") void ConsiderThreatPosition(FVector Position, float RadiusBonus);
    UFUNCTION(BlueprintCallable, Category="Lizard|Threat") void SetFleeDirectionFromThreat(FVector Origin);
    UFUNCTION(BlueprintCallable, Category="Lizard|Catch") void TryCatchLeft();
    UFUNCTION(BlueprintCallable, Category="Lizard|Catch") void TryCatchRight();
    UFUNCTION(BlueprintCallable, Category="Lizard|Catch") void CanCatchWithHand(USceneComponent* Hand, bool& Allowed);
    UFUNCTION(BlueprintCallable, Category="Lizard|Catch") void CacheCatchSurface();
    UFUNCTION(BlueprintCallable, Category="Lizard|Catch") void ReleaseCaughtHand(USceneComponent* Hand);
    UFUNCTION(BlueprintCallable, Category="Lizard|Catch") void FinishDrop(FVector Location, FVector Normal);
    UFUNCTION(BlueprintCallable, Category="Lizard|Catch") void UpdateDroppedMotion(float DeltaSeconds, bool& CanMove);
    UFUNCTION(BlueprintCallable, Category="Lizard|Collection") void TryStoreInBottle(AActor* Bottle, bool& Accepted);
    UFUNCTION(BlueprintCallable, Category="Lizard|Broom") void ReceiveRake(FVector RakeOrigin, bool& Accepted);
    UFUNCTION(BlueprintCallable, Category="Lizard|Broom") void FindReachableRakeSurface(bool& Found);
    UFUNCTION(BlueprintCallable, Category="Lizard|Broom") float ResolveRakeFloor();
    UFUNCTION(BlueprintCallable, Category="Lizard|Broom") void EvaluateRakeCandidate(FVector Start, FVector End);
    UFUNCTION(BlueprintCallable, Category="Lizard|Broom") void UpdateRakeDaze(bool& CanMove);
    UFUNCTION(BlueprintCallable, Category="Lizard|Smoke") void ReceiveSprayHit(ASmokePuffGameplayActor* Bubble, bool& Accepted);
    UFUNCTION(BlueprintCallable, Category="Lizard|Smoke") void BeginSmokeStun();
    UFUNCTION(BlueprintCallable, Category="Lizard|Smoke") void UpdateSmokeStatus(float DeltaSeconds, bool& CanMove);
    UFUNCTION(BlueprintCallable, Category="Lizard|Feedback") void ApplySmokeStunFeedback();
    UFUNCTION(BlueprintCallable, Category="Lizard|Feedback") void RestoreCatchFeedback();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void ResolveInteractionComponents();
    void BindCatchInput();
    void TryCatch(USceneComponent* Hand, UPrimitiveComponent* HandVolume);
    void ReleaseLeft();
    void ReleaseRight();
    void MoveOnSurface(float DeltaSeconds);
    void ApplyFeedback(const FVector& Color, const FVector& Scale, bool ShowHalo);
    void StartFeedbackPulse();
    void StartThreatTimer();
    void UpdateExposureDecay(float DeltaSeconds);
    void UpdateStunnedFall(float DeltaSeconds);
    bool TryLandFromStun(const FVector& Start, const FVector& End);
    void LandFromStun(const FHitResult& Ground);
    void CancelSmokeState();
    void UpdateSprayWidget();
    bool GameplayInputLocked() const;
    float Now() const;
    FCollisionQueryParams TraceParameters(bool IgnoreRakeActors = false) const;
    TWeakObjectPtr<UStaticMeshComponent> BodyMesh;
    TWeakObjectPtr<UPrimitiveComponent> CatchVolume;
    TWeakObjectPtr<USceneComponent> StunHalo;
    TWeakObjectPtr<UWidgetComponent> SprayStatusWidget;
    TSet<TWeakObjectPtr<ASmokePuffGameplayActor>> ReceivedSprayBubbles;
    FVector SmokeFallVelocity = FVector::ZeroVector;
    float LastSprayHitTime = -100000.f;
    FTimerHandle ThreatTimer;
    FTimerHandle FeedbackTimer;
};
