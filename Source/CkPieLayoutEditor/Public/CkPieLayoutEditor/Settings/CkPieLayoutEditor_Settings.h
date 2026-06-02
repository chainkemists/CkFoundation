#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkPieLayoutEditor/CkPieLayoutEditor_Types.h"

#include "CkSettings/UserSettings/CkUserSettings.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkPieLayoutEditor_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "PIE Layout"))
class CKPIELAYOUTEDITOR_API UCk_PieLayout_Settings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_PieLayout_Settings_UE);

private:
    // ---- Automation ----

    UPROPERTY(Config, EditAnywhere, Category = "Automation",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "Automatically arrange the PIE windows into the grid after Play starts."))
    bool _AutoArrangeOnPIE = true;

    UPROPERTY(Config, EditAnywhere, Category = "Automation",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "Arrange even when there is only a single PIE window. Disable to leave a lone window where the editor placed it."))
    bool _ArrangeSingleWindow = false;

    UPROPERTY(Config, EditAnywhere, Category = "Automation",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "When Play stops, move the PIE windows back to where the editor opened them (before this plugin arranged them). Leave off to keep the arranged layout."))
    bool _RestoreWindowPositionsOnStop = true;

    UPROPERTY(Config, EditAnywhere, Category = "Automation",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "Time to wait after Play starts before the first arrange pass (lets the windows finish opening)."))
    FCk_Time _InitialArrangeDelay = FCk_Time{0.15};

    UPROPERTY(Config, EditAnywhere, Category = "Automation",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "How long to keep re-checking after the first pass, so client windows created shortly after Play are still placed."))
    FCk_Time _RetryDuration = FCk_Time{6.0};

    UPROPERTY(Config, EditAnywhere, Category = "Automation",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "Time between retry passes during the retry window. Already-arranged windows are not re-moved."))
    FCk_Time _RetryInterval = FCk_Time{0.1};

    // ---- Monitor ----

    UPROPERTY(Config, EditAnywhere, Category = "Monitor",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "Which monitor the grid is arranged on."))
    ECk_PieLayout_TargetMonitor _TargetMonitor = ECk_PieLayout_TargetMonitor::PrimaryMonitor;

    UPROPERTY(Config, EditAnywhere, Category = "Monitor",
              meta = (AllowPrivateAccess = true, ClampMin = "0", UIMin = "0",
                      EditCondition = "_TargetMonitor == ECk_PieLayout_TargetMonitor::SpecificMonitor",
                      ToolTip = "Monitor index to target when Target Monitor is set to Specific Monitor. Clamped to the available monitor count."))
    int32 _SpecificMonitorIndex = 0;

    // ---- Layout ----

    UPROPERTY(Config, EditAnywhere, Category = "Layout",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "How the windows are packed into the work area."))
    ECk_PieLayout_Mode _LayoutMode = ECk_PieLayout_Mode::AutoBalanced;

    UPROPERTY(Config, EditAnywhere, Category = "Layout",
              meta = (AllowPrivateAccess = true, ClampMin = "0", UIMin = "0", UIMax = "200",
                      ToolTip = "Pixels of empty space between the grid and the edges of the monitor work area."))
    int32 _OuterMargin = 8;

    UPROPERTY(Config, EditAnywhere, Category = "Layout",
              meta = (AllowPrivateAccess = true, ClampMin = "0", UIMin = "0", UIMax = "200",
                      ToolTip = "Pixels of empty space between adjacent windows."))
    int32 _WindowGap = 8;

    UPROPERTY(Config, EditAnywhere, Category = "Layout",
              meta = (AllowPrivateAccess = true, ClampMin = "64", UIMin = "64",
                      ToolTip = "Smallest width (pixels) a PIE window is allowed to be shrunk to."))
    int32 _MinimumWindowWidth = 420;

    UPROPERTY(Config, EditAnywhere, Category = "Layout",
              meta = (AllowPrivateAccess = true, ClampMin = "64", UIMin = "64",
                      ToolTip = "Smallest height (pixels) a PIE window is allowed to be shrunk to."))
    int32 _MinimumWindowHeight = 280;

    UPROPERTY(Config, EditAnywhere, Category = "Layout",
              meta = (AllowPrivateAccess = true, ClampMin = "0.1", UIMin = "0.1", UIMax = "4.0",
                      ToolTip = "Auto Balanced avoids cell layouts whose width/height aspect falls below this (keeps cells from going portrait)."))
    float _MinimumLandscapeRatio = 1.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Layout",
              meta = (AllowPrivateAccess = true, ClampMin = "0.1", UIMin = "0.1", UIMax = "4.0",
                      ToolTip = "Auto Balanced targets this cell width/height aspect ratio (1.777778 = 16:9)."))
    float _PreferredAspectRatio = 1.777778f;

    // ---- PIE Windows ----

    UPROPERTY(Config, EditAnywhere, Category = "PIE Windows",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "Include the listen-server window in the grid. Disable to arrange only the client windows."))
    bool _IncludeListenServer = true;

    UPROPERTY(Config, EditAnywhere, Category = "PIE Windows",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "Place the server/host window in the first grid slot. Disable to order strictly by PIE instance index."))
    bool _ServerFirst = true;

public:
    CK_PROPERTY_GET(_AutoArrangeOnPIE);
    CK_PROPERTY_GET(_ArrangeSingleWindow);
    CK_PROPERTY_GET(_RestoreWindowPositionsOnStop);
    CK_PROPERTY_GET(_InitialArrangeDelay);
    CK_PROPERTY_GET(_RetryDuration);
    CK_PROPERTY_GET(_RetryInterval);
    CK_PROPERTY_GET(_TargetMonitor);
    CK_PROPERTY_GET(_SpecificMonitorIndex);
    CK_PROPERTY_GET(_LayoutMode);
    CK_PROPERTY_GET(_OuterMargin);
    CK_PROPERTY_GET(_WindowGap);
    CK_PROPERTY_GET(_MinimumWindowWidth);
    CK_PROPERTY_GET(_MinimumWindowHeight);
    CK_PROPERTY_GET(_MinimumLandscapeRatio);
    CK_PROPERTY_GET(_PreferredAspectRatio);
    CK_PROPERTY_GET(_IncludeListenServer);
    CK_PROPERTY_GET(_ServerFirst);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKPIELAYOUTEDITOR_API UCk_Utils_PieLayout_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_PieLayout_Settings_UE);

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static bool
    Get_AutoArrangeOnPIE();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static bool
    Get_ArrangeSingleWindow();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static bool
    Get_RestoreWindowPositionsOnStop();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static FCk_Time
    Get_InitialArrangeDelay();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static FCk_Time
    Get_RetryDuration();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static FCk_Time
    Get_RetryInterval();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static ECk_PieLayout_TargetMonitor
    Get_TargetMonitor();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static int32
    Get_SpecificMonitorIndex();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static ECk_PieLayout_Mode
    Get_LayoutMode();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static int32
    Get_OuterMargin();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static int32
    Get_WindowGap();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static int32
    Get_MinimumWindowWidth();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static int32
    Get_MinimumWindowHeight();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static float
    Get_MinimumLandscapeRatio();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static float
    Get_PreferredAspectRatio();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static bool
    Get_IncludeListenServer();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PieLayout|Settings")
    static bool
    Get_ServerFirst();
};

// --------------------------------------------------------------------------------------------------------------------
