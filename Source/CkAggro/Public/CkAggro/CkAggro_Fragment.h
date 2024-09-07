#pragma once

#include "CkAggro/CkAggro_Fragment_Data.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Entity_ConstructionScript_PDA;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAggro, FCk_Handle_Aggro);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_Aggro);


    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(UAggroedEntity_Utils, AggroedEntity);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKAGGRO_API, OnNewAggroAdded, FCk_Delegate_Aggro_OnNewAggroAdded_MC, FCk_Handle, FCk_Handle_Aggro);
}

// --------------------------------------------------------------------------------------------------------------------
