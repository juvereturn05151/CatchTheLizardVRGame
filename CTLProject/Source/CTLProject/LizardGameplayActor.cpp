#include "LizardGameplayActor.h"

#include "LizardBlueprintBridge.h"
#include "LizardStatusWidget.h"
#include "SmokePuffGameplayActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RotationMatrix.h"
#include "TimerManager.h"

using namespace LizardBlueprintBridge;

namespace
{
    template<typename T> T* NamedComponent(AActor* Actor, FName Name)
    {
        if (!IsValid(Actor)) return nullptr;
        TArray<T*> Components;
        Actor->GetComponents<T>(Components);
        for (T* Component : Components)
            if (Component->GetFName() == Name) return Component;
        return nullptr;
    }
}

ALizardGameplayActor::ALizardGameplayActor()
{
    PrimaryActorTick.bCanEverTick = true;
    // Do not create replacement components: the existing Blueprint SCS owns them and their saved overrides.
    BroomClass = FSoftObjectPath(TEXT("/Game/CatchTheLizard/BP_Broom.BP_Broom_C"));
    LeftCatchAction = FSoftObjectPath(TEXT("/Game/XRFramework/Input/Actions/IA_Grab_Left_Pressed.IA_Grab_Left_Pressed"));
    RightCatchAction = FSoftObjectPath(TEXT("/Game/XRFramework/Input/Actions/IA_Grab_Right_Pressed.IA_Grab_Right_Pressed"));
    LeftReleaseAction = FSoftObjectPath(TEXT("/Game/XRFramework/Input/Actions/IA_Grab_Left_Released.IA_Grab_Left_Released"));
    RightReleaseAction = FSoftObjectPath(TEXT("/Game/XRFramework/Input/Actions/IA_Grab_Right_Released.IA_Grab_Right_Released"));
}

float ALizardGameplayActor::Now() const
{
    return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

void ALizardGameplayActor::ResolveInteractionComponents()
{
    BodyMesh = NamedComponent<UStaticMeshComponent>(this, TEXT("LizardBody"));
    CatchVolume = NamedComponent<UPrimitiveComponent>(this, TEXT("CatchSphere"));
    StunHalo = NamedComponent<USceneComponent>(this, TEXT("SmokeStunHalo"));
    SprayStatusWidget = NamedComponent<UWidgetComponent>(this, TEXT("SprayStatusWidget"));
    APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    LeftHand = Cast<USceneComponent>(Object(Pawn, TEXT("MotionControllerLeftGrip")));
    RightHand = Cast<USceneComponent>(Object(Pawn, TEXT("MotionControllerRightGrip")));
    HandOverlapLeft = Cast<UPrimitiveComponent>(Object(Pawn, TEXT("HandCatchLeft")));
    HandOverlapRight = Cast<UPrimitiveComponent>(Object(Pawn, TEXT("HandCatchRight")));
}

void ALizardGameplayActor::BeginPlay()
{
    Super::BeginPlay();
    ResolveInteractionComponents();
    BroomClass.LoadSynchronous();
    if (!BodyMesh.IsValid() || !CatchVolume.IsValid() || !LeftHand || !RightHand)
        UE_LOG(LogTemp, Error, TEXT("%s: missing BP_Lizard visual/catch components or XR Pawn hand references"), *GetName());
    if (!bStored)
    {
        BindCatchInput();
        if (!Caught) StartThreatTimer();
    }
    if (SprayStatusWidget.IsValid()) SprayStatusWidget->InitWidget();
    else UE_LOG(LogTemp, Error, TEXT("%s: missing Blueprint SprayStatusWidget component"), *GetName());
    UpdateSprayWidget();
}

void ALizardGameplayActor::BindCatchInput()
{
    CreateInputComponent(UEnhancedInputComponent::StaticClass());
    auto* Enhanced = Cast<UEnhancedInputComponent>(InputComponent);
    if (!Enhanced) return;
    struct FBinding { TSoftObjectPtr<UInputAction>* Action; void(ALizardGameplayActor::*Method)(); };
    const FBinding Bindings[] = {
        {&LeftCatchAction, &ALizardGameplayActor::TryCatchLeft}, {&RightCatchAction, &ALizardGameplayActor::TryCatchRight},
        {&LeftReleaseAction, &ALizardGameplayActor::ReleaseLeft}, {&RightReleaseAction, &ALizardGameplayActor::ReleaseRight}
    };
    for (const FBinding& Binding : Bindings)
    {
        if (const UInputAction* Action = Binding.Action->LoadSynchronous())
            Enhanced->BindAction(Action, ETriggerEvent::Triggered, this, Binding.Method);
        else UE_LOG(LogTemp, Error, TEXT("%s: missing catch/release action %s"), *GetName(), *Binding.Action->ToString());
    }
    EnableInput(UGameplayStatics::GetPlayerController(this, 0));
}

void ALizardGameplayActor::StartThreatTimer()
{
    GetWorldTimerManager().SetTimer(ThreatTimer, this, &ALizardGameplayActor::SenseThreat, FMath::Max(.01f, FleeReevaluateInterval), true);
}

void ALizardGameplayActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(ThreatTimer);
    GetWorldTimerManager().ClearTimer(FeedbackTimer);
    Super::EndPlay(EndPlayReason);
}

void ALizardGameplayActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Caught || bStored) return;
    bool CanMove = false;
    // Incapacitation has priority over ordinary drop motion, broom daze and surface attachment.
    UpdateSmokeStatus(DeltaSeconds, CanMove);
    UpdateSprayWidget();
    if (!CanMove) return;
    UpdateDroppedMotion(DeltaSeconds, CanMove);
    if (!CanMove) return;
    UpdateRakeDaze(CanMove);
    if (CanMove && IsValid(LeftHand)) MoveOnSurface(DeltaSeconds);
}

FCollisionQueryParams ALizardGameplayActor::TraceParameters(bool IgnoreRakeActors) const
{
    FCollisionQueryParams Parameters(SCENE_QUERY_STAT(LizardSurface), false, this);
    if (IgnoreRakeActors)
        for (const AActor* Actor : RakeIgnoredActors) if (IsValid(Actor)) Parameters.AddIgnoredActor(Actor);
    return Parameters;
}

void ALizardGameplayActor::MoveOnSurface(float DeltaSeconds)
{
    const FVector Position = GetActorLocation();
    const float Speed = Fleeing ? (Now() - ReactionTime > AlertDuration ? FleeSpeed : 0.f) : WanderSpeed;
    const float Step = Speed * DeltaSeconds;
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Position, Position + MoveDirection * (Step + 12.f), ECC_Visibility, TraceParameters()))
    {
        // Preserve the prototype's front-hit turn and stand-off, rather than replacing it with navigation/physics.
        SurfaceNormal = Hit.Normal;
        MoveDirection = SurfaceNormal;
        SetActorLocation(Hit.Location + Hit.Normal * 9.f, false, nullptr, ETeleportType::TeleportPhysics);
    }
    else
    {
        const FVector Destination = Position + MoveDirection * Step;
        if (GetWorld()->LineTraceSingleByChannel(Hit, Destination, Destination - SurfaceNormal * 35.f, ECC_Visibility, TraceParameters()))
        {
            SurfaceNormal = Hit.Normal;
            SetActorLocation(Hit.Location + Hit.Normal * 9.f, false, nullptr, ETeleportType::TeleportPhysics);
        }
        else MoveDirection = -MoveDirection;
    }
    SetActorRotation(FRotationMatrix::MakeFromXZ(MoveDirection, SurfaceNormal).Rotator(), ETeleportType::TeleportPhysics);
}

void ALizardGameplayActor::ConsiderThreatPosition(FVector Position, float RadiusBonus)
{
    const float Distance = FVector::Distance(GetActorLocation(), Position) - FMath::Max(0.f, RadiusBonus);
    if (Distance < ClosestDistance) { ClosestDistance = Distance; ThreatPosition = Position; }
}

