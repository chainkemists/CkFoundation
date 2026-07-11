#include "CkObjective_Fragment.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

#include "Serialization/Archive.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_ByteAttribute_Objective_Status, TEXT("ByteAttribute.Objective.Status"));
UE_DEFINE_GAMEPLAY_TAG(TAG_FloatAttribute_Objective_ProgressA, TEXT("FloatAttribute.Objective.ProgressA"));
UE_DEFINE_GAMEPLAY_TAG(TAG_FloatAttribute_Objective_ProgressB, TEXT("FloatAttribute.Objective.ProgressB"));
UE_DEFINE_GAMEPLAY_TAG(TAG_FloatAttribute_Objective_ProgressC, TEXT("FloatAttribute.Objective.ProgressC"));

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_SNAPSHOTABLE(FCk_Objective_ParamsData);

using FSnap_Objective_Current = ck::FFragment_Objective_Current;
CK_REGISTER_SNAPSHOTABLE(FSnap_Objective_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FFragment_Objective_Current::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& InCtx)
    -> void
{
    InCtx.Snapshot_Handle(InAr, _StatusAttribute);
    InAr << _CompletionTag;
    InAr << _FailureTag;
}

// --------------------------------------------------------------------------------------------------------------------