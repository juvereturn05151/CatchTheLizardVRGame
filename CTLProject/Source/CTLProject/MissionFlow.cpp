#include "MissionFlow.h"
#include "PhysicalPressInteraction.h"
#include "TutorialWidgets.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CTLBackgroundMusic.h"
#include "CTLSoundEffects.h"
#include "LizardGameplayActor.h"
#include "SprayGameplayActor.h"
#include "LizardBlueprintBridge.h"
#include "VRPanelInteractor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#if WITH_EDITOR
#include "Editor.h"
#include "EditorBuildUtils.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "AI/NavDataGenerator.h"
#endif

namespace
{
    void SetBool(UObject* Object, FName Name, bool Value)
    {
        if (auto* Property = Object ? FindFProperty<FBoolProperty>(Object->GetClass(), Name) : nullptr)
            Property->SetPropertyValue_InContainer(Object, Value);
    }
}

AMissionExitDoor::AMissionExitDoor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = .05f;
    Doorway = CreateDefaultSubobject<USceneComponent>(TEXT("Doorway"));
    SetRootComponent(Doorway);
    LeftPost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftPost"));
    RightPost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightPost"));
    Lintel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lintel"));
    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (auto* Part : {LeftPost.Get(), RightPost.Get(), Lintel.Get(), DoorMesh.Get()})
    {
        Part->SetupAttachment(Doorway);
        Part->SetStaticMesh(Cube.Object);
        Part->SetCollisionProfileName(TEXT("BlockAll"));
    }
    LeftPost->SetRelativeLocation(FVector(0, -105, 120)); LeftPost->SetRelativeScale3D(FVector(.18f, .1f, 2.4f));
    RightPost->SetRelativeLocation(FVector(0, 105, 120)); RightPost->SetRelativeScale3D(FVector(.18f, .1f, 2.4f));
    Lintel->SetRelativeLocation(FVector(0, 0, 245)); Lintel->SetRelativeScale3D(FVector(.18f, 2.2f, .1f));
    Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge")); Hinge->SetupAttachment(Doorway);
    Hinge->SetRelativeLocation(FVector(0, -99, 0)); Hinge->SetMobility(EComponentMobility::Movable);
    DoorMesh->SetupAttachment(Hinge); DoorMesh->SetMobility(EComponentMobility::Movable);
    DoorMesh->SetRelativeLocation(FVector(0, 99, 119)); DoorMesh->SetRelativeScale3D(FVector(.08f, 1.98f, 2.38f));
    DoorMesh->SetRelativeRotation(FRotator::ZeroRotator);
    DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
    DoorMesh->SetCanEverAffectNavigation(false); // Static nav stays continuous; the visible leaf blocks teleport traces.
    Handle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Handle")); Handle->SetupAttachment(Hinge);
    Handle->SetStaticMesh(Cube.Object); Handle->SetRelativeLocation(FVector(-12, 173, 105));
    Handle->SetRelativeScale3D(FVector(.12f, .22f, .06f)); Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HandleInteraction = CreateDefaultSubobject<UPhysicalPressInteraction>(TEXT("HandleInteraction")); HandleInteraction->PressPoint = Handle;
    ExteriorHandle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExteriorHandle")); ExteriorHandle->SetupAttachment(Hinge);
    ExteriorHandle->SetStaticMesh(Cube.Object); ExteriorHandle->SetRelativeLocation(FVector(12, 173, 105));
    ExteriorHandle->SetRelativeScale3D(FVector(.12f, .22f, .06f)); ExteriorHandle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ExteriorHandleInteraction = CreateDefaultSubobject<UPhysicalPressInteraction>(TEXT("ExteriorHandleInteraction")); ExteriorHandleInteraction->PressPoint = ExteriorHandle;
    EntryThreshold = CreateDefaultSubobject<UBoxComponent>(TEXT("EntryThreshold"));
    SafeInteriorZone = CreateDefaultSubobject<UBoxComponent>(TEXT("SafeInteriorZone"));
    ClosingClearance = CreateDefaultSubobject<UBoxComponent>(TEXT("ClosingClearance"));
    for (auto* Zone : {EntryThreshold.Get(), SafeInteriorZone.Get(), ClosingClearance.Get()})
    {
        Zone->SetupAttachment(Doorway); Zone->SetCollisionEnabled(ECollisionEnabled::NoCollision); Zone->SetGenerateOverlapEvents(false);
    }
    EntryThreshold->SetRelativeLocation(FVector(0, 0, 130)); EntryThreshold->SetBoxExtent(FVector(45, 98, 165));
    SafeInteriorZone->SetRelativeLocation(FVector(-800, 0, 130)); SafeInteriorZone->SetBoxExtent(FVector(700, 480, 165));
    ClosingClearance->SetRelativeLocation(FVector(100, 0, 125)); ClosingClearance->SetBoxExtent(FVector(125, 130, 145));
    OutdoorZone = CreateDefaultSubobject<UBoxComponent>(TEXT("OutdoorZone"));
    OutdoorZone->SetupAttachment(Doorway);
    OutdoorZone->SetRelativeLocation(FVector(215, 0, 130)); OutdoorZone->SetBoxExtent(FVector(95, 130, 170));
    OutdoorZone->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Head-position test, never hands/held-object overlaps.
    ExitSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ExitSign"));
    ExitSign->SetupAttachment(Doorway); ExitSign->SetRelativeLocation(FVector(-20, 0, 280));
    ExitSign->SetRelativeRotation(FRotator(0, 180, 0)); ExitSign->SetHorizontalAlignment(EHTA_Center);
    ExitSign->SetWorldSize(25);
    ExteriorSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ExteriorSign")); ExteriorSign->SetupAttachment(Doorway);
    ExteriorSign->SetRelativeLocation(FVector(20, 0, 280)); ExteriorSign->SetHorizontalAlignment(EHTA_Center); ExteriorSign->SetWorldSize(20);
    ExteriorSign->SetCollisionEnabled(ECollisionEnabled::NoCollision); ExteriorSign->SetCastShadow(false);
    EntranceText = NSLOCTEXT("CTLMission", "EntranceAccess", "ENTRANCE\nOpen the handle to enter.");
    EntryClosingText = NSLOCTEXT("CTLMission", "DoorClosing", "Please keep the doorway clear.\nThe door will close for this assignment.");
    LockedHandleFeedback = NSLOCTEXT("CTLMission", "LockedHandle", "Catch all required lizards to unlock the exit.");
    LockedText = NSLOCTEXT("CTLMission", "ExitLocked", "EXIT\nComplete the capture objective\nHand at handle + index trigger");
    ReadyText = NSLOCTEXT("CTLMission", "ExitReady", "EXIT READY\nOPEN HANDLE AND LEAVE\nHand at handle + index trigger");
    DestinationRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationRing")); DestinationRing->SetupAttachment(OutdoorZone);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
    DestinationRing->SetStaticMesh(Plane.Object);
    DestinationLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DestinationLabel")); DestinationLabel->SetupAttachment(OutdoorZone);
    DestinationLabel->SetHorizontalAlignment(EHTA_Center); DestinationLabel->SetWorldSize(GuidanceTextSize);
    for (auto* Visual : {static_cast<UPrimitiveComponent*>(DestinationRing), static_cast<UPrimitiveComponent*>(DestinationLabel)})
    {
        Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision); Visual->SetGenerateOverlapEvents(false);
        Visual->SetCanEverAffectNavigation(false); Visual->SetCastShadow(false); Visual->SetVisibility(false);
    }
    DoorMarkerText = NSLOCTEXT("CTLMission", "HighlightedExit", "EXIT");
    DestinationText = NSLOCTEXT("CTLMission", "ExitDestination", "Exit Here");
    SetReady(false);
}

