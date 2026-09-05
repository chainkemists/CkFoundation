#include "CkSceneNode_Utils.h"

#include "CkEcsExt/CkEcsExt_Log.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Handle/CkHandle_ReadOnly.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "Components/SceneComponent.h"
#include "Components/MeshComponent.h"

// --------------------------------------------------------------------------------------------------------------------

static auto
Get_SceneNodeAnchorLabel(
    const FCk_Handle& InHandle) -> FString
{
    const auto DebugName = UCk_Utils_Handle_UE::Get_DebugName(InHandle);
    if (NOT DebugName.IsNone())
    { return DebugName.ToString(); }

    return ck::Format_UE(TEXT("#{}"), InHandle.Get_Entity().Get_EntityNumber());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FUtils_SceneNodePropagation::
    Queue(
        FCk_Handle& InSceneNode)
    -> void
{
    if (ck::Is_NOT_Valid(InSceneNode) ||
        NOT InSceneNode.Has_All<ck::SceneNodeParent, ck::FFragment_SceneNode_Current>() ||
        InSceneNode.Has<ck::FFragment_SceneNode_UnrealAnchor>())
    { return; }

    auto& State = InSceneNode.AddOrGet<ck::FFragment_SceneNode_PropagationState>();
    ++State._QueueGeneration;
    InSceneNode.AddOrGet<ck::FTag_SceneNode_PropagationQueued>();
}

auto
    ck::FUtils_SceneNodePropagation::
    DeferConsume(
        const FCk_Handle_ReadOnly& InSceneNode,
        uint64 InGeneration)
    -> void
{
    InSceneNode.DeferCustom([InGeneration](FCk_Handle& InDeferredSceneNode)
    {
        if (ck::Is_NOT_Valid(InDeferredSceneNode) ||
            NOT InDeferredSceneNode.Has<ck::FFragment_SceneNode_PropagationState>())
        { return; }

        const auto& State = InDeferredSceneNode.Get<ck::FFragment_SceneNode_PropagationState>();
        if (State.Get_QueueGeneration() == InGeneration)
        { InDeferredSceneNode.Try_Remove<ck::FTag_SceneNode_PropagationQueued>(); }
    });
}

auto
    ck::FUtils_SceneNodePropagation::
    PublishChildrenIfChanged(
        FCk_Handle& InParent,
        const FTransform& InWorldTransform)
    -> void
{
    if (ck::Is_NOT_Valid(InParent) || NOT InParent.Has<ck::FFragment_RecordOfSceneNodes>())
    { return; }

    auto& State = InParent.AddOrGet<ck::FFragment_SceneNode_PropagationState>();
    if (State._HasPublishedWorldTransform && State._LastPublishedWorldTransform.Equals(InWorldTransform))
    { return; }

    State._LastPublishedWorldTransform = InWorldTransform;
    State._HasPublishedWorldTransform = true;

    ck::FUtils_RecordOfSceneNodes::ForEach_ValidEntry(InParent, [](FCk_Handle_SceneNode InChild)
    {
        Queue(InChild);
    });
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Utils_SceneNode_UE::
    Log_ProvenanceCensus(
        const FCk_Handle& InContext,
        const FString& InLabel,
        int32 InTopN)
{
#if !UE_BUILD_SHIPPING
    if (NOT ck::IsValid(InContext))
    {
        UE_LOG(CkEcsExt, Display, TEXT("[SceneNodeCensus] label=\"%s\" invalid context"), *InLabel);
        return;
    }

    const auto TopN = FMath::Clamp(InTopN, 1, 20);
    const auto Registry = InContext.Get_RegistryView();

    struct FCounts
    {
        int32 Total = 0;
        int32 Layers[3] = {};
        int32 UnrealComponent = 0;
        int32 UnrealMeshSocket = 0;
        int32 RootComponent = 0;
        int32 MeshSocket = 0;
        int32 Bare = 0;
        int32 ExternallyDriven = 0;
        int32 UnrealComponentExternal = 0;
        int32 UnrealMeshSocketExternal = 0;
        int32 RootComponentExternal = 0;
        int32 MeshSocketExternal = 0;
        int32 BareExternal = 0;
        int32 PropagationState = 0;
        int32 PropagationQueued = 0;
    } Counts;

    auto CompositeBuckets = TMap<FString, int32>{};
    auto NodeMarginals = TMap<FString, int32>{};
    auto ParentMarginals = TMap<FString, int32>{};

    const auto GetName = [](const FCk_Handle& InHandle) -> FString
    {
        if (NOT ck::IsValid(InHandle))
        { return TEXT("<invalid>"); }

        const auto Name = UCk_Utils_Handle_UE::Get_DebugName(InHandle);
        return Name.IsNone()
            ? ck::Format_UE(TEXT("#{}"), InHandle.Get_Entity().Get_EntityNumber())
            : Name.ToString();
    };

    const auto CensusNode = [&](const FCk_Entity InEntity,
                                const ck::SceneNodeParent& InParent,
                                const int32 InLayer)
    {
        const auto Node = FCk_Handle{InEntity, Registry.Get_RegistryHandle()};
        const auto Parent = FCk_Handle{InParent.Get_Entity().Get_Entity(), Registry.Get_RegistryHandle()};
        const auto NodeName = GetName(Node);
        const auto ParentName = GetName(Parent);
        const auto IsExternallyDriven = Node.Has<ck::FTag_Transform_ExternallyDriven>();

        auto AnchorKind = FString{TEXT("Bare")};
        if (Node.Has<ck::FFragment_SceneNode_UnrealAnchor>())
        {
            const auto& Anchor = Node.Get<ck::FFragment_SceneNode_UnrealAnchor>();
            AnchorKind = Anchor.Get_Socket().IsNone() ? TEXT("UnrealComponent") : TEXT("UnrealMeshSocket");
        }
        else if (Node.Has<ck::FFragment_Transform_RootComponent>())
        { AnchorKind = TEXT("RootComponent"); }
        else if (Node.Has<ck::FFragment_Transform_MeshSocket>())
        { AnchorKind = TEXT("MeshSocket"); }

        ++Counts.Total;
        ++Counts.Layers[InLayer];
        if (IsExternallyDriven)
        { ++Counts.ExternallyDriven; }
        if (Node.Has<ck::FFragment_SceneNode_PropagationState>())
        { ++Counts.PropagationState; }
        if (Node.Has<ck::FTag_SceneNode_PropagationQueued>())
        { ++Counts.PropagationQueued; }

        if (AnchorKind == TEXT("UnrealComponent"))
        {
            ++Counts.UnrealComponent;
            Counts.UnrealComponentExternal += IsExternallyDriven ? 1 : 0;
        }
        else if (AnchorKind == TEXT("UnrealMeshSocket"))
        {
            ++Counts.UnrealMeshSocket;
            Counts.UnrealMeshSocketExternal += IsExternallyDriven ? 1 : 0;
        }
        else if (AnchorKind == TEXT("RootComponent"))
        {
            ++Counts.RootComponent;
            Counts.RootComponentExternal += IsExternallyDriven ? 1 : 0;
        }
        else if (AnchorKind == TEXT("MeshSocket"))
        {
            ++Counts.MeshSocket;
            Counts.MeshSocketExternal += IsExternallyDriven ? 1 : 0;
        }
        else
        {
            ++Counts.Bare;
            Counts.BareExternal += IsExternallyDriven ? 1 : 0;
        }

        ++CompositeBuckets.FindOrAdd(FString::Printf(TEXT("L%d|%s|%s|%s|external=%d"),
            InLayer, *NodeName, *ParentName, *AnchorKind, IsExternallyDriven ? 1 : 0));
        ++NodeMarginals.FindOrAdd(FString::Printf(TEXT("L%d|%s"), InLayer, *NodeName));
        ++ParentMarginals.FindOrAdd(FString::Printf(TEXT("L%d|%s"), InLayer, *ParentName));
    };

    Registry.View<ck::FTag_SceneNode_Layer0, ck::FFragment_SceneNode_Current, ck::SceneNodeParent,
                  ck::FFragment_Transform, ck::TExclude<ck::FTag_DestroyEntity_Initiate>>().ForEach(
        [&](const FCk_Entity Entity, const auto&, const ck::SceneNodeParent& Parent, const auto&)
        { CensusNode(Entity, Parent, 0); });
    Registry.View<ck::FTag_SceneNode_Layer1, ck::FFragment_SceneNode_Current, ck::SceneNodeParent,
                  ck::FFragment_Transform, ck::TExclude<ck::FTag_DestroyEntity_Initiate>>().ForEach(
        [&](const FCk_Entity Entity, const auto&, const ck::SceneNodeParent& Parent, const auto&)
        { CensusNode(Entity, Parent, 1); });
    Registry.View<ck::FTag_SceneNode_Layer2, ck::FFragment_SceneNode_Current, ck::SceneNodeParent,
                  ck::FFragment_Transform, ck::TExclude<ck::FTag_DestroyEntity_Initiate>>().ForEach(
        [&](const FCk_Entity Entity, const auto&, const ck::SceneNodeParent& Parent, const auto&)
        { CensusNode(Entity, Parent, 2); });

    const auto LogTop = [TopN, &InLabel](const TCHAR* InKind, const TMap<FString, int32>& InBuckets)
    {
        auto Rows = TArray<TPair<FString, int32>>{};
        Rows.Reserve(InBuckets.Num());
        for (const auto& Pair : InBuckets)
        { Rows.Emplace(Pair.Key, Pair.Value); }
        Rows.Sort([](const TPair<FString, int32>& A, const TPair<FString, int32>& B)
        {
            return A.Value != B.Value ? A.Value > B.Value : A.Key < B.Key;
        });

        const auto NumRows = FMath::Min(TopN, Rows.Num());
        auto ShownCount = int32{0};
        for (int32 Index = 0; Index < NumRows; ++Index)
        {
            const auto& Row = Rows[Index];
            ShownCount += Row.Value;
            UE_LOG(CkEcsExt, Display, TEXT("[SceneNodeCensus] label=\"%s\" %s rank=%d count=%d key=\"%s\""),
                *InLabel, InKind, Index + 1, Row.Value, *Row.Key);
        }

        auto TotalCount = int32{0};
        for (const auto& Pair : InBuckets)
        { TotalCount += Pair.Value; }
        UE_LOG(CkEcsExt, Display,
            TEXT("[SceneNodeCensus] label=\"%s\" %s distinct=%d shown=%d shownCount=%d otherCount=%d"),
            *InLabel, InKind, Rows.Num(), NumRows, ShownCount, TotalCount - ShownCount);
    };

    UE_LOG(CkEcsExt, Display,
        TEXT("[SceneNodeCensus] label=\"%s\" total=%d L0=%d L1=%d L2=%d anchors={UnrealComponent=%d UnrealMeshSocket=%d RootComponent=%d MeshSocket=%d Bare=%d} externallyDriven=%d propagation={state=%d queued=%d} topN=%d"),
        *InLabel, Counts.Total, Counts.Layers[0], Counts.Layers[1], Counts.Layers[2],
        Counts.UnrealComponent, Counts.UnrealMeshSocket, Counts.RootComponent, Counts.MeshSocket, Counts.Bare,
        Counts.ExternallyDriven, Counts.PropagationState, Counts.PropagationQueued, TopN);
    UE_LOG(CkEcsExt, Display,
        TEXT("[SceneNodeCensus] label=\"%s\" anchorExternal={UnrealComponent=%d/%d UnrealMeshSocket=%d/%d RootComponent=%d/%d MeshSocket=%d/%d Bare=%d/%d}"),
        *InLabel,
        Counts.UnrealComponentExternal, Counts.UnrealComponent,
        Counts.UnrealMeshSocketExternal, Counts.UnrealMeshSocket,
        Counts.RootComponentExternal, Counts.RootComponent,
        Counts.MeshSocketExternal, Counts.MeshSocket,
        Counts.BareExternal, Counts.Bare);
    LogTop(TEXT("composite"), CompositeBuckets);
    LogTop(TEXT("node"), NodeMarginals);
    LogTop(TEXT("parent"), ParentMarginals);
#else
    static_cast<void>(InContext);
    static_cast<void>(InLabel);
    static_cast<void>(InTopN);
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SceneNode_UE::
    Add(
        FCk_Handle_Transform& InHandle,
        FCk_Handle_Transform& InAttachTo,
        FTransform InLocalTransform)
    -> FCk_Handle_SceneNode
{
    // Parent-driven: anchor-bound children follow the scene-node parent like Unreal AttachToComponent.
    return DoAdd(InHandle, InAttachTo, InLocalTransform, ECk_SceneNode_DrivenBy::Parent);
}

auto
    UCk_Utils_SceneNode_UE::
    Create(
        FCk_Handle_Transform& InOwner,
        FTransform InLocalTransform)
    -> FCk_Handle_SceneNode
{
    auto SceneNodeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    UCk_Utils_Handle_UE::Set_DebugName(SceneNodeEntity, *ck::Format_UE(TEXT("SceneNode({})"), Get_SceneNodeAnchorLabel(InOwner)));

    const auto& OwnerTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InOwner);

    auto SceneNodeWithTransform = UCk_Utils_Transform_UE::Add(SceneNodeEntity, InLocalTransform * OwnerTransform, ECk_Replication::DoesNotReplicate);

    return DoAdd(SceneNodeWithTransform, InOwner, InLocalTransform, ECk_SceneNode_DrivenBy::Anchor);
}

auto
    UCk_Utils_SceneNode_UE::
    CreateAndAttachToUnrealComponent(
        FCk_Handle_Transform& InAttachTo,
        USceneComponent* InSceneComponent,
        FTransform InLocalTransform)
    -> FCk_Handle_SceneNode
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSceneComponent),
        TEXT("Unable to attach SceneNode to [{}]: the Unreal SceneComponent is INVALID"), InAttachTo)
    { return {}; }

    auto SceneNodeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttachTo);
    UCk_Utils_Handle_UE::Set_DebugName(SceneNodeEntity, *ck::Format_UE(TEXT("SceneNode({} > {})"), Get_SceneNodeAnchorLabel(InAttachTo), GetNameSafe(InSceneComponent)));

    // Read-only follower: a plain Transform seeded at the composed world plus the SceneNode-owned anchor
    // fragment — NOT FFragment_Transform_RootComponent, so SyncFromActor / SyncToActor never engage.
    auto SceneNodeWithTransform = UCk_Utils_Transform_UE::Add(SceneNodeEntity, InLocalTransform * InSceneComponent->GetComponentTransform(), ECk_Replication::DoesNotReplicate);
    SceneNodeWithTransform.Add<ck::FFragment_SceneNode_UnrealAnchor>(InSceneComponent, NAME_None);

    return DoAdd(SceneNodeWithTransform, InAttachTo, InLocalTransform, ECk_SceneNode_DrivenBy::Anchor);
}

