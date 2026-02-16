#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkSettings/ProjectSettings/CkProjectSettings.h"
#include "CkWatermark/CkWatermark_Types.h"

#include "CkWatermark_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Watermark"))
class CKWATERMARK_API UCk_Watermark_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Watermark_ProjectSettings_UE);

private:
    // ---- Color Bands ---------------------------------------------------------
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Color Bands|FPS",
              meta = (AllowPrivateAccess = true))
    FCk_Watermark_ColorBands_HigherIsBetter _Watermark_FPS_ColorBands;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Color Bands|Server FPS",
              meta = (AllowPrivateAccess = true))
    FCk_Watermark_ColorBands_HigherIsBetter _Watermark_ServerFPS_ColorBands;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Color Bands|Ping",
              meta = (AllowPrivateAccess = true))
    FCk_Watermark_ColorBands_LowerIsBetter _Watermark_Ping_ColorBands;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Color Bands|Ensure Count",
              meta = (AllowPrivateAccess = true))
    FCk_Watermark_ColorBands_LowerIsBetter _Watermark_EnsureCount_ColorBands;

    // ---- Font sizes ----------------------------------------------------------
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Fonts",
              meta = (AllowPrivateAccess = true, ClampMin = 4, ClampMax = 72))
    int32 _Watermark_ValueFontSize = 8;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Fonts",
              meta = (AllowPrivateAccess = true, ClampMin = 4, ClampMax = 72))
    int32 _Watermark_LabelFontSize = 8;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Fonts",
              meta = (AllowPrivateAccess = true, ClampMin = 4, ClampMax = 72))
    int32 _Watermark_BracketFontSize = 16;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Fonts",
              meta = (AllowPrivateAccess = true, ClampMin = 4, ClampMax = 72))
    int32 _Watermark_BuildType_FontSize = 8;

    // Text outline thickness (0 = no outline).
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Fonts",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_TextOutlineSize = 1;

    // Color of the text outline. Only visible when TextOutlineSize > 0.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Fonts",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_TextOutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.45f);

    // ---- Layout --------------------------------------------------------------
    // Horizontal padding applied to each side of a stat cell within a row.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Layout",
              meta = (AllowPrivateAccess = true, ClampMin = 0.f, ClampMax = 32.f))
    float _Watermark_StatCell_HorizontalPadding = 0.f;

    // Vertical padding applied above and below each row of stat cells.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Layout",
              meta = (AllowPrivateAccess = true, ClampMin = 0.f, ClampMax = 32.f))
    float _Watermark_Row_VerticalPadding = 0.f;

    // Space between a bracket character and the stat value/name column.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Layout",
              meta = (AllowPrivateAccess = true, ClampMin = 0.f, ClampMax = 32.f))
    float _Watermark_Bracket_InnerPadding = 0.f;

    // ---- Bracket characters --------------------------------------------------
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Brackets",
              meta = (AllowPrivateAccess = true))
    FString _Watermark_BracketOpen = TEXT("[");

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Brackets",
              meta = (AllowPrivateAccess = true))
    FString _Watermark_BracketClose = TEXT("]");

    // Color of the stat name label rendered below the value.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Colors",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_LabelColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

    // Color used for stats that have no good/bad meaning (RAM, VRAM, Time, Frame, ECS groups).
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Colors",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_UncoloredStat_Color = FLinearColor(0.55f, 0.55f, 0.55f, 1.f);

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Colors|Build Config",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_BuildConfig_Debug_Color = FLinearColor(1.0f, 0.5f, 0.15f, 1.f);

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Colors|Build Config",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_BuildConfig_Dev_Color = FLinearColor(0.3f, 0.7f, 1.0f, 1.f);

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Colors|Build Config",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_BuildConfig_Test_Color = FLinearColor(1.0f, 0.85f, 0.0f, 1.f);

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Colors|Build Config",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_BuildConfig_Shipping_Color = FLinearColor(0.0f, 1.0f, 0.4f, 1.f);

    // ---- Stat Visibility — Performance (BottomRight group) -------------------
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Performance",
              meta = (AllowPrivateAccess = true))
    bool _Watermark_Show_MemoryPressure = false;

    // ---- Stat Visibility — Device Info (BottomCenter info group) -------------
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Device Info",
              meta = (AllowPrivateAccess = true))
    bool _Watermark_Show_CpuBrand = true;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Device Info",
              meta = (AllowPrivateAccess = true))
    bool _Watermark_Show_OsVersion = true;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Device Info",
              meta = (AllowPrivateAccess = true))
    bool _Watermark_Show_CpuCores = false;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Device Info",
              meta = (AllowPrivateAccess = true))
    bool _Watermark_Show_NetworkType = false;

    // ---- Stat Visibility — Build Info (BottomLeft info group, Row C) ---------
    // Show the baked-in git build-id hash(es) in the info bar.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Build Info",
              meta = (AllowPrivateAccess = true))
    bool _Watermark_Show_BuildId = true;

    // ---- Stat Visibility — ECS Debug (BottomCenter info group) ---------------
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|ECS Debug",
              meta = (AllowPrivateAccess = true))
    bool _Watermark_Show_EcsDebugger = true;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|ECS Debug",
              meta = (AllowPrivateAccess = true))
    bool _Watermark_Show_EcsEntityMap = false;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|ECS Debug",
              meta = (AllowPrivateAccess = true))
    bool _Watermark_Show_EcsCallstacks = false;

    // ---- Info Bar appearance -------------------------------------------------
    // String placed between consecutive entries in the info bar (default "  |  ").
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Info Bar",
              meta = (AllowPrivateAccess = true))
    FString _Watermark_InfoBar_Separator = TEXT("  |  ");

    // String placed between a key and its value (default ": ").
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Info Bar",
              meta = (AllowPrivateAccess = true))
    FString _Watermark_InfoBar_KeyValueSeparator = TEXT(": ");

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Info Bar|Fonts",
              meta = (AllowPrivateAccess = true, ClampMin = 4, ClampMax = 72))
    int32 _Watermark_InfoBar_FontSize = 8;

    // Color of each entry's key label.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Info Bar|Colors",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_InfoBar_KeyColor = FLinearColor(0.55f, 0.55f, 0.55f, 1.f);

    // Color of each entry's value text.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Info Bar|Colors",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_InfoBar_ValueColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

    // Color of both the inter-item separator and the key:value separator.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Info Bar|Colors",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_InfoBar_SeparatorColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.f);

    // ---- Widget Setup --------------------------------------------------------
    // Z-order at which the watermark widget is added to the viewport.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Watermark",
              meta = (AllowPrivateAccess = true, UIMin = 0, ClampMin = 0))
    int32 _Watermark_Widget_ZOrder = INT_MAX;

    // ---- Stat Row Assignments ------------------------------------------------
    // Each setting controls which row (0-based) a stat appears in.
    // Default values preserve the original two-row layout.

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Rows",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_Row_Ensures = 0;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Rows",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_Row_UniqueEnsures = 0;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Rows",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_Row_Ram = 0;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Rows",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_Row_Vram = 0;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Rows",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_Row_MemPressure = 0;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Rows",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_Row_Ping = 1;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Rows",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_Row_ServerFps = 1;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Rows",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_Row_Fps = 1;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Rows",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_Row_Time = 1;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Rows",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_Row_Frame = 1;

