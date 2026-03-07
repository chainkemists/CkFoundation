#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkSettings/ProjectSettings/CkProjectSettings.h"
#include "CkWatermark/CkWatermark_Types.h"
#include "CkWatermark/CkWatermark_Widget.h"

#include <Fonts/SlateFontInfo.h>

#include "CkWatermark_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Watermark"))
class CKWATERMARK_API UCk_Watermark_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Watermark_ProjectSettings_UE);

    explicit UCk_Watermark_ProjectSettings_UE(
        const FObjectInitializer& InObjectInitializer);

    // Forces a full Slate rebuild of all active watermark widgets.
    // Use this after changing settings that require a rebuild (font size, font family, layout).
    UFUNCTION(CallInEditor, Category = "Watermark")
    void ForceRebuildWatermark();

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

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Color Bands|Unique Ensure Count",
              meta = (AllowPrivateAccess = true))
    FCk_Watermark_ColorBands_LowerIsBetter _Watermark_UniqueEnsureCount_ColorBands;

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

    // Custom font asset. When set, replaces the engine default (Roboto) for all watermark text.
    // Size and outline are still driven by the settings below.
    // Leave empty to use the Slate Core Style default font.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Fonts",
              meta = (AllowPrivateAccess = true))
    FSlateFontInfo _Watermark_FontOverride;

    // Text outline thickness (0 = no outline).
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Fonts",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_TextOutlineSize = 1;

    // Color of the text outline. Only visible when TextOutlineSize > 0.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Fonts",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_TextOutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.45f);

    // Drop-shadow pixel offset. (0, 0) disables the shadow entirely.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Fonts",
              meta = (AllowPrivateAccess = true))
    FVector2D _Watermark_TextShadowOffset = FVector2D(0.f, 0.f);

    // Color and opacity of the drop shadow. Only visible when TextShadowOffset is non-zero.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Fonts",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_TextShadowColor = FLinearColor(0.f, 0.f, 0.f, 0.5f);

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

    // ---- Minimum Display Policy — Performance (BottomRight stat group) -------
    // The minimum display policy at which this stat becomes visible.
    // Hidden = never shown, Minimal/Regular/Detailed = shown at that level and above.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Performance",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_Ensures = ECk_Watermark_DisplayPolicy::Regular;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Performance",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_UniqueEnsures = ECk_Watermark_DisplayPolicy::Regular;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Performance",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_Ram = ECk_Watermark_DisplayPolicy::Regular;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Performance",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_Vram = ECk_Watermark_DisplayPolicy::Regular;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Performance",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_MemoryPressure = ECk_Watermark_DisplayPolicy::Hidden;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Performance",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_Ping = ECk_Watermark_DisplayPolicy::Minimal;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Performance",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_ServerFps = ECk_Watermark_DisplayPolicy::Regular;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Performance",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_Fps = ECk_Watermark_DisplayPolicy::Minimal;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Performance",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_Time = ECk_Watermark_DisplayPolicy::Regular;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Performance",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_Frame = ECk_Watermark_DisplayPolicy::Regular;

    // ---- Minimum Display Policy — Replication --------------------------------
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Replication",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_RepActors = ECk_Watermark_DisplayPolicy::Regular;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Replication",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_RepComponents = ECk_Watermark_DisplayPolicy::Regular;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Replication",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_RepObjects = ECk_Watermark_DisplayPolicy::Regular;

    // ---- Minimum Display Policy — Device Info (info bar) ---------------------
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Device Info",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_CpuBrand = ECk_Watermark_DisplayPolicy::Regular;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Device Info",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_OsVersion = ECk_Watermark_DisplayPolicy::Hidden;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Device Info",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_CpuCores = ECk_Watermark_DisplayPolicy::Hidden;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Device Info",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_NetworkType = ECk_Watermark_DisplayPolicy::Hidden;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Device Info",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_NetMode = ECk_Watermark_DisplayPolicy::Hidden;

    // ---- Net Mode Labels -----------------------------------------------------
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Net Mode Labels",
              meta = (AllowPrivateAccess = true))
    FString _Watermark_NetMode_Label_SinglePlayer = TEXT("SINGLE PLAYER");

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Net Mode Labels",
              meta = (AllowPrivateAccess = true))
    FString _Watermark_NetMode_Label_Server = TEXT("SERVER");

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Net Mode Labels",
              meta = (AllowPrivateAccess = true))
    FString _Watermark_NetMode_Label_ListenServer = TEXT("LISTEN SERVER");

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Net Mode Labels",
              meta = (AllowPrivateAccess = true))
    FString _Watermark_NetMode_Label_Client = TEXT("CLIENT");

    // ---- Minimum Display Policy — Build Info (info bar Row C) -----------------
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Build Info",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_BuildId = ECk_Watermark_DisplayPolicy::Regular;

    // ---- Minimum Display Policy — Info Bar Row B (Jolt + ECS Debug) ----------
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Jolt",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_Jolt = ECk_Watermark_DisplayPolicy::Regular;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|ECS Debug",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_EcsDebugger = ECk_Watermark_DisplayPolicy::Regular;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|ECS Debug",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_EcsEntityMap = ECk_Watermark_DisplayPolicy::Hidden;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|ECS Debug",
              meta = (AllowPrivateAccess = true))
    ECk_Watermark_DisplayPolicy _Watermark_MinPolicy_EcsCallstacks = ECk_Watermark_DisplayPolicy::Hidden;

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

    // Color of a build-id entry whose merge-base equals HEAD (we are on that branch tip).
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Info Bar|Colors|Build Id",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_BuildId_Active_Color = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Color of a build-id entry whose merge-base does NOT equal HEAD (we have diverged).
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Info Bar|Colors|Build Id",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_BuildId_Inactive_Color = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);

    // Branches to show in the info bar. Any branch not listed here is hidden.
    // Add branch names (e.g. "dev", "main") to opt-in to displaying them.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Visibility|Build Info",
              meta = (AllowPrivateAccess = true))
    TArray<FString> _Watermark_BuildId_EnabledBranches;

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

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Rows",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_Row_RepActors = 2;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Rows",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_Row_RepComponents = 2;

    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Stat Rows",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 8))
    int32 _Watermark_Row_RepObjects = 2;

    // ---- Activity Bar ------------------------------------------------------------
    // Maximum number of signal entries to retain in the activity bar history.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Activity Bar",
              meta = (AllowPrivateAccess = true, ClampMin = 1, ClampMax = 64))
    int32 _Watermark_ActivityBar_MaxHistory = 16;

    // Color of active (pressed) signal chips.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Activity Bar|Colors",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_ActivityBar_ActiveColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

    // Accent underline color shown under signals that are held (active > 1 frame).
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Activity Bar|Colors",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_ActivityBar_HeldAccentColor = FLinearColor(1.0f, 0.7f, 0.1f, 1.0f);

    // Color of inactive (released) signal chips.
    UPROPERTY(Config, EditAnywhere, Category = "Watermark|Activity Bar|Colors",
              meta = (AllowPrivateAccess = true))
    FLinearColor _Watermark_ActivityBar_InactiveColor = FLinearColor(0.35f, 0.35f, 0.35f, 1.0f);

