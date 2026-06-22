#pragma once

#include "CkCore/Enums/CkEnums.h"

// --------------------------------------------------------------------------------------------------------------------
// Icon Shape Fragments (C++ only - not exposed to BP/AS)
// Warning, Prohibition, NoEntry, InfoCircle
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKPMG_API FFragment_Pmg_Warning_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Warning_Params);

    public:
        friend class FProcessor_Pmg_Warning_Setup;

    private:
        float _Size = 100.0f;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::YZ;

    public:
        CK_PROPERTY(_Size);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Prohibition_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Prohibition_Params);

    public:
        friend class FProcessor_Pmg_Prohibition_Setup;

    private:
        float _Radius = 100.0f;
        int32 _Segments = 32;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::YZ;

    public:
        CK_PROPERTY(_Radius);
        CK_PROPERTY(_Segments);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_NoEntry_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_NoEntry_Params);

    public:
        friend class FProcessor_Pmg_NoEntry_Setup;

    private:
        float _Radius = 100.0f;
        int32 _Segments = 32;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::YZ;

    public:
        CK_PROPERTY(_Radius);
        CK_PROPERTY(_Segments);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_InfoCircle_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_InfoCircle_Params);

    public:
        friend class FProcessor_Pmg_InfoCircle_Setup;

    private:
        float _Radius = 100.0f;
        int32 _Segments = 32;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::YZ;

    public:
        CK_PROPERTY(_Radius);
        CK_PROPERTY(_Segments);
        CK_PROPERTY(_Axis);
    };
}

// --------------------------------------------------------------------------------------------------------------------