void ALizardGameplayActor::SetFleeDirectionFromThreat(FVector Origin)
{
    FVector Away = FVector::VectorPlaneProject(GetActorLocation() - Origin, SurfaceNormal);
    if (Away.Size() < .1) Away = FVector::VectorPlaneProject(FVector::ForwardVector, SurfaceNormal);
    if (Away.Size() < .1) Away = FVector::VectorPlaneProject(FVector::RightVector, SurfaceNormal);
    MoveDirection = Away.GetSafeNormal();
}

void ALizardGameplayActor::SenseThreat()
{
    if (Caught || bStored || bDropped || bSmokeStunned || bWasRaked) return;
    ClosestDistance = 100000.f;
    if (IsValid(LeftHand)) ConsiderThreatPosition(LeftHand->GetComponentLocation(), 0.f);
    if (IsValid(RightHand)) ConsiderThreatPosition(RightHand->GetComponentLocation(), 0.f);
    if (UClass* Class = BroomClass.Get())
    {
        TArray<AActor*> Brooms;
        UGameplayStatics::GetAllActorsOfClass(this, Class, Brooms);
        for (AActor* Broom : Brooms)
        {
            FCall Query(Broom, TEXT("GetHeldThreat"));
            if (Query.Invoke() && Query.GetBool(TEXT("Held"))) ConsiderThreatPosition(Query.GetVector(TEXT("Position")), BroomDetectionRadiusBonus);
        }
    }
    if (ClosestDistance < DetectionRadius)
    {
        if (!Fleeing) ReactionTime = Now();
        Fleeing = true;
        SetFleeDirectionFromThreat(ThreatPosition);
    }
    else if (Now() < ForceFleeUntil)
    {
        Fleeing = true;
        SetFleeDirectionFromThreat(RakeThreatPosition);
    }
    else
    {
        Fleeing = false;
        if (Now() - LastWanderTime >= 2.f)
        {
            LastWanderTime = Now();
            MoveDirection = FVector::VectorPlaneProject(FMath::VRand(), SurfaceNormal).GetSafeNormal();
        }
    }
}

bool ALizardGameplayActor::GameplayInputLocked() const
{
    return Bool(UGameplayStatics::GetPlayerPawn(this, 0), TEXT("bCollectionInputLocked"));
}

void ALizardGameplayActor::CanCatchWithHand(USceneComponent* Hand, bool& Allowed)
{
    Allowed = false;
    if (Caught || bStored || !IsValid(Hand) || GameplayInputLocked()) return;
    APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    FCall Occupied(Pawn, TEXT("IsToolHandOccupied"));
    if (!Occupied.SetObject(TEXT("Hand"), Hand) || !Occupied.Invoke() || Occupied.GetBool(TEXT("Occupied"))) return;
    // The existing pickup query takes priority on a shared grip press. This avoids catch + tool pickup in either event order.
    FCall Nearby(Pawn, TEXT("GetGrabComponentNearMotionController"));
    if (!Nearby.SetObject(TEXT("MotionController"), Hand) || !Nearby.Invoke()) return;
    Allowed = !IsValid(Nearby.GetObject(TEXT("NearestComponent")));
}

void ALizardGameplayActor::CacheCatchSurface()
{
    if (!bDropped && !bSmokeFalling) { CatchSurfaceLocation = GetActorLocation(); CatchSurfaceNormal = SurfaceNormal; }
}

void ALizardGameplayActor::TryCatch(USceneComponent* Hand, UPrimitiveComponent* HandVolume)
{
    bool Allowed = false;
    CanCatchWithHand(Hand, Allowed);
    if (!Allowed || !CatchVolume.IsValid() || !IsValid(HandVolume) || !CatchVolume->IsOverlappingComponent(HandVolume)) return;
    CacheCatchSurface();
    const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false);
    if (!AttachToComponent(Hand, Rules)) return;
    Caught = true;
    CancelSmokeState();
    bDropped = false;
    Fleeing = false;
    SetActorTickEnabled(false);
    GetWorldTimerManager().ClearTimer(ThreatTimer);
    ApplyFeedback(CatchFeedbackColor, CatchAndDazeBodyScale, false);
    StartFeedbackPulse();
}

