#include "CkProcessorScript_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ProcessorScript_Subsystem_UE::
    Request_CreateNewProcessorScript(
        const TSubclassOf<UCk_Ecs_ProcessorScript_Base>& InProcessorScriptClass)
    -> UCk_Ecs_ProcessorScript_Base*
{
    auto* ProcessorScript = NewObject<UCk_Ecs_ProcessorScript_Base>(GetTransientPackage(), InProcessorScriptClass);
    _Processors.Emplace(ProcessorScript);

    return ProcessorScript;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProcessorScript_Subsystem_UE::
    Request_CreateNewProcessorScript(
        const UWorld* InWorld,
        const TSubclassOf<UCk_Ecs_ProcessorScript_Base>& InProcessorScriptClass)
    -> UCk_Ecs_ProcessorScript_Base*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorld),
        TEXT("World is [{}]. Unable to create new ProcessorScript [{}]"), InProcessorScriptClass)
    { return {}; }

    return InWorld->GetSubsystem<UCk_ProcessorScript_Subsystem_UE>()->Request_CreateNewProcessorScript(InProcessorScriptClass);
}

// --------------------------------------------------------------------------------------------------------------------
