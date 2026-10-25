#include "StoredLizardDisplayComponent.h"
#include "LizardGameplayActor.h"
#include "ToolBlueprintBridge.h"

UStoredLizardDisplayComponent::UStoredLizardDisplayComponent() { PrimaryComponentTick.bCanEverTick = false; }

void UStoredLizardDisplayComponent::ShowStoredLizard(ALizardGameplayActor* Lizard)
{
    if (!IsValid(Lizard) || !Lizard->bStored || Occupants.Contains(Lizard)) return;
    int32 Index = Occupants.IndexOfByPredicate([](const auto& Actor) { return !Actor.IsValid(); });
    if (Index == INDEX_NONE) { Index = Occupants.Num(); Occupants.Add(Lizard); }
    else Occupants[Index] = Lizard;
    USceneComponent* Slot = InteriorSlots.IsValidIndex(Index)
        ? ToolBlueprintBridge::NamedComponent<USceneComponent>(GetOwner(), InteriorSlots[Index]) : nullptr;
    if (!Slot)
    {
        // Preserve existing storage rules even if an author omitted a visual slot.
        Slot = NewObject<USceneComponent>(GetOwner());
        Slot->SetupAttachment(GetOwner()->GetRootComponent());
        Slot->SetRelativeLocation(OverflowSlotStep * (Index + 1));
        Slot->RegisterComponent();
        UE_LOG(LogTemp, Warning, TEXT("%s: author an interior slot for stored lizard %d"), *GetOwner()->GetName(), Index + 1);
    }
    Lizard->DisplayInsideContainer(Slot, StoredModelScale, StoredModelOffset);
}

void UStoredLizardDisplayComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    // World teardown already owns actor destruction. Explicitly clean up if just the jar is destroyed.
    if (Reason == EEndPlayReason::Destroyed)
        for (const auto& Lizard : Occupants) if (Lizard.IsValid() && Lizard->bStored) Lizard->Destroy();
    Occupants.Reset();
    Super::EndPlay(Reason);
}
