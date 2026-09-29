#pragma once

#include "CkSway/CkSway_Fragment.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Fragment_Data.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkSway_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Sway"))
class CKSWAY_API UCk_Utils_Sway_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Sway_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Sway);

    friend class UCk_Utils_Ecs_Base_UE;

public:
    // Takes ownership of an existing scene node's offset: the node's CURRENT offset becomes the rest pose and
    // from now on Sway writes SwayOffset * Rest. Works for parent-, anchor- and socket-driven nodes.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] Add Feature")
    static FCk_Handle_Sway
    Add(
        UPARAM(ref) FCk_Handle_SceneNode& InSceneNode,
        const FCk_Sway_Spec& InSpec);

    // Creates a parent-driven scene node under InParent at InLocalRest and adds Sway to it (Create = SceneNode::Create + Add).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] Create New Sway Node")
    static FCk_Handle_Sway
    Create(
        UPARAM(ref) FCk_Handle_Transform& InParent,
        FTransform InLocalRest,
        const FCk_Sway_Spec& InSpec);

    static auto Has(const FCk_Handle& InHandle) -> bool;

private:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] DoCast", meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Sway
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] DoCastChecked", meta = (CompactNodeTitle = "<AsSway>", BlueprintAutocast))
    static FCk_Handle_Sway
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Sway", DisplayName = "[Ck] Get Invalid Sway Handle",
        meta = (CompactNodeTitle = "INVALID_SwayHandle", Keywords = "make"))
    static FCk_Handle_Sway
    Get_InvalidHandle() { return {}; }

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] Get Spec")
    static FCk_Sway_Spec
    Get_Spec(
        const FCk_Handle_Sway& InSway);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] Get Is Enabled")
    static bool
    Get_IsEnabled(
        const FCk_Handle_Sway& InSway);

    // True when both channels are within the kernel epsilons of rest.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] Get Is Settled")
    static bool
    Get_IsSettled(
        const FCk_Handle_Sway& InSway);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] Get Rest Offset")
    static FTransform
    Get_RestOffset(
        const FCk_Handle_Sway& InSway);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] Get Location Offset")
    static FVector
    Get_LocationOffset(
        const FCk_Handle_Sway& InSway);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] Get Rotation Offset")
    static FRotator
    Get_RotationOffset(
        const FCk_Handle_Sway& InSway);

    // The spring-only offset (Compose_Offset of the two channels), in the rest frame. Not the node offset.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] Get Sway Offset")
    static FTransform
    Get_SwayOffset(
        const FCk_Handle_Sway& InSway);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] Get Last Stimulus")
    static FCk_Sway_Stimulus
    Get_LastStimulus(
        const FCk_Handle_Sway& InSway);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] Request Update Spec", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Sway
    Request_UpdateSpec(
        UPARAM(ref) FCk_Handle_Sway& InSway,
        const FCk_Request_Sway_UpdateSpec& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] Request Enable/Disable", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Sway
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_Sway& InSway,
        const FCk_Request_Sway_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] Request Reset", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Sway
    Request_Reset(
        UPARAM(ref) FCk_Handle_Sway& InSway,
        const FCk_Request_Sway_Reset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Sway", DisplayName = "[Ck][Sway] Request Set Rest Offset", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Sway
    Request_SetRestOffset(
        UPARAM(ref) FCk_Handle_Sway& InSway,
        const FCk_Request_Sway_SetRestOffset& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
