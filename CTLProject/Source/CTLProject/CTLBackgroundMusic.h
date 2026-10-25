#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/GameInstance.h"
#include "Containers/Ticker.h"
#include "Engine/EngineBaseTypes.h"
#include "AudioMixerBlueprintLibrary.h"
#include "CTLBackgroundMusic.generated.h"

class UAudioComponent;
class USoundBase;
class UCTLSoundEffectLibrary;
struct FWorldContext;
class IInputProcessor;
struct FAudioOutputDeviceInfo;
struct FSwapAudioOutputResult;

USTRUCT(BlueprintType)
struct FCTLMapMusic
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UWorld> Map;
    // Maps in the same group with the same track share uninterrupted playback.
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ContinuityGroup;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<USoundBase> Track;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1")) float Volume = .16f;
};

UCLASS(BlueprintType)
class CTLPROJECT_API UCTLMusicLibrary : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Music") TArray<FCTLMapMusic> Maps;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Music", meta=(ClampMin="0", ClampMax="1")) float MasterVolume = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Music", meta=(ClampMin="0", Units="s")) float FadeInSeconds = .8f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Music", meta=(ClampMin="0", Units="s")) float FadeOutSeconds = .45f;
    const FCTLMapMusic* Find(const FString& MapName) const;
};

/** One non-spatial music voice per game instance, independent of pawn/map lifetime. */
UCLASS(Blueprintable)
class CTLPROJECT_API UCTLGameInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    virtual void Init() override;
    virtual void OnStart() override;
    virtual void Shutdown() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Music") TObjectPtr<UCTLMusicLibrary> MusicLibrary;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sound Effects") TObjectPtr<UCTLSoundEffectLibrary> SoundEffects;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Music") TObjectPtr<UAudioComponent> MusicPlayer;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Music") FName ActiveGroup;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Quest Link") bool bPreferQuestLinkAudio = true;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audio|Quest Link") FString QuestLinkOutputName = TEXT("Oculus Virtual Audio Device");

    // Call before OpenLevel and wait the returned duration. Same-group travel returns zero.
    UFUNCTION(BlueprintCallable, Category="Music") float PrepareMusicTravel(TSoftObjectPtr<UWorld> Destination);
    UFUNCTION(BlueprintCallable, Category="Music") float FadeMusicForExit();
    void DuckMusicForCue(float Duration, float Multiplier);
    // Optional PC VR diagnostics: enter these commands in the in-game/editor console.
    UFUNCTION(Exec, BlueprintCallable, Category="Audio|Diagnostics") void CTLAudioStatus();
    UFUNCTION(Exec, BlueprintCallable, Category="Audio|Diagnostics") void CTLAudioTest();
    // The template menu restarts the actual current map without a persistent XR splash.
    UFUNCTION(BlueprintCallable, Category="VR|Travel", meta=(WorldContext="WorldContextObject"))
    static bool RestartCurrentVRStage(const UObject* WorldContextObject);
    // Opt-in development diagnostics; implementation is excluded from Shipping/Test.
    UFUNCTION(Exec) void CTLVRDebug(int32 Enabled = 1);
    UFUNCTION(Exec) void CTLVRStatus();
private:
    bool ShouldRouteQuestLinkAudio() const;
    void ConfigureQuestLinkAudio();
    UFUNCTION() void CheckQuestLinkOutput(const FString& CurrentDeviceName);
    UFUNCTION() void ChooseQuestLinkOutput(const TArray<FAudioOutputDeviceInfo>& AvailableDevices);
    UFUNCTION() void QuestLinkOutputSwapped(const FSwapAudioOutputResult& Result);
    bool bQuestAudioRoutingPending = false;
    void HandleTravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Reason);
    bool TickVRDebug(float DeltaSeconds);
    void StopVRDebug();
    bool bVRRestartRequested = false;
    FString LastVRSnapshot;
    FString LastVRContexts;
    TWeakObjectPtr<APawn> LastVRPawn;
    FVector LastVRPawnLocation = FVector::ZeroVector;
    double NextVRHeartbeat = 0;
    FTSTicker::FDelegateHandle VRDebugTicker;
    TSharedPtr<IInputProcessor> VRDebugInput;
    FDelegateHandle TravelFailureHandle;
    UFUNCTION() void ReportAudioOutput(const FString& DeviceName);
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> AudioTestPlayer;
    void BeforeMapLoad(const FWorldContext& Context, const FString& MapName);
    void AfterMapLoad(UWorld* World);
    void ApplyMapMusic(UWorld* World);
    float PrepareMap(const FString& MapName);
    bool CanContinue(const FCTLMapMusic* Entry) const;
    bool bFadingOut = false;
    bool bMusicDucked = false;
    FTimerHandle MusicDuckTimer;
    void RestoreMusicAfterCue();
    FDelegateHandle BeforeLoadHandle;
    FDelegateHandle AfterLoadHandle;
};
