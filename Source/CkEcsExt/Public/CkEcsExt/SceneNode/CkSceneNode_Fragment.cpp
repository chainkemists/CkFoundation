#include "CkSceneNode_Fragment.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
// The CK_REGISTER_SNAPSHOTABLE save/load closures call entt's get<T>(Archive&), so the Archive writer/reader
// types must be COMPLETE here (not just forward-declared as in the registry header). This is why snapshot
// registration lives in a .cpp, never a widely-included fragment header.
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

#include "Serialization/Archive.h"

#include "Components/SceneComponent.h"

// --------------------------------------------------------------------------------------------------------------------

ck::FFragment_SceneNode_UnrealAnchor::
    FFragment_SceneNode_UnrealAnchor(
        const USceneComponent* InComponent,
        FName InSocket)
    : _Component(InComponent)
    , _Socket(InSocket)
{
}

// --------------------------------------------------------------------------------------------------------------------
// ck:: types hoisted to file-scope aliases because CK_REGISTER_SNAPSHOTABLE token-pastes the type name into a
// generated identifier (the `::` cannot be pasted). Registering the SceneNode hierarchy makes a save/load
// preserve the parent link, the child list (both entity-handle-bearing — remapped via FSnapshotContext) and the
// relative transform; previously the whole hierarchy was silently dropped on restore.

using FSnap_SceneNodeParent      = ck::SceneNodeParent;                  // parent link  (EntityHolder<FCk_Handle_Transform>)
using FSnap_RecordOfSceneNodes   = ck::FFragment_RecordOfSceneNodes;     // child list   (RecordOfEntities<FCk_Handle_SceneNode>)
using FSnap_SceneNode_Current    = ck::FFragment_SceneNode_Current;      // relative transform (plain data)

CK_REGISTER_SNAPSHOTABLE(FSnap_SceneNodeParent);
CK_REGISTER_SNAPSHOTABLE(FSnap_RecordOfSceneNodes);
CK_REGISTER_SNAPSHOTABLE(FSnap_SceneNode_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FFragment_SceneNode_Current::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& /*InCtx*/)
    -> void
{
    InAr << _RelativeTransform;
}

// --------------------------------------------------------------------------------------------------------------------
