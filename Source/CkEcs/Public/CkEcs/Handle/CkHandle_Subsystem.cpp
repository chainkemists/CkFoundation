#include "CkHandle_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Settings/CkEcs_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_HandleDebugger_Subsystem_UE::
    GetOrAdd_FragmentsDebug(
        const FCk_Handle& InHandle)
    -> UCk_Handle_FragmentsDebug*
{
    if (UCk_Utils_Ecs_ProjectSettings_UE::Get_HandleDebuggerBehavior() == ECk_Ecs_HandleDebuggerBehavior::Disable)
    { return {}; }

    if (ck::Is_NOT_Valid(GetTransientPackage()))
    { return Cast<UCk_Handle_FragmentsDebug>(UCk_Handle_FragmentsDebug::StaticClass()); }

    if (const auto Found = _EntityToDebug.Find(InHandle.Get_Entity()))
    {
        Found->Set_ShareCount(Found->Get_ShareCount() + 1);
        return Found->Get_FragmentsDebug();
    }

    const auto NewObject = UCk_Utils_Object_UE::Request_CreateNewObject_TransientPackage<UCk_Handle_FragmentsDebug>();

    _EntityToDebug.Add(InHandle.Get_Entity(), FCk_FragmentsDebug_SharingInfo{NewObject, 1});

    return NewObject;
}

auto
    UCk_HandleDebugger_Subsystem_UE::
    Remove_FragmentsDebug(
        const FCk_Handle& InHandle)
    -> void
{
    if (UCk_Utils_Ecs_ProjectSettings_UE::Get_HandleDebuggerBehavior() == ECk_Ecs_HandleDebuggerBehavior::Disable)
    { return; }

    if (ck::Is_NOT_Valid(GetTransientPackage()))
    { return; }

    const auto Found = _EntityToDebug.Find(InHandle.Get_Entity());

    CK_ENSURE_IF_NOT(Found,
        TEXT("Handle [{}] was never tracked OR it was tracked but got removed (i.e. somehow, more remove were called "
            "than GetOrAdd for this Handle)"), InHandle)
    { return; }

    Found->Set_ShareCount(Found->Get_ShareCount() - 1);

    if (Found->Get_ShareCount() > 0)
    { return; }

    _EntityToDebug.Remove(InHandle.Get_Entity());
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_handle_subsystem
{
    TSet<FCk_Entity> EntitiesAddedToEarlyToTrack;
}

auto
    UCk_Utils_HandleDebugger_Subsystem_UE::
    GetOrAdd_FragmentsDebug(
        const FCk_Handle& InHandle)
    -> UCk_Handle_FragmentsDebug*
{
    using namespace ck_handle_subsystem;

    if (ck::Is_NOT_Valid(GEngine))
    {
        ck::ecs::Log(TEXT("Could not add debugging data for Entity [{}] because GEngine is INVALID"), InHandle);
        EntitiesAddedToEarlyToTrack.Emplace(InHandle.Get_Entity());
        return nullptr;
    }

    const auto Subsystem = GEngine->GetEngineSubsystem<SubsystemType>();

    if (ck::Is_NOT_Valid(Subsystem))
    {
        ck::ecs::Log(TEXT("Could not add debugging data for Entity [{}] because [{}] could NOT be retrieved from GEngine"),
            ck::Get_RuntimeTypeToString<SubsystemType>());
        EntitiesAddedToEarlyToTrack.Emplace(InHandle.Get_Entity());
        return nullptr;
    }

    if (EntitiesAddedToEarlyToTrack.Contains(InHandle.Get_Entity()))
    {
        EntitiesAddedToEarlyToTrack.Remove(InHandle.Get_Entity());
    }

    return Subsystem->GetOrAdd_FragmentsDebug(InHandle);
}

auto
    UCk_Utils_HandleDebugger_Subsystem_UE::
    Remove_FragmentsDebug(
        const FCk_Handle& InHandle)
    -> void
{
    using namespace ck_handle_subsystem;

    if (EntitiesAddedToEarlyToTrack.Contains(InHandle.Get_Entity()))
    { return; }

    if (ck::Is_NOT_Valid(GEngine))
    { return; }

    const auto Subsystem = GEngine->GetEngineSubsystem<SubsystemType>();

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->Remove_FragmentsDebug(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
