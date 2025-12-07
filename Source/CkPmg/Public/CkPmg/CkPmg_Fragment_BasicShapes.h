#pragma once

#include "CkCore/Enums/CkEnums.h"

// --------------------------------------------------------------------------------------------------------------------
// Basic Shape Fragments (C++ only - not exposed to BP/AS)
// Sphere, Box, Cone, Cylinder, Capsule, Pyramid, Hemisphere, Torus
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKPMG_API FFragment_Pmg_Sphere_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Sphere_Params);

    public:
        friend class FProcessor_Pmg_Sphere_Setup;

    private:
        float _Radius = 100.0f;
        int32 _Segments = 16;
        int32 _Rings = 16;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Radius);
        CK_PROPERTY(_Segments);
        CK_PROPERTY(_Rings);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Box_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Box_Params);

    public:
        friend class FProcessor_Pmg_Box_Setup;

    private:
        FVector _Extent = FVector{100.0f, 100.0f, 100.0f};
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Extent);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Cone_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Cone_Params);

    public:
        friend class FProcessor_Pmg_Cone_Setup;

    private:
        float _Radius = 100.0f;
        float _Height = 200.0f;
        int32 _Segments = 16;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Radius);
        CK_PROPERTY(_Height);
        CK_PROPERTY(_Segments);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Cylinder_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Cylinder_Params);

    public:
        friend class FProcessor_Pmg_Cylinder_Setup;

    private:
        float _Radius = 100.0f;
        float _Height = 200.0f;
        int32 _Segments = 16;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Radius);
        CK_PROPERTY(_Height);
        CK_PROPERTY(_Segments);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Capsule_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Capsule_Params);

    public:
        friend class FProcessor_Pmg_Capsule_Setup;

    private:
        float _Radius = 50.0f;
        float _HalfHeight = 100.0f;
        int32 _Segments = 16;
        int32 _Rings = 8;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Radius);
        CK_PROPERTY(_HalfHeight);
        CK_PROPERTY(_Segments);
        CK_PROPERTY(_Rings);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Pyramid_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Pyramid_Params);

    public:
        friend class FProcessor_Pmg_Pyramid_Setup;

    private:
        float _BaseSize = 100.0f;
        float _Height = 100.0f;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_BaseSize);
        CK_PROPERTY(_Height);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Hemisphere_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Hemisphere_Params);

    public:
        friend class FProcessor_Pmg_Hemisphere_Setup;

    private:
        float _Radius = 100.0f;
        int32 _Segments = 16;
        int32 _Rings = 8;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Radius);
        CK_PROPERTY(_Segments);
        CK_PROPERTY(_Rings);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Torus_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Torus_Params);

    public:
        friend class FProcessor_Pmg_Torus_Setup;

    private:
        float _MajorRadius = 100.0f;
        float _MinorRadius = 25.0f;
        int32 _MajorSegments = 32;
        int32 _MinorSegments = 16;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_MajorRadius);
        CK_PROPERTY(_MinorRadius);
        CK_PROPERTY(_MajorSegments);
        CK_PROPERTY(_MinorSegments);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