void AMissionExitDoor::BeginPlay()
{
    Super::BeginPlay(); bManuallyOpened = false; Hinge->SetRelativeRotation(FRotator::ZeroRotator);
    AccessState = EExitDoorAccess::Unlocked; ClosingClearSeconds = 0; LastHandlePressTime = -1000;
    ExteriorHandle->SetVisibility(UsesOutdoorEntry()); ExteriorHandleInteraction->SetComponentTickEnabled(UsesOutdoorEntry());
    // Migrate pre-existing placed doors whose serialized leaf was attached to Doorway.
    DoorMesh->AttachToComponent(Hinge, FAttachmentTransformRules::KeepRelativeTransform);
    DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
    OriginalSignTransform = ExitSign->GetRelativeTransform(); OriginalSignSize = ExitSign->WorldSize;
    for (int32 Index = 0; Index < DoorMesh->GetNumMaterials(); ++Index) OriginalDoorMaterials.Add(DoorMesh->GetMaterial(Index));
    for (int32 Index = 0; Index < Handle->GetNumMaterials(); ++Index) OriginalHandleMaterials.Add(Handle->GetMaterial(Index));
    for (int32 Index = 0; Index < ExteriorHandle->GetNumMaterials(); ++Index) OriginalExteriorHandleMaterials.Add(ExteriorHandle->GetMaterial(Index));
    bPreviouslyOutside = bPreviouslyPassable = false; SetReady(false);
    HandleInteraction->OnPressed.AddUniqueDynamic(this, &AMissionExitDoor::ToggleByHandle);
    if (UsesOutdoorEntry()) ExteriorHandleInteraction->OnPressed.AddUniqueDynamic(this, &AMissionExitDoor::ToggleByHandle);
}
bool AMissionExitDoor::UsesOutdoorEntry() const { return Mission && Mission->UsesOutdoorEntry(); }
bool AMissionExitDoor::IsLocked() const { return AccessState == EExitDoorAccess::ClosingToLock || AccessState == EExitDoorAccess::Locked; }
bool AMissionExitDoor::ReportLockedHandle()
{
    if (!IsLocked()) return false;
    if (Mission) Mission->ReportLockedDoor(LockedHandleFeedback);
    return true;
}
void AMissionExitDoor::OpenByHandle() { if (!ReportLockedHandle() && (!Mission || !Mission->bComplete)) bManuallyOpened = true; }
void AMissionExitDoor::CloseByHandle() { if (!ReportLockedHandle() && (!Mission || !Mission->bComplete)) bManuallyOpened = false; }
void AMissionExitDoor::ToggleByHandle()
{
    // A hand between the two reachable handles must not toggle twice on the same press.
    if (UsesOutdoorEntry())
    {
        if (GetWorld()->GetTimeSeconds() - LastHandlePressTime < .2f) return;
        LastHandlePressTime = GetWorld()->GetTimeSeconds();
    }
    if (bManuallyOpened) CloseByHandle(); else OpenByHandle();
}
void AMissionExitDoor::RequestEntryLock()
{
    if (!UsesOutdoorEntry() || AccessState != EExitDoorAccess::Unlocked) return;
    AccessState = EExitDoorAccess::ClosingToLock; bManuallyOpened = false; ClosingClearSeconds = 0; SetReady(false);
}
void AMissionExitDoor::UnlockForExit()
{
    if (AccessState != EExitDoorAccess::Locked) return;
    AccessState = EExitDoorAccess::ExitUnlocked; bManuallyOpened = false; SetReady(true);
}
void AMissionExitDoor::CancelEntryLock()
{
    if (AccessState != EExitDoorAccess::ClosingToLock) return;
    // A retreat while closure is deferred must never strand the player outside a locked house.
    AccessState = EExitDoorAccess::Unlocked; SetReady(false);
}
bool AMissionExitDoor::CrossedIntoHouse(const FVector& Previous, const FVector& Current) const
{
    const FVector A = EntryThreshold->GetComponentTransform().InverseTransformPosition(Previous);
    const FVector B = EntryThreshold->GetComponentTransform().InverseTransformPosition(Current);
    if (A.X <= 0 || B.X > 0 || FMath::IsNearlyEqual(A.X, B.X)) return false;
    const FVector Crossing = FMath::Lerp(A, B, A.X / (A.X - B.X));
    const FVector Extent = EntryThreshold->GetUnscaledBoxExtent();
    return FMath::Abs(Crossing.Y) <= Extent.Y && FMath::Abs(Crossing.Z) <= Extent.Z;
}
bool AMissionExitDoor::ContainsSafeInterior(const FVector& Position) const
{
    const FVector Extent = SafeInteriorZone->GetUnscaledBoxExtent();
    return FBox(-Extent, Extent).IsInsideOrOn(SafeInteriorZone->GetComponentTransform().InverseTransformPosition(Position));
}
bool AMissionExitDoor::IsClosingAreaClear() const
{
    FVector Head;
    if (!Mission || !Mission->GetPlayerHeadLocation(Head)) return false;
    if (AccessState == EExitDoorAccess::ClosingToLock && !ContainsSafeInterior(Head)) return false;
    const FVector Extent = ClosingClearance->GetUnscaledBoxExtent();
    const auto Near = [&](const FVector& Position, float Radius)
    {
        const FVector P = ClosingClearance->GetComponentTransform().InverseTransformPosition(Position);
        const FVector LocalRadius = FVector(Radius) / ClosingClearance->GetComponentScale().GetAbs().ComponentMax(FVector(.01f));
        return FMath::Abs(P.X) <= Extent.X + LocalRadius.X && FMath::Abs(P.Y) <= Extent.Y + LocalRadius.Y && FMath::Abs(P.Z) <= Extent.Z + 180.f;
    };
    if (Near(Head, PlayerClearanceRadius)) return false;
    APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    for (FName Name : {FName(TEXT("MotionControllerLeftGrip")), FName(TEXT("MotionControllerRightGrip"))})
        if (auto* Hand = Cast<USceneComponent>(LizardBlueprintBridge::Object(Pawn, Name)))
            if (Near(Hand->GetComponentLocation(), HandClearanceRadius)) return false;
    TArray<FOverlapResult> Overlaps;
    FCollisionObjectQueryParams Types; Types.AddObjectTypesToQuery(ECC_WorldDynamic); Types.AddObjectTypesToQuery(ECC_PhysicsBody); Types.AddObjectTypesToQuery(ECC_Pawn);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(EntryDoorClearance), false, this); Query.AddIgnoredActor(Pawn);
    GetWorld()->OverlapMultiByObjectType(Overlaps, ClosingClearance->GetComponentLocation(), ClosingClearance->GetComponentQuat(), Types,
        FCollisionShape::MakeBox(ClosingClearance->GetScaledBoxExtent()), Query);
    for (const auto& Hit : Overlaps)
        if (auto* Part = Hit.GetComponent(); Part && Part->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block) return false;
    return true;
}
bool AMissionExitDoor::CanCloseTo(float Yaw) const
{
    const FTransform HingePose(FRotator(0, Yaw, 0), Hinge->GetRelativeLocation(), Hinge->GetRelativeScale3D());
    const FTransform LeafPose = DoorMesh->GetRelativeTransform() * HingePose * Doorway->GetComponentTransform();
    const FVector Extent = DoorMesh->GetStaticMesh()->GetBounds().BoxExtent * LeafPose.GetScale3D().GetAbs() - FVector(1, 2, 2);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(EntryDoorLeaf), false, this); Query.AddIgnoredActor(UGameplayStatics::GetPlayerPawn(this, 0));
    return !GetWorld()->OverlapBlockingTestByChannel(LeafPose.GetLocation(), LeafPose.GetRotation(), ECC_Pawn,
        FCollisionShape::MakeBox(Extent.ComponentMax(FVector(1))), Query);
}
bool AMissionExitDoor::IsPassable() const { return FMath::Abs(Hinge->GetRelativeRotation().Yaw) >= FMath::Min(PassableAngle, OpenAngle); }
bool AMissionExitDoor::ContainsHeadPosition(FVector Position) const
{
    return FBox(-OutdoorZone->GetUnscaledBoxExtent(), OutdoorZone->GetUnscaledBoxExtent()).IsInsideOrOn(
        OutdoorZone->GetComponentTransform().InverseTransformPosition(Position));
}
void AMissionExitDoor::SetReady(bool Ready)
{
    Ready = Ready && !(IsValid(Mission) && Mission->bComplete);
    if (Ready != bGuidanceActive)
    {
        bGuidanceActive = Ready;
        if (Ready)
        {
            PulseElapsed = 0.f;
            if (HighlightMaterial)
            {
                DoorGlow = UMaterialInstanceDynamic::Create(HighlightMaterial, this);
                HandleGlow = UMaterialInstanceDynamic::Create(HighlightMaterial, this);
                for (int32 Index = 0; Index < OriginalDoorMaterials.Num(); ++Index) DoorMesh->SetMaterial(Index, DoorGlow);
                for (int32 Index = 0; Index < OriginalHandleMaterials.Num(); ++Index) Handle->SetMaterial(Index, HandleGlow);
                for (int32 Index = 0; Index < OriginalExteriorHandleMaterials.Num(); ++Index) ExteriorHandle->SetMaterial(Index, HandleGlow);
            }
            if (RingMaterial) { RingGlow = UMaterialInstanceDynamic::Create(RingMaterial, this); DestinationRing->SetMaterial(0, RingGlow); }
            ExitSign->SetRelativeLocation(DoorMarkerOffset); ExitSign->SetWorldSize(GuidanceTextSize * 1.5f);
            DestinationLabel->SetText(DestinationText); DestinationLabel->SetWorldSize(GuidanceTextSize);
            PlaceDestination(); UpdateGuidancePulse();
        }
        else
        {
            for (int32 Index = 0; Index < OriginalDoorMaterials.Num(); ++Index) DoorMesh->SetMaterial(Index, OriginalDoorMaterials[Index]);
            for (int32 Index = 0; Index < OriginalHandleMaterials.Num(); ++Index) Handle->SetMaterial(Index, OriginalHandleMaterials[Index]);
            for (int32 Index = 0; Index < OriginalExteriorHandleMaterials.Num(); ++Index) ExteriorHandle->SetMaterial(Index, OriginalExteriorHandleMaterials[Index]);
            DoorGlow = nullptr; HandleGlow = nullptr; RingGlow = nullptr;
            ExitSign->SetRelativeTransform(OriginalSignTransform); ExitSign->SetWorldSize(OriginalSignSize);
        }
    }
    DestinationRing->SetVisibility(Ready && bDestinationGroundValid);
    DestinationLabel->SetVisibility(Ready && bDestinationGroundValid);
    ExitSign->SetVisibility(!(IsValid(Mission) && Mission->bComplete));
    FText DisplayText = Ready ? DoorMarkerText : LockedText;
    if (UsesOutdoorEntry() && !Ready)
        DisplayText = AccessState == EExitDoorAccess::ClosingToLock ? EntryClosingText : (IsLocked() ? LockedHandleFeedback : EntranceText);
    ExitSign->SetText(DisplayText);
    ExteriorSign->SetVisibility(UsesOutdoorEntry() && !(Mission && Mission->bComplete)); ExteriorSign->SetText(DisplayText);
    ExteriorSign->SetTextRenderColor(Ready ? HighlightColor.ToFColor(true) : FColor(255, 210, 100));
    ExitSign->SetTextRenderColor(Ready ? HighlightColor.ToFColor(true) : FColor(255, 210, 100));
    for (auto* Post : {LeftPost.Get(), RightPost.Get(), Lintel.Get()})
        Post->SetVectorParameterValueOnMaterials(TEXT("BodyColor"), Ready ? FVector(HighlightColor.R, HighlightColor.G, HighlightColor.B) : FVector(.22f, .3f, .32f));
}
void AMissionExitDoor::PlaceDestination()
{
    const FVector Extent = OutdoorZone->GetScaledBoxExtent();
    EffectiveMarkerDiameter = FMath::Clamp(MarkerDiameter, 1.f, FMath::Max(1.f, 2.f * FMath::Min(Extent.X, Extent.Y) - 12.f));
    const float Radius = EffectiveMarkerDiameter * .5f;
    const FVector LocalOffset(FMath::Clamp(DestinationOffset.X, -FMath::Max(0.f, Extent.X - Radius - 5.f), FMath::Max(0.f, Extent.X - Radius - 5.f)),
        FMath::Clamp(DestinationOffset.Y, -FMath::Max(0.f, Extent.Y - Radius - 5.f), FMath::Max(0.f, Extent.Y - Radius - 5.f)), 0);
    const FVector Center = OutdoorZone->GetComponentLocation() + OutdoorZone->GetComponentQuat().RotateVector(LocalOffset);
    FHitResult Ground;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ExitGuidanceGround), false, this);
    bDestinationGroundValid = GetWorld()->LineTraceSingleByObjectType(Ground, Center, Center - FVector(0, 0, Extent.Z + 500.f),
        FCollisionObjectQueryParams(ECC_WorldStatic), Query) && Ground.ImpactNormal.Z >= .7f;
    if (!bDestinationGroundValid) { UE_LOG(LogTemp, Warning, TEXT("%s: exit guidance requires a walkable floor beneath OutdoorZone"), *GetName()); return; }
    const FVector Floor = Ground.ImpactPoint + FVector(0, 0, MarkerGroundClearance);
    DestinationRing->SetWorldLocationAndRotation(Floor, FRotator::ZeroRotator);
    DestinationLabel->SetWorldLocationAndRotation(Floor + Doorway->GetComponentQuat().RotateVector(DestinationTextOffset), Doorway->GetComponentRotation() + FRotator(0, 180, 0));
}
void AMissionExitDoor::UpdateGuidancePulse()
{
    const float Pulse = .8f + .2f * FMath::Sin(PulseElapsed * GuidancePulseSpeed * 2.f * PI);
    if (DoorGlow) DoorGlow->SetVectorParameterValue(TEXT("GuidanceColor"), HighlightColor * Pulse);
    if (HandleGlow) HandleGlow->SetVectorParameterValue(TEXT("GuidanceColor"), HighlightColor * (Pulse * 1.5f));
    if (RingGlow) RingGlow->SetVectorParameterValue(TEXT("GuidanceColor"), HighlightColor * (Pulse * 1.5f));
    DestinationLabel->SetTextRenderColor(HighlightColor.ToFColor(true));
    const float Scale = EffectiveMarkerDiameter / 100.f * (.97f + .03f * Pulse);
    DestinationRing->SetWorldScale3D(FVector(Scale, Scale, 1.f));
}
void AMissionExitDoor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float CurrentYaw = Hinge->GetRelativeRotation().Yaw;
    float NewYaw = FMath::FInterpConstantTo(CurrentYaw, bManuallyOpened ? -OpenAngle : 0.f, DeltaSeconds, OpenSpeed);
    if (UsesOutdoorEntry() && !bManuallyOpened && FMath::Abs(CurrentYaw) > .1f)
    {
        const bool Clear = IsClosingAreaClear() && CanCloseTo(NewYaw);
        if (Clear) ClosingClearSeconds += DeltaSeconds;
        else ClosingClearSeconds = 0;
        if (!Clear || ClosingClearSeconds < ClearBeforeClosingSeconds) NewYaw = CurrentYaw;
    }
    else ClosingClearSeconds = 0;
    Hinge->SetRelativeRotation(FRotator(0, NewYaw, 0));
    if (AccessState == EExitDoorAccess::ClosingToLock && FMath::Abs(NewYaw) < .1f)
    {
        AccessState = EExitDoorAccess::Locked; SetReady(false);
    }
    if (bGuidanceActive) { PulseElapsed += DeltaSeconds; UpdateGuidancePulse(); }
    if (!Mission) return;
    const bool Outside = Mission->IsPlayerOutside(), Passable = IsPassable();
    if (Outside != bPreviouslyOutside || Passable != bPreviouslyPassable)
    {
        bPreviouslyOutside = Outside; bPreviouslyPassable = Passable; Mission->EvaluateExit();
    }
}
void AMissionExitDoor::EndPlay(const EEndPlayReason::Type Reason)
{
    SetReady(false); ExitSign->SetVisibility(false); ExteriorSign->SetVisibility(false);
    Super::EndPlay(Reason);
}

