#pragma once

#include "CkAnimAsset_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKANIMATION_API FFragment_AnimAsset_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_AnimAsset_Params);

    public:
        using ParamsType = FCk_Fragment_AnimAsset_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_AnimAsset_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAnimAssets, FCk_Handle_AnimAsset);
}

// --------------------------------------------------------------------------------------------------------------------
