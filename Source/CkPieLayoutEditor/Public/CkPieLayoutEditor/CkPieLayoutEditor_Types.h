#pragma once

#include "CkCore/Format/CkFormat.h"

#include "CkPieLayoutEditor_Types.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// How PIE windows are packed into the target monitor's work area.
UENUM(BlueprintType)
enum class ECk_PieLayout_Mode : uint8
{
    // Pick the rows/cols split whose per-cell aspect ratio is closest to the preferred aspect ratio.
    AutoBalanced,

    // Square-ish grid: cols = ceil(sqrt(N)), rows balanced. Every cell is the same size.
    EqualGrid,

    // The server (host) window gets an enlarged region; clients tile the remaining space.
    HostFocus,

    // Pack windows at their minimum size, top-left, no stretching to fill the work area.
    Compact,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PieLayout_Mode);

// --------------------------------------------------------------------------------------------------------------------

// Which monitor the grid is arranged on.
UENUM(BlueprintType)
enum class ECk_PieLayout_TargetMonitor : uint8
{
    // The OS primary monitor.
    PrimaryMonitor,

    // The monitor the Unreal Editor main window is currently on.
    EditorMonitor,

    // The monitor the mouse cursor is currently on.
    MouseCursorMonitor,

    // A specific monitor by index (see Specific Monitor Index).
    SpecificMonitor,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PieLayout_TargetMonitor);

// --------------------------------------------------------------------------------------------------------------------
