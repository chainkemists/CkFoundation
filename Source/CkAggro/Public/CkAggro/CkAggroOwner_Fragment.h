#pragma once

#include "CkAggro/CkAggroOwner_Fragment_Data.h"
#include "CkAggro/CkAggro_Fragment_Data.h"

#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Entity_ConstructionScript_PDA;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKAGGRO_API, OnNewAggroAdded, FCk_Delegate_Aggro_OnNewAggroAdded_MC, FCk_Handle_AggroOwner, FCk_Handle_Aggro);
}

// --------------------------------------------------------------------------------------------------------------------