auto
    UCk_Utils_SceneNode_UE::
    CreateAndAttachToUnrealMesh(
        FCk_Handle_Transform& InAttachTo,
        const UMeshComponent* InMeshComponent,
        FName InSocketName,
        FTransform InLocalTransform)
    -> FCk_Handle_SceneNode
{
    CK_ENSURE_IF_NOT(ck::IsValid(InMeshComponent),
        TEXT("Unable to attach SceneNode to [{}]: the Unreal MeshComponent is INVALID"), InAttachTo)
    { return {}; }

    CK_ENSURE_IF_NOT(InMeshComponent->DoesSocketExist(InSocketName),
        TEXT("Socket [{}] does NOT exist on MeshComponent [{}]. If you wanted the component's world transform "
             "(no socket), use CreateAndAttachToUnrealComponent instead; otherwise check the name for typos."),
        InSocketName, InMeshComponent)
    { return {}; }

    auto SceneNodeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttachTo);
    UCk_Utils_Handle_UE::Set_DebugName(SceneNodeEntity, *ck::Format_UE(TEXT("SceneNode({} > {}:{})"), Get_SceneNodeAnchorLabel(InAttachTo), GetNameSafe(InMeshComponent), InSocketName));

    // Read-only follower (see CreateAndAttachToUnrealComponent for the fragment rationale).
    auto SceneNodeWithTransform = UCk_Utils_Transform_UE::Add(SceneNodeEntity, InLocalTransform * InMeshComponent->GetSocketTransform(InSocketName), ECk_Replication::DoesNotReplicate);
    SceneNodeWithTransform.Add<ck::FFragment_SceneNode_UnrealAnchor>(InMeshComponent, InSocketName);

    return DoAdd(SceneNodeWithTransform, InAttachTo, InLocalTransform, ECk_SceneNode_DrivenBy::Anchor);
}