public:
    CK_PROPERTY_GET(_Watermark_Widget_ZOrder);
    CK_PROPERTY_GET(_Watermark_FPS_ColorBands);
    CK_PROPERTY_GET(_Watermark_ServerFPS_ColorBands);
    CK_PROPERTY_GET(_Watermark_Ping_ColorBands);
    CK_PROPERTY_GET(_Watermark_EnsureCount_ColorBands);
    CK_PROPERTY_GET(_Watermark_ValueFontSize);
    CK_PROPERTY_GET(_Watermark_LabelFontSize);
    CK_PROPERTY_GET(_Watermark_BracketFontSize);
    CK_PROPERTY_GET(_Watermark_BuildType_FontSize);
    CK_PROPERTY_GET(_Watermark_TextOutlineSize);
    CK_PROPERTY_GET(_Watermark_TextOutlineColor);
    CK_PROPERTY_GET(_Watermark_StatCell_HorizontalPadding);
    CK_PROPERTY_GET(_Watermark_Row_VerticalPadding);
    CK_PROPERTY_GET(_Watermark_Bracket_InnerPadding);
    CK_PROPERTY_GET(_Watermark_BracketOpen);
    CK_PROPERTY_GET(_Watermark_BracketClose);
    CK_PROPERTY_GET(_Watermark_LabelColor);
    CK_PROPERTY_GET(_Watermark_UncoloredStat_Color);
    CK_PROPERTY_GET(_Watermark_BuildConfig_Debug_Color);
    CK_PROPERTY_GET(_Watermark_BuildConfig_Dev_Color);
    CK_PROPERTY_GET(_Watermark_BuildConfig_Test_Color);
    CK_PROPERTY_GET(_Watermark_BuildConfig_Shipping_Color);
    CK_PROPERTY_GET(_Watermark_Show_BuildId);
    CK_PROPERTY_GET(_Watermark_Show_MemoryPressure);
    CK_PROPERTY_GET(_Watermark_Show_CpuBrand);
    CK_PROPERTY_GET(_Watermark_Show_OsVersion);
    CK_PROPERTY_GET(_Watermark_Show_CpuCores);
    CK_PROPERTY_GET(_Watermark_Show_NetworkType);
    CK_PROPERTY_GET(_Watermark_Show_EcsDebugger);
    CK_PROPERTY_GET(_Watermark_Show_EcsEntityMap);
    CK_PROPERTY_GET(_Watermark_Show_EcsCallstacks);
    CK_PROPERTY_GET(_Watermark_InfoBar_Separator);
    CK_PROPERTY_GET(_Watermark_InfoBar_KeyValueSeparator);
    CK_PROPERTY_GET(_Watermark_InfoBar_FontSize);
    CK_PROPERTY_GET(_Watermark_InfoBar_KeyColor);
    CK_PROPERTY_GET(_Watermark_InfoBar_ValueColor);
    CK_PROPERTY_GET(_Watermark_InfoBar_SeparatorColor);
    CK_PROPERTY_GET(_Watermark_Row_Ensures);
    CK_PROPERTY_GET(_Watermark_Row_UniqueEnsures);
    CK_PROPERTY_GET(_Watermark_Row_Ram);
    CK_PROPERTY_GET(_Watermark_Row_Vram);
    CK_PROPERTY_GET(_Watermark_Row_MemPressure);
    CK_PROPERTY_GET(_Watermark_Row_Ping);
    CK_PROPERTY_GET(_Watermark_Row_ServerFps);
    CK_PROPERTY_GET(_Watermark_Row_Fps);
    CK_PROPERTY_GET(_Watermark_Row_Time);
    CK_PROPERTY_GET(_Watermark_Row_Frame);
};

