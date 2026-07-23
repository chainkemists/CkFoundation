#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkDialog/Line/CkDialogLine_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Has/Cast key for a dialogue line entity.
    CK_DEFINE_ECS_TAG(FTag_DialogLine);

    // Marks a child entity as one of a line's condition-evaluation entities.
    CK_DEFINE_ECS_TAG(FTag_DialogCondition);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_DialogLine_Params = FCk_Fragment_DialogLine_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    // A line's condition-evaluation child entities. TRANSIENT: registry content is rebuilt from banks on load, never
    // round-tripped through a save. Entries are plain FCk_Handle (the condition entity is an EntityScript entity read
    // through FFragment_EntityScript_Current, not a typesafe-handle feature).
    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfDialogConditions, FCk_Handle);
}

// --------------------------------------------------------------------------------------------------------------------