void UMissionCompleteWidget::NativeConstruct()
{
    Super::NativeConstruct();
    NextMissionButton->OnClicked.AddUniqueDynamic(this, &UMissionCompleteWidget::NextClicked);
    MenuButton->OnClicked.AddUniqueDynamic(this, &UMissionCompleteWidget::MenuClicked);
    for (auto* Button : {NextMissionButton.Get(), MenuButton.Get()}) Button->OnHovered.AddUniqueDynamic(this, &UMissionCompleteWidget::Hovered);
}
void UMissionCompleteWidget::NativeDestruct()
{
    NextMissionButton->OnClicked.RemoveDynamic(this, &UMissionCompleteWidget::NextClicked);
    MenuButton->OnClicked.RemoveDynamic(this, &UMissionCompleteWidget::MenuClicked);
    for (auto* Button : {NextMissionButton.Get(), MenuButton.Get()}) Button->OnHovered.RemoveDynamic(this, &UMissionCompleteWidget::Hovered);
    Mission.Reset(); Super::NativeDestruct();
}
void UMissionCompleteWidget::Present(AMissionDirector* InMission, int32 Count, int32 Target, bool HasNext)
{
    Mission = InMission;
    CountsText->SetText(FText::Format(NSLOCTEXT("CTLMission", "Counts", "Lizards captured: {0} / {1}"), FText::AsNumber(Count), FText::AsNumber(Target)));
    if (DebriefText) { DebriefText->SetText(InMission->Debrief); DebriefText->SetVisibility(InMission->Debrief.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible); }
    if (RatingText) { RatingText->SetText(InMission->bEmergencyAssisted ? InMission->AssistedResult : InMission->UnassistedResult); RatingText->SetVisibility(InMission->bEmergencyRating ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed); }
    int32 Index = 0;
    for (auto* Star : {Star1.Get(), Star2.Get(), Star3.Get()}) {
        if (Star) { Star->SetVisibility(InMission->bEmergencyRating ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
            Star->SetBrushFromTexture(++Index <= InMission->AwardedStars ? FilledStar : EmptyStar); }
    }
    NextMissionButton->SetVisibility(HasNext ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
void UMissionCompleteWidget::SetBusy() { NextMissionButton->SetIsEnabled(false); MenuButton->SetIsEnabled(false); }
void UMissionCompleteWidget::NextClicked() { if (Mission.IsValid()) Mission->GoToNextMission(); }
void UMissionCompleteWidget::MenuClicked() { if (Mission.IsValid()) Mission->ReturnToMenu(); }
void UMissionCompleteWidget::Hovered() { if (auto* Audio = UCTLSoundEffectsSubsystem::Get(this)) Audio->PlayEffect(this, ECTLSoundEffect::UIHover); }
void UMissionObjectiveWidget::Present(const FText& Objective, int32 Count, int32 Target, const FText& Hint)
{
    ObjectiveText->SetText(Objective);
    ProgressText->SetText(FText::Format(NSLOCTEXT("CTLMission", "Progress", "{0}/{1}"), FText::AsNumber(Count), FText::AsNumber(Target)));
    HintText->SetText(Hint);
}

AMissionDirector::AMissionDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("MissionRoot")));
    DialoguePanel = CreateDefaultSubobject<UWidgetComponent>(TEXT("DialoguePanel"));
    ObjectivePanel = CreateDefaultSubobject<UWidgetComponent>(TEXT("ObjectivePanel"));
    CompletionPanel = CreateDefaultSubobject<UWidgetComponent>(TEXT("CompletionPanel"));
    for (auto* Panel : {ObjectivePanel.Get(), CompletionPanel.Get(), DialoguePanel.Get()})
    {
        Panel->SetupAttachment(RootComponent); Panel->SetWidgetSpace(EWidgetSpace::World);
        Panel->SetTwoSided(false); Panel->SetWindowFocusable(false); Panel->SetCastShadow(false); Panel->SetTickWhenOffscreen(true);
        Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Panel->SetCollisionResponseToAllChannels(ECR_Ignore); Panel->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
    }
    ObjectivePanel->SetDrawSize(FVector2D(1000, 400)); ObjectivePanel->SetRelativeScale3D(FVector(.14f));
    CompletionPanel->SetDrawSize(FVector2D(1200, 900)); CompletionPanel->SetRelativeScale3D(FVector(.12f));
    CompletionPanel->SetVisibility(false);
    DialoguePanel->SetDrawSize(FVector2D(1000, 900)); DialoguePanel->SetRelativeScale3D(FVector(.15f)); DialoguePanel->SetVisibility(false);
    UnassistedResult = NSLOCTEXT("CTLMission", "Unassisted", "Completed without emergency assistance!");
    AssistedResult = NSLOCTEXT("CTLMission", "Assisted", "Completed with emergency assistance!");
    Interaction = CreateDefaultSubobject<UVRPanelInteractor>(TEXT("CompletionInteraction"));
    ExitObjective = NSLOCTEXT("CTLMission", "OpenHighlightedExit", "Open the highlighted door and head outside.");
    HeadOutsideObjective = NSLOCTEXT("CTLMission", "HeadOutside", "Head outside to finish the mission.");
    EarlyExitMessage = NSLOCTEXT("CTLMission", "TooEarly", "Catch the remaining lizards before leaving.");
    EntryObjective = NSLOCTEXT("CTLMission", "EnterHouse", "Open the entrance door and step inside.");
    EntryClosingObjective = NSLOCTEXT("CTLMission", "EntryClosingObjective", "Step clear of the doorway so the door can close.");
}
void AMissionDirector::BeginPlay()
{
    Super::BeginPlay();
    Captured.Reset(); CapturedCount = 0; bComplete = bTraveling = false;
    bGameplayActive = bEmergencyAssisted = false; ActiveGameplaySeconds = 0; AwardedStars = 0;
    EntryState = UsesOutdoorEntry() ? EMissionEntryState::AwaitingEntry : EMissionEntryState::Legacy;
    bHasEnteredHouse = bHaveHeadSample = bCrossedEntrance = false; DoorFeedbackUntil = 0;
    CaptureTarget = FMath::Max(1, CaptureTarget);
    for (ALizardGameplayActor* Lizard : MissionLizards)
        if (IsValid(Lizard)) { Lizard->SetChallengeActive(false); Lizard->OnStoredInContainer.AddUniqueDynamic(this, &AMissionDirector::Stored); }
    ObjectivePanel->InitWidget(); CompletionPanel->InitWidget();
    ObjectiveWidget = Cast<UMissionObjectiveWidget>(ObjectivePanel->GetUserWidgetObject());
    ObjectiveHome = ObjectivePanel->GetComponentTransform();
    CompletionWidget = Cast<UMissionCompleteWidget>(CompletionPanel->GetUserWidgetObject());
    if (ExitDoor) { ExitDoor->Mission = this; ExitDoor->SetReady(false); }
    if (!ExitDoor || !ObjectiveWidget || !CompletionWidget || MissionLizards.Num() < CaptureTarget)
        UE_LOG(LogTemp, Error, TEXT("Mission configuration incomplete: %s"), *GetName());
    InitializationTimer = GetWorldTimerManager().SetTimerForNextTick(this, &AMissionDirector::InitializeMission);
}
void AMissionDirector::InitializeMission()
{
    PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (PlayerPawn.IsValid()) Head = PlayerPawn->FindComponentByClass<UCameraComponent>();
    if (CollectionManager)
        if (auto* LegacyPanel = Cast<UWidgetComponent>(LizardBlueprintBridge::Object(CollectionManager, TEXT("VictoryPanel"))))
        { LegacyPanel->SetVisibility(false); LegacyPanel->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
    if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
        if (PC->PlayerCameraManager) PC->PlayerCameraManager->StartCameraFade(1, 0, ViewFadeSeconds, FLinearColor::Black, false, false);
    if (CaptureObjective.IsEmpty()) CaptureObjective = FText::Format(NSLOCTEXT("CTLMission", "Catch", "Catch {0} {1}"),
        FText::AsNumber(CaptureTarget), CaptureTarget == 1 ? NSLOCTEXT("CTLMission", "One", "lizard") : NSLOCTEXT("CTLMission", "Many", "lizards"));
    UpdateObjective();
    if (!PlayerPawn.IsValid() || !Head.IsValid()) {
        GetWorldTimerManager().SetTimer(InitializationTimer, this, &AMissionDirector::InitializeMission, .1f, false); return;
    }
    if (!bTutorial && !Introduction.IsEmpty()) {
        DialoguePanel->InitWidget(); DialogueWidget = Cast<UTutorialDialogueWidget>(DialoguePanel->GetUserWidgetObject());
        if (!DialogueWidget) { UE_LOG(LogTemp, Error, TEXT("Mission introduction widget missing")); return; }
        DialogueWidget->InitializeMission(this);
        DialogueWidget->Present(Introduction, NSLOCTEXT("CTLMission", "Begin", "Begin challenge"), true);
        DialoguePanel->SetVisibility(true); DialoguePanel->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        ObjectivePanel->SetVisibility(false);
        Interaction->Initialize(PlayerPawn.Get(), DialogueWidget, LeftTriggerAction, RightTriggerAction, RayMaterial);
    } else if (!bTutorial) AcknowledgeIntroduction();
}
void AMissionDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // World delta excludes pause and loading; this single clock owns emergency availability.
    if (bGameplayActive && !bComplete) ActiveGameplaySeconds += DeltaSeconds;
    UpdateEntrySequence();
    if (DoorFeedbackUntil > 0 && GetWorld()->GetTimeSeconds() >= DoorFeedbackUntil)
    {
        DoorFeedbackUntil = 0; UpdateObjective();
    }
}
bool AMissionDirector::GetPlayerHeadLocation(FVector& Position) const
{
    if (!Head.IsValid()) return false;
    Position = Head->GetComponentLocation(); return true;
}
bool AMissionDirector::IsExitReady() const
{
    return CapturedCount >= CaptureTarget && (!UsesOutdoorEntry() || EntryState == EMissionEntryState::ExitUnlocked);
}
void AMissionDirector::UpdateEntrySequence()
{
    FVector Position;
    if (!UsesOutdoorEntry() || bComplete || !ExitDoor || !GetPlayerHeadLocation(Position)) return;
    const bool OnOutdoorSide = ExitDoor->EntryThreshold->GetComponentTransform().InverseTransformPosition(Position).X > 0;
    if (EntryState == EMissionEntryState::AwaitingEntry)
    {
        if (bHaveHeadSample && ExitDoor->IsPassable() && !ExitDoor->IsLocked() && ExitDoor->CrossedIntoHouse(PreviousHeadPosition, Position)) bCrossedEntrance = true;
        if (OnOutdoorSide) bCrossedEntrance = false;
        if (bGameplayActive && bCrossedEntrance && ExitDoor->ContainsSafeInterior(Position))
        {
            bHasEnteredHouse = true; EntryState = EMissionEntryState::ClosingAfterEntry;
            ExitDoor->RequestEntryLock(); UpdateObjective();
        }
    }
    else if (EntryState == EMissionEntryState::ClosingAfterEntry)
    {
        if (OnOutdoorSide)
        {
            ExitDoor->CancelEntryLock(); EntryState = EMissionEntryState::AwaitingEntry;
            bHasEnteredHouse = bCrossedEntrance = false; UpdateObjective();
        }
        else if (ExitDoor->AccessState == EExitDoorAccess::Locked)
        {
            EntryState = EMissionEntryState::InsideLocked; UpdateObjective();
        }
    }
    if (EntryState == EMissionEntryState::InsideLocked && CapturedCount >= CaptureTarget)
    {
        EntryState = EMissionEntryState::ExitUnlocked; ExitDoor->UnlockForExit(); UpdateObjective();
    }
    PreviousHeadPosition = Position; bHaveHeadSample = true;
}
void AMissionDirector::ReportLockedDoor(const FText& Message)
{
    if (!UsesOutdoorEntry() || bComplete) return;
    DoorFeedback = Message; DoorFeedbackUntil = GetWorld()->GetTimeSeconds() + LockedFeedbackSeconds; UpdateObjective();
}
void AMissionDirector::AcknowledgeIntroduction()
{
    if (bGameplayActive || bComplete) return;
    bGameplayActive = true;
    DialoguePanel->SetVisibility(false); DialoguePanel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ObjectivePanel->SetVisibility(true);
    for (ALizardGameplayActor* Lizard : MissionLizards) if (IsValid(Lizard)) Lizard->SetChallengeActive(true);
    Interaction->LockForSeconds(.5f);
    UpdateObjective();
}
void AMissionDirector::RecordEmergencyDispensed(ASprayGameplayActor* Spray)
{
    if (!bEmergencyRating || !bGameplayActive || bComplete || !IsValid(Spray)) return;
    bEmergencyAssisted = true; MissionTools.AddUnique(Spray);
}
void AMissionDirector::Stored(ALizardGameplayActor* Lizard, AActor* Container)
{
    if (!bGameplayActive || bComplete || !IsValid(Lizard) || !Lizard->bStored || (Lizard->bTrainingDoll && !bTutorial) || !MissionLizards.Contains(Lizard)
        || !Containers.Contains(Container) || Captured.Contains(Lizard)) return;
    Captured.Add(Lizard); CapturedCount = Captured.Num();
    UpdateObjective();
}
bool AMissionDirector::IsPlayerOutside() const
{
    // Current camera position includes room-scale tracking AND the pawn's teleport offset.
    return PlayerPawn.IsValid() && Head.IsValid() && ExitDoor && ExitDoor->ContainsHeadPosition(Head->GetComponentLocation());
}
void AMissionDirector::UpdateObjective()
{
    const bool Ready = IsExitReady();
    const bool EarlyExit = !Ready && IsPlayerOutside() && (!UsesOutdoorEntry() || bHasEnteredHouse);
    const bool LockHint = DoorFeedbackUntil > GetWorld()->GetTimeSeconds();
    if ((EarlyExit || LockHint) && !bWarningPlaced && Head.IsValid())
    {
        const FVector Eye = Head->GetComponentLocation();
        const FVector Position = Eye + Head->GetForwardVector() * 150.f;
        ObjectivePanel->SetWorldLocationAndRotation(Position, (Eye - Position).Rotation());
        bWarningPlaced = true;
    }
    else if (!EarlyExit && !LockHint && bWarningPlaced) { ObjectivePanel->SetWorldTransform(ObjectiveHome); bWarningPlaced = false; }
    if (UsesOutdoorEntry() && !bWarningPlaced)
        ObjectivePanel->SetWorldTransform(EntryState == EMissionEntryState::AwaitingEntry ? DialoguePanel->GetComponentTransform() : ObjectiveHome);
    if (bTutorial) ObjectivePanel->SetVisibility((Ready || EarlyExit) && !bComplete);
    if (ExitDoor) ExitDoor->SetReady(Ready && !bComplete);
    const FText& ReadyObjective = ExitDoor && ExitDoor->IsPassable() ? HeadOutsideObjective : ExitObjective;
    const FText* CurrentObjective = Ready ? &ReadyObjective : &CaptureObjective;
    if (UsesOutdoorEntry() && EntryState == EMissionEntryState::AwaitingEntry) CurrentObjective = &EntryObjective;
    else if (UsesOutdoorEntry() && EntryState == EMissionEntryState::ClosingAfterEntry) CurrentObjective = &EntryClosingObjective;
    if (ObjectiveWidget) ObjectiveWidget->Present(*CurrentObjective, CapturedCount, CaptureTarget,
        LockHint ? DoorFeedback : (EarlyExit ? EarlyExitMessage : FText::GetEmpty()));
}
void AMissionDirector::EvaluateExit()
{
    if (bComplete) return;
    UpdateObjective();
    const bool Outside = IsPlayerOutside();
    const bool EnteredOutside = Outside && !bExitPreviouslyOutside;
    bExitPreviouslyOutside = Outside;
    if (bGameplayActive && ExitDoor && ExitDoor->IsPassable() && EnteredOutside && IsExitReady()
        && (!UsesOutdoorEntry() || bHasEnteredHouse)) CompleteMission();
}
void AMissionDirector::CompleteMission()
{
    if (bComplete || !Head.IsValid() || !PlayerPawn.IsValid() || !CompletionWidget) return;
    bComplete = true; bGameplayActive = false;
    if (UsesOutdoorEntry()) EntryState = EMissionEntryState::Complete;
    if (ExitDoor) ExitDoor->SetReady(false);
    AwardedStars = bEmergencyRating ? (bEmergencyAssisted ? 1 : 3) : 0;
    DialoguePanel->SetVisibility(false); DialoguePanel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto* PC = Cast<APlayerController>(PlayerPawn->GetController());
    SetBool(PlayerPawn.Get(), TEXT("bCollectionInputLocked"), true);
    PlayerPawn->DisableInput(PC); // Input only: XR camera/head tracking remains live.
    LizardBlueprintBridge::FCall EndTeleport(PlayerPawn.Get(), TEXT("EndTeleportTrace")); EndTeleport.Invoke();
    if (CollectionManager)
    {
        SetBool(CollectionManager, TEXT("bVictory"), true);
        CollectionManager->DisableInput(PC); CollectionManager->SetActorTickEnabled(false);
    }
    for (AActor* Tool : MissionTools) if (IsValid(Tool))
    {
        if (auto* Spray = Cast<ASprayGameplayActor>(Tool)) Spray->UpdateEmission(0, false);
        // Held jar/broom collision must not intercept either controller's UI ray.
        if (auto* Body = Cast<UPrimitiveComponent>(Tool->GetRootComponent())) Body->SetSimulatePhysics(false);
        Tool->SetActorEnableCollision(false);
        Tool->DisableInput(PC); Tool->SetActorTickEnabled(false);
    }
    for (ALizardGameplayActor* Lizard : MissionLizards) if (IsValid(Lizard)) { Lizard->DisableInput(PC); Lizard->SetActorTickEnabled(false); GetWorldTimerManager().ClearAllTimersForObject(Lizard); }
    ObjectivePanel->SetVisibility(false);
    const FVector Eye = Head->GetComponentLocation();
    const FVector Position = Eye + Head->GetForwardVector() * CompletionPanelDistance;
    CompletionPanel->SetWorldLocationAndRotation(Position, (Eye - Position).Rotation());
    CompletionPanel->SetVisibility(true); CompletionPanel->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CompletionWidget->Present(this, CapturedCount, CaptureTarget, !NextMission.IsNull());
    Interaction->Initialize(PlayerPawn.Get(), CompletionWidget, LeftTriggerAction, RightTriggerAction, RayMaterial);
    Interaction->LockForSeconds(.6f);
    if (auto* Audio = UCTLSoundEffectsSubsystem::Get(this)) Audio->PlayEffect(this, ECTLSoundEffect::MissionComplete);
}
void AMissionDirector::GoToNextMission() { if (!NextMission.IsNull()) Travel(NextMission); }
void AMissionDirector::ReturnToMenu() { Travel(MainMenu); }
void AMissionDirector::Travel(const TSoftObjectPtr<UWorld>& Map)
{
    if (!bComplete || bTraveling || Map.IsNull() || !FPackageName::DoesPackageExist(Map.ToSoftObjectPath().GetLongPackageName())) return;
    bTraveling = true; PendingMap = Map; CompletionWidget->SetBusy(); Interaction->LockForSeconds(1000.f);
    if (auto* Audio = UCTLSoundEffectsSubsystem::Get(this)) Audio->PlayEffect(this, ECTLSoundEffect::UIConfirm);
    if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
        if (PC->PlayerCameraManager) PC->PlayerCameraManager->StartCameraFade(0, 1, ViewFadeSeconds, FLinearColor::Black, false, true);
    auto* GI = GetGameInstance<UCTLGameInstance>();
    const float MusicFade = GI ? GI->PrepareMusicTravel(Map) : 0.f;
    GetWorldTimerManager().SetTimer(TravelTimer, this, &AMissionDirector::FinishTravel, FMath::Max(.05f, FMath::Max(ViewFadeSeconds, MusicFade) + .03f), false);
}
void AMissionDirector::FinishTravel()
{
    if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->ResetIgnoreMoveInput(); PC->ResetIgnoreLookInput(); PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor = false;
        if (PC->PlayerCameraManager) PC->PlayerCameraManager->StopCameraFade();
    }
    UGameplayStatics::OpenLevelBySoftObjectPtr(this, PendingMap, true);
}
void AMissionDirector::EndPlay(const EEndPlayReason::Type Reason)
{
    if (IsValid(ExitDoor)) ExitDoor->SetReady(false);
    for (ALizardGameplayActor* Lizard : MissionLizards) if (IsValid(Lizard)) Lizard->OnStoredInContainer.RemoveDynamic(this, &AMissionDirector::Stored);
    GetWorldTimerManager().ClearTimer(InitializationTimer); GetWorldTimerManager().ClearTimer(TravelTimer);
    Super::EndPlay(Reason);
}
bool AMissionDirector::BuildMissionGeometryAndNavigation()
{
#if WITH_EDITOR
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World) return false;
    if (!FEditorBuildUtils::EditorBuild(World, FBuildOptions::BuildGeometry, false)) return false;
    FlushAsyncLoading();
    auto* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!Navigation) return false;
    // Scripted map/soft-world loading may leave this transient lock behind after loading finishes.
    // Clear only that load lock, preserving the user's other navigation build preferences.
    Navigation->RemoveNavigationBuildLock(ENavigationBuildLock::AsyncLoadLock, UNavigationSystemV1::ELockRemovalRebuildAction::NoRebuild);
    Navigation->Build();
    for (TActorIterator<ANavigationData> It(World); It; ++It)
        if (FNavDataGenerator* Generator = It->GetGenerator()) Generator->EnsureBuildCompletion();
    return true;
#else
    return false;
#endif
}

bool AMissionDirector::CanTeleportTo(APawn* Pawn, FVector Destination)
{
    if (!Pawn) return false;
    const auto* Camera = Pawn->FindComponentByClass<UCameraComponent>();
    const FVector Start = Camera ? Camera->GetComponentLocation() : Pawn->GetActorLocation();
    for (TActorIterator<AMissionExitDoor> It(Pawn->GetWorld()); It; ++It)
    {
        if (It->IsPassable() && !It->IsLocked()) continue;
        const FTransform Transform = It->GetActorTransform();
        const FVector A = Transform.InverseTransformPosition(Start), B = Transform.InverseTransformPosition(Destination);
        if ((A.X < 0) == (B.X < 0) || FMath::IsNearlyEqual(A.X, B.X)) continue;
        const float Alpha = -A.X / (B.X - A.X);
        if (FMath::Abs(FMath::Lerp(A.Y, B.Y, Alpha)) <= 110.f) return false;
    }
    return true;
}


