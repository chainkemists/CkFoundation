#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkDialog/Line/CkDialogLine_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_DialogLine);

    CK_DEFINE_ECS_TAG(FTag_DialogCondition);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_DialogLine_Params = FCk_DialogLine_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    // TRANSIENT: registry content is rebuilt from banks on load, never round-tripped through a save. Entries are
    // plain FCk_Handle — a condition is an EntityScript entity, not a typesafe-handle feature.
    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfDialogConditions, FCk_Handle);
}

// --------------------------------------------------------------------------------------------------------------------
