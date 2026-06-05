#include "CkLabel/CkLabel_Fragment.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h"
#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

#include <GameplayTagContainer.h>

// --------------------------------------------------------------------------------------------------------------------
// GameplayLabel snapshot registration lives in CkLabel (not CkEcs) to avoid a CkEcs->CkLabel cycle: CkLabel already
// depends on CkEcs and links GameplayTags, so FGameplayTag::StaticStruct() resolves here. Lets CkSnapshot drop both deps.

auto
    ck::FFragment_GameplayLabel::
    SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& /*InCtx*/)
    -> void
{
    FGameplayTag::StaticStruct()->SerializeItem(InAr, &_Label, /*Defaults=*/nullptr);
}

// --------------------------------------------------------------------------------------------------------------------

using FSnap_GameplayLabel = ck::FFragment_GameplayLabel;
CK_REGISTER_SNAPSHOTABLE(FSnap_GameplayLabel);

// --------------------------------------------------------------------------------------------------------------------
