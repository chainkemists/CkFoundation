#pragma once

#include "CkAggro/CkAggro_Fragment_Data.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Entity_ConstructionScript_PDA;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAggro, FCk_Handle_Aggro);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_Aggro);


    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(UAggroedEntity_Utils, AggroedEntity);
}

// --------------------------------------------------------------------------------------------------------------------
