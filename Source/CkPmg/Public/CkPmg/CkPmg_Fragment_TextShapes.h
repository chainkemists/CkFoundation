#pragma once

#include "CkPmg_Fragment_Data.h"   // ECk_Pmg_TextAlign, ECk_Plane_Axis, and the CK_* macros (transitively)

#include "UObject/StrongObjectPtr.h"

#include <Engine/FontFace.h>

// --------------------------------------------------------------------------------------------------------------------
// Text Shape Fragment (C++ only - not exposed to BP/AS)
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKPMG_API FFragment_Pmg_Text_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Text_Params);

    public:
        friend class FProcessor_Pmg_Text_Setup;

    private:
        FString                     _Text;
        TStrongObjectPtr<UFontFace> _FontOverride;   // empty -> bundled default
        float                       _Size = 100.0f;
        ECk_Pmg_TextAlign           _Align = ECk_Pmg_TextAlign::Left;
        ECk_Plane_Axis              _Axis = ECk_Plane_Axis::XZ; // upright (spec axis note)
        bool                        _DrawFilled = true;
        float                       _LineSpacing = 1.2f;        // multiple of font line height
        int32                       _MaxGlyphs = 4096;

    public:
        CK_PROPERTY(_Text);
        CK_PROPERTY(_FontOverride);
        CK_PROPERTY(_Size);
        CK_PROPERTY(_Align);
        CK_PROPERTY(_Axis);
        CK_PROPERTY(_DrawFilled);
        CK_PROPERTY(_LineSpacing);
        CK_PROPERTY(_MaxGlyphs);
    };
}
