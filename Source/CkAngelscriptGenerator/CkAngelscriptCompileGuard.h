#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR && WITH_ANGELSCRIPT_CK

namespace ck::angelscriptgenerator
{
    // Fires CK_ENSURE_IF_NOT on PIE start when AngelScript has outstanding compile errors,
    // so devs cannot accidentally start play with broken script. Editor-only.
    class FCk_AngelscriptCompileGuard
    {
    public:
        static auto Install()   -> void;
        static auto Uninstall() -> void;
    };
}

#endif

// --------------------------------------------------------------------------------------------------------------------
