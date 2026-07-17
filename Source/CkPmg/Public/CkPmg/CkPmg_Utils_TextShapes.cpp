#include "CkPmg_Utils_TextShapes.h"
#include "CkPmg_Utils_DebugShapes.h"
#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_TextShapes.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_TextShapes::
    Add_Text(
        FCk_Handle& InHandle, FTransform InTransform, FString InText, float InSize,
        FLinearColor InColor, bool InDrawLines, bool InDrawFilled, float InLineThickness,
        ECk_Pmg_TextAlign InAlign, ECk_Plane_Axis InDefaultAxis, UFontFace* InFontOverride, float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_Text_Params{};
    Params.Set_Text(InText);
    Params.Set_FontOverride(TStrongObjectPtr<UFontFace>{InFontOverride});
    Params.Set_Size(InSize);
    Params.Set_Align(InAlign);
    Params.Set_Axis(InDefaultAxis);
    Params.Set_DrawFilled(InDrawFilled);
    InHandle.Add<ck::FFragment_Pmg_Text_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_TextShapes::
    Create_Text(
        FCk_Handle& InOwningEntity, FTransform InTransform, FString InText, float InSize,
        FLinearColor InColor, bool InDrawLines, bool InDrawFilled, float InLineThickness,
        ECk_Pmg_TextAlign InAlign, ECk_Plane_Axis InDefaultAxis, UFontFace* InFontOverride, float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Text(NewEntity, InTransform, InText, InSize, InColor, InDrawLines, InDrawFilled,
        InLineThickness, InAlign, InDefaultAxis, InFontOverride, InDuration);
}

auto
    UCk_Utils_Pmg_TextShapes::
    DrawText(
        const UObject* InWorldContextObject, FVector InCenter, FString InText, float InSize,
        FLinearColor InColor, bool InDrawLines, bool InDrawFilled, float InLineThickness,
        ECk_Pmg_TextAlign InAlign, ECk_Plane_Axis InDefaultAxis, UFontFace* InFontOverride, float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Text(NewEntity, Transform, InText, InSize, InColor, InDrawLines, InDrawFilled,
        InLineThickness, InAlign, InDefaultAxis, InFontOverride, InDuration);
}

auto
    UCk_Utils_Pmg_TextShapes::
    MakeText_FromHexCodepoints(
        const FString& InHexCodepoints)
    -> FString
{
    FString Result;
    TArray<FString> Tokens;
    InHexCodepoints.ParseIntoArray(Tokens, TEXT(" "), true);
    for (const FString& Token : Tokens)
    {
        const uint32 Codepoint = static_cast<uint32>(FCString::Strtoui64(*Token, nullptr, 16));
        if (Codepoint == 0) { continue; }
        if (Codepoint <= 0xFFFF)
        {
            Result.AppendChar(static_cast<TCHAR>(Codepoint));
        }
        else
        {
            const uint32 V = Codepoint - 0x10000;
            Result.AppendChar(static_cast<TCHAR>(0xD800 + (V >> 10)));
            Result.AppendChar(static_cast<TCHAR>(0xDC00 + (V & 0x3FF)));
        }
    }
    return Result;
}