void ALizardGameplayActor::TryCatchLeft() { TryCatch(LeftHand, HandOverlapLeft); }
void ALizardGameplayActor::TryCatchRight() { TryCatch(RightHand, HandOverlapRight); }
void ALizardGameplayActor::ReleaseLeft() { ReleaseCaughtHand(LeftHand); }
void ALizardGameplayActor::ReleaseRight() { ReleaseCaughtHand(RightHand); }

void ALizardGameplayActor::ReleaseCaughtHand(USceneComponent* Hand)
{
    if (!Caught || bStored || !IsValid(Hand) || !GetRootComponent() || GetRootComponent()->GetAttachParent() != Hand) return;
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Caught = false;
    bDropped = true;
    DropSpeedCmPerSecond = 0.f;
    DropStartedTime = Now();
    CancelSmokeState();
    bWasRaked = Fleeing = false;
    ForceFleeUntil = 0.f;
    GetWorldTimerManager().ClearTimer(FeedbackTimer);
    RestoreCatchFeedback();
    SetActorTickEnabled(true);
    StartThreatTimer();
}

void ALizardGameplayActor::FinishDrop(FVector Location, FVector Normal)
{
    SurfaceNormal = Normal;
    SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
    SetActorRotation(FRotationMatrix::MakeFromXZ(MoveDirection, SurfaceNormal).Rotator(), ETeleportType::TeleportPhysics);
    bDropped = false;
    SenseThreat();
}

void ALizardGameplayActor::UpdateDroppedMotion(float DeltaSeconds, bool& CanMove)
{
    CanMove = false;
    if (Caught || bStored) return;
    if (!bDropped) { CanMove = true; return; }
    DropSpeedCmPerSecond = FMath::Min(DropSpeedCmPerSecond + DeltaSeconds * 980.f, 1000.f);
    const FVector Destination = GetActorLocation() - FVector(0, 0, DropSpeedCmPerSecond * DeltaSeconds);
    FHitResult Hit;
    if (GetWorld()->SweepSingleByObjectType(Hit, GetActorLocation(), Destination, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic), FCollisionShape::MakeSphere(8.f), TraceParameters()))
    {
        FinishDrop(Hit.ImpactPoint + Hit.ImpactNormal * 9.f, Hit.ImpactNormal);
        CanMove = true;
    }
    else if (Now() - DropStartedTime >= 2.5f)
    {
        FinishDrop(CatchSurfaceLocation, CatchSurfaceNormal);
        CanMove = true;
    }
    else SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);
}

void ALizardGameplayActor::TryStoreInBottle(AActor* Bottle, bool& Accepted)
{
    Accepted = false;
    if (!Caught || bStored || !IsValid(Bottle) || !GetRootComponent() || !CatchVolume.IsValid()) return;
    UObject* Grab = Object(Bottle, TEXT("GrabComponent"));
    USceneComponent* BottleHand = Cast<USceneComponent>(Object(Grab, TEXT("MotionControllerRef")));
    USceneComponent* CatchHand = GetRootComponent()->GetAttachParent();
    UPrimitiveComponent* Mouth = Cast<UPrimitiveComponent>(Object(Bottle, TEXT("DepositVolume")));
    if (!Bool(Grab, TEXT("bIsHeld")) || !IsValid(BottleHand) || (CatchHand != LeftHand && CatchHand != RightHand)
        || !IsValid(CatchHand) || CatchHand == BottleHand || !IsValid(Mouth) || !Mouth->IsOverlappingComponent(CatchVolume.Get())) return;
    bStored = true; // Set terminal state before any detach, overlap, or timer callbacks can run.
    CancelSmokeState();
    bDropped = bSmokeStunned = bWasRaked = Fleeing = false;
    ForceFleeUntil = 0.f;
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    SetActorTickEnabled(false);
    GetWorldTimerManager().ClearTimer(ThreatTimer);
    GetWorldTimerManager().ClearTimer(FeedbackTimer);
    DisableInput(UGameplayStatics::GetPlayerController(this, 0));
    if (StunHalo.IsValid()) StunHalo->SetVisibility(false);
    // Preserve Caught=true for existing collection/catch guards; bStored prevents every recovery path.
    Accepted = true;
}

