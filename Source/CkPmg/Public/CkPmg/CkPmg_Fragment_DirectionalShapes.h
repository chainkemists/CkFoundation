#pragma once

#include "CkCore/Enums/CkEnums.h"

// --------------------------------------------------------------------------------------------------------------------
// Directional Shape Fragments (C++ only - not exposed to BP/AS)
// Arrow, Pivot, DashedLine
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKPMG_API FFragment_Pmg_Arrow_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Arrow_Params);

    public:
        friend class FProcessor_Pmg_Arrow_Setup;

    private:
        float _Length = 200.0f;
        float _ShaftWidth = 20.0f;
        float _ArrowHeadRatio = 0.3f;
        float _ArrowHeadWidthMultiplier = 2.0f;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Length);
        CK_PROPERTY(_ShaftWidth);
        CK_PROPERTY(_ArrowHeadRatio);
        CK_PROPERTY(_ArrowHeadWidthMultiplier);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Pivot_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Pivot_Params);

    public:
        friend class FProcessor_Pmg_Pivot_Setup;

    private:
        float _AxisLength = 100.0f;
        float _ArrowSize = 10.0f;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_AxisLength);
        CK_PROPERTY(_ArrowSize);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_DashedLine_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_DashedLine_Params);

    public:
        friend class FProcessor_Pmg_DashedLine_Setup;

    private:
        float _Length = 100.0f;
        float _DashLength = 20.0f;
        float _GapLength = 10.0f;
        float _Thickness = 2.0f;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Length);
        CK_PROPERTY(_DashLength);
        CK_PROPERTY(_GapLength);
        CK_PROPERTY(_Thickness);
        CK_PROPERTY(_Axis);
    };

} // namespace ck

// --------------------------------------------------------------------------------------------------------------------
