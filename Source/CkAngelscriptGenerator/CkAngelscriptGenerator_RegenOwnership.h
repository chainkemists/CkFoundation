#pragma once

#include "CoreMinimal.h"

#include <GenericPlatform/GenericPlatformFile.h>
#include <Templates/UniquePtr.h>

// --------------------------------------------------------------------------------------------------------------------

// Cross-process single-writer guard for every generated-file mutation this module performs
// (Script/Generated/*.as, DynamicHandleTypes.json, net-test .spec.cpp stubs, _StubRecovery_*
// self-heal siblings).
//
// Why: two editor processes of the SAME project regenerate the same files from their own
// in-process view (e.g. TObjectIterator class sets), and each write trips the other instance's
// mtime-based AS hot-reload watcher. When their views differ even slightly the file flip-flops
// forever — the 2026-06-12 incident: 496 mirror-image rewrites of BusterBlock_EntitySpawnParams.as,
// 686 full AS reloads, a headless compile-check boot that never honored -ExecCmds="Quit".
// Compare-before-write cannot converge this (both processes are "right" relative to their own
// truth); only serializing writers can.
//
// Mechanism: an exclusive-write OS file handle on Saved/CkAngelscriptGenerator_RegenOwner.lock,
// held for process lifetime. Windows CreateFile sharing semantics (GENERIC_WRITE +
// FILE_SHARE_READ) make a second OpenWrite on the same path fail with a sharing violation; the
// OS releases the handle on ANY process exit (clean, crash, kill), so stale locks are
// impossible. PLATFORM CAVEAT: POSIX OpenWrite does not enforce write exclusivity — on
// non-Windows this guard degrades to always-granting ownership. Acceptable: the editor workflow
// this protects is Windows-only (documented in the module CLAUDE.md).
//
// Ownership is LAZY: every gated site calls Try_AcquireOrGet_IsOwner, so a long-lived secondary
// instance becomes the owner automatically at its next regen event after the prior owner exits.
// Once acquired, held until ShutdownModule.
class CKANGELSCRIPTGENERATOR_API FCkAngelscriptGenerator_RegenOwnership
{
public:
    // Returns true when this process owns (or just acquired) regen ownership.
    // Cheap when already owner (one bool check). On first refusal logs a prominent
    // one-time Warning; subsequent refusals log VeryVerbose with the gate-site name.
    // Game-thread only (ensured).
    static auto
    Try_AcquireOrGet_IsOwner(
        FStringView InGateSite) -> bool;

    static auto
    Release() -> void;

    static auto
    Get_StatusString() -> FString;

    static auto
    Get_LockFilePath() -> FString;

    // Exclusivity primitive on an arbitrary path — exposed so the unit test can validate
    // acquire/conflict/release semantics against a temp path without touching the real lock.
    static auto
    Try_OpenExclusiveWriteHandle(
        const FString& InLockFilePath) -> TUniquePtr<IFileHandle>;
};

// --------------------------------------------------------------------------------------------------------------------
