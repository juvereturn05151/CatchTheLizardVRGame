#pragma once

#include "LizardBlueprintBridge.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

// Read the existing BP_GrabComponent state; pickup, release and hand occupancy remain in XRFramework.
namespace ToolBlueprintBridge
{
    template<typename T> T* NamedComponent(AActor* Actor, FName Name)
    {
        TArray<T*> Components;
        Actor->GetComponents<T>(Components);
        for (T* Component : Components)
            if (Component->GetFName() == Name) return Component;
        return nullptr;
    }

    inline USceneComponent* HoldingController(UObject* Grab)
    {
        return LizardBlueprintBridge::Bool(Grab, TEXT("bIsHeld"))
            ? Cast<USceneComponent>(LizardBlueprintBridge::Object(Grab, TEXT("MotionControllerRef"))) : nullptr;
    }

    inline bool GameplayLocked(const AActor* Tool)
    {
        return LizardBlueprintBridge::Bool(UGameplayStatics::GetPlayerPawn(Tool, 0), TEXT("bCollectionInputLocked"));
    }

    inline bool ConnectGrabSignal(UObject* Grab, FName Signal, UObject* Listener, FName Method, bool Bind = true)
    {
        const auto* Property = IsValid(Grab) ? FindFProperty<FMulticastDelegateProperty>(Grab->GetClass(), Signal) : nullptr;
        if (!Property || !Property->SignatureFunction || Property->SignatureFunction->NumParms != 0) return false;
        FScriptDelegate Delegate;
        Delegate.BindUFunction(Listener, Method);
        void* Value = Property->ContainerPtrToValuePtr<void>(Grab);
        if (Bind) Property->AddDelegate(Delegate, Grab, Value);
        else Property->RemoveDelegate(Delegate, Grab, Value);
        return true;
    }
}
