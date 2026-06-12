#include "CkRenderTarget_Fragment_Data.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h"
#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(Tag_RenderTarget_CategoryName, TEXT("RenderTarget"));

// --------------------------------------------------------------------------------------------------------------------

// Tier-A: the RenderTarget params fragment round-trips via its SaveGame fields so the restored host can
// re-derive its drawable target + replication container (Setup/Construct is abstained on a load).
CK_REGISTER_SNAPSHOTABLE(FCk_Fragment_RenderTarget_ParamsData);

// --------------------------------------------------------------------------------------------------------------------
