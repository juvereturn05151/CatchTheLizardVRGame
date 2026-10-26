#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "MissionFlow.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UWidgetComponent;
class UCameraComponent;
class UButton;
class UTextBlock;
class UInputAction;
class UMaterialInterface;
class UVRPanelInteractor;
class ALizardGameplayActor;
class AMissionDirector;
class APawn;
class UPhysicalPressInteraction;
class UTutorialDialogueWidget;
class UImage;
class UTexture2D;
class ASprayGameplayActor;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class EExitDoorAccess : uint8 { Unlocked, ClosingToLock, Locked, ExitUnlocked };
UENUM(BlueprintType)
enum class EMissionEntryState : uint8 { Legacy, AwaitingEntry, ClosingAfterEntry, InsideLocked, ExitUnlocked, Complete };

UCLASS(Blueprintable)
class CTLPROJECT_API AMissionExitDoor : public AActor
{
    GENERATED_BODY()
public:
    AMissionExitDoor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit") TObjectPtr<USceneComponent> Doorway;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit") TObjectPtr<UStaticMeshComponent> LeftPost;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit") TObjectPtr<UStaticMeshComponent> RightPost;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit") TObjectPtr<UStaticMeshComponent> Lintel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit") TObjectPtr<UStaticMeshComponent> DoorMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit") TObjectPtr<USceneComponent> Hinge;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit") TObjectPtr<UStaticMeshComponent> Handle;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit") TObjectPtr<UPhysicalPressInteraction> HandleInteraction;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit|Entry") TObjectPtr<UStaticMeshComponent> ExteriorHandle;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit|Entry") TObjectPtr<UPhysicalPressInteraction> ExteriorHandleInteraction;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit|Entry") TObjectPtr<UTextRenderComponent> ExteriorSign;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit|Entry") TObjectPtr<UBoxComponent> EntryThreshold;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit|Entry") TObjectPtr<UBoxComponent> SafeInteriorZone;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit|Entry") TObjectPtr<UBoxComponent> ClosingClearance;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Entry", meta=(ClampMin="10", Units="cm")) float PlayerClearanceRadius = 35.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Entry", meta=(ClampMin="1", Units="cm")) float HandClearanceRadius = 12.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Entry", meta=(ClampMin="0", Units="s")) float ClearBeforeClosingSeconds = .35f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Exit|Entry") EExitDoorAccess AccessState = EExitDoorAccess::Unlocked;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Entry") FText EntranceText;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Entry") FText EntryClosingText;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Entry") FText LockedHandleFeedback;
    UFUNCTION(BlueprintPure, Category="Exit|Entry") bool IsLocked() const;
    UFUNCTION(BlueprintPure, Category="Exit|Entry") bool IsClosingAreaClear() const;
    bool CrossedIntoHouse(const FVector& Previous, const FVector& Current) const;
    bool ContainsSafeInterior(const FVector& Position) const;
    void RequestEntryLock();
    void CancelEntryLock();
    void UnlockForExit();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit", meta=(ClampMin="65", ClampMax="120")) float OpenAngle = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit", meta=(ClampMin="1")) float OpenSpeed = 90.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Exit") bool bManuallyOpened = false;
    UFUNCTION(BlueprintCallable, Category="Exit") void OpenByHandle();
    UFUNCTION(BlueprintCallable, Category="Exit") void CloseByHandle();
    UFUNCTION(BlueprintCallable, Category="Exit") void ToggleByHandle();
    UFUNCTION(BlueprintPure, Category="Exit") bool IsPassable() const;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit") TObjectPtr<UBoxComponent> OutdoorZone;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit") TObjectPtr<UTextRenderComponent> ExitSign;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Exit") TObjectPtr<AMissionDirector> Mission;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit") FText LockedText;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit") FText ReadyText;
    UFUNCTION(BlueprintPure, Category="Exit") bool ContainsHeadPosition(FVector HeadPosition) const;
    void SetReady(bool Ready);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit|Guidance") TObjectPtr<UStaticMeshComponent> DestinationRing;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Exit|Guidance") TObjectPtr<UTextRenderComponent> DestinationLabel;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Guidance") TObjectPtr<UMaterialInterface> HighlightMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Guidance") TObjectPtr<UMaterialInterface> RingMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Guidance") FLinearColor HighlightColor = FLinearColor(.15f, 1.f, .55f);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Guidance", meta=(ClampMin="0", ClampMax="3", Units="Hz")) float GuidancePulseSpeed = .65f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Guidance", meta=(ClampMin="10", Units="cm")) float MarkerDiameter = 80.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Guidance", meta=(ClampMin="1", Units="cm")) float MarkerGroundClearance = 3.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Guidance", meta=(Units="cm")) FVector2D DestinationOffset = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Guidance", meta=(Units="cm")) FVector DestinationTextOffset = FVector(0, 0, 65);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Guidance", meta=(Units="cm")) FVector DoorMarkerOffset = FVector(-20, 0, 285);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Guidance", meta=(ClampMin="1", Units="cm")) float GuidanceTextSize = 18.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Guidance") FText DoorMarkerText;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Guidance") FText DestinationText;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Exit|Guidance", meta=(ClampMin="1", ClampMax="120", Units="deg")) float PassableAngle = 75.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Exit|Guidance") bool bGuidanceActive = false;
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    bool UsesOutdoorEntry() const;
    bool CanCloseTo(float Yaw) const;
    bool ReportLockedHandle();
    float ClosingClearSeconds = 0.f;
    float LastHandlePressTime = -1000.f;
    bool bPreviouslyOutside = false;
    bool bPreviouslyPassable = false;
    bool bDestinationGroundValid = false;
    float PulseElapsed = 0.f;
    float EffectiveMarkerDiameter = 80.f;
    FTransform OriginalSignTransform;
    float OriginalSignSize = 25.f;
    void PlaceDestination();
    void UpdateGuidancePulse();
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> OriginalDoorMaterials;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> OriginalHandleMaterials;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> DoorGlow;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> HandleGlow;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RingGlow;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> OriginalExteriorHandleMaterials;
};

