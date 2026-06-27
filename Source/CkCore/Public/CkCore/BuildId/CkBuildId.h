#pragma once

#include "CkCore/Generated/CkCore_BuildId.h"

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------
// Build identity — the short git hash baked in at build time (the "build #"). Compared across the
// network (server <-> client) to flag version mismatches. See CkCore.Build.cs::GenerateBuildIdHeader.

namespace ck
{
    // The local build's short git hash (e.g. "fb32828"). Identical on two machines only when they were
    // built from the same commit. Returns "unknown" if git was unavailable at build time.
    inline auto Get_BuildId() -> FString
    {
        return FString{UTF8_TO_TCHAR(CkCoreBuildId::HeadHash)};
    }
}

// --------------------------------------------------------------------------------------------------------------------
