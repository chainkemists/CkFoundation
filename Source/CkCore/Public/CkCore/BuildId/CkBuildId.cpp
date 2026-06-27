#include "CkCore/BuildId/CkBuildId.h"

#include "CkCore/Macros/CkMacros.h"

#if !UE_BUILD_SHIPPING
#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------
// Dev-only override for the build id REPORTED across the network. In PIE the listen-server and client share
// one build, so the real hashes always match — set this to any non-empty value to force a version mismatch
// on both the client's "Server" row and the host's per-client rows (comparisons keep using the real id).
// Clear it to restore normal behavior. Compiled out of shipping.
static TAutoConsoleVariable<FString> GCVar_BuildIdOverride(
    TEXT("ck.Net.BuildIdOverride"),
    TEXT(""),
    TEXT("Dev-only: when non-empty, overrides the build id reported across the network (server stamp + ")
    TEXT("client report). Local comparisons keep the real build id, so this forces a client/server version ")
    TEXT("mismatch for testing the CkWatermark Server/Client version rows. Clear to restore normal behavior."),
    ECVF_Default);
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::Get_ReportedBuildId()
    -> FString
{
#if !UE_BUILD_SHIPPING
    if (const auto Override = GCVar_BuildIdOverride.GetValueOnGameThread();
        NOT Override.IsEmpty())
    { return Override; }
#endif
    return ck::Get_BuildId();
}

// --------------------------------------------------------------------------------------------------------------------
