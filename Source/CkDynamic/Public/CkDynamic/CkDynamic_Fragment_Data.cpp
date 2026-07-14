#include "CkDynamic_Fragment_Data.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"
#include "CkEcs/Snapshot/CkSnapshot_Context.h"     // ck::FSnapshotContext::Snapshot_Handle (full type for the body)
#include "CkEcs/Snapshot/CkSnapshot_HandleWalk.h"  // ck::snapshot::RemapHandles (shared walker — lifted OUT of here)

#include <StructUtils/InstancedStruct.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_DynamicFragment_Data::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& InCtx)
    -> void
{
    // (1) Serialize the held struct's TYPE + data. The proxy archive has ArIsSaveGame=true, so non-handle fields
    // round-trip via their CPF_SaveGame flag; FCk_Handle fields write ZERO bytes here (FCk_Handle's _Entity /
    // _RegistryHandle are not SaveGame), so there is no double-serialize with step (2). On load this materializes
    // the struct + any arrays first, so (2) only rewrites the handle ids in place.
    _StructData.Serialize(InAr);

    // (2) Remap every FCk_Handle the held struct carries onto the restored entities (a no-op set of stale ids
    // otherwise -- see CkSnapshot_Context.h). GetMutableMemory is valid on both save (read) and load (write).
    // The walker was lifted to CkEcs (ck::snapshot::RemapHandles) so the v3 save capture shares it verbatim.
    ck::snapshot::RemapHandles(_StructData.GetScriptStruct(), _StructData.GetMutableMemory(), InAr, InCtx);
}

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_SNAPSHOTABLE(FCk_Fragment_DynamicFragment_Data);
