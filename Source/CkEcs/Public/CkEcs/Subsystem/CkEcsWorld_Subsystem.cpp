#include "CkEcsWorld_Subsystem.h"

#include "AutomationBlueprintFunctionLibrary.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Settings/CkEcs_Settings.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_EcsWorldTickingGroup_JustBeforeDestructionPhase_Name, TEXT("EcsWorldTickingGroup.JustBeforeDestructionPhase"));
UE_DEFINE_GAMEPLAY_TAG(TAG_EcsWorldTickingGroup_EntityDestructionInPhases_Name, TEXT("EcsWorldTickingGroup.EntityDestructionInPhases"));
UE_DEFINE_GAMEPLAY_TAG(TAG_EcsWorldTickingGroup_DeltaTime_Name, TEXT("EcsWorldTickingGroup.DeltaTime"));
UE_DEFINE_GAMEPLAY_TAG(TAG_EcsWorldTickingGroup_GameplayWithPump_Name, TEXT("EcsWorldTickingGroup.GameplayWithPump"));
UE_DEFINE_GAMEPLAY_TAG(TAG_EcsWorldTickingGroup_GameplayWithoutPump_Name, TEXT("EcsWorldTickingGroup.GameplayWithoutPump"));
UE_DEFINE_GAMEPLAY_TAG(TAG_EcsWorldTickingGroup_Script_Name, TEXT("EcsWorldTickingGroup.Script"));
UE_DEFINE_GAMEPLAY_TAG(TAG_EcsWorldTickingGroup_ChaosDestruction_Name, TEXT("EcsWorldTickingGroup.ChaosDestruction"));
UE_DEFINE_GAMEPLAY_TAG(TAG_EcsWorldTickingGroup_Physics_Name, TEXT("EcsWorldTickingGroup.Physics"));
UE_DEFINE_GAMEPLAY_TAG(TAG_EcsWorldTickingGroup_Misc_Name, TEXT("EcsWorldTickingGroup.Misc"));
UE_DEFINE_GAMEPLAY_TAG(TAG_EcsWorldTickingGroup_Replication_Name, TEXT("EcsWorldTickingGroup.Replication"));
UE_DEFINE_GAMEPLAY_TAG(TAG_EcsWorldTickingGroup_EntityCreationAndDestruction_Name, TEXT("EcsWorldTickingGroup.EntityCreationAndDestruction"));
UE_DEFINE_GAMEPLAY_TAG(TAG_EcsWorldTickingGroup_Overlap_Name, TEXT("EcsWorldTickingGroup.Overlap"));

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("CkEcsWorldActor_Tick"), STATGROUP_CkEcsWorldActor_Tick, STATCAT_Advanced);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EcsWorld_ProcessorInjector_Base_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    CK_TRIGGER_ENSURE(TEXT("ProcessorInjector was NOT overridden in project settings. Using the default injector which has no Processors.{}"),
        ck::Context(this));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EcsWorld_ProcessorScriptInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
}

// --------------------------------------------------------------------------------------------------------------------

ACk_EcsWorld_Actor_UE::
    ACk_EcsWorld_Actor_UE()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = false;
    bReplicates = false;
    bAlwaysRelevant = true;
}

auto
    ACk_EcsWorld_Actor_UE::
    Tick(
        float DeltaSeconds)
    -> void
{
    Super::Tick(DeltaSeconds);

    const auto TickStatCounter = FScopeCycleCounter{_TickStatId};

    for (auto Pump = 0; Pump < _WorldToTick._MaxNumberOfPumps; ++Pump)
    {
        TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*ck::Format_UE(TEXT("{}::Tick, Pump [{}/{}]"), _TickStatName, Pump, _WorldToTick._MaxNumberOfPumps));

        _WorldToTick._EcsWorld->Tick(FCk_Time{DeltaSeconds});
    }
}

auto
    ACk_EcsWorld_Actor_UE::
    Initialize(
        const FCk_Registry& InRegistry,
        const FCk_Ecs_MetaProcessorInjectors_Info& InMetaInjectorInfo)
    -> void
{
    _WorldToTick = FWorldInfo{InMetaInjectorInfo.Get_MaximumNumberOfPumps(), EcsWorldType{InRegistry}};
    _EcsWorldTickingGroup = InMetaInjectorInfo.Get_EcsWorldTickingGroup();
    _StatCollectionPolicy = InMetaInjectorInfo.Get_StatCollectionPolicy();
    _UnrealTickingGroup = InMetaInjectorInfo.Get_UnrealTickingGroup();
    _EcsWorldDisplayName = InMetaInjectorInfo.Get_DisplayName();
    _TickStatName = ck::Format_UE(TEXT("[{}][{}] EcsWorld_Actor"), _UnrealTickingGroup, _EcsWorldTickingGroup);
    _TickStatId = FDynamicStats::CreateStatId<STAT_GROUP_TO_FStatGroup(STATGROUP_CkEcsWorldActor_Tick)>(_TickStatName);

    TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*ck::Format_UE(TEXT("{}::Initialize"), _TickStatName));

    SetTickGroup(_UnrealTickingGroup);

    const auto& InjectProcessorsIntoWorld = [this](const TSubclassOf<class UCk_EcsWorld_ProcessorInjector_Base_UE>& InInjectorClass)
    {
        const auto NewInjector = UCk_Utils_Object_UE::Request_CreateNewObject<UCk_EcsWorld_ProcessorInjector_Base_UE>(this, InInjectorClass, nullptr);

        CK_ENSURE_IF_NOT(ck::IsValid(NewInjector), TEXT("Failed to instance ProcessorInjector of class [{}]"), InInjectorClass)
        { return; }

        TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*ck::Format_UE(TEXT("Injecting Native Processor from Injector [{}]"), NewInjector));

        NewInjector->DoInjectProcessors(*_WorldToTick._EcsWorld);
    };

    const auto& InjectMetaInjectorProcessors = [&](const TScriptInterface<ICk_MetaProcessorInjector_Interface>& InMetaInjector)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InMetaInjector),
            TEXT("Encountered an INVALID MetaInjector in NewEcsWorld Actor [{}]"), this)
        { return; }

        for (const auto& ProcessorInjector : InMetaInjector->Get_ProcessorInjectors())
        {
            InjectProcessorsIntoWorld(ProcessorInjector);
        }
    };

    TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*ck::Format_UE(TEXT("Inject Meta Processor Into EcsWorldActor with EcsWorldTickingGroup [{}]"), _EcsWorldTickingGroup));

    for (const auto& MetaInjector : InMetaInjectorInfo.Get_MetaProcessorInjectors())
    {
        InjectMetaInjectorProcessors(MetaInjector);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EcsWorld_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    _TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(_Registry);
    UCk_Utils_Handle_UE::Set_DebugName(_TransientEntity, TEXT("Transient Entity"));
}

