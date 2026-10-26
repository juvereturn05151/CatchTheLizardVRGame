#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "../MissionFlow.h"
#include "../LizardGameplayActor.h"
#include "../LizardBlueprintBridge.h"
#include "../ToolBlueprintBridge.h"
#include "../TrainingDirector.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraParameterCollection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCTLVRInteractionRulesTest, "CTL.Gameplay.VRInteractionRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FCheckCTLVRInteractionRules, FAutomationTestBase*, Test);
bool FCheckCTLVRInteractionRules::Update()
{
    UWorld* World = GEditor->PlayWorld;
    if (!World || !World->GetFirstPlayerController()) { Test->AddError(TEXT("PIE player world missing")); return true; }
    APawn* Pawn = World->GetFirstPlayerController()->GetPawn();
    AMissionExitDoor* Door = nullptr;
    ATrainingDirector* Training = nullptr;
    for (TActorIterator<AMissionExitDoor> It(World); It; ++It) { Door = *It; break; }
    for (TActorIterator<ATrainingDirector> It(World); It; ++It) { Training = *It; break; }
    if (!Pawn || !Door || !Training) { Test->AddError(TEXT("Tutorial test fixture incomplete")); return true; }
    auto* Camera = Pawn->FindComponentByClass<UCameraComponent>();
    Test->TestEqual(TEXT("Floor-relative Pawn spawn Z"), Pawn->GetActorLocation().Z, 0.0);
    Test->TestTrue(TEXT("Pawn root is VROrigin, not a centered capsule"), Pawn->GetRootComponent()->GetName() == TEXT("VROrigin"));
    const FTransform Frame = Door->GetActorTransform();
    const auto Point = [&](double X, double Y) { return Frame.TransformPosition(FVector(X, Y, 0)); };
    Camera->SetRelativeLocation(FVector(0, 0, 160)); // Test headset height only; never saved to assets.
    Pawn->SetActorLocation(Point(250, 0));
    Door->AccessState = EExitDoorAccess::Unlocked;
    Door->Hinge->SetRelativeRotation(FRotator::ZeroRotator);
    Test->TestFalse(TEXT("Unlocked closed door blocks route inward"), AMissionDirector::CanTeleportTo(Pawn, Point(-250, 0)));
    Test->TestTrue(TEXT("Closed door preserves same-side teleport"), AMissionDirector::CanTeleportTo(Pawn, Point(350, 0)));
    auto* Colors = LoadObject<UNiagaraParameterCollection>(nullptr, TEXT("/Game/XRFramework/VFX/NPC_VRTemplate.NPC_VRTemplate"));
    Test->TestFalse(TEXT("Preview shares the closed-door route check"), AMissionDirector::UpdateTeleportPreview(Pawn, Point(-250, 0), true, Colors));
    auto* ColorInstance = UNiagaraFunctionLibrary::GetNiagaraParameterCollection(Pawn, Colors);
    Test->TestTrue(TEXT("Blocked preview is red"), ColorInstance->GetColorParameter(TEXT("TeleportColor")).R > .9f);
    Test->TestTrue(TEXT("Valid preview can recover"), AMissionDirector::UpdateTeleportPreview(Pawn, Point(350, 0), true, Colors));
    Test->TestTrue(TEXT("Valid preview restores authored color"), ColorInstance->GetColorParameter(TEXT("TeleportColor")).Equals(Colors->GetDefaultInstance()->GetColorParameter(TEXT("TeleportColor"))));
    Pawn->SetActorLocation(Point(-250, 0));
    Test->TestFalse(TEXT("Closed door blocks route outward"), AMissionDirector::CanTeleportTo(Pawn, Point(250, 0)));
    Door->AccessState = EExitDoorAccess::Locked;
    Test->TestFalse(TEXT("Locked door blocks route"), AMissionDirector::CanTeleportTo(Pawn, Point(250, 0)));
    Door->AccessState = EExitDoorAccess::Unlocked;
    Door->Hinge->SetRelativeRotation(FRotator(0, -40, 0));
    Test->TestFalse(TEXT("Partly open below threshold blocks route"), AMissionDirector::CanTeleportTo(Pawn, Point(250, 0)));
    Door->Hinge->SetRelativeRotation(FRotator(0, -Door->OpenAngle, 0));
    Test->TestTrue(TEXT("Physically clear open doorway allows route"), AMissionDirector::CanTeleportTo(Pawn, Point(250, 0)));
    FVector LeafDestination = Door->DoorMesh->GetComponentLocation(); LeafDestination.Z = Door->GetActorLocation().Z;
    Test->TestFalse(TEXT("Destination inside swung leaf is rejected"), AMissionDirector::CanTeleportTo(Pawn, LeafDestination));
    Door->Hinge->SetRelativeRotation(FRotator::ZeroRotator);
    Pawn->SetActorLocation(Point(-250, 500));
    Test->TestTrue(TEXT("Finite door check preserves alternate routes"), AMissionDirector::CanTeleportTo(Pawn, Point(250, 500)));
    Test->TestTrue(TEXT("Door collision swings with visible hinge"), Door->DoorMesh->GetAttachParent() == Door->Hinge);
    Test->TestEqual(TEXT("Visible leaf blocks Pawn collision"), Door->DoorMesh->GetCollisionResponseToChannel(ECC_Pawn), ECR_Block);

    // Exercise the real overlap/storage APIs with a held lizard. No input-button or
    // container-held requirement is injected into the production path.
    USceneComponent* Hand = Cast<USceneComponent>(LizardBlueprintBridge::Object(Pawn, TEXT("MotionControllerLeftGrip")));
    if (!Hand) { Test->AddError(TEXT("Left grip missing")); return true; }
    const auto PlaceHeld = [&](ALizardGameplayActor* Lizard, FVector Center)
    {
        Lizard->Caught = true;
        Lizard->AttachToComponent(Hand, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        auto* Catch = ToolBlueprintBridge::NamedComponent<UPrimitiveComponent>(Lizard, TEXT("CatchSphere"));
        Hand->SetWorldLocation(Hand->GetComponentLocation() + Center - Catch->GetComponentLocation());
        Catch->UpdateOverlaps();
    };
    ALizardGameplayActor* Doll = Training->TrainingDoll;
    AActor* TrainingJar = Training->CaptureContainer;
    auto* TrainingMouth = ToolBlueprintBridge::NamedComponent<USphereComponent>(TrainingJar, TEXT("DepositVolume"));
    bool Accepted = false;
    Training->ExitMission->AcknowledgeIntroduction();
    PlaceHeld(Doll, TrainingMouth->GetComponentLocation());
    TrainingMouth->UpdateOverlaps();
    Doll->TryStoreInBottle(TrainingJar, Accepted);
    Test->TestTrue(TEXT("Tutorial plush stores in stationary container"), Accepted && Doll->bStored);
    Test->TestEqual(TEXT("Tutorial progress increments once"), Training->ExitMission->CapturedCount, 1);
    Doll->TryStoreInBottle(TrainingJar, Accepted);
    Test->TestFalse(TEXT("Duplicate storage rejected"), Accepted);
    Test->TestEqual(TEXT("Duplicate storage cannot increment progress"), Training->ExitMission->CapturedCount, 1);

    const FVector Station(10000, 10000, 100);
    auto* RealClass = LoadClass<ALizardGameplayActor>(nullptr, TEXT("/Game/CatchTheLizard/BP_Lizard.BP_Lizard_C"));
    auto* ToyClass = LoadClass<ALizardGameplayActor>(nullptr, TEXT("/Game/CatchTheLizard/Missions/BP_MovingToyLizard.BP_MovingToyLizard_C"));
    ALizardGameplayActor* Real = World->SpawnActor<ALizardGameplayActor>(RealClass, Station, FRotator::ZeroRotator);
    ALizardGameplayActor* OtherReal = World->SpawnActor<ALizardGameplayActor>(RealClass, Station + FVector(100, 0, 0), FRotator::ZeroRotator);
    ALizardGameplayActor* Toy = World->SpawnActor<ALizardGameplayActor>(ToyClass, Station + FVector(200, 0, 0), FRotator::ZeroRotator);
    if (!Real || !OtherReal || !Toy) { Test->AddError(TEXT("Lizard fixtures failed to spawn")); return true; }
    const float Now = World->GetTimeSeconds();
    Test->TestTrue(TEXT("First real-lizard delay randomized within 45-60 seconds"), Real->NextLizardVoiceTime >= Now + 45.f && Real->NextLizardVoiceTime <= Now + 60.f);
    Test->TestTrue(TEXT("Each real lizard has an independent randomized deadline"), OtherReal->NextLizardVoiceTime != Real->NextLizardVoiceTime);
    Test->TestFalse(TEXT("Moving toy has no animal voice"), Toy->bEnableRealLizardVoice);
    Real->BeginSmokeStun();
    Test->TestEqual(TEXT("Stun cancels voice timer"), Real->NextLizardVoiceTime, -1.f);

    auto* ManagerClass = LoadClass<AActor>(nullptr, TEXT("/Game/CatchTheLizard/BP_LizardCollectionManager.BP_LizardCollectionManager_C"));
    AActor* Manager = World->SpawnActor<AActor>(ManagerClass, Station, FRotator::ZeroRotator);
    auto* JarClass = LoadClass<AActor>(nullptr, TEXT("/Game/CatchTheLizard/BP_CollectionBottle.BP_CollectionBottle_C"));
    AActor* Jar = World->SpawnActor<AActor>(JarClass, Station, FRotator::ZeroRotator);
    if (!Manager || !Jar) { Test->AddError(TEXT("Jar/manager fixtures missing")); return true; }
    auto* Mouth = ToolBlueprintBridge::NamedComponent<USphereComponent>(Jar, TEXT("DepositVolume"));
    PlaceHeld(Real, Mouth->GetComponentLocation() + FVector(Mouth->GetScaledSphereRadius() + 1.f, 0, 0));
    Mouth->UpdateOverlaps();
    Real->TryStoreInBottle(Jar, Accepted);
    Test->TestFalse(TEXT("Exterior fringe contact does not capture"), Accepted);
    PlaceHeld(Real, Mouth->GetComponentLocation());
    Mouth->UpdateOverlaps();
    LizardBlueprintBridge::FCall Deposit(Jar, TEXT("UpdateDeposits"));
    Test->TestTrue(TEXT("Existing jar deposit function callable"), Deposit.Invoke());
    Test->TestTrue(TEXT("Real lizard stores through manager into unheld jar"), Real->bStored);
    const auto* CountProperty = FindFProperty<FIntProperty>(Manager->GetClass(), TEXT("StoredLizards"));
    if (CountProperty)
    {
        const int32 Count = CountProperty->GetPropertyValue_InContainer(Manager);
        LizardBlueprintBridge::FCall Again(Jar, TEXT("UpdateDeposits")); Again.Invoke();
        Test->TestEqual(TEXT("Repeated overlap preserves unique collection count"), CountProperty->GetPropertyValue_InContainer(Manager), Count);
    }
    Test->TestTrue(TEXT("Stored lizard detached from grip"), Real->GetRootComponent()->GetAttachParent() != Hand);
    LizardBlueprintBridge::FCall Occupancy(Pawn, TEXT("IsToolHandOccupied"));
    Occupancy.SetObject(TEXT("Hand"), Hand); Occupancy.Invoke();
    Test->TestFalse(TEXT("Storage clears existing attachment-based hand occupancy"), Occupancy.GetBool(TEXT("Occupied")));
    // Repeat the same manager path with the moving toy and a jar held in the other hand.
    USceneComponent* OtherHand = Cast<USceneComponent>(LizardBlueprintBridge::Object(Pawn, TEXT("MotionControllerRightGrip")));
    UObject* Grab = LizardBlueprintBridge::Object(Jar, TEXT("GrabComponent"));
    LizardBlueprintBridge::FCall GrabJar(Grab, TEXT("TryGrab"));
    Test->TestTrue(TEXT("Jar grabs through the existing interaction system"),
        GrabJar.SetObject(TEXT("MotionController"), OtherHand) && GrabJar.Invoke() && GrabJar.GetBool(TEXT("IsHeld")));
    PlaceHeld(Toy, Mouth->GetComponentLocation()); Mouth->UpdateOverlaps();
    LizardBlueprintBridge::FCall DepositToy(Jar, TEXT("UpdateDeposits")); DepositToy.Invoke();
    Test->TestTrue(TEXT("Moving toy stores into held jar"), Toy->bStored);
    Test->TestTrue(TEXT("Holding the jar is preserved"), LizardBlueprintBridge::Bool(Grab, TEXT("bIsHeld"))
        && LizardBlueprintBridge::Object(Grab, TEXT("MotionControllerRef")) == OtherHand);
    return true;
}

bool FCTLVRInteractionRulesTest::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/Maps/Map_TrainingFacility")));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.f));
    ADD_LATENT_AUTOMATION_COMMAND(FCheckCTLVRInteractionRules(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif

