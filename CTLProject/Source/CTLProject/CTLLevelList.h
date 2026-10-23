#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CTLLevelList.generated.h"

USTRUCT(BlueprintType)
struct FCTLLevelEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UWorld> Map;
};

/** Shared editable destinations; no unlock state, so every entry is immediately available. */
UCLASS(BlueprintType)
class CTLPROJECT_API UCTLLevelList : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Levels") TArray<FCTLLevelEntry> Levels;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Navigation") TSoftObjectPtr<UWorld> MainMenuMap;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Navigation") TSoftObjectPtr<UWorld> SelectionMap;
    const FCTLLevelEntry* Find(FName Id) const
    {
        return Levels.FindByPredicate([Id](const FCTLLevelEntry& Entry) { return Entry.Id == Id; });
    }
};
