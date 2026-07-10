#include "CkDebugFeatureFlags_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DebugFeatureFlags_UE::
    Request_Enable(
        const FCk_Handle& InAnyHandleInWorld)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAnyHandleInWorld),
        TEXT("Cannot Enable DebugFeatureFlags — Handle [{}] is INVALID"), InAnyHandleInWorld)
    { return; }

    ck::debug_feature_flags::Enable(InAnyHandleInWorld.Get_RegistryView());
}

auto
    UCk_Utils_DebugFeatureFlags_UE::
    Request_Disable(
        const FCk_Handle& InAnyHandleInWorld)
    -> void
{
    if (ck::Is_NOT_Valid(InAnyHandleInWorld))
    { return; }

    ck::debug_feature_flags::Disable(InAnyHandleInWorld.Get_RegistryView());
}

auto
    UCk_Utils_DebugFeatureFlags_UE::
    Get_IsEnabled(
        const FCk_Handle& InAnyHandleInWorld)
    -> bool
{
    if (ck::Is_NOT_Valid(InAnyHandleInWorld))
    { return false; }

    return ck::debug_feature_flags::Get_IsEnabled(InAnyHandleInWorld.Get_RegistryView());
}

auto
    UCk_Utils_DebugFeatureFlags_UE::
    Get_Flags(
        const FCk_Handle& InHandle)
    -> int64
{
    if (ck::Is_NOT_Valid(InHandle))
    { return 0; }

    return static_cast<int64>(ck::debug_feature_flags::Get_Flags(
        InHandle.Get_RegistryView(), InHandle.Get_Entity()));
}

auto
    UCk_Utils_DebugFeatureFlags_UE::
    Get_HasFeature(
        const FCk_Handle& InHandle,
        FName InFeatureId)
    -> bool
{
    const auto Bit = ck::debug_feature_flags::Get_BitIndex(InFeatureId);
    if (Bit == INDEX_NONE)
    { return false; }

    return (static_cast<uint64>(Get_Flags(InHandle)) & (uint64{1} << Bit)) != 0;
}

auto
    UCk_Utils_DebugFeatureFlags_UE::
    Get_BitIndex(
        FName InFeatureId)
    -> int32
{
    return ck::debug_feature_flags::Get_BitIndex(InFeatureId);
}
