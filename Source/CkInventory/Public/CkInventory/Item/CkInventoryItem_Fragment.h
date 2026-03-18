#pragma once

#include "CkInventoryItem_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKINVENTORY_API FFragment_InventoryItem_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_InventoryItem_Params);

    public:
        using ParamsType = FCk_Fragment_InventoryItem_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_InventoryItem_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINVENTORY_API FFragment_InventoryItem_Dimensions
    {
    public:
        CK_GENERATED_BODY(FFragment_InventoryItem_Dimensions);

    public:
        using ParamsType = FCk_Fragment_InventoryItem_DimensionsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_InventoryItem_Dimensions, _Params);
    };
}

// --------------------------------------------------------------------------------------------------------------------
