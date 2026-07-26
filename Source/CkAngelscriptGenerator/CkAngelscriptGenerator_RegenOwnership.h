#pragma once

#include "CoreMinimal.h"

#include <GenericPlatform/GenericPlatformFile.h>
#include <Templates/UniquePtr.h>

// --------------------------------------------------------------------------------------------------------------------

// Cross-process single-writer guard for every generated-file mutation this module performs.
// Two editor processes of the same project regenerate from divergent in-process views and each
// write trips the other's mtime-based AS hot-reload watcher, so the file flip-flops forever;
// only serializing the writers converges it. Rationale, the gate list (G1-G11) and the POSIX
// degradation caveat: Claude.md § "Cross-process single-writer ownership".
class CKANGELSCRIPTGENERATOR_API FCkAngelscriptGenerator_RegenOwnership
{
public:
    // Ownership is LAZY — a secondary that keeps calling this becomes owner once the prior owner
    // exits. Game-thread only (ensured). Held until Release().
    static auto
    Try_AcquireOrGet_IsOwner(
        FStringView InGateSite) -> bool;

    static auto
    Release() -> void;

    static auto
    Get_StatusString() -> FString;

    static auto
    Get_LockFilePath() -> FString;

    // Exclusivity primitive on an arbitrary path — exposed so the unit test can exercise
    // acquire/conflict/release without touching the real lock.
    static auto
    Try_OpenExclusiveWriteHandle(
        const FString& InLockFilePath) -> TUniquePtr<IFileHandle>;
};

// --------------------------------------------------------------------------------------------------------------------