auto
    UCk_Utils_SceneNode_UE::
    DoAdd(
        FCk_Handle_Transform& InHandle,
        FCk_Handle_Transform& InAttachTo,
        FTransform InLocalTransform,
        ECk_SceneNode_DrivenBy InDrivenBy)
    -> FCk_Handle_SceneNode
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("InHandle [{}] is INVALID. Unable to proceed with SceneNode attaching to [{}]"), InHandle, InAttachTo)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InAttachTo),
        TEXT("InAttachTo [{}] is INVALID. Unable to proceed with adding the SceneNode feature to [{}]"), InAttachTo, InHandle)
    { return {}; }

    const auto ParentLayerIndex = Get_LayerIndex(InAttachTo);
    const auto MyLayerIndex = ParentLayerIndex.IsSet() ? ParentLayerIndex.GetValue() + 1 : 0;

    if (NOT AssignLayerByIndex(InHandle, MyLayerIndex))
    { return {}; }

    PropagateLayerToChildren(InHandle, MyLayerIndex);

    InHandle.Add<ck::FFragment_SceneNode_Current>(InLocalTransform);

    ck::USceneNodeParent_Utils::AddOrReplace(InHandle, InAttachTo);
    InHandle.AddOrGet<ck::FFragment_SceneNode_PropagationState>();
    InAttachTo.AddOrGet<ck::FFragment_SceneNode_PropagationState>();
    ck::FUtils_SceneNodePropagation::Queue(InHandle);

    if (InDrivenBy == ECk_SceneNode_DrivenBy::Parent)
    {
        const auto HasAnchor = InHandle.Has_Any<ck::FFragment_Transform_RootComponent, ck::FFragment_Transform_MeshSocket>();
        const auto AnchorIsMovable = InHandle.Has<ck::FTag_Transform_Movable>();

        CK_ENSURE_IF_NOT(NOT HasAnchor || AnchorIsMovable,
            TEXT("SceneNode::Add on entity [{}]: child has an anchor (RootComponent / MeshSocket) that is "
                 "NOT Movable. The parent-driven transform will update the ECS Transform fragment but "
                 "FProcessor_Transform_SyncToActor (which requires FTag_Transform_Movable) will NOT push "
                 "it onto the anchor — the visible actor will stay glued to its spawn pose. Set the "
                 "RootComponent's Mobility to EComponentMobility::Movable, or use a Create*-flavored "
                 "overload if you wanted anchor-authoritative behavior."),
            InHandle)
        { }

        InHandle.AddOrGet<ck::FTag_Transform_ExternallyDriven>();
    }

    auto SceneNodeHandle = Cast(InHandle);

    ck::FUtils_RecordOfSceneNodes::AddIfMissing(InAttachTo);
    ck::FUtils_RecordOfSceneNodes::Request_Connect(InAttachTo, SceneNodeHandle, ECk_Record_LabelRequirementPolicy::Optional);

    return SceneNodeHandle;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_SceneNode_UE, FCk_Handle_SceneNode,
    ck::SceneNodeParent, ck::FFragment_SceneNode_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SceneNode_UE::
    Get_Offset(
        const FCk_Handle_SceneNode& InSceneNode)
    -> FTransform
{
    return InSceneNode.Get<ck::FFragment_SceneNode_Current>().Get_RelativeTransform();
}

