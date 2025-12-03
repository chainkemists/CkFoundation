#pragma once

#include "CkPmg_Fragment_Data.h"

#include "CkCore/Chrono/CkChrono.h"

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
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Pmg_DebugShape_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_DebugShape_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_DebugShape_Params);

    public:
        friend class FProcessor_Pmg_DebugShape_Setup;
        friend class UCk_Utils_Pmg_DebugShape_UE;

    private:
        FCk_Fragment_Pmg_DebugShape_ParamsData _Params;

    public:
        CK_PROPERTY_GET(_Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_DebugShape_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_DebugShape_Current);

    public:
        friend class FProcessor_Pmg_DebugShape_Setup;
        friend class FProcessor_Pmg_DebugShape_UpdateTransform;
        friend class FProcessor_Pmg_DebugShape_CheckDuration;
        friend class FProcessor_Pmg_DebugShape_EndPlay;
        friend class UCk_Utils_Pmg_DebugShape_UE;

    private:
        TStrongObjectPtr<UProceduralMeshComponent> _MeshComponent;
        FCk_Time _SpawnTime;

    public:
        CK_PROPERTY_GET(_MeshComponent);
        CK_PROPERTY_GET(_SpawnTime);
    };
}

// --------------------------------------------------------------------------------------------------------------------
