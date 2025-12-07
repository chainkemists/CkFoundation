#pragma once

#include "CkCore/Enums/CkEnums.h"

// --------------------------------------------------------------------------------------------------------------------
// Angular Shape Fragments (C++ only - not exposed to BP/AS)
// Wedge, Arc, WedgeCone
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKPMG_API FFragment_Pmg_Wedge_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Wedge_Params);

    public:
        friend class FProcessor_Pmg_Wedge_Setup;

    private:
        float _Radius = 100.0f;
        float _StartAngle = 0.0f;
        float _EndAngle = 90.0f;
        int32 _Segments = 32;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Radius);
        CK_PROPERTY(_StartAngle);
        CK_PROPERTY(_EndAngle);
        CK_PROPERTY(_Segments);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Arc_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Arc_Params);

    public:
        friend class FProcessor_Pmg_Arc_Setup;

    private:
        float _Radius = 100.0f;
        float _StartAngle = 0.0f;
        float _EndAngle = 90.0f;
        float _Thickness = 5.0f;
        int32 _Segments = 32;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Radius);
        CK_PROPERTY(_StartAngle);
        CK_PROPERTY(_EndAngle);
        CK_PROPERTY(_Thickness);
        CK_PROPERTY(_Segments);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_WedgeCone_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_WedgeCone_Params);

    public:
        friend class FProcessor_Pmg_WedgeCone_Setup;

    private:
        float _Radius = 100.0f;
        float _Height = 200.0f;
        float _StartAngle = 0.0f;
        float _EndAngle = 90.0f;
        int32 _Segments = 16;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Radius);
        CK_PROPERTY(_Height);
        CK_PROPERTY(_StartAngle);
        CK_PROPERTY(_EndAngle);
        CK_PROPERTY(_Segments);
        CK_PROPERTY(_Axis);
    };
}

// --------------------------------------------------------------------------------------------------------------------
