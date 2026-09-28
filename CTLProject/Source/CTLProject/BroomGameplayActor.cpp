#include "BroomGameplayActor.h"
#include "LizardGameplayActor.h"
#include "ToolBlueprintBridge.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"

using namespace ToolBlueprintBridge;

ABroomGameplayActor::ABroomGameplayActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
}

void ABroomGameplayActor::ResolveComponents()
{
    GrabState = NamedComponent<USceneComponent>(this, TEXT("GrabComponent"));
    TipVolume = NamedComponent<UPrimitiveComponent>(this, TEXT("RakeHead"));
}

void ABroomGameplayActor::BeginPlay()
{
    Super::BeginPlay();
    ResolveComponents();
    RakeInterface = LoadObject<UClass>(nullptr, TEXT("/Game/CatchTheLizard/BPI_Rakeable.BPI_Rakeable_C"));
    bSampleValid = false;
    if (!GrabState.IsValid() || !TipVolume.IsValid())
        UE_LOG(LogTemp, Error, TEXT("%s: missing Blueprint GrabComponent or RakeHead"), *GetName());
    if (!ConnectGrabSignal(GrabState.Get(), TEXT("OnGrabbed"), this, TEXT("OnGrabStateChanged"))
        || !ConnectGrabSignal(GrabState.Get(), TEXT("OnDropped"), this, TEXT("OnGrabStateChanged")))
        UE_LOG(LogTemp, Error, TEXT("%s: incompatible BP_GrabComponent signals"), *GetName());
}

void ABroomGameplayActor::OnGrabStateChanged() { ResetSample(nullptr); }

void ABroomGameplayActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ConnectGrabSignal(GrabState.Get(), TEXT("OnGrabbed"), this, TEXT("OnGrabStateChanged"), false);
    ConnectGrabSignal(GrabState.Get(), TEXT("OnDropped"), this, TEXT("OnGrabStateChanged"), false);
    Super::EndPlay(EndPlayReason);
}

void ABroomGameplayActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateRake(DeltaSeconds);
}

void ABroomGameplayActor::GetHeldThreat(bool& Held, FVector& Position)
{
    if (!TipVolume.IsValid() || !GrabState.IsValid()) ResolveComponents();
    Held = HoldingController(GrabState.Get()) != nullptr;
    Position = TipVolume.IsValid() ? TipVolume->GetComponentLocation() : GetActorLocation();
}

void ABroomGameplayActor::ResetSample(USceneComponent* Holder)
{
    LastHolder = Holder;
    PreviousHeadPosition = TipVolume.IsValid() ? TipVolume->GetComponentLocation() : GetActorLocation();
    HeadVelocity = FVector::ZeroVector;
    ContactActors.Reset();
    NextContacts.Reset();
    AttemptedThisSample.Reset();
    bSampleValid = Holder != nullptr;
    PreviousHoldingPawn = Holder ? Holder->GetOwner() : nullptr;
    PreviousPawnTransform = PreviousHoldingPawn.IsValid() ? PreviousHoldingPawn->GetActorTransform() : FTransform::Identity;
}

void ABroomGameplayActor::ProcessRakeContact(AActor* Target, bool AtTip)
{
    if (!IsValid(Target) || GameplayLocked(this)) return;
    // Preserve the existing BPI_Rakeable contract, including non-lizard interface receivers.
    if (!RakeInterface.IsValid())
        RakeInterface = LoadObject<UClass>(nullptr, TEXT("/Game/CatchTheLizard/BPI_Rakeable.BPI_Rakeable_C"));
    if (!RakeInterface.IsValid() || !Target->GetClass()->ImplementsInterface(RakeInterface.Get())) return;
    UFunction* InterfaceFunction = Target->FindFunction(TEXT("Raked"));
    auto* Lizard = Cast<ALizardGameplayActor>(Target);
    if (!Lizard && !InterfaceFunction) return;
    if (ContactActors.Contains(Target))
    {
        if (AtTip) NextContacts.AddUnique(Target);
        return;
    }
    if (HeadVelocity.Size() < FMath::Max(0.f, RakeVelocityThreshold)) return;
    if (!AttemptedThisSample.Contains(Target))
    {
        AttemptedThisSample.Add(Target);
        if (Lizard)
        {
            bool Accepted = false;
            Lizard->ReceiveRake(PreviousHeadPosition, Accepted);
        }
        else
        {
            FStructOnScope Params(InterfaceFunction);
            const auto* Origin = FindFProperty<FStructProperty>(InterfaceFunction, TEXT("RakeOrigin"));
            if (Origin && Origin->Struct == TBaseStructure<FVector>::Get())
            {
                *Origin->ContainerPtrToValuePtr<FVector>(Params.GetStructMemory()) = PreviousHeadPosition;
                Target->ProcessEvent(InterfaceFunction, Params.GetStructMemory());
            }
        }
    }
    // Latch rejected cooldown attempts too, just as the original Blueprint did.
    if (AtTip) NextContacts.AddUnique(Target);
}

void ABroomGameplayActor::UpdateRake(float DeltaSeconds)
{
    if (!TipVolume.IsValid() || !GrabState.IsValid()) ResolveComponents();
    USceneComponent* Holder = HoldingController(GrabState.Get());
    if (!Holder || !TipVolume.IsValid() || GameplayLocked(this))
    {
        ResetSample(nullptr);
        return;
    }
    AActor* Pawn = Holder->GetOwner();
    // Pawn teleport/snap-turn is not controller swing motion. No Pawn graph changes are needed.
    const bool PawnMoved = PreviousHoldingPawn.Get() != Pawn || (Pawn && !Pawn->GetActorTransform().Equals(PreviousPawnTransform, .001f));
    if (!bSampleValid || LastHolder != Holder || PawnMoved)
    {
        ResetSample(Holder);
        return;
    }
    if (DeltaSeconds < .00001f) return;
    const FVector Current = TipVolume->GetComponentLocation();
    HeadVelocity = (Current - PreviousHeadPosition) / DeltaSeconds;
    NextContacts.Reset();
    AttemptedThisSample.Reset();
    TArray<UPrimitiveComponent*> Overlaps;
    TipVolume->GetOverlappingComponents(Overlaps);
    for (UPrimitiveComponent* Component : Overlaps)
        if (IsValid(Component) && Component->ComponentHasTag(TEXT("RakeTarget")))
            ProcessRakeContact(Component->GetOwner(), true);

    TArray<FHitResult> Hits;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(BroomTip), false, this);
    FCollisionObjectQueryParams Objects(ECC_WorldDynamic);
    const auto* Sphere = Cast<USphereComponent>(TipVolume.Get());
    const float Radius = Sphere ? Sphere->GetScaledSphereRadius() : 12.f;
    GetWorld()->SweepMultiByObjectType(Hits, PreviousHeadPosition, Current, FQuat::Identity, Objects,
        FCollisionShape::MakeSphere(FMath::Max(.01f, Radius)), Params);
    for (const FHitResult& Hit : Hits)
        if (Hit.GetComponent() && Hit.GetComponent()->ComponentHasTag(TEXT("RakeTarget")))
            ProcessRakeContact(Hit.GetActor(), TipVolume->IsOverlappingActor(Hit.GetActor()));
    ContactActors = NextContacts;
    PreviousHeadPosition = Current;
}