float ALizardGameplayActor::ResolveRakeFloor()
{
    if (!IsValid(LeftHand) || !IsValid(LeftHand->GetAttachParent())) return 0.f;
    const FVector Origin = LeftHand->GetAttachParent()->GetComponentLocation();
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByObjectType(Hit, Origin + FVector(0, 0, 20), Origin - FVector(0, 0, 2000), FCollisionObjectQueryParams(ECC_WorldStatic), TraceParameters())
        && Hit.ImpactNormal.Z >= .5) return Hit.ImpactPoint.Z;
    return Origin.Z;
}

void ALizardGameplayActor::EvaluateRakeCandidate(FVector Start, FVector End)
{
    FHitResult Hit;
    const FCollisionQueryParams Parameters = TraceParameters(true);
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Parameters)) return;
    const FVector Normal = Hit.ImpactNormal;
    const FVector Point = Hit.ImpactPoint + Normal * 9.f;
    const float Height = Point.Z - RakeFloorZ;
    const float Distance = FVector::Distance(GetActorLocation(), Point);
    if (Height < 0.f || Height >= MaxRakeHeight + .01f || Normal.Z < -.15 || Distance >= RakeBestDistance) return;
    FHitResult Obstruction;
    if (GetWorld()->LineTraceSingleByChannel(Obstruction, GetActorLocation(), Point, ECC_Visibility, Parameters)) return;
    if (GetWorld()->SweepSingleByChannel(Obstruction, Point, Point, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(8.f), Parameters)) return;
    RakeBestDistance = Distance;
    RakeTargetPosition = Point;
    RakeTargetNormal = Normal;
    bRakeTargetFound = true;
}

void ALizardGameplayActor::FindReachableRakeSurface(bool& Found)
{
    Found = bRakeTargetFound = false;
    RakeBestDistance = 100000.f;
    if (!IsValid(LeftHand) || !IsValid(LeftHand->GetAttachParent())) return;
    RakeFloorZ = ResolveRakeFloor();
    TArray<AActor*> Brooms;
    if (UClass* Class = BroomClass.Get()) UGameplayStatics::GetAllActorsOfClass(this, Class, Brooms);
    RakeIgnoredActors.Reset();
    for (AActor* Broom : Brooms) RakeIgnoredActors.Add(Broom);
    RakeIgnoredActors.AddUnique(UGameplayStatics::GetPlayerPawn(this, 0));
    const FVector Base = GetActorLocation() + SurfaceNormal * 40.f;
    EvaluateRakeCandidate(GetActorLocation(), FVector(Base.X, Base.Y, RakeFloorZ - 50.f));
    const float Top = FMath::Max(10.f, MaxRakeHeight - 10.f);
    const FVector2D Offsets[] = { {0,0}, {50,0}, {-50,0}, {0,50}, {0,-50} };
    for (const FVector2D& Offset : Offsets)
        EvaluateRakeCandidate(FVector(Base.X + Offset.X, Base.Y + Offset.Y, RakeFloorZ + Top), FVector(Base.X + Offset.X, Base.Y + Offset.Y, RakeFloorZ - 30.f));
    const float Fractions[] = { .25f, .6f, 1.f };
    const FVector Directions[] = { {300,0,0}, {-300,0,0}, {0,300,0}, {0,-300,0} };
    for (float Fraction : Fractions)
    {
        const FVector Start(Base.X, Base.Y, RakeFloorZ + Top * Fraction);
        for (const FVector& Direction : Directions) EvaluateRakeCandidate(Start, Start + Direction);
    }
    Found = bRakeTargetFound;
}

