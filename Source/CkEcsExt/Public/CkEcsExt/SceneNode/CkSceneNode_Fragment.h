#pragma once

#include "CkSceneNode_Fragment_Data.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_SceneNode_UE;
class FArchive;
class USceneComponent;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_SceneNode_RelativeTransformUpdated);

    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer0);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer1);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer2);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer3);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer4);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer5);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer6);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer7);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer8);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer9);

    // --------------------------------------------------------------------------------------------------------------------

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSEXT_API FFragment_SceneNode_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_SceneNode_Current);
        using IsSnapshotable = void;

    public:
        friend class FProcessor_SceneNode_Setup;
        friend class FProcessor_SceneNode_HandleRequests;
        friend class FProcessor_SceneNode_EndPlay;
        friend class FProcessor_SceneNode_FollowUnrealAnchor;
        friend class UCk_Utils_SceneNode_UE;

    private:
        FTransform _RelativeTransform;

    public:
        CK_PROPERTY(_RelativeTransform);

        CK_DEFINE_CONSTRUCTORS(FFragment_SceneNode_Current, _RelativeTransform);

    public:
        // Tier-C: the SceneNode's local offset is authoritative spatial state with no entity-handle ref.
        // Body in the .cpp (just the FTransform). Registered alongside SceneNodeParent/RecordOfSceneNodes so a
        // restored SceneNode keeps its relative transform instead of resetting to identity.
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& InCtx) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Foreign Unreal anchor for a scene-node that follows a USceneComponent (Socket == None) or a mesh
    // socket (Socket set) at the authored FFragment_SceneNode_Current offset. FProcessor_SceneNode_FollowUnrealAnchor
    // composes entity.world = offset * anchor.world every tick. Deliberately NOT the Transform module's
    // FFragment_Transform_RootComponent / _MeshSocket: those pair with SyncFromActor/SyncToActor (a bidirectional
    // actor bridge that would drag a Movable anchor). This keeps anchor-follow a read-only SceneNode concern and
    // leaves the Transform feature untouched. Not snapshotable — the live component ref can't be remapped
    // (parity with the Transform anchor fragments); the composed world pose is restored via FFragment_Transform.
    struct CKECSEXT_API FFragment_SceneNode_UnrealAnchor
    {
    public:
        CK_GENERATED_BODY(FFragment_SceneNode_UnrealAnchor);

    public:
        friend class FProcessor_SceneNode_FollowUnrealAnchor;
        friend class UCk_Utils_SceneNode_UE;

    public:
        FFragment_SceneNode_UnrealAnchor() = default;

        explicit
        FFragment_SceneNode_UnrealAnchor(
            const USceneComponent* InComponent,
            FName InSocket);

    private:
        TWeakObjectPtr<const USceneComponent> _Component;
        FName _Socket = NAME_None;

    public:
        CK_PROPERTY_GET(_Component);
        CK_PROPERTY_GET(_Socket);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSEXT_API FFragment_SceneNode_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_SceneNode_Requests);

    public:
        friend class FProcessor_SceneNode_HandleRequests;
        friend class UCk_Utils_SceneNode_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Request_SceneNode_UpdateRelativeTransform
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Snapshotable scene hierarchy: SceneNodeParent (the parent link) and FFragment_RecordOfSceneNodes (the
    // child list) are registered in CkSceneNode_Fragment.cpp, where the snapshot Archive types are complete.
    // Their stored FCk_Handle_Transform / FCk_Handle_SceneNode entries are remapped on restore via
    // FSnapshotContext, so a re-loaded SceneNode stays attached to its (remapped) parent and the parent keeps
    // its (remapped) children. (Authoritative state — nothing rebuilds SceneNodes on restore; Construct is not
    // re-run for a re-bridged entity.)
    CK_DEFINE_ENTITY_HOLDER_AND_UTILS_ROUNDTRIP(USceneNodeParent_Utils, SceneNodeParent, FCk_Handle_Transform);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_AND_UTILS_ROUNDTRIP(FUtils_RecordOfSceneNodes, FFragment_RecordOfSceneNodes, FCk_Handle_SceneNode);

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------