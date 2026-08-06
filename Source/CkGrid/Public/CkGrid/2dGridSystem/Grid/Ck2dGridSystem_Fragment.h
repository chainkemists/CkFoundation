#pragma once

#include "Ck2dGridSystem_Fragment_Data.h"

#include "CkEcs/World/CkEcsWorld.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Fragment_Data.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "Templates/UniquePtr.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_2dGridSystem_Params = FCk_2dGridSystem_Spec;

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
        FFragment_2dGridSystem_Current();
        explicit FFragment_2dGridSystem_Current(FCk_Handle_SceneNode InPivot);

        // Heap-owned because FEcsWorld is non-copyable / non-movable. Allocated
        // in the parametrized ctor so each grid entity gets its own private
        // entt registry (matches pre-generational-handle-migration behaviour
        // where FCk_Registry's default ctor auto-allocated).
        FFragment_2dGridSystem_Current(FFragment_2dGridSystem_Current&&) = default;
        FFragment_2dGridSystem_Current& operator=(FFragment_2dGridSystem_Current&&) = default;
        FFragment_2dGridSystem_Current(const FFragment_2dGridSystem_Current&) = delete;
        FFragment_2dGridSystem_Current& operator=(const FFragment_2dGridSystem_Current&) = delete;

    public:
        auto Request_CreateCellEntity() -> FCk_Handle;
        auto Get_CellRegistry() const -> FCk_Registry;

    private:
        TUniquePtr<ck::FEcsWorld> _CellEcsWorld;
        FCk_Handle_SceneNode _Pivot;

    public:
        CK_PROPERTY_GET(_Pivot);
    };
}

// --------------------------------------------------------------------------------------------------------------------