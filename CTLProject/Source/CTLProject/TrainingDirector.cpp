#include "TrainingDirector.h"
#include "MissionFlow.h"
#include "CTLBackgroundMusic.h"
#include "CTLSoundEffects.h"
#include "LizardGameplayActor.h"
#include "CTLLevelList.h"
#include "TutorialWidgets.h"
#include "VRPanelInteractor.h"
#include "LizardBlueprintBridge.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"
#include "Engine/World.h"

ATrainingDirector::ATrainingDirector()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")));
    DialoguePanel = CreateDefaultSubobject<UWidgetComponent>(TEXT("DialoguePanel"));
    ObjectivePanel = CreateDefaultSubobject<UWidgetComponent>(TEXT("ObjectivePanel"));
    for (auto* Panel : {DialoguePanel.Get(), ObjectivePanel.Get()})
    {
        Panel->SetupAttachment(RootComponent);
        Panel->SetWidgetSpace(EWidgetSpace::World);
        Panel->SetTwoSided(false);
        Panel->SetWindowFocusable(false);
        Panel->SetCastShadow(false);
        Panel->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Panel->SetCollisionResponseToAllChannels(ECR_Ignore);
        Panel->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
        Panel->SetGenerateOverlapEvents(false);
        Panel->SetTickWhenOffscreen(true);
    }
    DialoguePanel->SetDrawSize(FVector2D(1000, 660));
    DialoguePanel->SetRelativeScale3D(FVector(.125f));
    DialoguePanel->SetRelativeLocation(FVector(210, -115, 170));
    DialoguePanel->SetRelativeRotation(FRotator(0, 151, 0));
    ObjectivePanel->SetDrawSize(FVector2D(850, 360));
    ObjectivePanel->SetRelativeScale3D(FVector(.12f));
    ObjectivePanel->SetRelativeLocation(FVector(195, 115, 175));
    ObjectivePanel->SetRelativeRotation(FRotator(0, -149, 0));
    ObjectivePanel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PanelInteraction = CreateDefaultSubobject<UVRPanelInteractor>(TEXT("PanelInteraction"));
    WelcomeDialogue = NSLOCTEXT("CTLTraining", "Welcome", "Welcome aboard, our newest lizard-catching recruit! Before your first assignment, let's practice with something that won't run away.");
    PracticeDialogue = NSLOCTEXT("CTLTraining", "Practice", "This is our training lizard. Pick it up and place it inside the container on the table.");
    SuccessDialogue = NSLOCTEXT("CTLTraining", "Success", "Nice work! That's your first capture. The real ones may be a little less cooperative.");
    BroomDialogue = FText::GetEmpty();
    FinishDialogue = NSLOCTEXT("CTLTraining", "ManualExit", "Training complete! Open the door and leave to begin your first assignment.");
    SuccessDialogue = FinishDialogue;
    CaptureObjective = NSLOCTEXT("CTLTraining", "Objective", "Pick up the training lizard and place it inside the container.");
    GripInstructions = NSLOCTEXT("CTLTraining", "Grip", "Touch the doll with either hand and hold that controller's GRIP button. While still holding grip, move the doll into the open container. Release grip to drop and retry. Use the INDEX TRIGGER for Continue.");
    ContinueLabel = NSLOCTEXT("CTLTraining", "Continue", "Continue");
    BeginLevelLabel = NSLOCTEXT("CTLTraining", "Begin", "Begin Level 1");
    InProgressLabel = NSLOCTEXT("CTLTraining", "InProgress", "CAPTURE PRACTICE");
    CompleteLabel = NSLOCTEXT("CTLTraining", "Complete", "COMPLETE - First capture recorded");
}

