#include "CkUsf/Outline/CkUsf_Outline_Utils.h"

#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"
#include "CkUsf/Outline/CkUsf_OutlinePreset.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_usf_outline_utils
{
    auto
        DoStampTarget(
            FCk_Handle& InHandle,
            UCkUsf_OutlinePreset* InPreset,
            bool InIsCascadeDerived)
        -> void
    {
        if (InHandle.Has<ck::FFragment_Usf_OutlineTarget>())
        {
            // A cascade never downgrades an explicitly-outlined dependent.
            if (InIsCascadeDerived &&
                NOT InHandle.Get<ck::FFragment_Usf_OutlineTarget>().Get_IsCascadeDerived())
            { return; }
        }

        InHandle.AddOrGet<ck::FFragment_Usf_OutlineTarget>() =
            ck::FFragment_Usf_OutlineTarget{InPreset, InIsCascadeDerived};
    }

    auto
        DoStampDependents_Recursive(
            const FCk_Handle& InHandle,
            UCkUsf_OutlinePreset* InPreset)
        -> void
    {
        for (auto& Dependent : UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(InHandle))
        {
            if (ck::Is_NOT_Valid(Dependent))
            { continue; }

            DoStampTarget(Dependent, InPreset, true);
            DoStampDependents_Recursive(Dependent, InPreset);
        }
    }

    auto
        DoRemoveDerivedTargets_Recursive(
            const FCk_Handle& InHandle)
        -> void
    {
        for (auto& Dependent : UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(InHandle))
        {
            if (ck::Is_NOT_Valid(Dependent))
            { continue; }

            if (Dependent.Has<ck::FFragment_Usf_OutlineTarget>() &&
                Dependent.Get<ck::FFragment_Usf_OutlineTarget>().Get_IsCascadeDerived())
            {
                Dependent.Remove<ck::FFragment_Usf_OutlineTarget>();
            }

            DoRemoveDerivedTargets_Recursive(Dependent);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Usf_Outline_UE::
    Request_ApplyOutline(
        FCk_Handle& InHandle,
        UCkUsf_OutlinePreset* InPreset,
        ECk_Usf_OutlineScope InScope)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Request_ApplyOutline: INVALID handle"))
    { return InHandle; }

    CK_ENSURE_IF_NOT(ck::IsValid(InPreset, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Request_ApplyOutline on [{}]: null preset"), InHandle)
    { return InHandle; }

    ck_usf_outline_utils::DoStampTarget(InHandle, InPreset, false);

    if (InScope == ECk_Usf_OutlineScope::EntityAndDependents)
    { ck_usf_outline_utils::DoStampDependents_Recursive(InHandle, InPreset); }

    return InHandle;
}

auto
    UCk_Utils_Usf_Outline_UE::
    Request_RemoveOutline(
        FCk_Handle& InHandle)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Request_RemoveOutline: INVALID handle"))
    { return InHandle; }

    InHandle.Try_Remove<ck::FFragment_Usf_OutlineTarget>();

    // Derived targets only ever come from a cascade rooted here (or above) — stripping them is always safe;
    // explicitly-outlined dependents keep theirs.
    ck_usf_outline_utils::DoRemoveDerivedTargets_Recursive(InHandle);

    return InHandle;
}

auto
    UCk_Utils_Usf_Outline_UE::
    Has_Outline(
        const FCk_Handle& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    return InHandle.Has<ck::FFragment_Usf_OutlineTarget>();
}

auto
    UCk_Utils_Usf_Outline_UE::
    TryGet_OutlinePreset(
        const FCk_Handle& InHandle)
    -> UCkUsf_OutlinePreset*
{
    if (ck::Is_NOT_Valid(InHandle))
    { return nullptr; }

    if (NOT InHandle.Has<ck::FFragment_Usf_OutlineTarget>())
    { return nullptr; }

    return InHandle.Get<ck::FFragment_Usf_OutlineTarget>().Get_Preset().Get();
}

// --------------------------------------------------------------------------------------------------------------------
