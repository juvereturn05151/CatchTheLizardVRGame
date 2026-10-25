#include "CTLBackgroundMusic.h"

#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
#include "LizardBlueprintBridge.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "IXRTrackingSystem.h"
#include "IXRLoadingScreen.h"
#include "EnhancedPlayerInput.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"
#include "Input/Events.h"

DEFINE_LOG_CATEGORY_STATIC(LogCTLVR, Log, All);

// Observes input without handling or consuming it. Runtime-reserved system keys
// may never be delivered to Unreal and therefore cannot appear in these logs.
class FCTLVRInputObserver : public IInputProcessor
{
    TMap<FKey, int32> AxisZones;
public:
    virtual void Tick(float, FSlateApplication&, TSharedRef<ICursor>) override {}
    virtual bool HandleKeyDownEvent(FSlateApplication&, const FKeyEvent& Event) override
    {
        if (Event.GetKey().IsGamepadKey() && !Event.IsRepeat()) UE_LOG(LogCTLVR, Display, TEXT("Input down: %s"), *Event.GetKey().ToString());
        return false;
    }
    virtual bool HandleKeyUpEvent(FSlateApplication&, const FKeyEvent& Event) override
    {
        if (Event.GetKey().IsGamepadKey()) UE_LOG(LogCTLVR, Display, TEXT("Input up: %s"), *Event.GetKey().ToString());
        return false;
    }
    virtual bool HandleAnalogInputEvent(FSlateApplication&, const FAnalogInputEvent& Event) override
    {
        if (!Event.GetKey().IsGamepadKey()) return false;
        const float Value = Event.GetAnalogValue();
        const int32 Zone = Value > .5f ? 1 : (Value < -.5f ? -1 : 0);
        int32& Previous = AxisZones.FindOrAdd(Event.GetKey());
        if (Zone != Previous)
        {
            UE_LOG(LogCTLVR, Display, TEXT("Input axis: %s = %.2f"), *Event.GetKey().ToString(), Value);
            Previous = Zone;
        }
        return false;
    }
};
#endif

void UCTLGameInstance::CTLVRDebug(int32 Enabled)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
    StopVRDebug();
    if (!Enabled) return;
    LastVRSnapshot.Reset(); LastVRContexts.Reset(); LastVRPawn.Reset(); NextVRHeartbeat = 0;
    if (FSlateApplication::IsInitialized())
    {
        VRDebugInput = MakeShared<FCTLVRInputObserver>();
        FSlateApplication::Get().RegisterInputPreProcessor(VRDebugInput);
    }
    VRDebugTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UCTLGameInstance::TickVRDebug), .1f);
    UE_LOG(LogCTLVR, Display, TEXT("Diagnostics enabled; CTLVRDebug 0 disables. System-reserved keys may not reach Unreal."));
    CTLVRStatus();
#endif
}

void UCTLGameInstance::StopVRDebug()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
    FTSTicker::GetCoreTicker().RemoveTicker(VRDebugTicker); VRDebugTicker.Reset();
    if (VRDebugInput && FSlateApplication::IsInitialized()) FSlateApplication::Get().UnregisterInputPreProcessor(VRDebugInput);
    VRDebugInput.Reset();
#endif
}

void UCTLGameInstance::CTLVRStatus()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
    LastVRSnapshot.Reset(); NextVRHeartbeat = 0;
    TickVRDebug(0);
#endif
}

bool UCTLGameInstance::TickVRDebug(float)
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
    UWorld* World = GetWorld();
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    if (!PC) return true;
    APawn* Pawn = PC->GetPawn();
    AActor* Target = PC->GetViewTarget();
    TArray<UCameraComponent*> Cameras;
    if (Target) Target->GetComponents(Cameras);
    FString CameraState;
    for (const auto* Camera : Cameras)
        CameraState += FString::Printf(TEXT("%s(active=%d hmd=%d parent=%s) "), *Camera->GetName(), Camera->IsActive(), Camera->bLockToHmd, *GetNameSafe(Camera->GetAttachParent()));
    bool UseFocus = false, HasFocus = false;
    UHeadMountedDisplayFunctionLibrary::GetVRFocusState(UseFocus, HasFocus);
    auto* Splash = GEngine && GEngine->XRSystem ? GEngine->XRSystem->GetLoadingScreen() : nullptr;
    const FString Snapshot = FString::Printf(TEXT("map=%s pawn=%s target=%s camera=%s HMD=%d connected=%d tracked=%d worn=%d focus=%d/%d splash=%d teleport=%d valid=%d marker=%s menu=%s paused=%d"),
        *World->GetName(), *GetNameSafe(Pawn), *GetNameSafe(Target), *CameraState,
        UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled(), UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayConnected(),
        UHeadMountedDisplayFunctionLibrary::HasValidTrackingPosition(), int32(UHeadMountedDisplayFunctionLibrary::GetHMDWornState()), UseFocus, HasFocus,
        Splash && Splash->IsShown(), LizardBlueprintBridge::Bool(Pawn, TEXT("bTeleportTraceActive")), LizardBlueprintBridge::Bool(Pawn, TEXT("bValidTeleportLocation")),
        *GetNameSafe(LizardBlueprintBridge::Object(Pawn, TEXT("TeleportVisualizerReference"))), *GetNameSafe(LizardBlueprintBridge::Object(Pawn, TEXT("MenuReference"))), World->IsPaused());
    const double Now = FPlatformTime::Seconds();
    if (Snapshot != LastVRSnapshot || Now >= NextVRHeartbeat)
    {
        UE_LOG(LogCTLVR, Display, TEXT("%s; pawnPosition=%s cameraFade=%.2f"), *Snapshot, Pawn ? *Pawn->GetActorLocation().ToCompactString() : TEXT("none"), PC->PlayerCameraManager ? PC->PlayerCameraManager->FadeAmount : 0.f);
        LastVRSnapshot = Snapshot; NextVRHeartbeat = Now + 5.;
    }
    if (Pawn && Pawn == LastVRPawn && FVector::DistSquared(LastVRPawnLocation, Pawn->GetActorLocation()) > FMath::Square(50.f))
        UE_LOG(LogCTLVR, Display, TEXT("Pawn relocation: %s -> %s"), *LastVRPawnLocation.ToCompactString(), *Pawn->GetActorLocation().ToCompactString());
    LastVRPawn = Pawn; if (Pawn) LastVRPawnLocation = Pawn->GetActorLocation();
    FString Contexts;
    if (auto* Input = Cast<UEnhancedPlayerInput>(PC->PlayerInput))
        for (const auto& Mapping : Input->GetEnhancedActionMappingsView()) Contexts += Mapping.Key.ToString() + TEXT("=") + GetNameSafe(Mapping.Action) + TEXT(";");
    if (Contexts != LastVRContexts)
    {
        UE_LOG(LogCTLVR, Display, TEXT("Active input mappings: %s"), *Contexts);
        LastVRContexts = Contexts;
    }
#endif
    return true;
}
