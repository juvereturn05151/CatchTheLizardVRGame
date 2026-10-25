#include "CTLSoundEffects.h"
#include "CTLBackgroundMusic.h"
#include "ToolBlueprintBridge.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

const FCTLSoundEffectSettings& UCTLSoundEffectLibrary::Find(ECTLSoundEffect Effect) const
{
    switch (Effect)
    {
        case ECTLSoundEffect::UIHover: return UIHover;
        case ECTLSoundEffect::UIConfirm: return UIConfirm;
        case ECTLSoundEffect::ObjectPickup: return ObjectPickup;
        case ECTLSoundEffect::BroomSwing: return BroomSwing;
        case ECTLSoundEffect::SprayLoop: return SprayLoop;
        case ECTLSoundEffect::StunActivated: return StunActivated;
        case ECTLSoundEffect::ContainerCapture: return ContainerCapture;
        case ECTLSoundEffect::MissionComplete: return MissionComplete;
        default: return TutorialComplete;
    }
}

UCTLSoundEffectsSubsystem* UCTLSoundEffectsSubsystem::Get(const UObject* Context)
{
    UGameInstance* GI = UGameplayStatics::GetGameInstance(Context);
    return GI ? GI->GetSubsystem<UCTLSoundEffectsSubsystem>() : nullptr;
}

UCTLSoundEffectLibrary* UCTLSoundEffectsSubsystem::GetLibrary() const
{
    const auto* GI = Cast<UCTLGameInstance>(GetGameInstance());
    return GI ? GI->SoundEffects : nullptr;
}

UAudioComponent* UCTLSoundEffectsSubsystem::PlayEffect(UObject* Source, ECTLSoundEffect Effect, FVector WorldLocation)
{
    const UCTLSoundEffectLibrary* Library = GetLibrary();
    if (!Library || !Library->Find(Effect).Sound || Effect == ECTLSoundEffect::SprayLoop) return nullptr;
    const FCTLSoundEffectSettings& Entry = Library->Find(Effect);
    const bool UI = Effect == ECTLSoundEffect::UIHover || Effect == ECTLSoundEffect::UIConfirm || Effect == ECTLSoundEffect::TutorialComplete || Effect == ECTLSoundEffect::MissionComplete;
    // Both controllers route through the same UI cooldown. World events are guarded per source.
    UObject* Key = UI || !IsValid(Source) ? GetGameInstance() : Source;
    if (LastPlayed.Num() > 256)
        for (auto It = LastPlayed.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
    auto& Times = LastPlayed.FindOrAdd(Key);
    const double Now = FPlatformTime::Seconds();
    if (const double* Previous = Times.Find(Effect))
        if (Now - *Previous < Entry.MinIntervalSeconds) return nullptr;
    Times.Add(Effect, Now);
    const float Volume = FMath::Clamp(Entry.Volume * Library->MasterVolume, 0.f, 1.f);
    if (UI)
    {
        auto* Player = UGameplayStatics::SpawnSound2D(GetGameInstance(), Entry.Sound, Volume, 1.f, 0.f, nullptr, true, true);
        if (Player && Effect == ECTLSoundEffect::MissionComplete)
            if (auto* GI = Cast<UCTLGameInstance>(GetGameInstance())) GI->DuckMusicForCue(Entry.Sound->GetDuration() + .1f, Library->CompletionMusicDuckMultiplier);
        return Player;
    }
    return UGameplayStatics::SpawnSoundAtLocation(GetGameInstance(), Entry.Sound, WorldLocation, FRotator::ZeroRotator,
        Volume, 1.f, 0.f, Library->WorldAttenuation);
}

void UCTLPickupAudioComponent::BeginPlay()
{
    Super::BeginPlay();
    GrabState = ToolBlueprintBridge::NamedComponent<USceneComponent>(GetOwner(), TEXT("GrabComponent"));
    bWasHeld = ToolBlueprintBridge::HoldingController(GrabState.Get()) != nullptr;
    ToolBlueprintBridge::ConnectGrabSignal(GrabState.Get(), TEXT("OnGrabbed"), this, TEXT("GrabChanged"));
    ToolBlueprintBridge::ConnectGrabSignal(GrabState.Get(), TEXT("OnDropped"), this, TEXT("GrabChanged"));
}

void UCTLPickupAudioComponent::GrabChanged()
{
    const bool Held = ToolBlueprintBridge::HoldingController(GrabState.Get()) != nullptr;
    if (Held && !bWasHeld)
        if (auto* Audio = UCTLSoundEffectsSubsystem::Get(this)) Audio->PlayEffect(GetOwner(), ECTLSoundEffect::ObjectPickup, GetOwner()->GetActorLocation());
    bWasHeld = Held;
}

void UCTLPickupAudioComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    ToolBlueprintBridge::ConnectGrabSignal(GrabState.Get(), TEXT("OnGrabbed"), this, TEXT("GrabChanged"), false);
    ToolBlueprintBridge::ConnectGrabSignal(GrabState.Get(), TEXT("OnDropped"), this, TEXT("GrabChanged"), false);
    Super::EndPlay(Reason);
}
