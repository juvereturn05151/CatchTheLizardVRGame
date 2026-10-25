#include "CTLBackgroundMusic.h"
#include "CTLSoundEffects.h"
#include "AudioDevice.h"
#include "AudioMixerBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Misc/App.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Sound/SoundBase.h"
#include "UObject/UObjectGlobals.h"
#include "TimerManager.h"
#include "XRLoadingScreenFunctionLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "HeadMountedDisplayFunctionLibrary.h"

const FCTLMapMusic* UCTLMusicLibrary::Find(const FString& MapName) const
{
    const FString Name = FPackageName::GetShortName(UWorld::RemovePIEPrefix(MapName));
    return Maps.FindByPredicate([&Name](const FCTLMapMusic& Entry)
    {
        return !Entry.Map.IsNull() && FPackageName::GetShortName(Entry.Map.ToSoftObjectPath().GetLongPackageName()) == Name;
    });
}

void UCTLGameInstance::Init()
{
    Super::Init();
    BeforeLoadHandle = FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &UCTLGameInstance::BeforeMapLoad);
    AfterLoadHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UCTLGameInstance::AfterMapLoad);
    TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UCTLGameInstance::HandleTravelFailure);
}

void UCTLGameInstance::OnStart()
{
    Super::OnStart();
    UXRLoadingScreenFunctionLibrary::HideLoadingScreen();
    GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UCTLGameInstance::ConfigureQuestLinkAudio);
    ApplyMapMusic(GetWorld());
    CTLAudioStatus();
}

void UCTLGameInstance::ReportAudioOutput(const FString& DeviceName)
{
    UE_LOG(LogTemp, Display, TEXT("CTL audio output: %s"), *DeviceName);
}

void UCTLGameInstance::CTLAudioStatus()
{
    UWorld* World = GetWorld();
    if (!World) return;
    FOnMainAudioOutputDeviceObtained Callback;
    Callback.BindDynamic(this, &UCTLGameInstance::ReportAudioOutput);
    UAudioMixerBlueprintLibrary::GetCurrentAudioOutputDeviceName(this, Callback);
    const FAudioDeviceHandle Device = World->GetAudioDevice();
    UE_LOG(LogTemp, Display, TEXT("CTL audio: app volume %.3f, unfocused multiplier %.3f, VR focus used=%d has=%d, primary volume %.3f, paused=%d"),
        FApp::GetVolumeMultiplier(), FApp::GetUnfocusedVolumeMultiplier(), FApp::UseVRFocus(), FApp::HasVRFocus(),
        Device.IsValid() ? Device->GetPrimaryVolume() : -1.f, UGameplayStatics::IsGamePaused(World));
    UE_LOG(LogTemp, Display, TEXT("CTL audio music: asset=%s playing=%d component volume=%.3f library master=%.3f group=%s; SFX master=%.3f"),
        *GetPathNameSafe(MusicPlayer ? MusicPlayer->Sound.Get() : nullptr), MusicPlayer && MusicPlayer->IsPlaying(),
        MusicPlayer ? MusicPlayer->VolumeMultiplier : 0.f, MusicLibrary ? MusicLibrary->MasterVolume : 0.f,
        *ActiveGroup.ToString(), SoundEffects ? SoundEffects->MasterVolume : 0.f);
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        FVector Location, Front, Right;
        PC->GetAudioListenerPosition(Location, Front, Right);
        UE_LOG(LogTemp, Display, TEXT("CTL audio listener: %s forward=%s"), *Location.ToCompactString(), *Front.ToCompactString());
    }
}

void UCTLGameInstance::CTLAudioTest()
{
    CTLAudioStatus();
    if (!SoundEffects || !SoundEffects->TutorialComplete.Sound)
    {
        UE_LOG(LogTemp, Warning, TEXT("CTL audio test: completion sound assignment is missing"));
        return;
    }
    if (AudioTestPlayer) { AudioTestPlayer->Stop(); AudioTestPlayer->DestroyComponent(); }
    AudioTestPlayer = UGameplayStatics::SpawnSound2D(this, SoundEffects->TutorialComplete.Sound, .5f, 1.f, 0.f, nullptr, false, false);
    UE_LOG(LogTemp, Display, TEXT("CTL audio test: requested=%s non-spatial volume=0.5 started=%d"),
        *GetPathNameSafe(SoundEffects->TutorialComplete.Sound), AudioTestPlayer && AudioTestPlayer->IsPlaying());
}

bool UCTLGameInstance::CanContinue(const FCTLMapMusic* Entry) const
{
    return Entry && Entry->Track && MusicPlayer && MusicPlayer->IsPlaying() && !bFadingOut
        && Entry->ContinuityGroup == ActiveGroup && Entry->Track == MusicPlayer->Sound;
}

