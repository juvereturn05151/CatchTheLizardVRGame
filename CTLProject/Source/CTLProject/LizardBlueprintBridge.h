#pragma once

#include "CoreMinimal.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

// Narrow, name-checked calls into the existing Blueprint tool/Pawn APIs. No tool gameplay lives here.
namespace LizardBlueprintBridge
{
    inline UObject* Object(UObject* Owner, FName Name)
    {
        const auto* Property = Owner ? FindFProperty<FObjectPropertyBase>(Owner->GetClass(), Name) : nullptr;
        return Property ? Property->GetObjectPropertyValue_InContainer(Owner) : nullptr;
    }

    inline bool Bool(UObject* Owner, FName Name)
    {
        const auto* Property = Owner ? FindFProperty<FBoolProperty>(Owner->GetClass(), Name) : nullptr;
        return Property && Property->GetPropertyValue_InContainer(Owner);
    }

    class FCall
    {
    public:
        FCall(UObject* InTarget, FName Name)
            : Target(InTarget), Function(IsValid(InTarget) ? InTarget->FindFunction(Name) : nullptr), Parameters(Function) {}

        bool SetObject(FName Name, UObject* Value)
        {
            auto* Property = Function ? FindFProperty<FObjectPropertyBase>(Function, Name) : nullptr;
            if (!Property || (Value && !Value->IsA(Property->PropertyClass))) return false;
            Property->SetObjectPropertyValue_InContainer(Parameters.GetStructMemory(), Value);
            return true;
        }

        bool Invoke()
        {
            if (!Function || !IsValid(Target)) return false;
            Target->ProcessEvent(Function, Parameters.GetStructMemory());
            return true;
        }

        bool GetBool(FName Name) const
        {
            const auto* Property = Function ? FindFProperty<FBoolProperty>(Function, Name) : nullptr;
            return Property && Property->GetPropertyValue_InContainer(Parameters.GetStructMemory());
        }

        UObject* GetObject(FName Name) const
        {
            const auto* Property = Function ? FindFProperty<FObjectPropertyBase>(Function, Name) : nullptr;
            return Property ? Property->GetObjectPropertyValue_InContainer(Parameters.GetStructMemory()) : nullptr;
        }

        FVector GetVector(FName Name) const
        {
            const auto* Property = Function ? FindFProperty<FStructProperty>(Function, Name) : nullptr;
            return Property && Property->Struct == TBaseStructure<FVector>::Get()
                ? *Property->ContainerPtrToValuePtr<FVector>(Parameters.GetStructMemory()) : FVector::ZeroVector;
        }

    private:
        UObject* Target;
        UFunction* Function;
        FStructOnScope Parameters;
    };
}
