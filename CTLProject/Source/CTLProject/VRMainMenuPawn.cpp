#include "VRMainMenuPawn.h"
#include "VRMainMenuWidget.h"
#include "CTLLevelList.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

AVRMainMenuPawn::AVRMainMenuPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    bBlockInput = true;
    // Widget interactions tick first so input and ray feedback use this frame's hit result.
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
    SetRootComponent(VROrigin);
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(VROrigin);
    Camera->SetRelativeLocation(FVector(0, 0, 160));
    Camera->bLockToHmd = true;
    LeftAim = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftAim"));
    RightAim = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightAim"));
    LeftAim->SetupAttachment(VROrigin);
    RightAim->SetupAttachment(VROrigin);
    LeftAim->SetTrackingMotionSource(TEXT("LeftAim"));
    RightAim->SetTrackingMotionSource(TEXT("RightAim"));

    MenuPanel = CreateDefaultSubobject<UWidgetComponent>(TEXT("MenuPanel"));
    MenuPanel->SetupAttachment(VROrigin);
    MenuPanel->SetWidgetSpace(EWidgetSpace::World);
    MenuPanel->SetDrawSize(FVector2D(1000, 760));
    MenuPanel->SetRelativeScale3D(FVector(.12f)); // 120 x 91 cm at 1.8 m.
    MenuPanel->SetRelativeLocation(FVector(180, 0, 155));
    MenuPanel->SetRelativeRotation(FRotator(0, 180, 0));
    MenuPanel->SetPivot(FVector2D(.5f, .5f));
    MenuPanel->SetTwoSided(false);
    MenuPanel->SetWindowFocusable(false);
    MenuPanel->SetTickWhenOffscreen(true);
    MenuPanel->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MenuPanel->SetCollisionResponseToAllChannels(ECR_Ignore);
    MenuPanel->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
    MenuPanel->SetGenerateOverlapEvents(false);
    MenuPanel->SetCastShadow(false);

    LeftInteraction = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("LeftInteraction"));
    RightInteraction = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("RightInteraction"));
    LeftInteraction->SetupAttachment(LeftAim);
    RightInteraction->SetupAttachment(RightAim);
    for (int32 Hand = 0; Hand < 2; ++Hand)
    {
        UWidgetInteractionComponent* WI = Interaction(Hand);
        WI->VirtualUserIndex = 0;
        WI->PointerIndex = Hand;
        WI->TraceChannel = ECC_GameTraceChannel1; // Existing project's 3DWidget channel.
        WI->InteractionDistance = RayLengthCm;
        WI->bShowDebug = false;
    }

    LeftRay = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftRay"));
    RightRay = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightRay"));
    LeftPointer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftPointer"));
    RightPointer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightPointer"));
    LeftRay->SetupAttachment(LeftAim);
    RightRay->SetupAttachment(RightAim);
    LeftPointer->SetupAttachment(LeftAim);
    RightPointer->SetupAttachment(RightAim);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere"));
    for (UStaticMeshComponent* Mesh : {LeftRay.Get(), RightRay.Get(), LeftPointer.Get(), RightPointer.Get()})
    {
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCastShadow(false);
        Mesh->SetVisibility(false);
    }
    LeftRay->SetStaticMesh(Cylinder.Object);
    RightRay->SetStaticMesh(Cylinder.Object);
    LeftPointer->SetStaticMesh(Sphere.Object);
    RightPointer->SetStaticMesh(Sphere.Object);
}

