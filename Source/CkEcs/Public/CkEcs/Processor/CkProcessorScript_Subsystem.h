#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/Processor/CkProcessorScript.h"

#include "CkProcessorScript_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKECS_API UCk_ProcessorScript_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ProcessorScript_Subsystem_UE);

public:
    auto
    Request_CreateNewProcessorScript(
        const TSubclassOf<UCk_Ecs_ProcessorScript_Base>& InProcessorScriptClass) -> UCk_Ecs_ProcessorScript_Base*;

private:
    UPROPERTY(Transient)
    TSet<TObjectPtr<UCk_Ecs_ProcessorScript_Base>> _Processors;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_ProcessorScript_Subsystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    using SubsystemType = UCk_ProcessorScript_Subsystem_UE;

public:
    auto
    Request_CreateNewProcessorScript(
        const UWorld* InWorld,
        const TSubclassOf<UCk_Ecs_ProcessorScript_Base>& InProcessorScriptClass) -> UCk_Ecs_ProcessorScript_Base*;
};

// --------------------------------------------------------------------------------------------------------------------