void ALizardGameplayActor::ReceiveRake(FVector RakeOrigin, bool& Accepted)
{
    Accepted = false;
    if (Caught || bStored || bDropped || bSmokeStunned || Now() < RakeReadyTime) return;
    bool Found = false;
    FindReachableRakeSurface(Found);
    if (!Found) return;
    SetActorLocation(RakeTargetPosition, false, nullptr, ETeleportType::TeleportPhysics);
    SurfaceNormal = RakeTargetNormal;
    RakeThreatPosition = RakeOrigin;
    SetFleeDirectionFromThreat(RakeOrigin);
    SetActorRotation(FRotationMatrix::MakeFromXZ(MoveDirection, SurfaceNormal).Rotator(), ETeleportType::TeleportPhysics);
    bWasRaked = Fleeing = true;
    RakeReadyTime = Now() + FMath::Max(.01f, RakeCooldown);
    DazedUntil = Now() + FMath::Max(.01f, DazedDuration);
    ForceFleeUntil = DazedUntil + FMath::Max(.1f, ForcedFleeDuration);
    ApplyFeedback(DazeFeedbackColor, CatchAndDazeBodyScale, false);
    StartFeedbackPulse();
    Accepted = true;
}

void ALizardGameplayActor::UpdateRakeDaze(bool& CanMove)
{
    CanMove = false;
    if (Caught || bStored || bSmokeStunned) return;
    if (bWasRaked)
    {
        if (Now() < DazedUntil) return;
        bWasRaked = false;
        Fleeing = true;
        ReactionTime = Now() - AlertDuration - .01f;
        SetFleeDirectionFromThreat(RakeThreatPosition);
    }
    CanMove = true;
}

void ALizardGameplayActor::ReceiveSprayHit(ASmokePuffGameplayActor* Bubble, bool& Accepted)
{
    Accepted = false;
    if (!IsValid(Bubble)) return;
    for (auto It = ReceivedSprayBubbles.CreateIterator(); It; ++It)
        if (!It->IsValid()) It.RemoveCurrent();
    if (ReceivedSprayBubbles.Contains(Bubble)) return;
    ReceivedSprayBubbles.Add(Bubble);
    if (bDebugSprayState) UE_LOG(LogTemp, Display, TEXT("[Spray] %s hit %s%s"), *Bubble->GetName(), *GetName(),
        (Caught || bStored || bSmokeStunned) ? TEXT(" (ignored: caught/stored/incapacitated)") : TEXT(""));
    if (Caught || bStored || bSmokeStunned) return;
    const float Previous = SmokeExposurePercent;
    SmokeExposurePercent = FMath::Clamp(Previous + FMath::Clamp(SmokeExposurePerHitPercent, 0.f, 100.f), 0.f, 100.f);
    LastSprayHitTime = Now();
    Accepted = true;
    if (bDebugSprayState) UE_LOG(LogTemp, Display, TEXT("[Spray] %s exposure %.1f%% -> %.1f%%"), *GetName(), Previous, SmokeExposurePercent);
    if (SmokeExposurePercent >= 100.f) BeginSmokeStun();
    UpdateSprayWidget();
}

void ALizardGameplayActor::UpdateExposureDecay(float DeltaSeconds)
{
    if (SmokeExposurePercent <= 0.f) return;
    // Only the part of this tick after the delay expires contributes to decay.
    const float Seconds = FMath::Max(0.f, DeltaSeconds);
    const float EligibleSeconds = FMath::Clamp(Now() - LastSprayHitTime - FMath::Max(0.f, SmokeExposureDecayDelaySeconds), 0.f, Seconds);
    const float Previous = SmokeExposurePercent;
    SmokeExposurePercent = FMath::Max(0.f, Previous - FMath::Max(0.f, SmokeExposureDecayPercentPerSecond) * EligibleSeconds);
    if (bDebugSprayState && !FMath::IsNearlyEqual(Previous, SmokeExposurePercent))
        UE_LOG(LogTemp, Display, TEXT("[Spray] %s exposure decayed %.1f%% -> %.1f%%"), *GetName(), Previous, SmokeExposurePercent);
}