void ATrainingDirector::BeginPlay()
{
    Super::BeginPlay();
    DialoguePanel->InitWidget(); ObjectivePanel->InitWidget();
    Dialogue = Cast<UTutorialDialogueWidget>(DialoguePanel->GetUserWidgetObject());
    Objective = Cast<UTutorialObjectiveWidget>(ObjectivePanel->GetUserWidgetObject());
    Mouth = Cast<USphereComponent>(LizardBlueprintBridge::Object(CaptureContainer, TEXT("DepositVolume")));
    if (!TrainingDoll || !TrainingDoll->bTrainingDoll || !CaptureContainer || !Mouth || !Dialogue || !Objective || !LevelList)
    {
        UE_LOG(LogTemp, Error, TEXT("TrainingDirector requires its configured doll, container/DepositVolume, exit mission, UMG widgets and level list."));
        if (Dialogue) Dialogue->Present(NSLOCTEXT("CTLTraining", "SetupError", "Training setup is incomplete. Check the director's references."), FText::GetEmpty(), false);
        return;
    }
    TrainingDoll->bTrainingPickupEnabled = false;
    TrainingDoll->TrainingContainer = CaptureContainer;
    TableDollTransform = TrainingDoll->GetActorTransform();
    TrainingDoll->OnStoredInContainer.AddUniqueDynamic(this, &ATrainingDirector::DollStored);
    TrainingDoll->OnCaughtByHand.AddUniqueDynamic(this, &ATrainingDirector::DollCaught);
    Mouth->OnComponentBeginOverlap.AddUniqueDynamic(this, &ATrainingDirector::MouthEntered);
    Dialogue->InitializeDirector(this);
    InitializationTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ATrainingDirector::InitializeTutorial);
    GetWorldTimerManager().SetTimer(RecoveryTimer, this, &ATrainingDirector::CheckRecovery, .5f, true);
}

void ATrainingDirector::InitializeTutorial()
{
    APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Pawn) { UE_LOG(LogTemp, Error, TEXT("Tutorial has no gameplay VR pawn.")); return; }
    if (!UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
        if (auto* Camera = Pawn->FindComponentByClass<UCameraComponent>()) Camera->SetRelativeLocation(FVector(0, 0, 160));
    PanelInteraction->Initialize(Pawn, Dialogue, LeftTriggerAction, RightTriggerAction, RayMaterial);
    EnterState(ETrainingState::Welcome);
}

void ATrainingDirector::EnterState(ETrainingState NewState)
{
    State = NewState;
    NextAdvanceTime = GetWorld()->GetTimeSeconds() + .5f;
    PanelInteraction->LockForSeconds(.5f);
    const bool Capture = State == ETrainingState::Capture;
    if (DollHighlight) DollHighlight->SetActorHiddenInGame(!Capture);
    if (ContainerHighlight) ContainerHighlight->SetActorHiddenInGame(!Capture);
    if (BroomHighlight) BroomHighlight->SetActorHiddenInGame(true);
    TrainingDoll->bTrainingPickupEnabled = Capture;
    const bool Complete = State >= ETrainingState::Success;
    ObjectivePanel->SetVisibility(State >= ETrainingState::Capture);
    Objective->Present(CaptureObjective, Complete ? FText::GetEmpty() : GripInstructions, Complete ? CompleteLabel : InProgressLabel);
    const FText* Line = &WelcomeDialogue;
    switch (State)
    {
        case ETrainingState::Instructions: case ETrainingState::Capture: Line = &PracticeDialogue; break;
        case ETrainingState::Success: Line = &SuccessDialogue; break;
        case ETrainingState::BroomExplanation: Line = &BroomDialogue; break;
        case ETrainingState::Finish: Line = &FinishDialogue; break;
        default: break;
    }
    Dialogue->Present(*Line, State == ETrainingState::Finish ? BeginLevelLabel : ContinueLabel,
        State == ETrainingState::Welcome || State == ETrainingState::Instructions);
    if (Capture) { if (ExitMission) ExitMission->AcknowledgeIntroduction(); StartDepositCheck(); }
}

void ATrainingDirector::AdvanceDialogue()
{
    if (!Dialogue || GetWorld()->GetTimeSeconds() < NextAdvanceTime) return;
    switch (State)
    {
        case ETrainingState::Welcome: EnterState(ETrainingState::Instructions); break;
        case ETrainingState::Instructions: EnterState(ETrainingState::Capture); break;

        default: break;
    }
}

