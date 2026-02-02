#pragma once

#include "CkPmg_Fragment_Data.h"

#include "CkCore/Chrono/CkChrono.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include <ProceduralMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Pmg_Donut_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Pmg_Donut_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Donut_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Donut_Params);

    public:
        friend class FProcessor_Pmg_Donut_Setup;
        friend class UCk_Utils_Pmg_Donut_UE;

    private:
        FCk_Fragment_Pmg_Donut_ParamsData _Params;

    public:
        CK_PROPERTY_GET(_Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Donut_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Donut_Current);

    public:
        friend class FProcessor_Pmg_Donut_Setup;
        friend class FProcessor_Pmg_Donut_HandleRequests;
        friend class FProcessor_Pmg_Donut_UpdateTransform;
        friend class FProcessor_Pmg_Donut_EndPlay;
        friend class UCk_Utils_Pmg_Donut_UE;

    private:
        TStrongObjectPtr<UProceduralMeshComponent> _MeshComponent;

        float _InnerRadius = 50.0f;
        float _OuterRadius = 100.0f;
        int32 _Segments = 32;
        float _FillAngle = 360.0f;
        TObjectPtr<UMaterialInterface> _Material;
        bool _EnableCollision = false;
        ECk_Pmg_RenderMode _RenderMode = ECk_Pmg_RenderMode::DoubleSided;

    public:
        CK_PROPERTY_GET(_MeshComponent);
        CK_PROPERTY_GET(_InnerRadius);
        CK_PROPERTY_GET(_OuterRadius);
        CK_PROPERTY_GET(_Segments);
        CK_PROPERTY_GET(_FillAngle);
        CK_PROPERTY_GET(_Material);
        CK_PROPERTY_GET(_EnableCollision);
        CK_PROPERTY_GET(_RenderMode);
    };

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Pmg_Donut_UpdateParams = FCk_Request_Pmg_Donut_UpdateParams;

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Pmg_Donut_UpdateParams);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Pmg_DebugShape_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    // Common properties shared by all debug shapes (C++ only - not exposed to BP/AS)
    struct CKPMG_API FFragment_Pmg_DebugShape_Common
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_DebugShape_Common);

    public:
        friend class FProcessor_Pmg_Wedge_Setup;
        friend class FProcessor_Pmg_Arc_Setup;
        friend class FProcessor_Pmg_WedgeCone_Setup;
        friend class FProcessor_Pmg_Sphere_Setup;
        friend class FProcessor_Pmg_Box_Setup;
        friend class FProcessor_Pmg_Cone_Setup;
        friend class FProcessor_Pmg_Cylinder_Setup;
        friend class FProcessor_Pmg_Capsule_Setup;
        friend class FProcessor_Pmg_Pyramid_Setup;
        friend class FProcessor_Pmg_Hemisphere_Setup;
        friend class FProcessor_Pmg_Torus_Setup;
        friend class FProcessor_Pmg_Circle_Setup;
        friend class FProcessor_Pmg_Plane_Setup;
        friend class FProcessor_Pmg_Ring_Setup;
        friend class FProcessor_Pmg_Cross_Setup;
        friend class FProcessor_Pmg_Star_Setup;
        friend class FProcessor_Pmg_Checkmark_Setup;
        friend class FProcessor_Pmg_Diamond_Setup;
        friend class FProcessor_Pmg_Warning_Setup;
        friend class FProcessor_Pmg_Prohibition_Setup;
        friend class FProcessor_Pmg_NoEntry_Setup;
        friend class FProcessor_Pmg_MagnifyingGlass_Setup;
        friend class FProcessor_Pmg_QuestionMark_Setup;
        friend class FProcessor_Pmg_ExclamationMark_Setup;
        friend class FProcessor_Pmg_Flag_Setup;
        friend class FProcessor_Pmg_InfoCircle_Setup;
        friend class FProcessor_Pmg_Pin_Setup;
        friend class FProcessor_Pmg_Arrow_Setup;
        friend class FProcessor_Pmg_Pivot_Setup;
        friend class FProcessor_Pmg_DashedLine_Setup;

    private:
        FLinearColor _Color = FLinearColor::White;
        bool _DrawLines = true;
        float _LineThickness = 2.0f;
        FCk_Time _Duration = FCk_Time{0.0f};
        bool _EnableCollision = false;
        ECk_Pmg_RenderMode _RenderMode = ECk_Pmg_RenderMode::DoubleSided;

    public:
        CK_PROPERTY(_Color);
        CK_PROPERTY(_DrawLines);
        CK_PROPERTY(_LineThickness);
        CK_PROPERTY(_Duration);
        CK_PROPERTY(_EnableCollision);
        CK_PROPERTY(_RenderMode);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_DebugShape_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_DebugShape_Current);

    public:
        friend class FProcessor_Pmg_DebugShape_UpdateTransform;
        friend class FProcessor_Pmg_DebugShape_CheckDuration;
        friend class FProcessor_Pmg_DebugShape_EndPlay;
        friend class FProcessor_Pmg_Wedge_Setup;
        friend class FProcessor_Pmg_Arc_Setup;
        friend class FProcessor_Pmg_WedgeCone_Setup;
        friend class FProcessor_Pmg_Sphere_Setup;
        friend class FProcessor_Pmg_Box_Setup;
        friend class FProcessor_Pmg_Cone_Setup;
        friend class FProcessor_Pmg_Cylinder_Setup;
        friend class FProcessor_Pmg_Capsule_Setup;
        friend class FProcessor_Pmg_Pyramid_Setup;
        friend class FProcessor_Pmg_Hemisphere_Setup;
        friend class FProcessor_Pmg_Torus_Setup;
        friend class FProcessor_Pmg_Circle_Setup;
        friend class FProcessor_Pmg_Plane_Setup;
        friend class FProcessor_Pmg_Ring_Setup;
        friend class FProcessor_Pmg_Cross_Setup;
        friend class FProcessor_Pmg_Star_Setup;
        friend class FProcessor_Pmg_Checkmark_Setup;
        friend class FProcessor_Pmg_Diamond_Setup;
        friend class FProcessor_Pmg_Warning_Setup;
        friend class FProcessor_Pmg_Prohibition_Setup;
        friend class FProcessor_Pmg_NoEntry_Setup;
        friend class FProcessor_Pmg_MagnifyingGlass_Setup;
        friend class FProcessor_Pmg_QuestionMark_Setup;
        friend class FProcessor_Pmg_ExclamationMark_Setup;
        friend class FProcessor_Pmg_Flag_Setup;
        friend class FProcessor_Pmg_InfoCircle_Setup;
        friend class FProcessor_Pmg_Pin_Setup;
        friend class FProcessor_Pmg_Arrow_Setup;
        friend class FProcessor_Pmg_Pivot_Setup;
        friend class FProcessor_Pmg_DashedLine_Setup;

    private:
        TStrongObjectPtr<UProceduralMeshComponent> _MeshComponent;
        FCk_Time _SpawnTime;

    public:
        CK_PROPERTY_GET(_MeshComponent);
        CK_PROPERTY_GET(_SpawnTime);

        CK_DEFINE_CONSTRUCTORS(FFragment_Pmg_DebugShape_Current, _MeshComponent, _SpawnTime);
    };
}

// --------------------------------------------------------------------------------------------------------------------
