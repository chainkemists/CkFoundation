#include "CkEcsProcessorInjector.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Processor.h"
#include "CkEcs/EntityScript/CkEntityScript_Processor.h"
#include "CkEcs/OwningActor/CkOwningActor_Processors.h"
#include "CkEcs/Processor/CkProcessorScript_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ecs_ProcessorInjector_EntityDestruction::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_OwningActor_Destroy>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_EntityLifetime_DestructionPhase_Finalize>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_EntityLifetime_DestructionPhase_Await>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ecs_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_EntityLifetime_EntityJustCreated>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_EntityLifetime_DestroyEntity>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_EntityLifetime_DestructionPhase_InitiateConfirm>(InWorld.Get_Registry());

#if CK_ENABLE_MEMORY_TRACKING
    InWorld.Add<ck::FProcessor_Memory_Stats>();
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ecs_ProcessorInjector_Requests::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_EntityScript_SpawnEntity_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_EntityScript_Construct>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_EntityScript_BeginPlay>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ecs_ProcessorInjector_Teardown::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_EntityScript_EndPlay>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ecs_ProcessorScriptInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    for (auto ProcessorClass : _Processors)
    {
        const auto& Processor = UCk_Utils_ProcessorScript_Subsystem_UE::Request_CreateNewProcessorScript(GetWorld(), ProcessorClass);
        InWorld.Add(Processor);
    }
}

// --------------------------------------------------------------------------------------------------------------------
