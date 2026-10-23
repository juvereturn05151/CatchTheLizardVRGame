#include "VRMainMenuGameMode.h"
#include "VRMainMenuPawn.h"

AVRMainMenuGameMode::AVRMainMenuGameMode()
{
    DefaultPawnClass = AVRMainMenuPawn::StaticClass();
    bUseSeamlessTravel = false;
}
