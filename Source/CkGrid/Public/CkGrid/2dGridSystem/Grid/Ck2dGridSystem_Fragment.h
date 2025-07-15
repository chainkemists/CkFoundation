#pragma once

#include "Ck2dGridSystem_Fragment_Data.h"

#include "CkEcs/SceneNode/CkSceneNode_Fragment_Data.h"
#include "CkEcs/Transform/CkTransform_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_2dGridSystem_Params = FCk_Fragment_2dGridSystem_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGRID_API FFragment_2dGridSystem_Transform
    {
    public:
        CK_GENERATED_BODY(FFragment_2dGridSystem_Transform);

    private:
        FTransform _Transform = FTransform::Identity;

    public:
        CK_PROPERTY(_Transform);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_2dGridSystem_Transform, _Transform);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGRID_API FFragment_2dGridSystem_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_2dGridSystem_Current);

    public:
        auto Request_CreateCellEntity() -> FCk_Handle;

    private:
        FCk_Registry _CellRegistry;
        FCk_Handle_SceneNode _Pivot;

    public:
        CK_PROPERTY_GET(_CellRegistry);
        CK_PROPERTY_GET(_Pivot);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_2dGridSystem_Current, _Pivot);
    };
}

// --------------------------------------------------------------------------------------------------------------------