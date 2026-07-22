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
    // Plain (NOT counted): a display-definition child has exactly ONE parent, so "parent is hidden" has a single
    // source — there is nothing to reference-count. The counted-tag complexity lives in CkVisibleRange's own
    // FTag_VisibleRange_Hidden (two independent vote sources); this cascade tag is deliberately simpler. Consumers
    // (Gate 4 projectors) exclude BOTH FTag_VisibleRange_Hidden and this tag.
    CK_DEFINE_ECS_TAG(FTag_PoiDisplayDefinition_ParentHidden);

    // Bind-once guard on the OWNER entity: set the first time Create wires the owner's OnHiddenChanged cascade so a
    // second Create on the same owner does not double-bind.
    CK_DEFINE_ECS_TAG(FTag_PoiDisplayDefinition_CascadeBound);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_PoiDisplayDefinition_Params = FCk_Fragment_PoiDisplayDefinition_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    // TRANSIENT documents intent: display-definition children carry only static config and rebuild from their owner's
    // Construct recipe on load — they are never round-tripped through a save. (The _ROUNDTRIP/_TRANSIENT policy split
    // is inert post-Model-A purge; the name is the surviving documentation.)
    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfPoiDisplayDefinitions, FCk_Handle_PoiDisplayDefinition);
}

// --------------------------------------------------------------------------------------------------------------------