float UCTLGameInstance::FadeMusicForExit()
{
    if (!MusicPlayer || !MusicPlayer->IsPlaying() || bFadingOut) return 0.f;
    bFadingOut = true;
    const float Duration = MusicLibrary ? FMath::Max(0.f, MusicLibrary->FadeOutSeconds) : 0.f;
    if (Duration > 0.f) MusicPlayer->FadeOut(Duration, 0.f);
    else MusicPlayer->Stop();
    return Duration;
}

float UCTLGameInstance::PrepareMap(const FString& MapName)
{
    RestoreMusicAfterCue(); // Clear a completion duck before any same-group or cross-group travel.
    const FCTLMapMusic* Entry = MusicLibrary ? MusicLibrary->Find(MapName) : nullptr;
    return CanContinue(Entry) ? 0.f : FadeMusicForExit();
}

void UCTLGameInstance::DuckMusicForCue(float Duration, float Multiplier)
{
    UWorld* World = GetWorld();
    const auto* Entry = MusicLibrary && World ? MusicLibrary->Find(World->GetOutermost()->GetName()) : nullptr;
    if (!World || !Entry || !MusicPlayer || !MusicPlayer->IsPlaying() || bFadingOut) return;
    bMusicDucked = true;
    MusicPlayer->AdjustVolume(.08f, FMath::Clamp(Entry->Volume * MusicLibrary->MasterVolume * Multiplier, 0.f, 1.f));
    World->GetTimerManager().SetTimer(MusicDuckTimer, this, &UCTLGameInstance::RestoreMusicAfterCue, FMath::Clamp(Duration, .1f, 30.f), false);
}

void UCTLGameInstance::RestoreMusicAfterCue()
{
    if (!bMusicDucked) return;
    bMusicDucked = false;
    UWorld* World = GetWorld();
    if (World) World->GetTimerManager().ClearTimer(MusicDuckTimer);
    const auto* Entry = MusicLibrary && World ? MusicLibrary->Find(World->GetOutermost()->GetName()) : nullptr;
    if (Entry && MusicPlayer && MusicPlayer->IsPlaying() && !bFadingOut)
        MusicPlayer->AdjustVolume(.3f, FMath::Clamp(Entry->Volume * MusicLibrary->MasterVolume, 0.f, 1.f));
}

float UCTLGameInstance::PrepareMusicTravel(TSoftObjectPtr<UWorld> Destination)
{
    return PrepareMap(Destination.ToSoftObjectPath().GetLongPackageName());
}

void UCTLGameInstance::BeforeMapLoad(const FWorldContext& Context, const FString& MapName)
{
    // Delegates are global; never touch a second PIE instance's audio.
    if (Context.OwningGameInstance == this) PrepareMap(MapName);
}

void UCTLGameInstance::AfterMapLoad(UWorld* World)
{
    if (World && World->GetGameInstance() == this)
    {
        bVRRestartRequested = false;
        UXRLoadingScreenFunctionLibrary::HideLoadingScreen();
        World->GetTimerManager().SetTimerForNextTick(this, &UCTLGameInstance::ConfigureQuestLinkAudio);
    }
    ApplyMapMusic(World);
}

bool UCTLGameInstance::ShouldRouteQuestLinkAudio() const
{
#if PLATFORM_WINDOWS
    const FString HMD = UHeadMountedDisplayFunctionLibrary::GetHMDDeviceName().ToString();
    return bPreferQuestLinkAudio && !QuestLinkOutputName.IsEmpty() && GetWorld()
        && UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled()
        && (HMD.Contains(TEXT("Quest")) || HMD.Contains(TEXT("Oculus")));
#else
    return false;
#endif
}

void UCTLGameInstance::ConfigureQuestLinkAudio()
{
    if (!ShouldRouteQuestLinkAudio() || bQuestAudioRoutingPending) return;
    bQuestAudioRoutingPending = true;
    FOnMainAudioOutputDeviceObtained Callback;
    Callback.BindDynamic(this, &UCTLGameInstance::CheckQuestLinkOutput);
    UAudioMixerBlueprintLibrary::GetCurrentAudioOutputDeviceName(this, Callback);
}

void UCTLGameInstance::CheckQuestLinkOutput(const FString& CurrentDeviceName)
{
    if (!ShouldRouteQuestLinkAudio() || CurrentDeviceName.Contains(QuestLinkOutputName))
    {
        bQuestAudioRoutingPending = false;
        return;
    }
    UE_LOG(LogTemp, Display, TEXT("CTL Quest Link audio: current=%s; looking for %s"), *CurrentDeviceName, *QuestLinkOutputName);
    FOnAudioOutputDevicesObtained Callback;
    Callback.BindDynamic(this, &UCTLGameInstance::ChooseQuestLinkOutput);
    UAudioMixerBlueprintLibrary::GetAvailableAudioOutputDevices(this, Callback);
}

