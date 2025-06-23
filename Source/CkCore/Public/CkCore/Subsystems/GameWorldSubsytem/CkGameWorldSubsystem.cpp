#include "CkGameWorldSubsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Game_WorldSubsystem_Base_UE::
    Deinitialize()
    -> void
{
    OnDeinitialize();

    Super::Deinitialize();
}

auto
    UCk_Game_WorldSubsystem_Base_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    OnInitialize();
}

auto
    UCk_Game_WorldSubsystem_Base_UE::
    PostInitialize()
    -> void
{
    Super::PostInitialize();

    _AllWorldSubsystemsInitialized = true;
    OnAllWorldSubsystemsInitialized();
}

auto
    UCk_Game_WorldSubsystem_Base_UE::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
    if (const auto& ShouldCreateSubsystem = Super::ShouldCreateSubsystem(InOuter);
        NOT ShouldCreateSubsystem)
    { return false; }

    if (ck::Is_NOT_Valid(InOuter))
    { return true; }

    const auto& World = InOuter->GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return true; }

    return DoesSupportWorldType(World->WorldType);
}

auto
    UCk_Game_WorldSubsystem_Base_UE::
    DoesSupportWorldType(
        const EWorldType::Type WorldType) const
    -> bool
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

auto
    UCk_Game_WorldSubsystem_Base_UE::
    Get_AreAllWorldSubsystemsInitialized() const
    -> bool
{
    return _AllWorldSubsystemsInitialized;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Game_WorldSubsystemWithReplicatedActor_Base_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    const auto& ActorToSpawn = Get_ActorToSpawn();

    CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(ActorToSpawn), TEXT("Invalid Actor specified in Get_ActorToSpawn.{}"), ck::Context(this))
    { return; }

    const auto& DefaultObject = UCk_Utils_Object_UE::Get_ClassDefaultObject<AActor>(ActorToSpawn);
    if (DefaultObject->GetIsReplicated() && GetWorld()->GetNetMode() == NM_Client && GetWorld()->GetNetMode() != NM_ListenServer)
    { return; }

    _Actor = GetWorld()->SpawnActor<AActor>(Get_ActorToSpawn());
}

auto
    UCk_Game_WorldSubsystemWithReplicatedActor_Base_UE::
    Deinitialize()
    -> void
{
    Super::Deinitialize();
}

auto
    UCk_Game_WorldSubsystemWithReplicatedActor_Base_UE::
    Get_ActorToSpawn()
    -> TSubclassOf<AActor>
{
    CK_TRIGGER_ENSURE(TEXT("It's expected that WorldSubsystemWithReplicationActor is overridden to provide the Actor to spawn.{}"),
        ck::Context(this));
    return TSubclassOf<AActor>{};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Game_TickableWorldSubsystem_Base_UE::
    Deinitialize()
    -> void
{
    OnDeinitialize();

    Super::Deinitialize();
}

auto
    UCk_Game_TickableWorldSubsystem_Base_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    OnInitialize();
}

auto
    UCk_Game_TickableWorldSubsystem_Base_UE::
    PostInitialize()
    -> void
{
    Super::PostInitialize();

    _AllWorldSubsystemsInitialized = true;
    OnAllWorldSubsystemsInitialized();
}

auto
    UCk_Game_TickableWorldSubsystem_Base_UE::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
    if (const auto& ShouldCreateSubsystem = Super::ShouldCreateSubsystem(InOuter);
        NOT ShouldCreateSubsystem)
    { return false; }

    if (ck::Is_NOT_Valid(InOuter))
    { return true; }

    const auto& World = InOuter->GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return true; }

    return DoesSupportWorldType(World->WorldType);
}

auto
    UCk_Game_TickableWorldSubsystem_Base_UE::
    GetStatId() const
    -> TStatId
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCk_Game_TickableWorldSubsystem_Base_UE, STATGROUP_Tickables);
}

auto
    UCk_Game_TickableWorldSubsystem_Base_UE::
    DoesSupportWorldType(
        const EWorldType::Type WorldType) const
    -> bool
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

auto
    UCk_Game_TickableWorldSubsystem_Base_UE::
    Get_AreAllWorldSubsystemsInitialized() const
    -> bool
{
    return _AllWorldSubsystemsInitialized;
}

// --------------------------------------------------------------------------------------------------------------------
