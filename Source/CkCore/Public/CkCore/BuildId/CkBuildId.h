#pragma once

#include "CkCore/Generated/CkCore_BuildId.h"

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------
// The short git hash is baked in at build time by CkCore.Build.cs::GenerateBuildIdHeader and compared
// across the network (server <-> client) to flag version mismatches.

namespace ck
{
    // "unknown" when git was unavailable at build time.
    inline auto Get_BuildId() -> FString
    {
        return FString{UTF8_TO_TCHAR(CkCoreBuildId::HeadHash)};
    }

    // The id REPORTED over the network. Equals Get_BuildId() except when the non-shipping CVar
    // `ck.Net.BuildIdOverride` is set; comparisons keep using Get_BuildId(), so an override forces a
    // client/server mismatch on purpose.
    CKCORE_API auto Get_ReportedBuildId() -> FString;
}

// --------------------------------------------------------------------------------------------------------------------
