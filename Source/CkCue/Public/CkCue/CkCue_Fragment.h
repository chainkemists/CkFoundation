#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkRecord/Record/CkRecord_Utils.h"

/*─────────────────────────────────────────────────────────────────────────────┐
│                             FORWARD DECLARATIONS                             │
└─────────────────────────────────────────────────────────────────────────────*/

class UCk_Utils_Cue_UE;

/*─────────────────────────────────────────────────────────────────────────────┐
│                             ACTIVE CUES RECORD                               │
└─────────────────────────────────────────────────────────────────────────────*/

namespace ck
{
    CK_DEFINE_RECORD_OF_ENTITIES_AND_UTILS(ActiveCues_Utils, FFragment_RecordOfActiveCues, FCk_Handle);
}
