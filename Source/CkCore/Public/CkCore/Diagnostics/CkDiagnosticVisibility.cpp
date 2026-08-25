#include "CkDiagnosticVisibility.h"

#include <HAL/IConsoleManager.h>
#include <Misc/CommandLine.h>
#include <Misc/Parse.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::diagnostic_visibility
{
    namespace streamer_mode
    {
        TAutoConsoleVariable<int32> CVar_Enabled(
            TEXT("ck.Debug.StreamerMode"),
            0,
            TEXT("Hide all Ck runtime diagnostics for marketing captures. "
                 "0 = show diagnostics, 1 = hide diagnostics. "
                 "The -CkStreamerMode launch parameter also enables this policy."),
            ECVF_Default);
    }

    auto
    Get_Mode() -> ECk_DiagnosticVisibility_Mode
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("CkStreamerMode")) ||
            streamer_mode::CVar_Enabled.GetValueOnAnyThread() != 0)
        {
            return ECk_DiagnosticVisibility_Mode::HiddenForStreamerMode;
        }

        return ECk_DiagnosticVisibility_Mode::Visible;
    }

    auto
    Is_HiddenForStreamerMode() -> bool
    {
        return Get_Mode() == ECk_DiagnosticVisibility_Mode::HiddenForStreamerMode;
    }
}
