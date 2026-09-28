#include "SmokePuffGameplayActor.h"
#include "LizardGameplayActor.h"
#include "ToolBlueprintBridge.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ASmokePuffGameplayActor::ASmokePuffGameplayActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ASmokePuffGameplayActor::BeginPlay()
{
    Super::BeginPlay();
    VisualMesh = ToolBlueprintBridge::NamedComponent<UStaticMeshComponent>(this, TEXT("SmokeMesh"));
    if (!VisualMesh.IsValid()) UE_LOG(LogTemp, Error, TEXT("%s: missing Blueprint SmokeMesh"), *GetName());
}

FCollisionQueryParams ASmokePuffGameplayActor::TraceParameters() const
{
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SpraySmoke), false, this);
    for (AActor* Actor : TraceIgnoredActors)
        if (IsValid(Actor)) Params.AddIgnoredActor(Actor);
    // Lizard meshes must be hit targets, never occluders that stop bubbles before contact is credited.
    for (TActorIterator<ALizardGameplayActor> It(GetWorld()); It; ++It) Params.AddIgnoredActor(*It);
    return Params;
}

void ASmokePuffGameplayActor::InitializeSmoke(float RangeCm, float SmokeRadiusCm, float LifeSeconds, AActor* SourceTool)
{
    TravelRangeCm = FMath::Clamp(RangeCm, 1.f, 1000.f);
    RadiusCm = FMath::Clamp(SmokeRadiusCm, 2.f, 100.f);
    LifetimeSeconds = FMath::Clamp(LifeSeconds, .1f, 5.f);
    SpawnPosition = GetActorLocation();
    TraceIgnoredActors.AddUnique(SourceTool);
    TraceIgnoredActors.AddUnique(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!VisualMesh.IsValid()) VisualMesh = ToolBlueprintBridge::NamedComponent<UStaticMeshComponent>(this, TEXT("SmokeMesh"));
    if (VisualMesh.IsValid())
    {
        VisualMesh->SetRelativeScale3D(FVector(RadiusCm / 50.f));
        SmokeMaterial = VisualMesh->CreateDynamicMaterialInstance(0);
    }
    SetLifeSpan(LifetimeSeconds);
    ApplyGameplayHits(GetActorLocation(), GetActorLocation(), TraceParameters());
}

void ASmokePuffGameplayActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateSmoke(DeltaSeconds);
}

void ASmokePuffGameplayActor::UpdateSmoke(float DeltaSeconds)
{
    AgeSeconds += DeltaSeconds;
    if (AgeSeconds >= LifetimeSeconds)
    {
        Destroy();
        return;
    }
    if (SmokeMaterial)
        SmokeMaterial->SetScalarParameterValue(OpacityParameter, FMath::Clamp(1.f - AgeSeconds / LifetimeSeconds, 0.f, 1.f) * InitialOpacity);
    const FVector Start = GetActorLocation();
    const FCollisionQueryParams OcclusionParameters = TraceParameters();
    if (bBlocked)
    {
        ApplyGameplayHits(Start, Start, OcclusionParameters);
        return;
    }
    // Preserve the original travel speed: full range over 60 percent of the puff's lifetime.
    const float Speed = TravelRangeCm / (LifetimeSeconds * .6f);
    MoveStepCm = FMath::Min(TravelRangeCm - TravelledCm, Speed * DeltaSeconds);
    FVector End = Start + GetActorForwardVector() * MoveStepCm;
    FHitResult Hit;
    if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility,
        FCollisionShape::MakeSphere(RadiusCm), OcclusionParameters))
    {
        bBlocked = true;
        End = Hit.bStartPenetrating ? Start : Hit.Location;
    }
    // Query only the unobstructed segment. Actor-level latches merge mesh/catch-volume hits.
    ApplyGameplayHits(Start, End, OcclusionParameters);
    SetActorLocation(End, false, nullptr, ETeleportType::TeleportPhysics);
    TravelledCm += FVector::Distance(Start, End);
}

void ASmokePuffGameplayActor::ApplyGameplayHits(const FVector& Start, const FVector& End, const FCollisionQueryParams& OcclusionParameters)
{
    TArray<FHitResult> Hits;
    FCollisionQueryParams Targets(SCENE_QUERY_STAT(SprayBubbleHit), false, this);
    Targets.bFindInitialOverlaps = true;
    GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldDynamic),
        FCollisionShape::MakeSphere(RadiusCm), Targets);
    for (const FHitResult& Hit : Hits)
    {
        auto* Lizard = Cast<ALizardGameplayActor>(Hit.GetActor());
        if (!IsValid(Lizard) || HitLizards.Contains(Lizard)) continue;
        const FVector BubbleCenter = FMath::Lerp(Start, End, FMath::Clamp(Hit.Time, 0.f, 1.f));
        FHitResult Wall;
        // Radius sweeps can overlap across thin walls; a center-to-target LOS prevents that credit.
        if (GetWorld()->LineTraceSingleByChannel(Wall, BubbleCenter, Lizard->GetActorLocation(), ECC_Visibility, OcclusionParameters)) continue;
        HitLizards.Add(Lizard); // Consume this contact even if the lizard is currently incapacitated/caught.
        bool Accepted = false;
        Lizard->ReceiveSprayHit(this, Accepted);
    }
}
