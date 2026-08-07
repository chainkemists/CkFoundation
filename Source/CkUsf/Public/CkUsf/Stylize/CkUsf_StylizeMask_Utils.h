#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkUsf/Outline/CkUsf_Outline_Utils.h"
#include "CkUsf/Stylize/CkUsf_CelShade_Params.h"
#include "CkUsf/Stylize/CkUsf_StylizeMask_Params.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkUsf_StylizeMask_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Entity-level membership of the stylize EFFECT MASK, plus the range validation every effect's
// Request_SetSettings runs. Both live here because they are two halves of one contract: the entity API
// stamps the project's mask Custom-Stencil value, and the validation is what keeps that value from
// colliding with the two other claims on the same byte.
//
// THE THREE CLAIMS ON THE CUSTOM-STENCIL BYTE, IN PRECEDENCE ORDER: outline > cel pattern > effect mask.
// An entity carries at most one. The asymmetry mirrors the one the cel pattern already has with the
// outline: a claim REFUSES loudly when a higher-precedence one is already present (upward), and is
// silently taken over when a higher-precedence one arrives afterwards (downward) — the sync processors
// exclude their betters, and the drop processors retire the now-false applied-state so the lower claim
// reappears once the higher one is removed.
//
// The scope enum is UCk_Utils_Usf_Outline_UE's, for the reason the cel-pattern API reuses it: all three
// features cascade over lifetime dependents the same way and a third identical enum would only be
// something to keep in sync.
UCLASS(NotBlueprintable)
class CKUSF_API UCk_Utils_Usf_StylizeMask_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Usf_StylizeMask_UE);

public:
    // NOTE: InScope has NO default (AngelScript only permits defaults on trailing parameters, and the
    // completion delegate is trailing); the dropped default was the type's zero value (EntityOnly).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Usf|StylizeMask",
              DisplayName = "[Ck][Usf] Request Add To Stylize Mask (Entity)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle
    Request_AddToStylizeMask(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Usf_OutlineScope InScope,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Usf|StylizeMask",
              DisplayName = "[Ck][Usf] Request Remove From Stylize Mask (Entity)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle
    Request_RemoveFromStylizeMask(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|StylizeMask",
              DisplayName = "[Ck][Usf] Has Stylize Mask (Entity)")
    static bool
    Has_StylizeMask(
        const FCk_Handle& InHandle);

    // Whether the ACTOR-path sync processor has actually written the stencil for this entity. Distinct
    // from Has_StylizeMask, which reports only the declarative request: an entity can carry the target
    // and never be applied (no owning actor, a higher-precedence claim present, an unusable project
    // value). Parity with the renderer proxies' Get_Is*Applied getters, and what lets a test tell
    // "asked for" from "landed".
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|StylizeMask",
              DisplayName = "[Ck][Usf] Get Is Stylize Mask Applied (Entity)")
    static bool
    Get_IsStylizeMaskApplied(
        const FCk_Handle& InHandle);

    // The Custom-Stencil value the actor path wrote, or 0 when nothing is applied.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|StylizeMask",
              DisplayName = "[Ck][Usf] Get Stylize Mask Applied Stencil Value (Entity)")
    static int32
    Get_StylizeMaskAppliedStencilValue(
        const FCk_Handle& InHandle);

public:
    // ---- Range validation ----
    // Split into named halves so every effect's Request_SetSettings can name WHICH rule a rejected mask
    // broke, exactly as UCkUsf_CelShadeSubsystem::Get_StencilRangeIsFree's two halves do.

    // The range is usable at all: ordered, and clear of the engine's "nothing written here" value 0.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|StylizeMask",
              DisplayName = "[Ck][Usf] Get Stylize Mask Range Is Addressable")
    static bool
    Get_MaskRangeIsAddressable(
        const FCk_Usf_StylizeMask_Params& InMask);

    // The range does not intersect UCkUsf_OutlineSubsystem's allocated range in this world.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|StylizeMask",
              DisplayName = "[Ck][Usf] Get Stylize Mask Range Avoids Outline",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Get_MaskRangeAvoidsOutline(
        const UObject* InWorldContextObject,
        const FCk_Usf_StylizeMask_Params& InMask);

    // The range does not intersect the cel-pattern span of the PASSED cel settings. The explicit form is
    // what UCkUsf_CelShadeSubsystem itself validates against — it must check the INCOMING settings, whose
    // pattern span and mask range are two fields of one value and can collide with each other.
    static auto
    Get_MaskRangeAvoidsCelSpan(
        const FCk_Usf_StylizeMask_Params& InMask,
        const FCk_Usf_CelShade_Params& InCelSettings) -> bool;

    // The same rule against whatever the world's CelShade subsystem currently holds — what the effects
    // that do NOT own cel settings validate against.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|StylizeMask",
              DisplayName = "[Ck][Usf] Get Stylize Mask Range Avoids Cel Patterns",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Get_MaskRangeAvoidsCelPatterns(
        const UObject* InWorldContextObject,
        const FCk_Usf_StylizeMask_Params& InMask);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|StylizeMask",
              DisplayName = "[Ck][Usf] Get Stylize Mask Range Is Free",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Get_MaskRangeIsFree(
        const UObject* InWorldContextObject,
        const FCk_Usf_StylizeMask_Params& InMask);

    // Whether the PROJECT's single mask stencil value is safe to stamp in this world — i.e. it is
    // addressable and lands outside both the outline subsystem's allocated range and the CelShade
    // subsystem's live pattern span.
    //
    // This is the guard the per-effect range validation cannot supply. `_MaskStencilValue` is a plain
    // config int clamped only to 1..255, and 240..255 is the outline's allocation while the cel span
    // moves with `_StencilBase`. A value landing in either would make every entity added to the mask
    // sprout an outline silhouette (or a cel pattern) with nothing naming the cause — the settings-side
    // checks never see it, because it is not part of any effect's settings value.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|StylizeMask",
              DisplayName = "[Ck][Usf] Get Stylize Mask Stencil Value Is Free",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Get_MaskStencilValueIsFree(
        const UObject* InWorldContextObject);

    // The disjointness rule run the OTHER way: whether an incoming CelShade pattern span clears the mask
    // ranges the world's other stylize subsystems have already stored.
    //
    // Both directions are needed because either value can move second. A sibling's mask is validated
    // against the cel span that exists WHEN THE MASK IS SET; without this, moving `_StencilBase`
    // afterwards would walk the cel span straight onto a mask that was legal when it was authored, and
    // the collision would exist with nothing having rejected anything.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|StylizeMask",
              DisplayName = "[Ck][Usf] Get Cel Span Avoids Sibling Masks",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Get_CelSpanAvoidsSiblingMasks(
        const UObject* InWorldContextObject,
        const FCk_Usf_CelShade_Params& InCelSettings);
};

// --------------------------------------------------------------------------------------------------------------------
