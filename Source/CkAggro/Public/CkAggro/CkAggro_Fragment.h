#pragma once

#include "CkAggro/CkAggro_Fragment_Data.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Entity_ConstructionScript_PDA;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_RECORD_OF_ENTITIES_AND_UTILS(FUtils_RecordOfAggros, FFragment_RecordOfAggro, FCk_Handle_Aggro);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_Aggro);
    CK_DEFINE_ECS_TAG(FTag_Aggro_Excluded);


    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(UAggroedEntity_Utils, AggroedEntity);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAGGRO_API FFragment_AggroOwner_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_AggroOwner_Params);

    public:
        using ParamsType = FCk_Fragment_AggroOwner_Params;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);
        CK_DEFINE_CONSTRUCTORS(FFragment_AggroOwner_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAGGRO_API FFragment_AggroOwner_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_AggroOwner_Current);

    private:
        FCk_Handle_Aggro _BestAggro;

    public:
        CK_PROPERTY_GET(_BestAggro);
        CK_DEFINE_CONSTRUCTORS(FFragment_AggroOwner_Current, _BestAggro);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAGGRO_API FFragment_Aggro_Current
    {
    private:
        double _Score = std::numeric_limits<float>::max();

    public:
        CK_PROPERTY_GET(_Score);

        CK_DEFINE_CONSTRUCTORS(FFragment_Aggro_Current, _Score);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKAGGRO_API, OnNewAggroAdded, FCk_Delegate_Aggro_OnNewAggroAdded_MC, FCk_Handle, FCk_Handle_Aggro);
}

// --------------------------------------------------------------------------------------------------------------------
