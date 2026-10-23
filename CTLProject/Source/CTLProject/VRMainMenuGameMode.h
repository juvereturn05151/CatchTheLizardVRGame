#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VRMainMenuGameMode.generated.h"

/** Assigned only to Map_MainMenu; gameplay keeps BP_XRGameMode. */
UCLASS(Blueprintable)
class CTLPROJECT_API AVRMainMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AVRMainMenuGameMode();
};
