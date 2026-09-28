#include "SprayGameplayActor.h"
#include "SmokePuffGameplayActor.h"
#include "LizardGameplayActor.h"
#include "ToolBlueprintBridge.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"

using namespace ToolBlueprintBridge;

ASprayGameplayActor::ASprayGameplayActor()
{
    PrimaryActorTick.bCanEverTick = true;
    // BP_Spray explicitly assigns puff/input assets, retaining serialized cook dependencies.
}

void ASprayGameplayActor::ResolveComponents()
{
    GrabState = NamedComponent<USceneComponent>(this, TEXT("GrabComponent"));
    NozzleComponent = NamedComponent<USceneComponent>(this, TEXT("Nozzle"));
}

void ASprayGameplayActor::BeginPlay()
{
    Super::BeginPlay();
    ResolveComponents();
    if (!SmokePuffClass) UE_LOG(LogTemp, Error, TEXT("%s: missing Blueprint smoke puff class"), *GetName());
    if (!GrabState.IsValid() || !NozzleComponent.IsValid())
        UE_LOG(LogTemp, Error, TEXT("%s: missing Blueprint GrabComponent or Nozzle"), *GetName());
    if (!ConnectGrabSignal(GrabState.Get(), TEXT("OnGrabbed"), this, TEXT("OnGrabStateChanged"))
        || !ConnectGrabSignal(GrabState.Get(), TEXT("OnDropped"), this, TEXT("OnGrabStateChanged")))
        UE_LOG(LogTemp, Error, TEXT("%s: incompatible BP_GrabComponent signals"), *GetName());
    BindTriggerInput();
}

void ASprayGameplayActor::OnGrabStateChanged()
{
    bEmitting = false;
    EmissionClock = 0.f;
}

void ASprayGameplayActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ConnectGrabSignal(GrabState.Get(), TEXT("OnGrabbed"), this, TEXT("OnGrabStateChanged"), false);
    ConnectGrabSignal(GrabState.Get(), TEXT("OnDropped"), this, TEXT("OnGrabStateChanged"), false);
    OnGrabStateChanged();
    Super::EndPlay(EndPlayReason);
}

void ASprayGameplayActor::BindTriggerInput()
{
    CreateInputComponent(UEnhancedInputComponent::StaticClass());
    auto* Enhanced = Cast<UEnhancedInputComponent>(InputComponent);
    if (!Enhanced) return;
    if (const UInputAction* Left = LeftTriggerAction.LoadSynchronous())
    {
        Enhanced->BindAction(Left, ETriggerEvent::Triggered, this, &ASprayGameplayActor::SetLeftTrigger);
        Enhanced->BindAction(Left, ETriggerEvent::Completed, this, &ASprayGameplayActor::ClearLeftTrigger);
        Enhanced->BindAction(Left, ETriggerEvent::Canceled, this, &ASprayGameplayActor::ClearLeftTrigger);
    }
    else UE_LOG(LogTemp, Error, TEXT("%s: missing left spray trigger action"), *GetName());
    if (const UInputAction* Right = RightTriggerAction.LoadSynchronous())
    {
        Enhanced->BindAction(Right, ETriggerEvent::Triggered, this, &ASprayGameplayActor::SetRightTrigger);
        Enhanced->BindAction(Right, ETriggerEvent::Completed, this, &ASprayGameplayActor::ClearRightTrigger);
        Enhanced->BindAction(Right, ETriggerEvent::Canceled, this, &ASprayGameplayActor::ClearRightTrigger);
    }
    else UE_LOG(LogTemp, Error, TEXT("%s: missing right spray trigger action"), *GetName());
    EnableInput(UGameplayStatics::GetPlayerController(this, 0));
}

void ASprayGameplayActor::SetLeftTrigger(const FInputActionValue& Value) { LeftTriggerValue = Value.Get<float>(); }
void ASprayGameplayActor::SetRightTrigger(const FInputActionValue& Value) { RightTriggerValue = Value.Get<float>(); }
void ASprayGameplayActor::ClearLeftTrigger() { LeftTriggerValue = 0.f; }
void ASprayGameplayActor::ClearRightTrigger() { RightTriggerValue = 0.f; }

void ASprayGameplayActor::ReadSprayTrigger(bool& Pressed)
{
    Pressed = false;
    if (!GrabState.IsValid()) ResolveComponents();
    USceneComponent* Holder = HoldingController(GrabState.Get());
    if (!Holder || GameplayLocked(this)) return;
    // BP_GrabComponent.GetHeldByHand classifies the first four MotionSource characters as "Left".
    const auto* Source = FindFProperty<FNameProperty>(Holder->GetClass(), TEXT("MotionSource"));
    if (!Source) return;
    const bool Left = Source->GetPropertyValue_InContainer(Holder).ToString().Left(4) == TEXT("Left");
    Pressed = (Left ? LeftTriggerValue : RightTriggerValue) >= TriggerThreshold;
}

void ASprayGameplayActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    bool Pressed = false;
    ReadSprayTrigger(Pressed);
    UpdateEmission(DeltaSeconds, Pressed);
}

void ASprayGameplayActor::UpdateEmission(float DeltaSeconds, bool TriggerPressed)
{
    if (!GrabState.IsValid()) ResolveComponents();
    if (!HoldingController(GrabState.Get()) || !TriggerPressed || GameplayLocked(this))
    {
        bEmitting = false;
        EmissionClock = 0.f;
        return;
    }
    if (!bEmitting)
    {
        bEmitting = true;
        EmissionClock = 0.f;
        EmitSmokePuff();
        return;
    }
    EmissionClock += DeltaSeconds;
    if (EmissionClock >= FMath::Max(EmissionIntervalSeconds, .08f))
    {
        EmissionClock = 0.f;
        EmitSmokePuff();
    }
}

void ASprayGameplayActor::EmitSmokePuff()
{
    if (!GrabState.IsValid() || !NozzleComponent.IsValid()) ResolveComponents();
    USceneComponent* Holder = HoldingController(GrabState.Get());
    if (!Holder || !NozzleComponent.IsValid() || GameplayLocked(this)) return;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SprayNozzle), false, this);
    // A lizard is a spray target, not a wall between the hand and nozzle.
    for (TActorIterator<ALizardGameplayActor> It(GetWorld()); It; ++It) Params.AddIgnoredActor(*It);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Holder->GetComponentLocation(), NozzleComponent->GetComponentLocation(), ECC_Visibility, Params)) return;
    UClass* PuffClass = SmokePuffClass.Get();
    if (!PuffClass || !PuffClass->IsChildOf(ASmokePuffGameplayActor::StaticClass())) return;
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    NewPuff = GetWorld()->SpawnActor<ASmokePuffGameplayActor>(PuffClass, NozzleComponent->GetComponentTransform(), Spawn);
    if (NewPuff) NewPuff->InitializeSmoke(SprayRangeCm, SmokeRadiusCm, SmokeLifetimeSeconds, this);
}
