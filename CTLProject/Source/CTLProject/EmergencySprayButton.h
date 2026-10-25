#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmergencySprayButton.generated.h"
class UStaticMeshComponent;
class UTextRenderComponent;
class UPhysicalPressInteraction;
class ASprayGameplayActor;
class AMissionDirector;

UCLASS(Blueprintable)
class CTLPROJECT_API AEmergencySprayButton : public AActor
{
    GENERATED_BODY()
public:
    AEmergencySprayButton();
    virtual void Tick(float Delta) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> ButtonMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UTextRenderComponent> StatusText;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> SpraySpawnPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UPhysicalPressInteraction> PressInteraction;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Emergency") TObjectPtr<AMissionDirector> Mission;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emergency", meta=(ClampMin="0", Units="s")) float UnlockDelay = 120.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emergency") TSubclassOf<ASprayGameplayActor> SprayClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emergency|Text") FText CountdownFormat;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emergency|Text") FText ReadyText;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emergency|Text") FText DispensedText;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Emergency|Text") FText FailedText;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Emergency") bool bDispensed = false;
    UFUNCTION(BlueprintCallable, Category="Emergency") void Press();
protected:
    virtual void BeginPlay() override;
private:
    bool bSpawning = false;
    void Refresh();
};
