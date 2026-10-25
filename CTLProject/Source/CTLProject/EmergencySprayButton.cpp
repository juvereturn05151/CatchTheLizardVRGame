#include "EmergencySprayButton.h"
#include "MissionFlow.h"
#include "SprayGameplayActor.h"
#include "PhysicalPressInteraction.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AEmergencySprayButton::AEmergencySprayButton()
{
    PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.TickInterval = .1f;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonMesh")); ButtonMesh->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    ButtonMesh->SetStaticMesh(Cube.Object); ButtonMesh->SetRelativeScale3D(FVector(.12f, .35f, .35f)); ButtonMesh->SetCollisionProfileName(TEXT("BlockAll"));
    StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusText")); StatusText->SetupAttachment(RootComponent);
    StatusText->SetRelativeLocation(FVector(-15, 0, 40)); StatusText->SetRelativeRotation(FRotator(0, 180, 0));
    StatusText->SetWorldSize(9); StatusText->SetHorizontalAlignment(EHTA_Center);
    SpraySpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpraySpawnPoint")); SpraySpawnPoint->SetupAttachment(RootComponent);
    SpraySpawnPoint->SetRelativeLocation(FVector(-45, 0, 15));
    PressInteraction = CreateDefaultSubobject<UPhysicalPressInteraction>(TEXT("PressInteraction")); PressInteraction->PressPoint = ButtonMesh;
    CountdownFormat = NSLOCTEXT("CTLEmergency", "Countdown", "Emergency spray available in {0}");
    ReadyText = NSLOCTEXT("CTLEmergency", "Ready", "Emergency spray ready — Press to dispense");
    DispensedText = NSLOCTEXT("CTLEmergency", "Dispensed", "Spray dispensed.");
    FailedText = NSLOCTEXT("CTLEmergency", "Failed", "Clear the dispenser and press to retry.");
}
void AEmergencySprayButton::BeginPlay()
{
    Super::BeginPlay(); bDispensed = bSpawning = false;
    PressInteraction->OnPressed.AddUniqueDynamic(this, &AEmergencySprayButton::Press); Refresh();
}
void AEmergencySprayButton::Tick(float Delta) { Super::Tick(Delta); Refresh(); }
void AEmergencySprayButton::Refresh()
{
    const float Remaining = FMath::Max(0.f, UnlockDelay - (Mission ? Mission->ActiveGameplaySeconds : 0.f));
    const bool Ready = Mission && Mission->bGameplayActive && !Mission->bComplete && Remaining <= 0;
    const int32 Seconds = FMath::CeilToInt(Remaining);
    StatusText->SetText(bDispensed ? DispensedText : Ready ? ReadyText : FText::Format(CountdownFormat, FText::FromString(FString::Printf(TEXT("%02d:%02d"), Seconds / 60, Seconds % 60))));
    const FColor Color = bDispensed ? FColor(90, 180, 255) : Ready ? FColor(70, 255, 100) : FColor(255, 180, 50);
    StatusText->SetTextRenderColor(Color);
    ButtonMesh->SetVectorParameterValueOnMaterials(TEXT("BodyColor"), FVector(Color.R / 255.f, Color.G / 255.f, Color.B / 255.f));
}
void AEmergencySprayButton::Press()
{
    if (bDispensed || bSpawning || !Mission || !Mission->bEmergencyRating || !Mission->bGameplayActive || Mission->bComplete || Mission->ActiveGameplaySeconds < UnlockDelay || !SprayClass) return;
    bSpawning = true;
    FActorSpawnParameters Params; Params.Owner = this; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    auto* Spray = GetWorld()->SpawnActor<ASprayGameplayActor>(SprayClass, SpraySpawnPoint->GetComponentTransform(), Params);
    bSpawning = false;
    if (!IsValid(Spray)) { StatusText->SetText(FailedText); return; }
    bDispensed = true;
    Mission->RecordEmergencyDispensed(Spray);
    Refresh();
}