void ATrainingDirector::MouthEntered(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Hit)
{
    if (Other == TrainingDoll) StartDepositCheck();
}
void ATrainingDirector::DollCaught(ALizardGameplayActor* Lizard) { if (Lizard == TrainingDoll) StartDepositCheck(); }
void ATrainingDirector::StartDepositCheck()
{
    if (State != ETrainingState::Capture || !Mouth || !Mouth->IsOverlappingActor(TrainingDoll)) return;
    // Only evaluate the cached candidate while inside the mouth. No actor census/polling loop.
    GetWorldTimerManager().SetTimer(DepositTimer, this, &ATrainingDirector::EvaluateDeposit, .1f, true);
    EvaluateDeposit();
}
void ATrainingDirector::EvaluateDeposit()
{
    if (State != ETrainingState::Capture || !TrainingDoll || TrainingDoll->bStored || !Mouth->IsOverlappingActor(TrainingDoll))
    {
        GetWorldTimerManager().ClearTimer(DepositTimer);
        return;
    }
    bool Accepted = false;
    TrainingDoll->TryStoreInBottle(CaptureContainer, Accepted);
}
void ATrainingDirector::DollStored(ALizardGameplayActor* Lizard, AActor* Container)
{
    if (State != ETrainingState::Capture || Lizard != TrainingDoll || Container != CaptureContainer || !Lizard->bStored) return;
    GetWorldTimerManager().ClearTimer(DepositTimer);
    EnterState(ETrainingState::Success);
    // The exit mission owns counting and travel. Keep just its objective visible after storage.
    DialoguePanel->SetVisibility(false); DialoguePanel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ObjectivePanel->SetVisibility(false);
    PanelInteraction->Suspend();
}
void ATrainingDirector::CheckRecovery()
{
    if (!TrainingDoll || TrainingDoll->Caught || TrainingDoll->bStored) return;
    if (!FBox(RecoveryBoundsMin, RecoveryBoundsMax).IsInsideOrOn(TrainingDoll->GetActorLocation()))
        TrainingDoll->ResetTrainingDoll(TableDollTransform);
}
void ATrainingDirector::BeginAssignment()
{
    const FCTLLevelEntry* Entry = LevelList ? LevelList->Find(NextLevelId) : nullptr;
    if (!Entry || Entry->Map.IsNull() || !FPackageName::DoesPackageExist(Entry->Map.ToSoftObjectPath().GetLongPackageName()))
    {
        Dialogue->Present(NSLOCTEXT("CTLTraining", "MissingLevel", "Level 1 is unavailable. Check the level list's map reference."), BeginLevelLabel, true);
        return;
    }
    PendingMap = Entry->Map;
    EnterState(ETrainingState::Traveling);
    Dialogue->Present(FinishDialogue, BeginLevelLabel, false);
    if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
        if (PC->PlayerCameraManager) PC->PlayerCameraManager->StartCameraFade(0, 1, .35f, FLinearColor::Black, true, true);
    const float MusicFade = GetGameInstance<UCTLGameInstance>() ? GetGameInstance<UCTLGameInstance>()->PrepareMusicTravel(PendingMap) : 0.f;
    GetWorldTimerManager().SetTimer(TravelTimer, this, &ATrainingDirector::LoadAssignment, FMath::Max(.4f, MusicFade + .02f), false);
}
void ATrainingDirector::LoadAssignment()
{
    if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
        // Prevent the outgoing held fade persisting on a reused player camera manager.
        if (PC->PlayerCameraManager) PC->PlayerCameraManager->StopCameraFade();
    }
    UGameplayStatics::OpenLevelBySoftObjectPtr(this, PendingMap, true);
}
void ATrainingDirector::EndPlay(const EEndPlayReason::Type Reason)
{
    GetWorldTimerManager().ClearTimer(DepositTimer);
    GetWorldTimerManager().ClearTimer(RecoveryTimer);
    GetWorldTimerManager().ClearTimer(TravelTimer);
    GetWorldTimerManager().ClearTimer(InitializationTimer);
    if (TrainingDoll)
    {
        TrainingDoll->OnStoredInContainer.RemoveDynamic(this, &ATrainingDirector::DollStored);
        TrainingDoll->OnCaughtByHand.RemoveDynamic(this, &ATrainingDirector::DollCaught);
    }
    if (Mouth) Mouth->OnComponentBeginOverlap.RemoveDynamic(this, &ATrainingDirector::MouthEntered);
    Super::EndPlay(Reason);
}