void AVRMainMenuPawn::BeginPlay()
{
    Super::BeginPlay();
    bUsingHMD = UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();
    if (bUsingHMD)
    {
        UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::LocalFloor);
        Camera->SetRelativeLocation(FVector::ZeroVector);
    }
    else
    {
        // Desktop preview uses the same Slate pointer route, never a second hardware click route.
        RightInteraction->InteractionSource = EWidgetInteractionSource::Mouse;
    }
    MenuPanel->SetVisibility(false);
    MenuPanel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        MenuPanel->SetOwnerPlayer(PC->GetLocalPlayer());
    }
    MenuPanel->InitWidget();
    MenuWidget = Cast<UVRMainMenuWidget>(MenuPanel->GetUserWidgetObject());
    if (MenuWidget) MenuWidget->InitializeMenu(this);
    else UE_LOG(LogTemp, Error, TEXT("Main menu requires a VRMainMenuWidget subclass on MenuPanel."));
    if (RayMaterial)
    {
        LeftRayMID = UMaterialInstanceDynamic::Create(RayMaterial, this);
        RightRayMID = UMaterialInstanceDynamic::Create(RayMaterial, this);
        LeftRay->SetMaterial(0, LeftRayMID);
        LeftPointer->SetMaterial(0, LeftRayMID);
        RightRay->SetMaterial(0, RightRayMID);
        RightPointer->SetMaterial(0, RightRayMID);
    }
    InstallMenuInput();
}

void AVRMainMenuPawn::PawnClientRestart()
{
    Super::PawnClientRestart();
    if (HasActorBegunPlay()) InstallMenuInput();
}

void AVRMainMenuPawn::InstallMenuInput()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || !PC->IsLocalController()) return;
    if (ULocalPlayer* Player = PC->GetLocalPlayer())
    {
        UEnhancedInputLocalPlayerSubsystem* Subsystem = Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
        if (Subsystem && MenuInputContext && !Subsystem->HasMappingContext(MenuInputContext))
        {
            FModifyContextOptions Options;
            Options.bIgnoreAllPressedKeysUntilRelease = true;
            Subsystem->AddMappingContext(MenuInputContext, 100, Options);
        }
        InputSubsystem = Subsystem;
    }
    PC->bShowMouseCursor = !bUsingHMD;
    if (bUsingHMD)
    {
        FInputModeGameOnly Mode;
        Mode.SetConsumeCaptureMouseDown(false);
        PC->SetInputMode(Mode);
    }
    else
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Mode);
    }
}

void AVRMainMenuPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // Block lower input stack entries; this pawn binds no gameplay actions.
    PlayerInputComponent->bBlockInput = true;
    if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (LeftTriggerAction)
        {
            Input->BindAction(LeftTriggerAction, ETriggerEvent::Triggered, this, &AVRMainMenuPawn::LeftTrigger);
            Input->BindAction(LeftTriggerAction, ETriggerEvent::Completed, this, &AVRMainMenuPawn::LeftReleased);
            Input->BindAction(LeftTriggerAction, ETriggerEvent::Canceled, this, &AVRMainMenuPawn::LeftCanceled);
        }
        if (RightTriggerAction)
        {
            Input->BindAction(RightTriggerAction, ETriggerEvent::Triggered, this, &AVRMainMenuPawn::RightTrigger);
            Input->BindAction(RightTriggerAction, ETriggerEvent::Completed, this, &AVRMainMenuPawn::RightReleased);
            Input->BindAction(RightTriggerAction, ETriggerEvent::Canceled, this, &AVRMainMenuPawn::RightCanceled);
        }
    }
    PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AVRMainMenuPawn::DesktopPressed);
    PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AVRMainMenuPawn::DesktopReleased);
}

void AVRMainMenuPawn::LeftTrigger(const FInputActionValue& Value) { TriggerValues[0] = Value.Get<float>(); }
void AVRMainMenuPawn::RightTrigger(const FInputActionValue& Value) { TriggerValues[1] = Value.Get<float>(); }
void AVRMainMenuPawn::LeftReleased() { TriggerValues[0] = 0.f; }
void AVRMainMenuPawn::RightReleased() { TriggerValues[1] = 0.f; }
void AVRMainMenuPawn::LeftCanceled() { TriggerValues[0] = 0.f; bArmed[0] = false; ReleasePointer(0, true); }
void AVRMainMenuPawn::RightCanceled() { TriggerValues[1] = 0.f; bArmed[1] = false; ReleasePointer(1, true); }
void AVRMainMenuPawn::DesktopPressed() { if (!bUsingHMD) TriggerValues[1] = 1.f; }
void AVRMainMenuPawn::DesktopReleased() { if (!bUsingHMD) TriggerValues[1] = 0.f; }

