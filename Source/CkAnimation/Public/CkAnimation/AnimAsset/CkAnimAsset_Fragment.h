#pragma once

#include "CkAnimAsset_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_AnimAsset_Params = FCk_AnimAsset_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfAnimAssets, FCk_Handle_AnimAsset);
}

// --------------------------------------------------------------------------------------------------------------------
