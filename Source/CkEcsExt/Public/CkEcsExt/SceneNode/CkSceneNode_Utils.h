#pragma once

#include "CkSceneNode_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkNet/CkNet_Utils.h"

#include "CkSceneNode_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKECSEXT_API UCk_Utils_SceneNode_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SceneNode_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_SceneNode);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SceneNode",
              DisplayName="[Ck][SceneNode] Create")
    static FCk_Handle_SceneNode
    Create(
        UPARAM(ref) FCk_Handle_Transform& InOwner,
        FTransform InLocalTransform);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SceneNode",
              DisplayName="[Ck][SceneNode] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SceneNode",
        DisplayName="[Ck][SceneNode] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_SceneNode
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SceneNode",
        DisplayName="[Ck][SceneNode] Handle -> SceneNode Handle",
        meta = (CompactNodeTitle = "<AsSceneNode>", BlueprintAutocast))
    static FCk_Handle_SceneNode
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid SceneNode Handle",
        Category = "Ck|Utils|SceneNode",
        meta = (CompactNodeTitle = "INVALID_SceneNodeHandle", Keywords = "make"))
    static FCk_Handle_SceneNode
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SceneNode",
        DisplayName="[Ck][SceneNode] Get Offset")
    static FTransform
    Get_Offset(
        const FCk_Handle_SceneNode& InSceneNode);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SceneNode",
        DisplayName="[Ck][SceneNode] Request UpdateOffset")
    static FCk_Handle_SceneNode
    Request_UpdateOffset(
        UPARAM(ref) FCk_Handle_SceneNode& InSceneNode,
        const FCk_Request_SceneNode_UpdateRelativeTransform& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------