void UCTLGameInstance::ChooseQuestLinkOutput(const TArray<FAudioOutputDeviceInfo>& AvailableDevices)
{
    if (ShouldRouteQuestLinkAudio())
        for (const auto& Device : AvailableDevices)
            if (Device.Name.Contains(QuestLinkOutputName))
            {
                FOnCompletedDeviceSwap Callback;
                Callback.BindDynamic(this, &UCTLGameInstance::QuestLinkOutputSwapped);
                UAudioMixerBlueprintLibrary::SwapAudioOutputDevice(this, Device.DeviceId, Callback);
                return;
            }
    bQuestAudioRoutingPending = false;
    UE_LOG(LogTemp, Warning, TEXT("CTL Quest Link audio: headset output unavailable; retaining current audio device."));
}

void UCTLGameInstance::QuestLinkOutputSwapped(const FSwapAudioOutputResult& Result)
{
    bQuestAudioRoutingPending = false;
    UE_LOG(LogTemp, Display, TEXT("CTL Quest Link audio swap: requested=%s result=%d"), *Result.RequestedDeviceId, int32(Result.Result));
    CTLAudioStatus();
}

bool UCTLGameInstance::RestartCurrentVRStage(const UObject* WorldContextObject)
{
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    auto* Instance = World ? World->GetGameInstance<UCTLGameInstance>() : nullptr;
    if (!Instance || Instance->bVRRestartRequested) return false;
    const FString Package = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
    if (!FPackageName::DoesPackageExist(Package))
    {
        UXRLoadingScreenFunctionLibrary::HideLoadingScreen();
        UE_LOG(LogTemp, Error, TEXT("CTL VR restart rejected: current map does not exist: %s"), *Package);
        return false;
    }
    Instance->bVRRestartRequested = true;
    UXRLoadingScreenFunctionLibrary::HideLoadingScreen();
    UXRLoadingScreenFunctionLibrary::ClearLoadingScreenSplashes();
    if (auto* PC = World->GetFirstPlayerController(); PC && PC->PlayerCameraManager) PC->PlayerCameraManager->StopCameraFade();
    UE_LOG(LogTemp, Display, TEXT("CTL VR restart: %s"), *Package);
    UGameplayStatics::OpenLevel(WorldContextObject, FName(*Package), true);
    return true;
}

void UCTLGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Reason)
{
    if (!World || World->GetGameInstance() != this) return;
    bVRRestartRequested = false;
    UXRLoadingScreenFunctionLibrary::HideLoadingScreen();
    UXRLoadingScreenFunctionLibrary::ClearLoadingScreenSplashes();
    if (auto* PC = World->GetFirstPlayerController(); PC && PC->PlayerCameraManager) PC->PlayerCameraManager->StopCameraFade();
    UE_LOG(LogTemp, Warning, TEXT("CTL VR recovered display after travel failure %d: %s"), int32(Type), *Reason);
}

void UCTLGameInstance::ApplyMapMusic(UWorld* World)
{
    if (!World || !World->IsGameWorld() || World->GetGameInstance() != this || World->GetNetMode() == NM_DedicatedServer) return;
    const FCTLMapMusic* Entry = MusicLibrary ? MusicLibrary->Find(World->GetOutermost()->GetName()) : nullptr;
    const float Volume = Entry ? FMath::Clamp(Entry->Volume * MusicLibrary->MasterVolume, 0.f, 1.f) : 0.f;
    if (CanContinue(Entry))
    {
        MusicPlayer->AdjustVolume(.15f, Volume);
        return;
    }
    if (MusicPlayer)
    {
        MusicPlayer->Stop();
        MusicPlayer->DestroyComponent();
        MusicPlayer = nullptr;
    }
    bFadingOut = false;
    ActiveGroup = NAME_None;
    if (!Entry || !Entry->Track) return;
    // Persist across world teardown; no attenuation, pawn ownership, or spatialization.
    MusicPlayer = UGameplayStatics::CreateSound2D(World, Entry->Track, 1.f, 1.f, 0.f, nullptr, true, false);
    if (MusicPlayer)
    {
        MusicPlayer->bAllowSpatialization = false;
        MusicPlayer->bIsUISound = true;
        ActiveGroup = Entry->ContinuityGroup;
        MusicPlayer->FadeIn(FMath::Max(0.f, MusicLibrary->FadeInSeconds), Volume);
        UE_LOG(LogTemp, Display, TEXT("CTL music: %s -> %s, group %s, volume %.2f"),
            *World->GetName(), *Entry->Track->GetName(), *ActiveGroup.ToString(), Volume);
    }
}

void UCTLGameInstance::Shutdown()
{
    StopVRDebug();
    if (GEngine) GEngine->OnTravelFailure().Remove(TravelFailureHandle);
    RestoreMusicAfterCue();
    if (AudioTestPlayer) { AudioTestPlayer->Stop(); AudioTestPlayer->DestroyComponent(); AudioTestPlayer = nullptr; }
    FCoreUObjectDelegates::PreLoadMapWithContext.Remove(BeforeLoadHandle);
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(AfterLoadHandle);
    if (MusicPlayer)
    {
        MusicPlayer->Stop();
        MusicPlayer->DestroyComponent();
        MusicPlayer = nullptr;
    }
    Super::Shutdown();
}