void ALizardGameplayActor::BeginSmokeStun()
{
    if (Caught || bStored || bSmokeStunned) return;
    CacheCatchSurface();
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    SmokeExposurePercent = 100.f;
    bSmokeStunned = true;
    bSmokeFalling = true;
    bDropped = false;
    SmokeFallVelocity = FVector::ZeroVector;
    SmokeStunUntil = 0.f; // No recovery deadline until supported by ground.
    bWasRaked = Fleeing = false;
    ForceFleeUntil = DazedUntil = 0.f;
    GetWorldTimerManager().ClearTimer(FeedbackTimer);
    ApplySmokeStunFeedback();
    if (bDebugSprayState) UE_LOG(LogTemp, Display, TEXT("[Spray] %s stun activated; ground countdown pending"), *GetName());
    const FVector Start = GetActorLocation();
    TryLandFromStun(Start, Start - FVector(0, 0, 2.f));
    UpdateSprayWidget();
}

bool ALizardGameplayActor::TryLandFromStun(const FVector& Start, const FVector& End)
{
    FHitResult Ground;
    if (GetWorld()->SweepSingleByObjectType(Ground, Start, End, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic),
        FCollisionShape::MakeSphere(FMath::Max(1.f, SmokeFallCollisionRadiusCm)), TraceParameters()) && Ground.ImpactNormal.Z >= .65f)
    {
        LandFromStun(Ground);
        return true;
    }
    return false;
}

void ALizardGameplayActor::LandFromStun(const FHitResult& Ground)
{
    if (Caught || bStored || !bSmokeStunned) return;
    SurfaceNormal = Ground.ImpactNormal.GetSafeNormal();
    MoveDirection = FVector::VectorPlaneProject(MoveDirection, SurfaceNormal).GetSafeNormal();
    if (MoveDirection.IsNearlyZero()) MoveDirection = FVector::VectorPlaneProject(FVector::ForwardVector, SurfaceNormal).GetSafeNormal();
    SetActorLocation(Ground.ImpactPoint + SurfaceNormal * (FMath::Max(1.f, SmokeFallCollisionRadiusCm) + 1.f), false, nullptr, ETeleportType::TeleportPhysics);
    SetActorRotation(FRotationMatrix::MakeFromXZ(MoveDirection, SurfaceNormal).Rotator(), ETeleportType::TeleportPhysics);
    SmokeFallVelocity = FVector::ZeroVector;
    bSmokeFalling = false;
    SmokeStunUntil = Now() + FMath::Max(.01f, SmokeStunDurationSeconds);
    CatchSurfaceLocation = GetActorLocation();
    CatchSurfaceNormal = SurfaceNormal;
    if (bDebugSprayState) UE_LOG(LogTemp, Display, TEXT("[Spray] %s landed; %.2fs grounded stun"), *GetName(), SmokeStunDurationSeconds);
    UpdateSprayWidget();
}

void ALizardGameplayActor::UpdateStunnedFall(float DeltaSeconds)
{
    // Small swept substeps preserve collision at low Editor/VR frame rates. No surface trace runs here.
    const float Seconds = FMath::Max(0.f, DeltaSeconds);
    const int32 Steps = FMath::Clamp(FMath::CeilToInt(Seconds / (1.f / 60.f)), 1, 120);
    const float Step = Seconds / Steps;
    const float Radius = FMath::Max(1.f, SmokeFallCollisionRadiusCm);
    for (int32 Index = 0; Index < Steps && bSmokeFalling; ++Index)
    {
        SmokeFallVelocity.Z = FMath::Max(-1000.f, SmokeFallVelocity.Z + GetWorld()->GetGravityZ() * FMath::Max(.01f, SmokeFallGravityScale) * Step);
        const FVector Start = GetActorLocation();
        const FVector End = Start + SmokeFallVelocity * Step;
        FHitResult Hit;
        if (GetWorld()->SweepSingleByObjectType(Hit, Start, End, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldStatic),
            FCollisionShape::MakeSphere(Radius), TraceParameters()))
        {
            if (Hit.ImpactNormal.Z >= .65f) { LandFromStun(Hit); return; }
            const FVector Normal = Hit.bStartPenetrating ? Hit.Normal : Hit.ImpactNormal;
            SetActorLocation(Hit.Location + Normal * (Hit.bStartPenetrating ? Hit.PenetrationDepth + .1f : .1f), false, nullptr, ETeleportType::TeleportPhysics);
            SmokeFallVelocity = FVector::VectorPlaneProject(SmokeFallVelocity, Normal);
        }
        else SetActorLocation(End, false, nullptr, ETeleportType::TeleportPhysics);
    }
}

