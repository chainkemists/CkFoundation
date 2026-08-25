#pragma once

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Global runtime visibility policy for Ck diagnostics.
 *
 * HiddenForStreamerMode is an absolute presentation override: consumers must not render diagnostic information while
 * it is active, regardless of their own local diagnostic settings.
 */
enum class ECk_DiagnosticVisibility_Mode : uint8
{
    Visible,
    HiddenForStreamerMode,
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::diagnostic_visibility
{
    /** Resolves the process-wide Ck diagnostic visibility policy. */
    CKCORE_API auto
    Get_Mode() -> ECk_DiagnosticVisibility_Mode;

    /** True when streamer mode requires all Ck diagnostics to remain off-screen. */
    CKCORE_API auto
    Is_HiddenForStreamerMode() -> bool;
}
