#pragma once

#include "CkCore/Enums/CkEnums.h"

// --------------------------------------------------------------------------------------------------------------------
// Symbol Shape Fragments (C++ only - not exposed to BP/AS)
// MagnifyingGlass, QuestionMark, ExclamationMark, Flag, Pin
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKPMG_API FFragment_Pmg_MagnifyingGlass_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_MagnifyingGlass_Params);

    public:
        friend class FProcessor_Pmg_MagnifyingGlass_Setup;

    private:
        float _Size = 100.0f;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Size);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_QuestionMark_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_QuestionMark_Params);

    public:
        friend class FProcessor_Pmg_QuestionMark_Setup;

    private:
        float _Size = 100.0f;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Size);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_ExclamationMark_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_ExclamationMark_Params);

    public:
        friend class FProcessor_Pmg_ExclamationMark_Setup;

    private:
        float _Size = 100.0f;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Size);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Flag_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Flag_Params);

    public:
        friend class FProcessor_Pmg_Flag_Setup;

    private:
        float _Size = 100.0f;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Size);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Pin_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Pin_Params);

    public:
        friend class FProcessor_Pmg_Pin_Setup;

    private:
        float _Size = 100.0f;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Size);
        CK_PROPERTY(_Axis);
    };
}

// --------------------------------------------------------------------------------------------------------------------