UCLASS(Blueprintable)
class CTLPROJECT_API UMissionCompleteWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Present(AMissionDirector* InMission, int32 Captured, int32 Target, bool HasNext);
    void SetBusy();
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> CountsText;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UButton> NextMissionButton;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UButton> MenuButton;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> DebriefText;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> RatingText;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> Star1;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> Star2;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UImage> Star3;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rating") TObjectPtr<UTexture2D> FilledStar;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rating") TObjectPtr<UTexture2D> EmptyStar;
protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    TWeakObjectPtr<AMissionDirector> Mission;
    UFUNCTION() void NextClicked();
    UFUNCTION() void MenuClicked();
    UFUNCTION() void Hovered();
};

UCLASS(Blueprintable)
class CTLPROJECT_API UMissionObjectiveWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Present(const FText& Objective, int32 Captured, int32 Target, const FText& Hint);
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> ObjectiveText;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> ProgressText;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidget)) TObjectPtr<UTextBlock> HintText;
};

UCLASS(Blueprintable)
class CTLPROJECT_API AMissionDirector : public AActor
{
    GENERATED_BODY()
public:
    AMissionDirector();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission") bool bTutorial = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|Entry") bool bStartOutside = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|Entry") FText EntryObjective;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|Entry") FText EntryClosingObjective;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|Entry", meta=(ClampMin=".5", Units="s")) float LockedFeedbackSeconds = 3.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Mission|Entry") EMissionEntryState EntryState = EMissionEntryState::Legacy;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Mission|Entry") bool bHasEnteredHouse = false;
    UFUNCTION(BlueprintPure, Category="Mission|Entry") bool UsesOutdoorEntry() const { return bStartOutside && !bTutorial; }
    bool GetPlayerHeadLocation(FVector& Position) const;
    void ReportLockedDoor(const FText& Message);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission") bool bEmergencyRating = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|Dialogue", meta=(MultiLine=true)) FText Introduction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|Dialogue", meta=(MultiLine=true)) FText Debrief;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|UI") FText UnassistedResult;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|UI") FText AssistedResult;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission|UI") TObjectPtr<UWidgetComponent> DialoguePanel;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Mission|Progress") bool bGameplayActive = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Mission|Progress") float ActiveGameplaySeconds = 0.f;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Mission|Progress") bool bEmergencyAssisted = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Mission|Progress") int32 AwardedStars = 0;
    UFUNCTION(BlueprintCallable, Category="Mission") void AcknowledgeIntroduction();
    void RecordEmergencyDispensed(ASprayGameplayActor* Spray);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission", meta=(ClampMin="1")) int32 CaptureTarget = 1;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Mission") TArray<TObjectPtr<ALizardGameplayActor>> MissionLizards;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Mission") TArray<TObjectPtr<AActor>> Containers;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Mission") TArray<TObjectPtr<AActor>> MissionTools;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Mission") TObjectPtr<AActor> CollectionManager;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Mission") TObjectPtr<AMissionExitDoor> ExitDoor;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|Travel") TSoftObjectPtr<UWorld> NextMission;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|Travel") TSoftObjectPtr<UWorld> MainMenu;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|Travel", meta=(ClampMin="0")) float ViewFadeSeconds = .4f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|UI") FText CaptureObjective;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|UI") FText ExitObjective;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|UI") FText HeadOutsideObjective;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|UI") FText EarlyExitMessage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mission|UI", meta=(ClampMin="75", Units="cm")) float CompletionPanelDistance = 155.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission|UI") TObjectPtr<UInputAction> LeftTriggerAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission|UI") TObjectPtr<UInputAction> RightTriggerAction;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Mission|UI") TObjectPtr<UMaterialInterface> RayMaterial;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission|UI") TObjectPtr<UWidgetComponent> ObjectivePanel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission|UI") TObjectPtr<UWidgetComponent> CompletionPanel;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mission|UI") TObjectPtr<UVRPanelInteractor> Interaction;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Mission|Progress") int32 CapturedCount = 0;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Mission|Progress") bool bComplete = false;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Mission|Progress") bool bTraveling = false;
    UFUNCTION(BlueprintCallable, Category="Mission") void EvaluateExit();
    UFUNCTION(BlueprintPure, Category="Mission") bool IsPlayerOutside() const;
    UFUNCTION(BlueprintCallable, Category="Mission") void GoToNextMission();
    UFUNCTION(BlueprintCallable, Category="Mission") void ReturnToMenu();
    // Editor authoring only; runtime builds return false.
    UFUNCTION(BlueprintCallable, Category="Mission|Editor") static bool BuildMissionGeometryAndNavigation();
    UFUNCTION(BlueprintPure, Category="Mission|Teleport") static bool CanTeleportTo(APawn* Pawn, FVector Destination);
    UFUNCTION(BlueprintCallable, Category="Mission|Teleport") static bool UpdateTeleportPreview(APawn* Pawn, FVector Destination, bool NavigationValid, class UNiagaraParameterCollection* Colors);
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void UpdateEntrySequence();
    bool IsExitReady() const;
    FVector PreviousHeadPosition = FVector::ZeroVector;
    bool bHaveHeadSample = false;
    bool bCrossedEntrance = false;
    FText DoorFeedback;
    float DoorFeedbackUntil = 0.f;
    void InitializeMission();
    void UpdateObjective();
    void CompleteMission();
    void Travel(const TSoftObjectPtr<UWorld>& Map);
    void FinishTravel();
    UFUNCTION() void Stored(ALizardGameplayActor* Lizard, AActor* Container);
    TSet<TWeakObjectPtr<ALizardGameplayActor>> Captured;
    TWeakObjectPtr<APawn> PlayerPawn;
    TWeakObjectPtr<UCameraComponent> Head;
    UPROPERTY(Transient) TObjectPtr<UMissionCompleteWidget> CompletionWidget;
    UPROPERTY(Transient) TObjectPtr<UMissionObjectiveWidget> ObjectiveWidget;
    TSoftObjectPtr<UWorld> PendingMap;
    FTimerHandle InitializationTimer;
    FTimerHandle TravelTimer;
    FTransform ObjectiveHome;
    bool bWarningPlaced = false;
    bool bExitPreviouslyOutside = false;
    UPROPERTY(Transient) TObjectPtr<UTutorialDialogueWidget> DialogueWidget;
};
