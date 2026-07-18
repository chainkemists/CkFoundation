#include "CkAssetExporter_RequestLoop.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/Server/CkAssetExporter_RequestProcessor.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include <HAL/PlatformProcess.h>
#include <HAL/PlatformTime.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_asset_exporter_requestloop
{
    constexpr auto IdleTimeoutSeconds = double{10.0 * 60.0};
    constexpr auto MaxRuntimeSeconds  = double{2.0 * 60.0 * 60.0};
    constexpr auto PollSeconds        = float{0.5f};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_RequestLoop::
    Run()
    -> int32
{
    using namespace ck_asset_exporter_requestloop;

    auto Processor = FCk_AssetExporter_RequestProcessor{};

    // WipeStale: the commandlet OWNS the queue at boot — clear anything a crashed prior server left behind.
    if (NOT Processor.Startup(ECk_AssetExporter_StaleRequestPolicy::WipeStale))
    {
        ck::asset_exporter::Error(TEXT("[Server] startup failed — cannot serve requests"));
        return 0;
    }

    ck::asset_exporter::Display(
        TEXT("[Server] ready — pid {}, polling [{}] every 500ms"),
        FPlatformProcess::GetCurrentProcessId(), Processor.Get_RequestsDir());

    const auto StartSeconds = FPlatformTime::Seconds();
    auto LastActivitySeconds = StartSeconds;

    auto Quit = false;
    while (NOT Quit)
    {
        const auto PollResult = Processor.ProcessPending();

        if (PollResult.QuitRequested)
        { break; }

        const auto Now = FPlatformTime::Seconds();

        // Actively serving — reset the idle clock, honor only the wall-clock cap, then re-poll immediately for
        // anything that arrived meanwhile (no idle sleep between back-to-back requests).
        if (PollResult.AnyProcessed)
        {
            LastActivitySeconds = Now;
            if (Now - StartSeconds >= MaxRuntimeSeconds)
            {
                ck::asset_exporter::Display(TEXT("[Server] {}h wall-clock cap reached — clean exit"), MaxRuntimeSeconds / 3600.0);
                break;
            }
            continue;
        }

        // Nothing to do this pass — evaluate the idle + wall-clock watchdogs, then sleep.
        if (Now - LastActivitySeconds >= IdleTimeoutSeconds)
        {
            ck::asset_exporter::Display(TEXT("[Server] idle {} min with no requests — clean exit"), IdleTimeoutSeconds / 60.0);
            break;
        }
        if (Now - StartSeconds >= MaxRuntimeSeconds)
        {
            ck::asset_exporter::Display(TEXT("[Server] {}h wall-clock cap reached — clean exit"), MaxRuntimeSeconds / 3600.0);
            break;
        }

        FPlatformProcess::Sleep(PollSeconds);
    }

    Processor.Shutdown();

    ck::asset_exporter::Display(TEXT("[Server] stopped"));
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
