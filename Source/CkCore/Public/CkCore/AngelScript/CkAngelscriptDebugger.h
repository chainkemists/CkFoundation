#pragma once

// FAngelscriptManager::DebugServer exists only when the engine's WITH_AS_DEBUGSERVER is
// set — i.e. (!UE_BUILD_TEST && !UE_BUILD_SHIPPING). Mirror that condition here (using the
// always-defined core build macros so we don't depend on include order) so this header
// compiles in Test/Shipping, where the AngelScript debug server is stripped.
#if WITH_ANGELSCRIPT_CK && !UE_BUILD_SHIPPING && !UE_BUILD_TEST
#include <AngelscriptCode/Private/Debugging/AngelscriptDebugServer.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Returns true when the Angelscript VS Code debugger is currently paused on a breakpoint.
    //
    // While paused, AS walks property getters to populate the Variables panel; invoking ensures from those
    // inspection calls is undesirable (and historically crashed). Used inside Ensure_Impl, FCk_Handle::Get<>,
    // FCk_Handle::Has<>, and FCk_Registry::Get<> to silently return default values instead of firing ensures
    // during AS-driven inspection.
    inline auto
        Is_AngelscriptDebugger_Paused()
        -> bool
    {
    #if WITH_ANGELSCRIPT_CK && !UE_BUILD_SHIPPING && !UE_BUILD_TEST
        auto* DebugServer = FAngelscriptManager::Get().DebugServer;
        return DebugServer != nullptr && DebugServer->bIsDebugging && DebugServer->bIsPaused;
    #else
        return false;
    #endif
    }
}

// --------------------------------------------------------------------------------------------------------------------