// --------------------------------------------------------------------------------------------------------------------

class CKWATERMARK_API UCk_Utils_Watermark_ProjectSettings_UE
{
public:
    static int32        Get_Watermark_Widget_ZOrder();
    static const FCk_Watermark_ColorBands_HigherIsBetter& Get_Watermark_FPS_ColorBands();
    static const FCk_Watermark_ColorBands_HigherIsBetter& Get_Watermark_ServerFPS_ColorBands();
    static const FCk_Watermark_ColorBands_LowerIsBetter&  Get_Watermark_Ping_ColorBands();
    static const FCk_Watermark_ColorBands_LowerIsBetter&  Get_Watermark_EnsureCount_ColorBands();
    static int32        Get_Watermark_ValueFontSize();
    static int32        Get_Watermark_LabelFontSize();
    static int32        Get_Watermark_BracketFontSize();
    static int32        Get_Watermark_BuildType_FontSize();
    static int32        Get_Watermark_TextOutlineSize();
    static FLinearColor Get_Watermark_TextOutlineColor();
    static float        Get_Watermark_StatCell_HorizontalPadding();
    static float        Get_Watermark_Row_VerticalPadding();
    static float        Get_Watermark_Bracket_InnerPadding();
    static FString      Get_Watermark_BracketOpen();
    static FString      Get_Watermark_BracketClose();
    static FLinearColor Get_Watermark_LabelColor();
    static FLinearColor Get_Watermark_UncoloredStat_Color();
    static FLinearColor Get_Watermark_BuildConfig_Debug_Color();
    static FLinearColor Get_Watermark_BuildConfig_Dev_Color();
    static FLinearColor Get_Watermark_BuildConfig_Test_Color();
    static FLinearColor Get_Watermark_BuildConfig_Shipping_Color();
    static bool         Get_Watermark_Show_BuildId();
    static bool         Get_Watermark_Show_MemoryPressure();
    static bool         Get_Watermark_Show_CpuBrand();
    static bool         Get_Watermark_Show_OsVersion();
    static bool         Get_Watermark_Show_CpuCores();
    static bool         Get_Watermark_Show_NetworkType();
    static bool         Get_Watermark_Show_EcsDebugger();
    static bool         Get_Watermark_Show_EcsEntityMap();
    static bool         Get_Watermark_Show_EcsCallstacks();
    static FString      Get_Watermark_InfoBar_Separator();
    static FString      Get_Watermark_InfoBar_KeyValueSeparator();
    static int32        Get_Watermark_InfoBar_FontSize();
    static FLinearColor Get_Watermark_InfoBar_KeyColor();
    static FLinearColor Get_Watermark_InfoBar_ValueColor();
    static FLinearColor Get_Watermark_InfoBar_SeparatorColor();
    static int32        Get_Watermark_Row_Ensures();
    static int32        Get_Watermark_Row_UniqueEnsures();
    static int32        Get_Watermark_Row_Ram();
    static int32        Get_Watermark_Row_Vram();
    static int32        Get_Watermark_Row_MemPressure();
    static int32        Get_Watermark_Row_Ping();
    static int32        Get_Watermark_Row_ServerFps();
    static int32        Get_Watermark_Row_Fps();
    static int32        Get_Watermark_Row_Time();
    static int32        Get_Watermark_Row_Frame();
};

// --------------------------------------------------------------------------------------------------------------------
