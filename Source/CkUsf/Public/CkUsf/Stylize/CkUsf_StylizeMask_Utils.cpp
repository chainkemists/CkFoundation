#include "CkUsf/Stylize/CkUsf_StylizeMask_Utils.h"

#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"
#include "CkUsf/Outline/CkUsf_OutlineSubsystem.h"
#include "CkUsf/Stylize/CkUsf_CelPattern_Fragment.h"
#include "CkUsf/Stylize/CkUsf_CelShadeSubsystem.h"
#include "CkUsf/Stylize/CkUsf_CrossHatchSubsystem.h"
#include "CkUsf/Stylize/CkUsf_HandDrawnSubsystem.h"
#include "CkUsf/Stylize/CkUsf_ScreenDitherSubsystem.h"
#include "CkUsf/Stylize/CkUsf_StylizeMask_Fragment.h"
#include "CkUsf/Stylize/CkUsf_Stylize_ProjectSettings.h"
#include "CkUsf_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_usf_stylize_mask_utils
{
    auto
        DoStampTarget(
            FCk_Handle& InHandle,
            bool InIsCascadeDerived)
        -> void
    {
        if (InHandle.Has<ck::FFragment_Usf_StylizeMaskTarget>())
        {
            // A cascade never downgrades an explicitly-masked dependent.
            if (InIsCascadeDerived &&
                NOT InHandle.Get<ck::FFragment_Usf_StylizeMaskTarget>().Get_IsCascadeDerived())
            { return; }
        }

        InHandle.AddOrGet<ck::FFragment_Usf_StylizeMaskTarget>() =
            ck::FFragment_Usf_StylizeMaskTarget{InIsCascadeDerived};
    }

    auto
        DoGet_ClaimsStencilAlready(
            const FCk_Handle& InHandle)
        -> bool
    {
        return InHandle.Has<ck::FFragment_Usf_OutlineTarget>()
            || InHandle.Has<ck::FFragment_Usf_CelPatternTarget>();
    }

    auto
        DoStampDependents_Recursive(
            const FCk_Handle& InHandle)
        -> void
    {
        for (auto& Dependent : UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(InHandle))
        {
            if (ck::Is_NOT_Valid(Dependent))
            { continue; }

            // A dependent already claiming the stencil byte with a higher-precedence feature keeps it.
            // Skipped rather than refused: the caller asked about the ROOT, and failing a whole cascade
            // because one leaf happens to be outlined would be worse than covering the rest. The direct
            // request on such an entity is still rejected loudly.
            if (DoGet_ClaimsStencilAlready(Dependent))
            {
                ck::usf::Verbose(TEXT("Stylize mask cascade skipped dependent [{}]: it carries an outline or "
                                      "cel-pattern target, which owns its Custom Stencil value"), Dependent);
            }
            else
            {
                DoStampTarget(Dependent, true);
            }

            DoStampDependents_Recursive(Dependent);
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

            if (Dependent.Has<ck::FFragment_Usf_StylizeMaskTarget>() &&
                Dependent.Get<ck::FFragment_Usf_StylizeMaskTarget>().Get_IsCascadeDerived())
            {
                Dependent.Remove<ck::FFragment_Usf_StylizeMaskTarget>();
            }

            DoRemoveDerivedTargets_Recursive(Dependent);
        }
    }

    // The entity's world, or null when it has none — WITHOUT the diagnostic
    // UCk_Utils_EntityLifetime_UE::Get_WorldForEntity fires.
    //
    // That helper ensures once it reaches an entity with no lifetime owner, which is the ordinary state
    // of a registry-only entity: a test fixture, or anything composed before it is parented. A guard has
    // no business reporting an error about the value it was asked to validate having no world — it
    // simply has nothing world-local to check, and says so by returning null. Same shape the
    // range validators already use for a missing outline or CelShade subsystem.
    auto
        DoGet_WorldForEntity_NoEnsure(
            const FCk_Handle& InHandle)
        -> UWorld*
    {
        auto Current = InHandle;

        while (ck::IsValid(Current))
        {
            if (Current.Has<TWeakObjectPtr<UWorld>>())
            { return Current.Get<TWeakObjectPtr<UWorld>>().Get(); }

            if (NOT Current.Has<ck::FFragment_LifetimeOwner>())
            { return nullptr; }

            auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Current);

            // A self-owned entity would spin this loop forever; the ensuring helper refuses it too.
            if (ck::Is_NOT_Valid(Owner) || Owner == Current)
            { return nullptr; }

            Current = Owner;
        }

        return nullptr;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Usf_StylizeMask_UE::
    Request_AddToStylizeMask(
        FCk_Handle& InHandle,
        ECk_Usf_OutlineScope InScope,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Request_AddToStylizeMask: INVALID handle"))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    // The mask is the LOWEST-precedence claim on the entity's Custom-Stencil byte, so it refuses rather
    // than displacing: accepting here would silently replace the outline or cel pattern the caller asked
    // for earlier. The reverse direction is deliberately silent — see this class's header comment.
    const auto StencilIsFree = NOT ck_usf_stylize_mask_utils::DoGet_ClaimsStencilAlready(InHandle);
    CK_ENSURE_IF_NOT(StencilIsFree,
        TEXT("Request_AddToStylizeMask on [{}]: entity already carries an OUTLINE or CEL-PATTERN target, "
             "which owns its Custom Stencil value; stylize mask NOT applied"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    // The VALUE this request will stamp is config, and config is validated against nothing on its own —
    // the per-effect range checks only ever see an effect's settings. A project value inside the outline
    // allocation or the cel span makes every masked entity sprout someone else's silhouette instead of
    // being masked, which is exactly the failure the disjointness rule exists to prevent.
    const auto* WorldContext = ck_usf_stylize_mask_utils::DoGet_WorldForEntity_NoEnsure(InHandle);
    const auto MaskValueIsFree = Get_MaskStencilValueIsFree(WorldContext);

    CK_ENSURE_IF_NOT(MaskValueIsFree,
        TEXT("Request_AddToStylizeMask on [{}]: the project's stylize mask stencil value [{}] is "
             "unaddressable or COLLIDES with the outline subsystem's allocated range or the CelShade "
             "pattern span in this world; stylize mask NOT applied"),
        InHandle, UCk_Utils_Usf_Stylize_Settings_UE::Get_MaskStencilValue())
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    ck_usf_stylize_mask_utils::DoStampTarget(InHandle, false);

    if (InScope == ECk_Usf_OutlineScope::EntityAndDependents)
    { ck_usf_stylize_mask_utils::DoStampDependents_Recursive(InHandle); }

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);

    return InHandle;
}

auto
    UCk_Utils_Usf_StylizeMask_UE::
    Request_RemoveFromStylizeMask(
        FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Request_RemoveFromStylizeMask: INVALID handle"))
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    InHandle.Try_Remove<ck::FFragment_Usf_StylizeMaskTarget>();

    // Derived targets only ever come from a cascade rooted here (or above) — stripping them is always
    // safe; explicitly-masked dependents keep theirs.
    ck_usf_stylize_mask_utils::DoRemoveDerivedTargets_Recursive(InHandle);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);

    return InHandle;
}

auto
    UCk_Utils_Usf_StylizeMask_UE::
    Has_StylizeMask(
        const FCk_Handle& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    return InHandle.Has<ck::FFragment_Usf_StylizeMaskTarget>();
}

auto
    UCk_Utils_Usf_StylizeMask_UE::
    Get_IsStylizeMaskApplied(
        const FCk_Handle& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    return InHandle.Has<ck::FFragment_Usf_StylizeMaskApplied_Actor>();
}

auto
    UCk_Utils_Usf_StylizeMask_UE::
    Get_StylizeMaskAppliedStencilValue(
        const FCk_Handle& InHandle)
    -> int32
{
    if (NOT Get_IsStylizeMaskApplied(InHandle))
    { return 0; }

    return InHandle.Get<ck::FFragment_Usf_StylizeMaskApplied_Actor>().Get_StencilValue();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Usf_StylizeMask_UE::
    Get_MaskRangeIsAddressable(
        const FCk_Usf_StylizeMask_Params& InMask)
    -> bool
{
    if (NOT InMask.Get_ClaimsStencil())
    { return true; }

    // 0 is what the renderer leaves in Custom Stencil for every mesh that wrote nothing. A range reaching
    // it hands the whole untagged view membership: Include would stylize only the untagged pixels while
    // reading as "nothing is masked", Exclude would stylize nothing at all. Both read as "the look is
    // broken", not as a misconfiguration. An inverted range is refused for the same class of reason —
    // it matches nothing, so Include renders no effect anywhere with no diagnostic.
    return InMask.Get_StencilMin() >= 1 && InMask.Get_StencilMax() >= InMask.Get_StencilMin();
}

auto
    UCk_Utils_Usf_StylizeMask_UE::
    Get_MaskRangeAvoidsOutline(
        const UObject* InWorldContextObject,
        const FCk_Usf_StylizeMask_Params& InMask)
    -> bool
{
    if (NOT InMask.Get_ClaimsStencil())
    { return true; }

    // No outline subsystem in this world means nothing else is claiming stencil values through CkUsf.
    // Renderer modules that write stencil directly are out of reach of any check we could make here.
    const auto* Outline = UCkUsf_OutlineSubsystem::Get_OutlineSubsystem(InWorldContextObject);
    if (ck::Is_NOT_Valid(Outline))
    { return true; }

    const auto OutlineMin = static_cast<int32>(Outline->Get_StencilMin());
    const auto OutlineMax = static_cast<int32>(Outline->Get_StencilMax());

    return InMask.Get_StencilMax() < OutlineMin || InMask.Get_StencilMin() > OutlineMax;
}

auto
    UCk_Utils_Usf_StylizeMask_UE::
    Get_MaskRangeAvoidsCelSpan(
        const FCk_Usf_StylizeMask_Params& InMask,
        const FCk_Usf_CelShade_Params& InCelSettings)
    -> bool
{
    if (NOT InMask.Get_ClaimsStencil())
    { return true; }

    // A disabled cel stencil contract claims nothing, so there is nothing to be disjoint from.
    if (InCelSettings.Get_EnableStencilPatterns() != ECk_EnableDisable::Enable)
    { return true; }

    return InMask.Get_StencilMax() < InCelSettings.Get_StencilRangeMin()
        || InMask.Get_StencilMin() > InCelSettings.Get_StencilRangeMax();
}

auto
    UCk_Utils_Usf_StylizeMask_UE::
    Get_MaskRangeAvoidsCelPatterns(
        const UObject* InWorldContextObject,
        const FCk_Usf_StylizeMask_Params& InMask)
    -> bool
{
    if (NOT InMask.Get_ClaimsStencil())
    { return true; }

    const auto* CelShade = UCkUsf_CelShadeSubsystem::Get_CelShadeSubsystem(InWorldContextObject);
    if (ck::Is_NOT_Valid(CelShade))
    { return true; }

    return Get_MaskRangeAvoidsCelSpan(InMask, CelShade->Get_Settings());
}

auto
    UCk_Utils_Usf_StylizeMask_UE::
    Get_MaskRangeIsFree(
        const UObject* InWorldContextObject,
        const FCk_Usf_StylizeMask_Params& InMask)
    -> bool
{
    return Get_MaskRangeIsAddressable(InMask)
        && Get_MaskRangeAvoidsOutline(InWorldContextObject, InMask)
        && Get_MaskRangeAvoidsCelPatterns(InWorldContextObject, InMask);
}

auto
    UCk_Utils_Usf_StylizeMask_UE::
    Get_MaskStencilValueIsFree(
        const UObject* InWorldContextObject)
    -> bool
{
    // Expressed as a single-value INCLUDE range so the three rules are stated once and cannot drift
    // apart from the settings-side validation they mirror.
    const auto Value = UCk_Utils_Usf_Stylize_Settings_UE::Get_MaskStencilValue();

    auto AsRange = FCk_Usf_StylizeMask_Params{};
    AsRange.Set_Mode(ECk_Usf_StylizeMaskMode::IncludeStencilRange)
           .Set_StencilMin(Value)
           .Set_StencilMax(Value);

    return Get_MaskRangeIsFree(InWorldContextObject, AsRange);
}

auto
    UCk_Utils_Usf_StylizeMask_UE::
    Get_CelSpanAvoidsSiblingMasks(
        const UObject* InWorldContextObject,
        const FCk_Usf_CelShade_Params& InCelSettings)
    -> bool
{
    // A disabled cel stencil contract claims nothing, so there is nothing for a mask to collide with.
    if (InCelSettings.Get_EnableStencilPatterns() != ECk_EnableDisable::Enable)
    { return true; }

    const auto CelAvoids = [&InCelSettings](const FCk_Usf_StylizeMask_Params& InMask) -> bool
    {
        return Get_MaskRangeAvoidsCelSpan(InMask, InCelSettings);
    };

    // CelShade is excluded on purpose: its own mask travels in the same settings value being validated,
    // so it is checked against the INCOMING span by Request_SetSettings directly. Reading the stored one
    // here would judge the wrong pair.
    if (const auto* ScreenDither = UCkUsf_ScreenDitherSubsystem::Get_ScreenDitherSubsystem(InWorldContextObject);
        ck::IsValid(ScreenDither) &&
        NOT CelAvoids(ScreenDither->Get_Settings().Get_Mask()))
    { return false; }

    if (const auto* HandDrawn = UCkUsf_HandDrawnSubsystem::Get_HandDrawnSubsystem(InWorldContextObject);
        ck::IsValid(HandDrawn) &&
        NOT CelAvoids(HandDrawn->Get_Settings().Get_Mask()))
    { return false; }

    if (const auto* CrossHatch = UCkUsf_CrossHatchSubsystem::Get_CrossHatchSubsystem(InWorldContextObject);
        ck::IsValid(CrossHatch) &&
        NOT CelAvoids(CrossHatch->Get_Settings().Get_Mask()))
    { return false; }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
