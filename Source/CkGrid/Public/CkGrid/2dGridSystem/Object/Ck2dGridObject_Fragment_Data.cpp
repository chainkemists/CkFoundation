#include "Ck2dGridObject_Fragment_Data.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-A (used directly as the occupant's GridObject params fragment).

CK_REGISTER_SNAPSHOTABLE(FCk_Fragment_2dGridObject_ParamsData);

// --------------------------------------------------------------------------------------------------------------------
