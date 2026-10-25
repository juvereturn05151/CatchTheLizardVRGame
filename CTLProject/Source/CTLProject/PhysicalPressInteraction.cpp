#include "PhysicalPressInteraction.h"
#include "ToolBlueprintBridge.h"
#include "EnhancedInputComponent.h"
#include "MotionControllerComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "Engine/World.h"

UPhysicalPressInteraction::UPhysicalPressInteraction()
{
    PrimaryComponentTick.bCanEverTick = true;
    LeftAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/XRFramework/Input/Actions/Hands/IA_Hand_IndexCurl_Left.IA_Hand_IndexCurl_Left"));
    RightAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/XRFramework/Input/Actions/Hands/IA_Hand_IndexCurl_Right.IA_Hand_IndexCurl_Right"));
}
void UPhysicalPressInteraction::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Tick)
{
    Super::TickComponent(Delta, Type, Tick);
    auto* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC || PressInput) return;
    PressInput = NewObject<UEnhancedInputComponent>(GetOwner());
    PressInput->RegisterComponent(); PressInput->Priority = 5; PressInput->bBlockInput = false;
    if (LeftAction) {
        PressInput->BindAction(LeftAction, ETriggerEvent::Triggered, this, &UPhysicalPressInteraction::Left);
        PressInput->BindAction(LeftAction, ETriggerEvent::Completed, this, &UPhysicalPressInteraction::LeftUp);
        PressInput->BindAction(LeftAction, ETriggerEvent::Canceled, this, &UPhysicalPressInteraction::LeftUp);
    }
    if (RightAction) {
        PressInput->BindAction(RightAction, ETriggerEvent::Triggered, this, &UPhysicalPressInteraction::Right);
        PressInput->BindAction(RightAction, ETriggerEvent::Completed, this, &UPhysicalPressInteraction::RightUp);
        PressInput->BindAction(RightAction, ETriggerEvent::Canceled, this, &UPhysicalPressInteraction::RightUp);
    }
    PC->PushInputComponent(PressInput);
    // Require release after level load, then a fresh press.
}
void UPhysicalPressInteraction::Left(const FInputActionValue& V) { Value(0, V.Get<float>()); }
void UPhysicalPressInteraction::Right(const FInputActionValue& V) { Value(1, V.Get<float>()); }
void UPhysicalPressInteraction::LeftUp() { Armed[0] = true; }
void UPhysicalPressInteraction::RightUp() { Armed[1] = true; }
void UPhysicalPressInteraction::Value(int32 Hand, float Amount)
{
    if (Amount <= .2f) { Armed[Hand] = true; return; }
    if (Amount < .65f || !Armed[Hand]) return;
    Armed[Hand] = false;
    APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Pawn || !PressPoint || ToolBlueprintBridge::GameplayLocked(GetOwner())) return;
    auto* Controller = Cast<UMotionControllerComponent>(LizardBlueprintBridge::Object(Pawn,
        Hand == 0 ? TEXT("MotionControllerLeftGrip") : TEXT("MotionControllerRightGrip")));
    if (!Controller || !Controller->IsTracked() || FVector::Dist(Controller->GetComponentLocation(), PressPoint->GetComponentLocation()) > ReachRadius) return;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(PhysicalControl), false, GetOwner()); Params.AddIgnoredActor(Pawn);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Controller->GetComponentLocation(), PressPoint->GetComponentLocation(), ECC_Visibility, Params)) return;
    OnPressed.Broadcast();
}
void UPhysicalPressInteraction::EndPlay(const EEndPlayReason::Type Reason)
{
    if (auto* PC = UGameplayStatics::GetPlayerController(this, 0)) if (PressInput) PC->PopInputComponent(PressInput);
    Super::EndPlay(Reason);
}
