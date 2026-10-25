#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Components/ActorComponent.h"
#include "CTLSoundEffects.generated.h"

class USoundBase;
class USoundAttenuation;
class UAudioComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class ECTLSoundEffect : uint8 { UIHover, UIConfirm, ObjectPickup, BroomSwing, SprayLoop, StunActivated, ContainerCapture, TutorialComplete, MissionComplete };

USTRUCT(BlueprintType)
struct FCTLSoundEffectSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<USoundBase> Sound;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1")) float Volume = .65f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", Units="s")) float MinIntervalSeconds = .12f;
};

UCLASS(BlueprintType)
class CTLPROJECT_API UCTLSoundEffectLibrary : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects") FCTLSoundEffectSettings UIHover;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects") FCTLSoundEffectSettings UIConfirm;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects") FCTLSoundEffectSettings ObjectPickup;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects") FCTLSoundEffectSettings BroomSwing;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects") FCTLSoundEffectSettings SprayLoop;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects") FCTLSoundEffectSettings StunActivated;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects") FCTLSoundEffectSettings ContainerCapture;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects") FCTLSoundEffectSettings TutorialComplete;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Effects") FCTLSoundEffectSettings MissionComplete;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mix", meta=(ClampMin="0", ClampMax="1")) float CompletionMusicDuckMultiplier = .35f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mix", meta=(ClampMin="0", ClampMax="1")) float MasterVolume = .8f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mix") TObjectPtr<USoundAttenuation> WorldAttenuation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spray", meta=(ClampMin="0", Units="s")) float SprayFadeInSeconds = .08f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spray", meta=(ClampMin="0", Units="s")) float SprayFadeOutSeconds = .12f;
    const FCTLSoundEffectSettings& Find(ECTLSoundEffect Effect) const;
};

/** Shared mix and per-source cooldowns; separate from the persistent music voice. */
UCLASS()
class CTLPROJECT_API UCTLSoundEffectsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    static UCTLSoundEffectsSubsystem* Get(const UObject* Context);
    UFUNCTION(BlueprintPure, Category="Sound Effects") UCTLSoundEffectLibrary* GetLibrary() const;
    UFUNCTION(BlueprintCallable, Category="Sound Effects") UAudioComponent* PlayEffect(UObject* Source, ECTLSoundEffect Effect, FVector WorldLocation = FVector::ZeroVector);
private:
    TMap<TWeakObjectPtr<UObject>, TMap<ECTLSoundEffect, double>> LastPlayed;
};

/** Listens to the existing BP_GrabComponent signals on the broom, bottle and spray. */
UCLASS(ClassGroup=Audio, meta=(BlueprintSpawnableComponent))
class CTLPROJECT_API UCTLPickupAudioComponent : public UActorComponent
{
    GENERATED_BODY()
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UFUNCTION() void GrabChanged();
    TWeakObjectPtr<USceneComponent> GrabState;
    bool bWasHeld = false;
};
