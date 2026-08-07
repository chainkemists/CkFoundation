#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkUsf/Outline/CkUsf_Outline_Utils.h"
#include "CkUsf/Stylize/CkUsf_CelShade_Params.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkUsf_CelPattern_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Entity-level per-object cel pattern. Writes a declarative ck::FFragment_Usf_CelPatternTarget on the
// entity; a per-renderer sync processor turns it into the Custom-Stencil value the CelShade look reads
// (StencilBase + pattern index). The alternative is tagging the mesh's stencil by hand in the level, which
// this exists to replace for entities.
//
// The scope enum is UCk_Utils_Usf_Outline_UE's: the two features cascade over lifetime dependents the same
// way, and a second identical enum would only be a thing to keep in sync.
//
// ONE ENTITY, ONE STENCIL VALUE. Entity outlines write the same Custom-Stencil byte, so an entity cannot
// carry both — Request_SetCelPattern REJECTS an entity that already has an outline target, and the sync
// processor yields to an outline target applied afterwards.
UCLASS(NotBlueprintable)
class CKUSF_API UCk_Utils_Usf_CelPattern_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Usf_CelPattern_UE);

public:
    // NOTE: InScope has NO default (dropped so the trailing delegate can be added — AngelScript only
    // permits defaults on trailing parameters); the dropped default was the type's zero value (EntityOnly).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Usf|CelPattern",
              DisplayName = "[Ck][Usf] Request Set Cel Pattern (Entity)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle
    Request_SetCelPattern(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Usf_CelPattern InPattern,
        ECk_Usf_OutlineScope InScope,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Usf|CelPattern",
              DisplayName = "[Ck][Usf] Request Clear Cel Pattern (Entity)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle
    Request_ClearCelPattern(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|CelPattern",
              DisplayName = "[Ck][Usf] Has Cel Pattern (Entity)")
    static bool
    Has_CelPattern(
        const FCk_Handle& InHandle);

    // Returns the entity's pattern, or the passed fallback when it has none — a cel pattern has no
    // "invalid" value to report absence with, so the caller states what absence means.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|CelPattern",
              DisplayName = "[Ck][Usf] Get Cel Pattern Or (Entity)")
    static ECk_Usf_CelPattern
    Get_CelPatternOr(
        const FCk_Handle& InHandle,
        ECk_Usf_CelPattern InFallback);
};

// --------------------------------------------------------------------------------------------------------------------
