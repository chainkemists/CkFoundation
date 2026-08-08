#pragma once

#include "CkPmg_Fragment_Data_DebugShapes.h"   // ECk_Pmg_TextAlign, ECk_Plane_Axis, and the CK_* macros (transitively)

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
        TStrongObjectPtr<UFontFace> _FontOverride;   // empty -> bundled default
        float                       _Size = 100.0f;
        ECk_Pmg_TextAlign           _Align = ECk_Pmg_TextAlign::Left;
        ECk_Plane_Axis              _Axis = ECk_Plane_Axis::YZ; // default: upright, facing the play camera (YZ is now the upright plane)
        bool                        _DrawFilled = true;
        float                       _LineSpacing = 1.2f;        // multiple of font line height
        int32                       _MaxGlyphs = 4096;

    public:
        CK_PROPERTY(_FontOverride);
        CK_PROPERTY(_Size);
        CK_PROPERTY(_Align);
        CK_PROPERTY(_Axis);
        CK_PROPERTY(_DrawFilled);
        CK_PROPERTY(_LineSpacing);
        CK_PROPERTY(_MaxGlyphs);
    };

    // --------------------------------------------------------------------------------------------------------------------

    /** The string being drawn. Request_SetText replaces it and re-flags NeedsSetup, so it is state --
     *  keeping it in the Params residue is what made that residue mutable. */
    struct CKPMG_API FFragment_Pmg_Text
    {
    public:
        CK_GENERATED_BODY(FFragment_Pmg_Text);

    public:
        friend class ::UCk_Utils_Pmg_DebugShape_UE;

    private:
        FString _Text;

    public:
        CK_PROPERTY_GET(_Text);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Pmg_Text, _Text);
    };
}
