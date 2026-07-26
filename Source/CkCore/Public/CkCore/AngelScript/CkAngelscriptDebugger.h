#pragma once

// Mirrors the engine's WITH_AS_DEBUGSERVER condition (!UE_BUILD_TEST && !UE_BUILD_SHIPPING) through the
// always-defined core build macros, so this header compiles in Test/Shipping where
// FAngelscriptManager::DebugServer does not exist.
#if WITH_ANGELSCRIPT_CK && !UE_BUILD_SHIPPING && !UE_BUILD_TEST
#include <AngelscriptCode/Private/Debugging/AngelscriptDebugServer.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Ensure sites (Ensure_Impl, FCk_Handle::Get/Has, FCk_Registry::Get) suppress themselves while this is
    // true: a paused debugger walks property getters to fill the Variables panel, and firing ensures from
    // that inspection has crashed before.
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