void ALizardGameplayActor::CancelSmokeState()
{
    bSmokeStunned = bSmokeFalling = false;
    SmokeStunUntil = SmokeExposurePercent = 0.f;
    SmokeFallVelocity = FVector::ZeroVector;
    LastSprayHitTime = -100000.f;
    if (StunHalo.IsValid()) StunHalo->SetVisibility(false);
    if (SprayStatusWidget.IsValid()) SprayStatusWidget->SetVisibility(false);
}

void ALizardGameplayActor::UpdateSmokeStatus(float DeltaSeconds, bool& CanMove)
{
    CanMove = false;
    if (Caught || bStored) return;
    if (bSmokeStunned)
    {
        if (bSmokeFalling) { UpdateStunnedFall(DeltaSeconds); return; }
        if (Now() < SmokeStunUntil) return;
        bSmokeStunned = false;
        SmokeStunUntil = SmokeExposurePercent = 0.f;
        RestoreCatchFeedback();
        SenseThreat();
        if (bDebugSprayState) UE_LOG(LogTemp, Display, TEXT("[Spray] %s recovered at current ground position"), *GetName());
        CanMove = true;
        return;
    }
    UpdateExposureDecay(DeltaSeconds);
    CanMove = true;
}

void ALizardGameplayActor::UpdateSprayWidget()
{
    if (!SprayStatusWidget.IsValid()) return;
    const bool Visible = !Caught && !bStored && (bSmokeStunned || SmokeExposurePercent > 0.f);
    SprayStatusWidget->SetVisibility(Visible);
    if (!Visible) return;
    const FVector Position = GetActorLocation() + SprayWidgetWorldOffset;
    SprayStatusWidget->SetWorldLocation(Position);
    if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
        SprayStatusWidget->SetWorldRotation((Camera->GetCameraLocation() - Position).Rotation());
    if (auto* Status = Cast<ULizardStatusWidget>(SprayStatusWidget->GetUserWidgetObject()))
        Status->SetStatus(SmokeExposurePercent, bSmokeStunned, bSmokeFalling, bSmokeStunned && !bSmokeFalling ? FMath::Max(0.f, SmokeStunUntil - Now()) : 0.f);
}

void ALizardGameplayActor::ApplyFeedback(const FVector& Color, const FVector& Scale, bool ShowHalo)
{
    if (BodyMesh.IsValid())
    {
        BodyMesh->SetVectorParameterValueOnMaterials(FeedbackColorParameter, Color);
        BodyMesh->SetRelativeScale3D(Scale);
    }
    if (StunHalo.IsValid()) StunHalo->SetVisibility(ShowHalo);
}

void ALizardGameplayActor::StartFeedbackPulse()
{
    GetWorldTimerManager().SetTimer(FeedbackTimer, this, &ALizardGameplayActor::RestoreCatchFeedback, FMath::Max(.01f, FeedbackPulseSeconds), false);
}

void ALizardGameplayActor::ApplySmokeStunFeedback()
{
    if (!Caught && !bStored) ApplyFeedback(StunFeedbackColor, StunnedBodyScale, true);
}

void ALizardGameplayActor::RestoreCatchFeedback()
{
    if (bStored) return;
    if (bSmokeStunned && !Caught) { ApplySmokeStunFeedback(); return; }
    ApplyFeedback(NormalFeedbackColor, NormalBodyScale, false);
}
