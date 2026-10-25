#include "VRPanelInteractor.h"
#include "ToolBlueprintBridge.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MotionControllerComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"

UVRPanelInteractor::UVRPanelInteractor()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}
void UVRPanelInteractor::Initialize(APawn* Pawn, UUserWidget* Widget, UInputAction* Left, UInputAction* Right, UMaterialInterface* RayMaterial)
{
    Target = Widget;
    if (!Pawn || MenuInput) return;
    Player = Cast<APlayerController>(Pawn->GetController());
    if (!Player.IsValid()) return;
    Target = Widget;
    bHMD = UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();
    Pointers = {ToolBlueprintBridge::NamedComponent<UWidgetInteractionComponent>(Pawn, TEXT("WidgetInteractionLeft")),
        ToolBlueprintBridge::NamedComponent<UWidgetInteractionComponent>(Pawn, TEXT("WidgetInteractionRight"))};
    if (!Pointers[0] || !Pointers[1]) { Pointers.Reset(); return; }
    for (int32 Hand = 0; Hand < 2; ++Hand)
    {
        auto* WI = Pointers[Hand].Get();
        WI->VirtualUserIndex = 0;
        WI->PointerIndex = Hand;
        WI->TraceChannel = ECC_GameTraceChannel1;
        WI->InteractionDistance = 500.f;
        WI->bShowDebug = false;
        WI->Activate();
        WI->SetComponentTickEnabled(true);
        if (!bHMD && Hand == 1) WI->InteractionSource = EWidgetInteractionSource::Mouse;
        auto* Ray = NewObject<UStaticMeshComponent>(GetOwner());
        auto* Dot = NewObject<UStaticMeshComponent>(GetOwner());
        Ray->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
        Dot->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        for (auto* Mesh : {Ray, Dot})
        {
            GetOwner()->AddInstanceComponent(Mesh);
            Mesh->SetupAttachment(WI);
            Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Mesh->SetCastShadow(false);
            Mesh->SetVisibility(false);
            Mesh->RegisterComponent();
        }
        auto* MID = RayMaterial ? UMaterialInstanceDynamic::Create(RayMaterial, this) : nullptr;
        if (MID) { Ray->SetMaterial(0, MID); Dot->SetMaterial(0, MID); }
        Materials.Add(MID); Rays.Add(Ray); Dots.Add(Dot);
    }
    MenuInput = NewObject<UEnhancedInputComponent>(GetOwner(), TEXT("TutorialPointerInput"));
    MenuInput->RegisterComponent();
    MenuInput->Priority = 10;
    MenuInput->bBlockInput = false; // Grips continue to reach the existing pawn/lizard grab bindings.
    if (Left)
    {
        MenuInput->BindAction(Left, ETriggerEvent::Triggered, this, &UVRPanelInteractor::LeftValue);
        MenuInput->BindAction(Left, ETriggerEvent::Completed, this, &UVRPanelInteractor::LeftReleased);
        MenuInput->BindAction(Left, ETriggerEvent::Canceled, this, &UVRPanelInteractor::LeftCanceled);
    }
    if (Right)
    {
        MenuInput->BindAction(Right, ETriggerEvent::Triggered, this, &UVRPanelInteractor::RightValue);
        MenuInput->BindAction(Right, ETriggerEvent::Completed, this, &UVRPanelInteractor::RightReleased);
        MenuInput->BindAction(Right, ETriggerEvent::Canceled, this, &UVRPanelInteractor::RightCanceled);
    }
    UInputComponent* DesktopInput = MenuInput.Get();
    DesktopInput->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &UVRPanelInteractor::MousePressed);
    DesktopInput->BindKey(EKeys::LeftMouseButton, IE_Released, this, &UVRPanelInteractor::MouseReleased);
    Player->PushInputComponent(MenuInput);
    Player->bShowMouseCursor = !bHMD;
    if (!bHMD)
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Player->SetInputMode(Mode);
    }
    LockForSeconds(.4f);
}
void UVRPanelInteractor::LeftValue(const FInputActionValue& Value) { Values[0] = Value.Get<float>(); }
void UVRPanelInteractor::RightValue(const FInputActionValue& Value) { Values[1] = Value.Get<float>(); }
void UVRPanelInteractor::LeftReleased() { Values[0] = 0; }
void UVRPanelInteractor::RightReleased() { Values[1] = 0; }
void UVRPanelInteractor::LeftCanceled() { Values[0] = 0; Release(0, true); }
void UVRPanelInteractor::RightCanceled() { Values[1] = 0; Release(1, true); }
void UVRPanelInteractor::MousePressed() { if (!bHMD) Values[1] = 1; }
void UVRPanelInteractor::MouseReleased() { if (!bHMD) Values[1] = 0; }
void UVRPanelInteractor::Release(int32 Hand, bool bCancel)
{
    if (!Pressed[Hand] || Pointers.Num() != 2) return;
    Pressed[Hand] = false;
    Active = INDEX_NONE;
    if (bCancel && Target.IsValid()) Target->SetIsEnabled(false);
    Pointers[Hand]->ReleasePointerKey(EKeys::LeftMouseButton);
    if (bCancel && Target.IsValid()) Target->SetIsEnabled(true);
}
void UVRPanelInteractor::LockForSeconds(float Seconds)
{
    LockedUntil = GetWorld()->GetTimeSeconds() + Seconds;
    Release(0, true); Release(1, true);
    Armed[0] = Armed[1] = false;
}
void UVRPanelInteractor::Suspend()
{
    LockForSeconds(1000.f);
    for (auto* Mesh : {&Rays, &Dots}) for (auto& Part : *Mesh) if (Part) Part->SetVisibility(false);
    if (Player.IsValid() && MenuInput) Player->PopInputComponent(MenuInput);
    SetComponentTickEnabled(false);
}
void UVRPanelInteractor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (Pointers.Num() != 2) return;
    for (int32 Hand = 0; Hand < 2; ++Hand)
    {
        auto* WI = Pointers[Hand].Get();
        auto* Controller = Cast<UMotionControllerComponent>(WI->GetAttachParent());
        const bool Available = bHMD ? Controller && Controller->IsTracked() : Hand == 1;
        const bool Ready = Available && GetWorld()->GetTimeSeconds() >= LockedUntil;
        WI->bEnableHitTesting = Available;
        if (!Ready) { Release(Hand, true); Armed[Hand] = false; }
        else if (Values[Hand] <= .2f) { Release(Hand, false); Armed[Hand] = true; }
        else if (Values[Hand] >= .65f && Armed[Hand])
        {
            Armed[Hand] = false;
            if (Active == INDEX_NONE && WI->IsOverInteractableWidget())
            {
                Active = Hand; Pressed[Hand] = true; WI->PressPointerKey(EKeys::LeftMouseButton);
            }
        }
        const bool Visible = bHMD && Available;
        const auto& Hit = WI->GetLastHitResult();
        Rays[Hand]->SetVisibility(Visible);
        Dots[Hand]->SetVisibility(Visible && Hit.bBlockingHit);
        if (!Visible) continue;
        const float Length = Hit.bBlockingHit ? FMath::Min(500.f, Hit.Distance) : 500.f;
        Rays[Hand]->SetRelativeLocation(FVector(Length * .5f, 0, 0));
        Rays[Hand]->SetRelativeRotation(FRotator(90, 0, 0));
        Rays[Hand]->SetRelativeScale3D(FVector(.0025f, .0025f, Length / 100.f));
        Dots[Hand]->SetWorldLocation(Hit.ImpactPoint + Hit.ImpactNormal * .2f);
        Dots[Hand]->SetRelativeScale3D(FVector(.009f));
        if (Materials[Hand]) Materials[Hand]->SetVectorParameterValue(TEXT("RayColor"), Pressed[Hand] ? FLinearColor(1.f, .65f, .12f)
            : WI->IsOverInteractableWidget() ? FLinearColor(.25f, 1.f, .45f) : FLinearColor(.1f, .65f, 1.f));
    }
}
void UVRPanelInteractor::EndPlay(const EEndPlayReason::Type Reason)
{
    LockForSeconds(1000.f);
    if (Player.IsValid() && MenuInput) Player->PopInputComponent(MenuInput);
    Super::EndPlay(Reason);
}
