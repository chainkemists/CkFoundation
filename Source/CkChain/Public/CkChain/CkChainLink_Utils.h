#pragma once

#include "CkChain/CkChain_Fragment.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkChainLink_Utils.generated.h"

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_ChainLink"))
class CKCHAIN_API UCk_Utils_ChainLink_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ChainLink_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ChainLink);

    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|ChainLink", DisplayName = "[Ck][ChainLink] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|ChainLink", DisplayName = "[Ck][ChainLink] DoCast", meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ChainLink
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);


    UFUNCTION(BlueprintPure, Category = "Ck|Utils|ChainLink", DisplayName = "[Ck][ChainLink] DoCastChecked", meta = (CompactNodeTitle = "<AsChainLink>", BlueprintAutocast))
    static FCk_Handle_ChainLink
    DoCastChecked(
        FCk_Handle InHandle);


    UFUNCTION(BlueprintPure, Category = "Ck|Utils|ChainLink", DisplayName = "[Ck] Get Invalid ChainLink Handle",
        meta = (CompactNodeTitle = "INVALID_ChainLinkHandle", Keywords = "make"))
    static FCk_Handle_ChainLink
    Get_InvalidHandle() { return {}; }

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|ChainLink", DisplayName = "[Ck][ChainLink] Get Chain")
    static FCk_Handle_Chain
    Get_Chain(
        const FCk_Handle_ChainLink& InLink);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|ChainLink", DisplayName = "[Ck][ChainLink] Get Distance From Head")
    static float
    Get_DistanceFromHeadCm(
        const FCk_Handle_ChainLink& InLink);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|ChainLink", DisplayName = "[Ck][ChainLink] Get Index")
    static int32
    Get_Index(
        const FCk_Handle_ChainLink& InLink);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|ChainLink", DisplayName="[Ck][ChainLink] Get Local Offset")
    static FTransform
    Get_LocalOffset(
        const FCk_Handle_ChainLink& InLink);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|ChainLink", DisplayName="[Ck][ChainLink] Get Orientation")
    static ECk_Chain_LinkOrientation
    Get_Orientation(
        const FCk_Handle_ChainLink& InLink);

    // The pose the chain last published for this link (before the transform drain applied it); the link's own
    // current transform until the first publish. C++/debugger only.
    static auto
    Get_TargetPose(
        const FCk_Handle_ChainLink& InLink) -> FTransform;

    // True once the chain has driven this link at least once (Get_TargetPose then returns the driven pose, not the live transform).
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|ChainLink", DisplayName = "[Ck][ChainLink] Get Has Target Pose")
    static bool
    Get_HasTargetPose(
        const FCk_Handle_ChainLink& InLink);

    // True while a HoldUntilCovered link's arc target lies behind the oldest recorded sample. C++/debugger only.
    static auto
    Get_IsHeld(
        const FCk_Handle_ChainLink& InLink) -> bool;

};