auto
    UCk_Utils_SceneNode_UE::
    Get_Parent(
        const FCk_Handle_SceneNode& InSceneNode)
    -> FCk_Handle_Transform
{
    return ck::USceneNodeParent_Utils::Get_StoredEntity_AsTypeSafe<FCk_Handle_Transform>(InSceneNode);
}

auto
    UCk_Utils_SceneNode_UE::
    Request_UpdateOffset(
        FCk_Handle_SceneNode& InSceneNode,
        const FCk_Request_SceneNode_UpdateRelativeTransform& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_SceneNode
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InSceneNode.AddOrGet<ck::FFragment_SceneNode_Requests>()._Requests.Emplace(InRequest);
    return InSceneNode;
}

auto
    UCk_Utils_SceneNode_UE::
    Request_Detach(
        FCk_Handle_SceneNode& InSceneNode,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_SceneNode
{
    const auto SceneNodeIsValid = ck::IsValid(InSceneNode);

    CK_ENSURE_IF_NOT(SceneNodeIsValid,
        TEXT("InSceneNode [{}] is INVALID. Unable to detach"), InSceneNode)
    {
        InDelegate.ExecuteIfBound(InSceneNode, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InSceneNode;
    }

    // The Transform fragment already holds the composed world pose, so removing parent / current / layer
    // below stops further composition and the world transform stays put without an explicit write.
    auto NodeAsTransform = UCk_Utils_Transform_UE::Cast(InSceneNode);

    auto Parent = ck::USceneNodeParent_Utils::Get_StoredEntity_AsTypeSafe<FCk_Handle_Transform>(InSceneNode);
    if (ck::IsValid(Parent))
    { ck::FUtils_RecordOfSceneNodes::Request_Disconnect(Parent, InSceneNode); }

    InSceneNode.Try_Remove<ck::SceneNodeParent>();
    InSceneNode.Try_Remove<ck::FFragment_SceneNode_Current>();
    InSceneNode.Try_Remove<ck::FTag_SceneNode_PropagationQueued>();

    RemoveExistingLayerTag(NodeAsTransform);
    NodeAsTransform.Try_Remove<ck::FTag_Transform_ExternallyDriven>();

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InSceneNode, ECk_Request_OperationResult::Succeeded);

    return InSceneNode;
}

auto
    UCk_Utils_SceneNode_UE::
    ForEach_SceneNode(
        FCk_Handle_Transform& InHandle,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_SceneNode>
{
    auto Abilities = TArray<FCk_Handle_SceneNode>{};

    ForEach_SceneNode
    (
        InHandle,
        [&](const FCk_Handle_SceneNode& InSceneNode)
        {
            if (InDelegate.IsBound())
            { InDelegate.Execute(InSceneNode, InOptionalPayload); }
            else
            { Abilities.Emplace(InSceneNode); }
        }
    );

    return Abilities;
}

auto
    UCk_Utils_SceneNode_UE::
    ForEach_SceneNode(
        FCk_Handle_Transform InHandle,
        const TFunction<void(FCk_Handle_SceneNode)>& InFunc)
    -> void
{
    ck::FUtils_RecordOfSceneNodes::ForEach_ValidEntry(InHandle, InFunc);
}

auto
    UCk_Utils_SceneNode_UE::
    Get_LayerIndex(
        const FCk_Handle& InHandle)
    -> TOptional<int32>
{
    if (InHandle.Has<ck::FTag_SceneNode_Layer0>())
    { return 0; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer1>())
    { return 1; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer2>())
    { return 2; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer3>())
    { return 3; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer4>())
    { return 4; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer5>())
    { return 5; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer6>())
    { return 6; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer7>())
    { return 7; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer8>())
    { return 8; }
    if (InHandle.Has<ck::FTag_SceneNode_Layer9>())
    { return 9; }
    return {};
}

auto
    UCk_Utils_SceneNode_UE::
    RemoveExistingLayerTag(
        FCk_Handle_Transform& InHandle)
    -> void
{
    InHandle.Try_Remove<ck::FTag_SceneNode_RelativeTransformUpdated>();

    InHandle.Try_Remove<ck::FTag_SceneNode_Layer0>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer1>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer2>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer3>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer4>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer5>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer6>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer7>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer8>();
    InHandle.Try_Remove<ck::FTag_SceneNode_Layer9>();
}

auto
    UCk_Utils_SceneNode_UE::
    AssignLayerByIndex(
        FCk_Handle_Transform& InHandle,
        int32 InLayerIndex)
    -> bool
{
    InHandle.AddOrGet<ck::FTag_SceneNode_RelativeTransformUpdated>();

    switch (InLayerIndex)
    {
        case 0: InHandle.Add<ck::FTag_SceneNode_Layer0>(); return true;
        case 1: InHandle.Add<ck::FTag_SceneNode_Layer1>(); return true;
        case 2: InHandle.Add<ck::FTag_SceneNode_Layer2>(); return true;
        case 3: InHandle.Add<ck::FTag_SceneNode_Layer3>(); return true;
        case 4: InHandle.Add<ck::FTag_SceneNode_Layer4>(); return true;
        case 5: InHandle.Add<ck::FTag_SceneNode_Layer5>(); return true;
        case 6: InHandle.Add<ck::FTag_SceneNode_Layer6>(); return true;
        case 7: InHandle.Add<ck::FTag_SceneNode_Layer7>(); return true;
        case 8: InHandle.Add<ck::FTag_SceneNode_Layer8>(); return true;
        case 9: InHandle.Add<ck::FTag_SceneNode_Layer9>(); return true;
        default:
        {
            CK_TRIGGER_ENSURE(TEXT("Layer index [{}] exceeds maximum supported SceneNode layers"), InLayerIndex);
            return false;
        }
    }
}

auto
    UCk_Utils_SceneNode_UE::
    PropagateLayerToChildren(
        FCk_Handle_Transform& InParent,
        int32 InParentLayerIndex)
    -> void
{
    const auto ChildLayerIndex = InParentLayerIndex + 1;

    ForEach_SceneNode(InParent, [ChildLayerIndex](FCk_Handle_SceneNode InChild)
    {
        if (const auto& CurrentLayerIndex = Get_LayerIndex(InChild);
            CurrentLayerIndex.IsSet() && CurrentLayerIndex.GetValue() == ChildLayerIndex)
        { return; }

        auto ChildTransform = UCk_Utils_Transform_UE::Cast(InChild);

        RemoveExistingLayerTag(ChildTransform);

        if (NOT AssignLayerByIndex(ChildTransform, ChildLayerIndex))
        { return; }

        ChildTransform.AddOrGet<ck::FFragment_SceneNode_PropagationState>();
        ck::FUtils_SceneNodePropagation::Queue(ChildTransform);

        PropagateLayerToChildren(ChildTransform, ChildLayerIndex);
    });
}

// --------------------------------------------------------------------------------------------------------------------
