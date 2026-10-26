#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "ModelContextProtocolSettings.h"
#include "UObject/UObjectGlobals.h"
#endif

class FCTLProjectModule : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override
    {
        FDefaultGameModuleImpl::StartupModule();
#if WITH_EDITOR
        if (IsRunningCommandlet())
        {
            // MCP starts at PostEngineInit. Commandlets must not inherit the editor's
            // auto-start preference and compete with its listener. Do not save this override.
            GetMutableDefault<UModelContextProtocolSettings>()->bAutoStartServer = false;
        }
#endif
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE(FCTLProjectModule, CTLProject, "CTLProject");
