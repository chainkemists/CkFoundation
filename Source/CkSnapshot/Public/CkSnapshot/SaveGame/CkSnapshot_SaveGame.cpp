#include "CkSnapshot_SaveGame.h"

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Snapshot_SaveGame::
    Serialize(
        FArchive& InAr)
    -> void
{
    Super::Serialize(InAr);

    ck::snapshot::Serialize_BulkBytes(InAr, _SnapshotBytesV3);
}

// --------------------------------------------------------------------------------------------------------------------