UWidgetInteractionComponent* AVRMainMenuPawn::Interaction(int32 Hand) const
{
    return Hand == 0 ? LeftInteraction.Get() : RightInteraction.Get();
}

bool AVRMainMenuPawn::HandAvailable(int32 Hand) const
{
    if (!bUsingHMD) return Hand == 1;
    return (Hand == 0 ? LeftAim : RightAim)->IsTracked();
}

void AVRMainMenuPawn::PlaceMenu()
{
    const FRotator Facing(0, Camera->GetComponentRotation().Yaw, 0);
    const FVector Location = Camera->GetComponentLocation() + Facing.Vector() * MenuDistanceCm
        + FVector(0, 0, MenuHeightOffsetCm);
    // Detach once. Head rotation, leaning and subsequent tracking changes cannot move the panel.
    MenuPanel->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    MenuPanel->SetWorldLocationAndRotation(Location, FRotator(0, Facing.Yaw + 180.f, 0));
    MenuPanel->SetVisibility(true);
    MenuPanel->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    bMenuPlaced = true;
}

void AVRMainMenuPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bMenuPlaced)
    {
        const bool bTrackingReady = !bUsingHMD || UHeadMountedDisplayFunctionLibrary::HasValidTrackingPosition();
        StableTrackingTime = bTrackingReady ? StableTrackingTime + DeltaSeconds : 0.f;
        if (StableTrackingTime >= TrackingSettleSeconds) PlaceMenu();
    }
    for (int32 Hand = 0; Hand < 2; ++Hand)
    {
        UpdateHand(Hand);
        UpdateRay(Hand);
    }
}

void AVRMainMenuPawn::UpdateHand(int32 Hand)
{
    UWidgetInteractionComponent* WI = Interaction(Hand);
    const bool bAvailable = bMenuPlaced && !bTransitionInProgress && HandAvailable(Hand);
    WI->bEnableHitTesting = bAvailable;
    if (!bAvailable)
    {
        ReleasePointer(Hand, true);
        bArmed[Hand] = false;
        return;
    }
    if (TriggerValues[Hand] <= TriggerReleaseThreshold)
    {
        // Clear our state before dispatch: OnClicked may immediately start travel/quit.
        ReleasePointer(Hand, false);
        bArmed[Hand] = true;
    }
    else if (TriggerValues[Hand] >= TriggerPressThreshold && bArmed[Hand])
    {
        bArmed[Hand] = false;
        if (ActivePointer == INDEX_NONE && WI->IsOverInteractableWidget())
        {
            ActivePointer = Hand;
            bPointerPressed[Hand] = true;
            WI->PressPointerKey(EKeys::LeftMouseButton);
        }
    }
}

void AVRMainMenuPawn::ReleasePointer(int32 Hand, bool bCancel)
{
    if (!bPointerPressed[Hand]) return;
    bPointerPressed[Hand] = false;
    if (ActivePointer == Hand) ActivePointer = INDEX_NONE;
    // Losing tracking or ending play must cancel a press rather than activate a hovered button.
    if (bCancel && MenuWidget) MenuWidget->SetIsEnabled(false);
    Interaction(Hand)->ReleasePointerKey(EKeys::LeftMouseButton);
    if (bCancel && MenuWidget && !bTransitionInProgress) MenuWidget->SetIsEnabled(true);
}

void AVRMainMenuPawn::UpdateRay(int32 Hand)
{
    UStaticMeshComponent* Ray = Hand == 0 ? LeftRay.Get() : RightRay.Get();
    UStaticMeshComponent* Pointer = Hand == 0 ? LeftPointer.Get() : RightPointer.Get();
    UWidgetInteractionComponent* WI = Interaction(Hand);
    const bool bVisible = bUsingHMD && bMenuPlaced && !bTransitionInProgress && HandAvailable(Hand);
    Ray->SetVisibility(bVisible);
    const FHitResult& Hit = WI->GetLastHitResult();
    Pointer->SetVisibility(bVisible && Hit.bBlockingHit);
    if (!bVisible) return;
    WI->InteractionDistance = RayLengthCm;
    const float Length = Hit.bBlockingHit ? FMath::Min(RayLengthCm, Hit.Distance) : RayLengthCm;
    Ray->SetRelativeLocation(FVector(Length * .5f, 0, 0));
    Ray->SetRelativeRotation(FRotator(90, 0, 0));
    Ray->SetRelativeScale3D(FVector(RayDiameterCm / 100.f, RayDiameterCm / 100.f, Length / 100.f));
    Pointer->SetWorldLocation(Hit.ImpactPoint + Hit.ImpactNormal * .2f);
    Pointer->SetRelativeScale3D(FVector(.009f));
    UMaterialInstanceDynamic* MID = Hand == 0 ? LeftRayMID.Get() : RightRayMID.Get();
    if (MID) MID->SetVectorParameterValue(TEXT("RayColor"), bPointerPressed[Hand] ? PressedRayColor
        : WI->IsOverInteractableWidget() ? HoverRayColor : RayColor);
}