auto
    UCk_EcsWorld_Subsystem_UE::
    OnWorldBeginPlay(
        UWorld& InWorld) -> void
{
    Super::OnWorldBeginPlay(InWorld);

    DoSpawnWorldActors(InWorld);
}

auto
    UCk_EcsWorld_Subsystem_UE::
    DoSpawnWorldActors(
        UWorld& InWorld)
    -> void
{
    const auto& EcsWorldActorClass = ACk_EcsWorld_Actor_UE::StaticClass();

    CK_ENSURE_IF_NOT(ck::IsValid(EcsWorldActorClass), TEXT("Invalid ECS World Actor class set in the Project Settings! ECS Framework won't work!"))
    { return; }

    _TransientEntity.Add<TWeakObjectPtr<UWorld>>(&InWorld);

    const auto ProcessorInjectors = UCk_Utils_Ecs_Settings_UE::Get_ProcessorInjectors();

    CK_LOG_ERROR_NOTIFY_IF_NOT(ck::ecs, ck::IsValid(ProcessorInjectors),
        TEXT("Could not get a valid instance of ProcessorInjectors PDA. Check ProjectSettings -> Ecs to make sure a valid DataAsset is referenced."))
    { return; }

    for (const auto& MetaInjectorInfo : ProcessorInjectors->Get_MetaProcessorInjectors())
    {
        const auto& UnrealTickingGroup = MetaInjectorInfo.Get_UnrealTickingGroup();
        const auto& EcsWorldTickingGroup = MetaInjectorInfo.Get_EcsWorldTickingGroup();
        const auto& ActorName = ck::Format_UE(TEXT("[{}][{}] EcsWorld_Actor"), UnrealTickingGroup, EcsWorldTickingGroup);

        auto WorldActor = Cast<ACk_EcsWorld_Actor_UE>
        (
            UCk_Utils_Actor_UE::Request_SpawnActor
            (
                FCk_Utils_Actor_SpawnActor_Params{&InWorld, EcsWorldActorClass}
                .Set_Label(ActorName)
                .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel),
                [&](AActor* InActor)
                {
                    const auto& NewWorldActor = Cast<ACk_EcsWorld_Actor_UE>(InActor);
                    NewWorldActor->Initialize(_Registry, MetaInjectorInfo);
                }
            )
        );

        CK_ENSURE_IF_NOT(ck::IsValid(WorldActor),
            TEXT("Failed to spawn Ecs World Actor [{}]"),
            ActorName)
        { continue; }

        if (const auto& FoundWorldActorsForTickingGroup = _WorldActors_ByUnrealTickingGroup.Find(UnrealTickingGroup);
            ck::IsValid(FoundWorldActorsForTickingGroup, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (NOT FoundWorldActorsForTickingGroup->IsEmpty())
            {
                const auto& TickPrerequisiteActor = FoundWorldActorsForTickingGroup->Last().Get();
                WorldActor->AddTickPrerequisiteActor(TickPrerequisiteActor);
            }
        }

        const auto WorldActorStrongPtr = TStrongObjectPtr(WorldActor);
        _WorldActors_ByUnrealTickingGroup.FindOrAdd(UnrealTickingGroup).Add(WorldActorStrongPtr);

        CK_ENSURE_IF_NOT(NOT _WorldActors_ByEcsWorldTickingGroup.Contains(EcsWorldTickingGroup),
            TEXT("More than 1 Ecs World Actor was spawn for the Ecs World Ticking Group [{}]"),
            EcsWorldTickingGroup)
        { continue; }

        _WorldActors_ByEcsWorldTickingGroup.Add(EcsWorldTickingGroup, WorldActorStrongPtr);
    }

    CK_ENSURE_IF_NOT(NOT _WorldActors_ByUnrealTickingGroup.IsEmpty(),
        TEXT("Failed to spawn ANY Ecs World Actors. ECS Pipeline will NOT work."))
    { return; }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EcsWorld_Subsystem_UE::
    Get_TransientEntity(
        const UWorld* InWorld)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorld),
        TEXT("Unable to get the EcsSubsystem to get the TransientEntity as the World is [{}]"), InWorld)
    { return {}; }

    const auto& Subsystem = InWorld->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem),
        TEXT("Unable to get the EcsSubsystem from the World [{}]. It's possible Get_TransientEntity is being called too early"),
        InWorld)
    { return {}; }

    return Subsystem->Get_TransientEntity();
}

// --------------------------------------------------------------------------------------------------------------------