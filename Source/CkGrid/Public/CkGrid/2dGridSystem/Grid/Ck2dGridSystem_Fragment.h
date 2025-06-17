#pragma once

#include "Ck2dGridSystem_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_2dGridSystem_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_2dGridSystem_Params = FCk_Fragment_2dGridSystem_ParamsData;
    using FFragment_2dGridSystem_Transform = FCk_Fragment_2dGridSystem_Transform;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGRID_API FFragment_2dGridSystem_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_2dGridSystem_Current);

    public:
        friend class UCk_Utils_2dGridSystem_UE;

    private:
        // Registry that contains all the grid cell entities
        FCk_Registry _CellRegistry;

    public:
        CK_PROPERTY_GET(_CellRegistry);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_2dGridSystem_Current, _CellRegistry);
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------