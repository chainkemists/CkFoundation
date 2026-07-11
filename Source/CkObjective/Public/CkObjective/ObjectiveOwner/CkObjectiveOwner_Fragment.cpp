#include "CkObjectiveOwner_Fragment.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_SNAPSHOTABLE(FCk_ObjectiveOwner_ParamsData);

using FSnap_ObjectiveOwner_Current = ck::FFragment_ObjectiveOwner_Current;
CK_REGISTER_SNAPSHOTABLE(FSnap_ObjectiveOwner_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FFragment_ObjectiveOwner_Current::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& InCtx)
    -> void
{
    InCtx.Snapshot_Handle(InAr, _ObjectivesEntityCollection);
}

// --------------------------------------------------------------------------------------------------------------------