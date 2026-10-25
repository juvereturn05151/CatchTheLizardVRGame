#include "ToolPalmGripComponent.h"
#include "ToolBlueprintBridge.h"
#include "UObject/UnrealType.h"

UToolPalmGripComponent::UToolPalmGripComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UToolPalmGripComponent::BeginPlay()
{
    Super::BeginPlay();
    GrabState = ToolBlueprintBridge::NamedComponent<USceneComponent>(GetOwner(), TEXT("GrabComponent"));
    if (!ToolBlueprintBridge::ConnectGrabSignal(GrabState.Get(), TEXT("OnGrabbed"), this, TEXT("AlignToPalm")))
        UE_LOG(LogTemp, Warning, TEXT("%s: PalmGrip requires the existing BP_GrabComponent"), *GetOwner()->GetName());
}

void UToolPalmGripComponent::AlignToPalm()
{
    USceneComponent* Holder = ToolBlueprintBridge::HoldingController(GrabState.Get());
    USceneComponent* Root = GetOwner()->GetRootComponent();
    if (!Holder || !Root || Root->GetAttachParent() != Holder) return;
    const auto* Source = FindFProperty<FNameProperty>(Holder->GetClass(), TEXT("MotionSource"));
    if (!Source) return;
    const bool Left = Source->GetPropertyValue_InContainer(Holder).ToString().StartsWith(TEXT("Left"));
    USceneComponent* Grip = ToolBlueprintBridge::NamedComponent<USceneComponent>(GetOwner(), Left ? LeftGripPoint : RightGripPoint);
    if (!Grip) return;
    USceneComponent* Palm = ToolBlueprintBridge::NamedComponent<USceneComponent>(Holder->GetOwner(), Left ? LeftPalmTarget : RightPalmTarget);
    // Other pawns can still use these tools with their tracked grip pose.
    const FTransform Target = Palm && Palm->IsAttachedTo(Holder) ? Palm->GetComponentTransform() : Holder->GetComponentTransform();
    const FTransform LocalGrip = Grip->GetComponentTransform().GetRelativeTransform(Root->GetComponentTransform());
    const FQuat Rotation = Target.GetRotation() * LocalGrip.GetRotation().Inverse();
    const FVector Location = Target.GetLocation() - Rotation.RotateVector(Root->GetComponentScale() * LocalGrip.GetTranslation());
    // Move the existing physics root, retaining scale, collision children and grab ownership.
    Root->SetWorldLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
}

void UToolPalmGripComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    ToolBlueprintBridge::ConnectGrabSignal(GrabState.Get(), TEXT("OnGrabbed"), this, TEXT("AlignToPalm"), false);
    Super::EndPlay(Reason);
}
