#include "CkUsf/Stylize/CkUsf_CelPattern_Utils.h"

#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"
#include "CkUsf/Stylize/CkUsf_CelPattern_Fragment.h"
#include "CkUsf_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_usf_cel_pattern_utils
{
    auto
        DoStampTarget(
            FCk_Handle& InHandle,
            ECk_Usf_CelPattern InPattern,
            bool InIsCascadeDerived)
        -> void
    {
        if (InHandle.Has<ck::FFragment_Usf_CelPatternTarget>())
        {
            // A cascade never downgrades an explicitly-patterned dependent.
            if (InIsCascadeDerived &&
                NOT InHandle.Get<ck::FFragment_Usf_CelPatternTarget>().Get_IsCascadeDerived())
            { return; }
        }

        InHandle.AddOrGet<ck::FFragment_Usf_CelPatternTarget>() =
            ck::FFragment_Usf_CelPatternTarget{InPattern, InIsCascadeDerived};
    }

    auto
        DoStampDependents_Recursive(
            const FCk_Handle& InHandle,
            ECk_Usf_CelPattern InPattern)
        -> void
    {
        for (auto& Dependent : UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(InHandle))
        {
            if (ck::Is_NOT_Valid(Dependent))
            { continue; }

            // An outlined dependent already owns its Custom-Stencil byte. Skipped rather than refused:
            // the caller asked about the ROOT, and failing a whole cascade because one leaf happens to be
            // outlined would be worse than covering the rest. Logged so it is discoverable — the direct
            // request on such an entity is still rejected loudly.
            if (Dependent.Has<ck::FFragment_Usf_OutlineTarget>())
            {
                ck::usf::Verbose(TEXT("Cel pattern cascade skipped dependent [{}]: it carries an outline "
                                      "target, which owns its Custom Stencil value"), Dependent);
            }
            else
            {
                DoStampTarget(Dependent, InPattern, true);
            }

            DoStampDependents_Recursive(Dependent, InPattern);
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

            if (Dependent.Has<ck::FFragment_Usf_CelPatternTarget>() &&
                Dependent.Get<ck::FFragment_Usf_CelPatternTarget>().Get_IsCascadeDerived())
            {
                Dependent.Remove<ck::FFragment_Usf_CelPatternTarget>();
            }

            DoRemoveDerivedTargets_Recursive(Dependent);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Usf_CelPattern_UE::
    Request_SetCelPattern(
        FCk_Handle& InHandle,
        ECk_Usf_CelPattern InPattern,
        ECk_Usf_OutlineScope InScope,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Request_SetCelPattern: INVALID handle"))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    // Both features write the SAME Custom-Stencil byte on the entity's primitives, so accepting this would
    // silently replace the outline the caller asked for earlier.
    const auto StencilIsFree = NOT InHandle.Has<ck::FFragment_Usf_OutlineTarget>();
    CK_ENSURE_IF_NOT(StencilIsFree,
        TEXT("Request_SetCelPattern on [{}]: entity already carries an OUTLINE target, which owns its "
             "Custom Stencil value; cel pattern NOT applied"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    ck_usf_cel_pattern_utils::DoStampTarget(InHandle, InPattern, false);

    if (InScope == ECk_Usf_OutlineScope::EntityAndDependents)
    { ck_usf_cel_pattern_utils::DoStampDependents_Recursive(InHandle, InPattern); }

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);

    return InHandle;
}

auto
    UCk_Utils_Usf_CelPattern_UE::
    Request_ClearCelPattern(
        FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Request_ClearCelPattern: INVALID handle"))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    InHandle.Try_Remove<ck::FFragment_Usf_CelPatternTarget>();

    // Derived targets only ever come from a cascade rooted here (or above) — stripping them is always safe;
    // explicitly-patterned dependents keep theirs.
    ck_usf_cel_pattern_utils::DoRemoveDerivedTargets_Recursive(InHandle);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);

    return InHandle;
}

auto
    UCk_Utils_Usf_CelPattern_UE::
    Has_CelPattern(
        const FCk_Handle& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    return InHandle.Has<ck::FFragment_Usf_CelPatternTarget>();
}

auto
    UCk_Utils_Usf_CelPattern_UE::
    Get_CelPatternOr(
        const FCk_Handle& InHandle,
        ECk_Usf_CelPattern InFallback)
    -> ECk_Usf_CelPattern
{
    if (ck::Is_NOT_Valid(InHandle))
    { return InFallback; }

    if (NOT InHandle.Has<ck::FFragment_Usf_CelPatternTarget>())
    { return InFallback; }

    return InHandle.Get<ck::FFragment_Usf_CelPatternTarget>().Get_Pattern();
}

// --------------------------------------------------------------------------------------------------------------------