public:
    CK_PROPERTY_GET(_Watermark_Widget_ZOrder);
    CK_PROPERTY_GET(_Watermark_FPS_ColorBands);
    CK_PROPERTY_GET(_Watermark_ServerFPS_ColorBands);
    CK_PROPERTY_GET(_Watermark_Ping_ColorBands);
    CK_PROPERTY_GET(_Watermark_EnsureCount_ColorBands);
    CK_PROPERTY_GET(_Watermark_UniqueEnsureCount_ColorBands);
    CK_PROPERTY_GET(_Watermark_FontOverride);
    CK_PROPERTY_GET(_Watermark_ValueFontSize);
    CK_PROPERTY_GET(_Watermark_LabelFontSize);
    CK_PROPERTY_GET(_Watermark_BracketFontSize);
    CK_PROPERTY_GET(_Watermark_BuildType_FontSize);
    CK_PROPERTY_GET(_Watermark_TextOutlineSize);
    CK_PROPERTY_GET(_Watermark_TextOutlineColor);
    CK_PROPERTY_GET(_Watermark_TextShadowOffset);
    CK_PROPERTY_GET(_Watermark_TextShadowColor);
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
    CK_PROPERTY_GET(_Watermark_MinPolicy_Ensures);
    CK_PROPERTY_GET(_Watermark_MinPolicy_UniqueEnsures);
    CK_PROPERTY_GET(_Watermark_MinPolicy_Ram);
    CK_PROPERTY_GET(_Watermark_MinPolicy_Vram);
    CK_PROPERTY_GET(_Watermark_MinPolicy_MemoryPressure);
    CK_PROPERTY_GET(_Watermark_MinPolicy_Ping);
    CK_PROPERTY_GET(_Watermark_MinPolicy_ServerFps);
    CK_PROPERTY_GET(_Watermark_MinPolicy_Fps);
    CK_PROPERTY_GET(_Watermark_MinPolicy_Time);
    CK_PROPERTY_GET(_Watermark_MinPolicy_Frame);
    CK_PROPERTY_GET(_Watermark_MinPolicy_RepActors);
    CK_PROPERTY_GET(_Watermark_MinPolicy_RepComponents);
    CK_PROPERTY_GET(_Watermark_MinPolicy_RepObjects);
    CK_PROPERTY_GET(_Watermark_MinPolicy_BuildId);
    CK_PROPERTY_GET(_Watermark_MinPolicy_CpuBrand);
    CK_PROPERTY_GET(_Watermark_MinPolicy_OsVersion);
    CK_PROPERTY_GET(_Watermark_MinPolicy_CpuCores);
    CK_PROPERTY_GET(_Watermark_MinPolicy_NetworkType);
    CK_PROPERTY_GET(_Watermark_MinPolicy_NetMode);
    CK_PROPERTY_GET(_Watermark_NetMode_Label_SinglePlayer);
    CK_PROPERTY_GET(_Watermark_NetMode_Label_Server);
    CK_PROPERTY_GET(_Watermark_NetMode_Label_ListenServer);
    CK_PROPERTY_GET(_Watermark_NetMode_Label_Client);
    CK_PROPERTY_GET(_Watermark_MinPolicy_Jolt);
    CK_PROPERTY_GET(_Watermark_MinPolicy_EcsDebugger);
    CK_PROPERTY_GET(_Watermark_MinPolicy_EcsEntityMap);
    CK_PROPERTY_GET(_Watermark_MinPolicy_EcsCallstacks);
    CK_PROPERTY_GET(_Watermark_InfoBar_Separator);
    CK_PROPERTY_GET(_Watermark_InfoBar_KeyValueSeparator);
    CK_PROPERTY_GET(_Watermark_InfoBar_FontSize);
    CK_PROPERTY_GET(_Watermark_InfoBar_KeyColor);
    CK_PROPERTY_GET(_Watermark_InfoBar_ValueColor);
    CK_PROPERTY_GET(_Watermark_InfoBar_SeparatorColor);
    CK_PROPERTY_GET(_Watermark_BuildId_Active_Color);
    CK_PROPERTY_GET(_Watermark_BuildId_Inactive_Color);
    CK_PROPERTY_GET(_Watermark_BuildId_EnabledBranches);
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
    CK_PROPERTY_GET(_Watermark_Row_RepActors);
    CK_PROPERTY_GET(_Watermark_Row_RepComponents);
    CK_PROPERTY_GET(_Watermark_Row_RepObjects);
    CK_PROPERTY_GET(_Watermark_ActivityBar_MaxHistory);
    CK_PROPERTY_GET(_Watermark_ActivityBar_ActiveColor);
    CK_PROPERTY_GET(_Watermark_ActivityBar_HeldAccentColor);
    CK_PROPERTY_GET(_Watermark_ActivityBar_InactiveColor);
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
    static const FCk_Watermark_ColorBands_LowerIsBetter&  Get_Watermark_UniqueEnsureCount_ColorBands();
    static FSlateFontInfo Get_Watermark_FontOverride();
    static int32          Get_Watermark_ValueFontSize();
    static int32          Get_Watermark_LabelFontSize();
    static int32        Get_Watermark_BracketFontSize();
    static int32        Get_Watermark_BuildType_FontSize();
    static int32        Get_Watermark_TextOutlineSize();
    static FLinearColor Get_Watermark_TextOutlineColor();
    static FVector2D    Get_Watermark_TextShadowOffset();
    static FLinearColor Get_Watermark_TextShadowColor();
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
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_Ensures();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_UniqueEnsures();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_Ram();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_Vram();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_MemoryPressure();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_Ping();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_ServerFps();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_Fps();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_Time();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_Frame();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_RepActors();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_RepComponents();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_RepObjects();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_BuildId();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_CpuBrand();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_OsVersion();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_CpuCores();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_NetworkType();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_NetMode();
    static FString      Get_Watermark_NetMode_Label_SinglePlayer();
    static FString      Get_Watermark_NetMode_Label_Server();
    static FString      Get_Watermark_NetMode_Label_ListenServer();
    static FString      Get_Watermark_NetMode_Label_Client();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_Jolt();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_EcsDebugger();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_EcsEntityMap();
    static ECk_Watermark_DisplayPolicy Get_Watermark_MinPolicy_EcsCallstacks();
    static FString      Get_Watermark_InfoBar_Separator();
    static FString      Get_Watermark_InfoBar_KeyValueSeparator();
    static int32        Get_Watermark_InfoBar_FontSize();
    static FLinearColor Get_Watermark_InfoBar_KeyColor();
    static FLinearColor Get_Watermark_InfoBar_ValueColor();
    static FLinearColor Get_Watermark_InfoBar_SeparatorColor();
    static FLinearColor                    Get_Watermark_BuildId_Active_Color();
    static FLinearColor                    Get_Watermark_BuildId_Inactive_Color();
    static const TArray<FString>&          Get_Watermark_BuildId_EnabledBranches();
    static int32                           Get_Watermark_Row_Ensures();
    static int32        Get_Watermark_Row_UniqueEnsures();
    static int32        Get_Watermark_Row_Ram();
    static int32        Get_Watermark_Row_Vram();
    static int32        Get_Watermark_Row_MemPressure();
    static int32        Get_Watermark_Row_Ping();
    static int32        Get_Watermark_Row_ServerFps();
    static int32        Get_Watermark_Row_Fps();
    static int32        Get_Watermark_Row_Time();
    static int32        Get_Watermark_Row_Frame();
    static int32        Get_Watermark_Row_RepActors();
    static int32        Get_Watermark_Row_RepComponents();
    static int32        Get_Watermark_Row_RepObjects();
    static int32        Get_Watermark_ActivityBar_MaxHistory();
    static FLinearColor Get_Watermark_ActivityBar_ActiveColor();
    static FLinearColor Get_Watermark_ActivityBar_HeldAccentColor();
    static FLinearColor Get_Watermark_ActivityBar_InactiveColor();
};

// --------------------------------------------------------------------------------------------------------------------