bool AVRMainMenuPawn::BeginTransition(const FText& Message)
{
    if (!bMenuPlaced || bTransitionInProgress) return false;
    bTransitionInProgress = true;
    if (MenuWidget) MenuWidget->SetStatus(Message, true);
    ReleasePointer(0, true);
    ReleasePointer(1, true);
    return true;
}

void AVRMainMenuPawn::StartGame()
{
    TravelTo(LevelList ? LevelList->SelectionMap : GameplayMap);
}

void AVRMainMenuPawn::SelectLevel(FName Id)
{
    if (LevelList)
        if (const FCTLLevelEntry* Entry = LevelList->Find(Id)) { TravelTo(Entry->Map); return; }
    if (MenuWidget) MenuWidget->SetStatus(NSLOCTEXT("CTLMenu", "MissingEntry", "Level entry is missing from the level list."), false);
}

void AVRMainMenuPawn::ReturnToMainMenu()
{
    if (LevelList) TravelTo(LevelList->MainMenuMap);
}

void AVRMainMenuPawn::TravelTo(const TSoftObjectPtr<UWorld>& Destination)
{
    if (bTransitionInProgress) return;
    const FString Package = Destination.ToSoftObjectPath().GetLongPackageName();
    if (Destination.IsNull() || !FPackageName::DoesPackageExist(Package))
    {
        if (MenuWidget) MenuWidget->SetStatus(NSLOCTEXT("CTLMenu", "MissingMap", "Map unavailable. Check DA_LevelList map references."), false);
        UE_LOG(LogTemp, Error, TEXT("Main menu destination is missing: %s"), *Package);
        return;
    }
    if (BeginTransition(NSLOCTEXT("CTLMenu", "Loading", "Loading...")))
    {
        PendingMap = Destination;
        // Let Slate finish routing the click before loading another world.
        TransitionTimer = GetWorldTimerManager().SetTimerForNextTick(this, &AVRMainMenuPawn::FinishStart);
    }
}

void AVRMainMenuPawn::FinishStart()
{
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }
    UGameplayStatics::OpenLevelBySoftObjectPtr(this, PendingMap, true);
}

void AVRMainMenuPawn::ExitGame()
{
    if (BeginTransition(NSLOCTEXT("CTLMenu", "Quitting", "Exiting...")))
        TransitionTimer = GetWorldTimerManager().SetTimerForNextTick(this, &AVRMainMenuPawn::FinishExit);
}

void AVRMainMenuPawn::FinishExit()
{
#if WITH_EDITOR
    if (GetWorld()->WorldType == EWorldType::PIE)
    {
        if (GEditor) GEditor->RequestEndPlayMap();
        return;
    }
#endif
    UKismetSystemLibrary::QuitGame(this, Cast<APlayerController>(GetController()), EQuitPreference::Quit, false);
}

void AVRMainMenuPawn::EndPlay(const EEndPlayReason::Type Reason)
{
    bTransitionInProgress = true;
    GetWorldTimerManager().ClearTimer(TransitionTimer);
    ReleasePointer(0, true);
    ReleasePointer(1, true);
    if (InputSubsystem.IsValid() && MenuInputContext) InputSubsystem->RemoveMappingContext(MenuInputContext);
    // A detached component is still owned by this pawn and is destroyed with it on travel.
    Super::EndPlay(Reason);
}
