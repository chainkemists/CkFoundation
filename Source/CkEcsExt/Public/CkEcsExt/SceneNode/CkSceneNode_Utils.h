#pragma once

#include "CkSceneNode_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkSceneNode_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_SceneNode"))
class CKECSEXT_API UCk_Utils_SceneNode_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SceneNode_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_SceneNode);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SceneNode",
              DisplayName="[Ck][SceneNode] Add Feature")
    static FCk_Handle_SceneNode
    Add(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        UPARAM(ref) FCk_Handle_Transform& InAttachTo,
        FTransform InLocalTransform);

    // InLocalTransform is the offset from the owner (KeepRelative semantics): the node's world becomes
    // InLocalTransform * ownerWorld and tracks the owner as it moves. Identity == glued exactly to the
    // owner. Offset is runtime-mutable via Request_UpdateOffset.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SceneNode",
              DisplayName="[Ck][SceneNode] Create")
    static FCk_Handle_SceneNode
    Create(
        UPARAM(ref) FCk_Handle_Transform& InAttachTo,
        FTransform InLocalTransform);

    // InLocalTransform is the offset from the anchor (KeepRelative semantics): the node's world becomes
    // InLocalTransform * componentWorld and tracks the component as it moves. The node is a read-only
    // follower — it never writes back onto the component (unlike an actor-bridge Transform). Identity ==
    // glued exactly to the component. Offset is runtime-mutable via Request_UpdateOffset.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SceneNode",
              DisplayName="[Ck][SceneNode] Create (AttachTo Unreal Component)")
    static FCk_Handle_SceneNode
    CreateAndAttachToUnrealComponent(
        UPARAM(ref) FCk_Handle_Transform& InAttachTo,
        USceneComponent* InSceneComponent,
        FTransform InLocalTransform);

    // InLocalTransform is the offset from the socket (KeepRelative semantics): the node's world becomes
    // InLocalTransform * socketWorld and tracks the socket as it moves. Read-only follower; identity ==
    // glued exactly to the socket. Offset is runtime-mutable via Request_UpdateOffset.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SceneNode",
              DisplayName="[Ck][SceneNode] Create (AttachTo Unreal Mesh Socket)")
    static FCk_Handle_SceneNode
    CreateAndAttachToUnrealMesh(
        UPARAM(ref) FCk_Handle_Transform& InAttachTo,
        const UMeshComponent* InMeshComponent,
        FName InSocketName,
        FTransform InLocalTransform);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SceneNode",
              DisplayName="[Ck][SceneNode] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    static FCk_Handle_SceneNode
    DoAdd(
        FCk_Handle_Transform& InHandle,
        FCk_Handle_Transform& InAttachTo,
        FTransform InLocalTransform,
        ECk_SceneNode_DrivenBy InDrivenBy);

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
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SceneNode",
        DisplayName="[Ck][SceneNode] Get Offset")
    static FTransform
    Get_Offset(
        const FCk_Handle_SceneNode& InSceneNode);

    UFUNCTION(BlueprintPure,
    Category = "Ck|Utils|SceneNode",
        DisplayName = "[Ck][SceneNode] Get Parent")
    static FCk_Handle_Transform
    Get_Parent(
            const FCk_Handle_SceneNode& InSceneNode);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SceneNode",
        DisplayName="[Ck][SceneNode] Request UpdateOffset",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_SceneNode
    Request_UpdateOffset(
        UPARAM(ref) FCk_Handle_SceneNode& InSceneNode,
        const FCk_Request_SceneNode_UpdateRelativeTransform& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Severs the scene-node's parent link, keeping the current world transform. After detach,
    // TProcessor_SceneNode_Update no longer iterates this entity, so the Transform fragment stays where it
    // was and can be authoritatively set by downstream callers.
    // Immediate mutator: the links are removed inline and nothing is enqueued, so the completion
    // delegate fires synchronously on the caller's stack.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SceneNode",
        DisplayName="[Ck][SceneNode] Request Detach (Keep World)",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_SceneNode
    Request_Detach(
        UPARAM(ref) FCk_Handle_SceneNode& InSceneNode,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SceneNode",
              DisplayName="[Ck][SceneNode] For Each SceneNode",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate", KeyWords = "get,all,scenenodes"))
    static TArray<FCk_Handle_SceneNode>
    ForEach_SceneNode(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_SceneNode(
        FCk_Handle_Transform InHandle,
        const TFunction<void(FCk_Handle_SceneNode)>& InFunc) -> void;

private:
    static auto
    PropagateLayerToChildren(
        FCk_Handle_Transform& InParent,
        int32 InParentLayerIndex) -> void;

    static auto
    Get_LayerIndex(
        const FCk_Handle& InHandle) -> TOptional<int32>;

    static auto
    RemoveExistingLayerTag(
        FCk_Handle_Transform& InHandle) -> void;

    static auto
    AssignLayerByIndex(
        FCk_Handle_Transform& InHandle,
        int32 InLayerIndex) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------