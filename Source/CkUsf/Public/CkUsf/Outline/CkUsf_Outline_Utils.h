#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkUsf_Outline_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkUsf_OutlinePreset;

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Usf_OutlineScope : uint8
{
    // Outline only the requested entity's renderables (its owning-actor primitives / IsmProxy / IskmProxy).
    EntityOnly,

    // Also stamp cascade-derived outline targets on the entity's lifetime dependents, recursively. Dependents
    // spawned AFTER the request are NOT retro-outlined; Request_RemoveOutline strips derived targets only.
    EntityAndDependents
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_OutlineScope);

// --------------------------------------------------------------------------------------------------------------------

// Entity-level outline API. Writes a declarative ck::FFragment_Usf_OutlineTarget on the entity; per-renderer
// sync processors (actor path here, shadow ISM in CkIsmRenderer, SKMC in CkIskmRenderer) apply it. Batched
// crowd members are NOT entities — use UCk_Utils_IskmBatched_UE::Set_CrowdMemberOutline for those.
UCLASS(NotBlueprintable)
class CKUSF_API UCk_Utils_Usf_Outline_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Usf_Outline_UE);

public:
    // NOTE: InScope has NO default (dropped so the trailing delegate can be added — AngelScript
    // only permits defaults on trailing parameters); the dropped default was the type's zero
    // value (EntityOnly).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Usf|Outline",
              DisplayName = "[Ck][Usf] Request Apply Outline (Entity)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle
    Request_ApplyOutline(
        UPARAM(ref) FCk_Handle& InHandle,
        UCkUsf_OutlinePreset* InPreset,
        ECk_Usf_OutlineScope InScope,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Usf|Outline",
              DisplayName = "[Ck][Usf] Request Remove Outline (Entity)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle
    Request_RemoveOutline(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|Outline",
              DisplayName = "[Ck][Usf] Has Outline (Entity)")
    static bool
    Has_Outline(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|Outline",
              DisplayName = "[Ck][Usf] Try Get Outline Preset (Entity)")
    static UCkUsf_OutlinePreset*
    TryGet_OutlinePreset(
        const FCk_Handle& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
