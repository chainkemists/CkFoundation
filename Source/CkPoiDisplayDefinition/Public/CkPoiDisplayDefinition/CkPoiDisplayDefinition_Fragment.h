#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Tag/CkTag.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkPoiDisplayDefinition/CkPoiDisplayDefinition_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_PoiDisplayDefinition_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Plain, not counted: one parent, one source — see CLAUDE.md "The cascade contract"
    CK_DEFINE_ECS_TAG(FTag_PoiDisplayDefinition_ParentHidden);

    // Bind-once guard on the OWNER entity, so a second Create does not double-bind the cascade
    CK_DEFINE_ECS_TAG(FTag_PoiDisplayDefinition_CascadeBound);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_PoiDisplayDefinition_Params = FCk_Fragment_PoiDisplayDefinition_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfPoiDisplayDefinitions, FCk_Handle_PoiDisplayDefinition);
}

// --------------------------------------------------------------------------------------------------------------------
