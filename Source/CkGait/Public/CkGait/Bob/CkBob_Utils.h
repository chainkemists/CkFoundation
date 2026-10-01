#pragma once

#include "CkGait/Bob/CkBob_Fragment.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Fragment_Data.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkBob_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Bob"))
class CKGAIT_API UCk_Utils_Bob_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Bob_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Bob);

    friend class UCk_Utils_Ecs_Base_UE;

public:
    // Takes ownership of an existing scene node's offset (its CURRENT offset becomes the rest) and bobs it to the spec's gait.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Bob", DisplayName = "[Ck][Bob] Add Feature")
    static FCk_Handle_Bob
    Add(
        UPARAM(ref) FCk_Handle_SceneNode& InSceneNode,
        const FCk_Bob_Spec& InSpec);

    // Creates a parent-driven scene node under InParent at InLocalRest and adds Bob to it (Create = SceneNode::Create + Add).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Bob", DisplayName = "[Ck][Bob] Create New Bob Node")
    static FCk_Handle_Bob
    Create(
        UPARAM(ref) FCk_Handle_Transform& InParent,
        FTransform InLocalRest,
        const FCk_Bob_Spec& InSpec);

    static auto Has(const FCk_Handle& InHandle) -> bool;

private:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Bob", DisplayName = "[Ck][Bob] DoCast", meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Bob
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Bob", DisplayName = "[Ck][Bob] DoCastChecked", meta = (CompactNodeTitle = "<AsBob>", BlueprintAutocast))
    static FCk_Handle_Bob
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Bob", DisplayName = "[Ck] Get Invalid Bob Handle",
        meta = (CompactNodeTitle = "INVALID_BobHandle", Keywords = "make"))
    static FCk_Handle_Bob
    Get_InvalidHandle() { return {}; }

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Bob", DisplayName = "[Ck][Bob] Get Spec")
    static FCk_Bob_Spec
    Get_Spec(
        const FCk_Handle_Bob& InBob);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Bob", DisplayName = "[Ck][Bob] Get Gait")
    static FCk_Handle_Gait
    Get_Gait(
        const FCk_Handle_Bob& InBob);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Bob", DisplayName = "[Ck][Bob] Get Is Enabled")
    static bool
    Get_IsEnabled(
        const FCk_Handle_Bob& InBob);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Bob", DisplayName = "[Ck][Bob] Get Rest Offset")
    static FTransform
    Get_RestOffset(
        const FCk_Handle_Bob& InBob);

    // The bob-only offset (Compose_Offset of the current smoothed, clamped target), in the rest frame. Not the node offset.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Bob", DisplayName = "[Ck][Bob] Get Bob Offset")
    static FTransform
    Get_BobOffset(
        const FCk_Handle_Bob& InBob);

    // The vertical spring's offset (cm, +Z up) before intensity: air lift and the landing dip.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Bob", DisplayName = "[Ck][Bob] Get Spring Offset")
    static float
    Get_SpringOffset(
        const FCk_Handle_Bob& InBob);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Bob", DisplayName = "[Ck][Bob] Request Update Spec", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Bob
    Request_UpdateSpec(
        UPARAM(ref) FCk_Handle_Bob& InBob,
        const FCk_Request_Bob_UpdateSpec& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Bob", DisplayName = "[Ck][Bob] Request Enable/Disable", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Bob
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_Bob& InBob,
        const FCk_Request_Bob_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Bob", DisplayName = "[Ck][Bob] Request Reset", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Bob
    Request_Reset(
        UPARAM(ref) FCk_Handle_Bob& InBob,
        const FCk_Request_Bob_Reset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Bob", DisplayName = "[Ck][Bob] Request Set Rest Offset", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Bob
    Request_SetRestOffset(
        UPARAM(ref) FCk_Handle_Bob& InBob,
        const FCk_Request_Bob_SetRestOffset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
