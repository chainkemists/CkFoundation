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

    // The build id REPORTED across the network (server stamp into GameState + client report via RPC).
    // Equals Get_BuildId() except when the dev-only CVar `ck.Net.BuildIdOverride` is non-empty
    // (non-shipping only) — then it returns the override. Comparisons keep using Get_BuildId(), so an
    // override forces a client/server version mismatch for testing the watermark rows (in PIE both sides
    // share one build, so the real hashes always match). Defined in CkBuildId.cpp.
    CKCORE_API auto Get_ReportedBuildId() -> FString;
}

// --------------------------------------------------------------------------------------------------------------------
