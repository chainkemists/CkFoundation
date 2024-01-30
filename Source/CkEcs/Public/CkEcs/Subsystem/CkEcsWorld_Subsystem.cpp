#include "CkEcsWorld_Subsystem.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/CkEcsLog.h"

#include <Engine/World.h>

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/ProcessorInjector/CkEcsProcessorInjector.h"
#include "CkEcs/Settings/CkEcs_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_ECS_TAG(FTag_NAME_TransientEntity);

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
    // InWorld.Add(
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
    Tick(float DeltaSeconds)
    -> void
{
    Super::Tick(DeltaSeconds);

    CK_ENSURE_IF_NOT(ck::IsValid(_EcsWorld), TEXT("ECS World is NOT set in the World Actor [{}]. Was Initialize() called?"), this)
    { return; }

    _EcsWorld->Tick(FCk_Time{DeltaSeconds});
}

auto
    ACk_EcsWorld_Actor_UE::
    Initialize(
        const FCk_Registry& InRegistry,
        ETickingGroup InTickingGroup)
    -> void
{
    SetTickGroup(InTickingGroup);

    _EcsWorld = EcsWorldType{InRegistry};

    const auto ProcessorInjectors = UCk_Utils_Ecs_Settings_UE::Get_ProcessorInjectors();

    CK_LOG_ERROR_NOTIFY_IF_NOT(ck::ecs, ck::IsValid(ProcessorInjectors),
        TEXT("Could not get a valid instance of ProcessorInjectors PDA. Check ProjectSettings -> Ecs to make sure a valid DataAsset is referenced."))
    { return; }

    for (auto Injectors : ProcessorInjectors->Get_ProcessorInjectors())
    {
        if (Injectors.Get_TickingGroup() != InTickingGroup)
        { continue; }

        for (const auto Injector : Injectors.Get_ProcessorInjectors())
        {
            CK_ENSURE_IF_NOT(ck::IsValid(Injector),
                TEXT("Encountered an INVALID Injector in ProcessorInjectors Asset [{}].{}"), ProcessorInjectors, ck::Context(this))
            { continue; }

            const auto NewInjector = UCk_Utils_Object_UE::Request_CreateNewObject<UCk_EcsWorld_ProcessorInjector_Base_UE>(this, Injector, nullptr);

            NewInjector->DoInjectProcessors(*_EcsWorld);
        }
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

    _TransientEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_Registry);
    _TransientEntity.Add<FTag_NAME_TransientEntity>();
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
    DoSpawnWorldActors(UWorld& InWorld)
    -> void
{
    const auto& EcsWorldActorClass = ACk_EcsWorld_Actor_UE::StaticClass();

    CK_ENSURE_IF_NOT(ck::IsValid(EcsWorldActorClass), TEXT("Invalid ECS World Actor class set in the Project Settings! ECS Framework won't work!"))
    { return; }

    _TransientEntity.Add<TWeakObjectPtr<UWorld>>(&InWorld);

    for (auto TickingGroup = TG_PrePhysics; TickingGroup < ETickingGroup::TG_NewlySpawned;
        TickingGroup = static_cast<ETickingGroup>(TickingGroup + 1))
    {
        const auto& ActorName = ck::Format_UE(TEXT("EcsWorld_Actor_{}"), TickingGroup);

        auto WorldActor = Cast<ACk_EcsWorld_Actor_UE>
        (
            UCk_Utils_Actor_UE::Request_SpawnActor
            (
                FCk_Utils_Actor_SpawnActor_Params{&InWorld, EcsWorldActorClass}
                .Set_Label(ActorName)
                .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel),
                [&](AActor* InActor)
                {
                    const auto NewWorldActor = Cast<ACk_EcsWorld_Actor_UE>(InActor);
                    NewWorldActor->Initialize(_Registry, TickingGroup);
                }
            )
        );

        _WorldActors.Add(TickingGroup, WorldActor);
    }

    CK_ENSURE_IF_NOT(NOT _WorldActors.IsEmpty(),
        TEXT("Failed to spawn ANY EcsWorld Actors. ECS Pipeline will NOT work."))
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

    const auto Subsystem = InWorld->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();

    CK_ENSURE_IF_NOT(Subsystem,
        TEXT("Unable to get the EcsSubsystem from the World [{}]. It's possible Get_TransientEntity is being called too early"),
        InWorld)
    { return {}; }

    return Subsystem->Get_TransientEntity();
}

// --------------------------------------------------------------------------------------------------------------------
