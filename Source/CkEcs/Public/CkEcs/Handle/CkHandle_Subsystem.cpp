#include "CkHandle_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_HandleDebugger_Subsystem_UE::
    GetOrAdd_FragmentsDebug(
        const FCk_Handle& InHandle)
    -> UCk_Handle_FragmentsDebug*
{
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

auto
    UCk_Utils_HandleDebugger_Subsystem_UE::
    Get_Subsystem()
    -> SubsystemType*
{
    CK_ENSURE_IF_NOT(ck::IsValid(GEngine, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("GEngine is INVALID. This function is being called too early"))
    { return nullptr; }

    const auto Subsystem = GEngine->GetEngineSubsystem<SubsystemType>();

    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem),
        TEXT("GEngine is Valid but could NOT get a valid [{}]. This function is being called too early."),
        ck::TypeToString<SubsystemType>)
    { return nullptr; }

    return Subsystem;
}
