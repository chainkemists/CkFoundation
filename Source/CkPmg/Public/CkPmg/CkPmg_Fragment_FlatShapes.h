#pragma once

#include "CkCore/Enums/CkEnums.h"

// --------------------------------------------------------------------------------------------------------------------
// Flat Shape Fragments (C++ only - not exposed to BP/AS)
// Circle, Plane, Ring, Cross, Star, Checkmark, Diamond
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKPMG_API FFragment_Pmg_Circle_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Circle_Params);

    public:
        friend class FProcessor_Pmg_Circle_Setup;

    private:
        float _Radius = 100.0f;
        int32 _Segments = 32;
        bool _DrawDirectionLine = false;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Radius);
        CK_PROPERTY(_Segments);
        CK_PROPERTY(_DrawDirectionLine);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Plane_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Plane_Params);

    public:
        friend class FProcessor_Pmg_Plane_Setup;

    private:
        float _Width = 100.0f;
        float _Height = 100.0f;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_Width);
        CK_PROPERTY(_Height);
        CK_PROPERTY(_Axis);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPMG_API FFragment_Pmg_Ring_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Ring_Params);

    public:
        friend class FProcessor_Pmg_Ring_Setup;

    private:
        float _OuterRadius = 100.0f;
        float _InnerRadius = 50.0f;
        int32 _Segments = 32;
        ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

    public:
        CK_PROPERTY(_OuterRadius);
        CK_PROPERTY(_InnerRadius);
        CK_PROPERTY(_Segments);
        CK_PROPERTY(_Axis);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKPMG_API FFragment_Pmg_Cross_Params
{
public:
    CK_GENERATED_BODY(FFragment_Pmg_Cross_Params);

public:
    friend class FProcessor_Pmg_Cross_Setup;

private:
    float _Size = 50.0f;
    float _Thickness = 5.0f;
    ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

public:
    CK_PROPERTY(_Size);
    CK_PROPERTY(_Thickness);
    CK_PROPERTY(_Axis);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKPMG_API FFragment_Pmg_Star_Params
{
public:
    CK_GENERATED_BODY(FFragment_Pmg_Star_Params);

public:
    friend class FProcessor_Pmg_Star_Setup;

private:
    float _OuterRadius = 100.0f;
    int32 _Points = 5;
    float _InnerRadiusRatio = 0.5f;
    ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

public:
    CK_PROPERTY(_OuterRadius);
    CK_PROPERTY(_Points);
    CK_PROPERTY(_InnerRadiusRatio);
    CK_PROPERTY(_Axis);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKPMG_API FFragment_Pmg_Checkmark_Params
{
public:
    CK_GENERATED_BODY(FFragment_Pmg_Checkmark_Params);

public:
    friend class FProcessor_Pmg_Checkmark_Setup;

private:
    float _Size = 50.0f;
    float _Thickness = 5.0f;
    ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

public:
    CK_PROPERTY(_Size);
    CK_PROPERTY(_Thickness);
    CK_PROPERTY(_Axis);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKPMG_API FFragment_Pmg_Diamond_Params
{
public:
    CK_GENERATED_BODY(FFragment_Pmg_Diamond_Params);

public:
    friend class FProcessor_Pmg_Diamond_Setup;

private:
    float _Width = 50.0f;
    float _Height = 50.0f;
    ECk_Plane_Axis _Axis = ECk_Plane_Axis::XY;

public:
    CK_PROPERTY(_Width);
    CK_PROPERTY(_Height);
    CK_PROPERTY(_Axis);
};
}

// --------------------------------------------------------------------------------------------------------------------
