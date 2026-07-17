#include "CkComponentHost_Subsystem.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    ACk_ComponentHost_Actor_UE::
    IsSelectionChild() const
    -> bool
{
    return _EditorSelectionOwner.IsValid();
}

auto
    ACk_ComponentHost_Actor_UE::
    GetSelectionParent() const
    -> AActor*
{
    return _EditorSelectionOwner.Get();
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ComponentHost_Subsystem_UE::
    Get(
        UWorld* InWorld)
    -> UCk_ComponentHost_Subsystem_UE*
{
    if (ck::Is_NOT_Valid(InWorld))
    { return nullptr; }

    return InWorld->GetSubsystem<UCk_ComponentHost_Subsystem_UE>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ComponentHost_Subsystem_UE::
    Get_HostActor()
    -> AActor*
{
    if (ck::IsValid(_HostActor))
    { return _HostActor; }

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    auto SpawnParams = FActorSpawnParameters{};
    SpawnParams.Name = TEXT("Ck_UnrealComponent_Host");
    SpawnParams.ObjectFlags = RF_Transient;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    _HostActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);

    return _HostActor;
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_ComponentHost_Subsystem_UE::
    Get_HostActor_ForEditorSelectionOwner(
        AActor* InSelectionOwner)
    -> AActor*
{
    if (ck::Is_NOT_Valid(InSelectionOwner))
    { return Get_HostActor(); }

    const auto Key = FObjectKey{InSelectionOwner};

    if (const auto* MaybeFound = _PerOwnerHostActors.Find(Key);
        MaybeFound != nullptr && MaybeFound->IsValid())
    { return MaybeFound->Get(); }

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    auto SpawnParams = FActorSpawnParameters{};
    SpawnParams.ObjectFlags = RF_Transient;
    SpawnParams.bHideFromSceneOutliner = true;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    auto* PerOwnerHost = World->SpawnActor<ACk_ComponentHost_Actor_UE>(
        ACk_ComponentHost_Actor_UE::StaticClass(), SpawnParams);

    if (ck::Is_NOT_Valid(PerOwnerHost))
    { return nullptr; }

    PerOwnerHost->_EditorSelectionOwner = InSelectionOwner;

    ck::editor_selection_owner::RegisterProxyActor(InSelectionOwner, PerOwnerHost);

    _PerOwnerHostActors.Add(Key, PerOwnerHost);

    return PerOwnerHost;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
