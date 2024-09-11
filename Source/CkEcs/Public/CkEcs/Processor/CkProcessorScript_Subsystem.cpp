#include "CkProcessorScript_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ProcessorScript_Subsystem_UE::
    Request_CreateNewProcessorScript(
        const TSubclassOf<UCk_Ecs_ProcessorScript_Base_UE>& InProcessorScriptClass)
    -> UCk_Ecs_ProcessorScript_Base_UE*
{
    auto* ProcessorScript = NewObject<UCk_Ecs_ProcessorScript_Base_UE>(GetWorld(), InProcessorScriptClass);
    _Processors.Emplace(ProcessorScript);

    return ProcessorScript;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProcessorScript_Subsystem_UE::
    Request_CreateNewProcessorScript(
        const UWorld* InWorld,
        const TSubclassOf<UCk_Ecs_ProcessorScript_Base_UE>& InProcessorScriptClass)
    -> UCk_Ecs_ProcessorScript_Base_UE*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorld),
        TEXT("World is [{}]. Unable to create new ProcessorScript [{}]"), InWorld, InProcessorScriptClass)
    { return {}; }

    return InWorld->GetSubsystem<UCk_ProcessorScript_Subsystem_UE>()->Request_CreateNewProcessorScript(InProcessorScriptClass);
}

// --------------------------------------------------------------------------------------------------------------------
