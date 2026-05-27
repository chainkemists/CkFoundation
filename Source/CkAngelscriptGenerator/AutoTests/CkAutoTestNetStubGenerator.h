#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Generates C++ `IMPLEMENT_SIMPLE_AUTOMATION_TEST` stubs for every AS net-mode AutoTest class
// (i.e. every subclass of `UCk_AutoTest_Base` whose CDO `_NetMode` differs from `Standalone`).
//
// Companion to `FCkAutoTestWrapperGenerator`, which handles Standalone-mode tests via AS-side
// `ACk_AutoTestRunner` placeable wrappers. Net-mode tests don't fit that shape — they need a
// multi-PIE harness, not a single-world placed actor — so the stub is C++ that drives the
// harness latent commands.
//
// Output convention (one file per "feature" derived from the AS source path's `Script/Ck<Feature>/`
// subdir; falls back to `AS` when no such subdir exists):
//   Plugins/CkTests/Source/CkTests/Private/Net/Generated/<Feature>_NetAutoTestStubs.spec.cpp
//
// The file is unconditionally rooted under `CkTests` so the emitted C++ sees the harness API
// (`CkTests/Net/CkAutoTest_NetSubject.h`, `CkTests/Net/CkNetAutomation_Common.h`) and the
// generator does not need a build-dependency on `CkTests` itself — only the runtime path is
// crossed. `_NetMode` and `_TimeoutSeconds` are read off the CDO via reflection (same pattern
// the wrapper generator uses), so this generator's module deps stay unchanged.
//
// Triggers: same as `FCkAutoTestWrapperGenerator` — `OnPostEngineInit` and every successful
// AS PostCompile. Determinism guarantees match: stable sort, no timestamps, LF-normalized diff
// skip, CRLF on Windows write. See `CkAngelscriptGenerator/CLAUDE.md` "Determinism + drift
// safety" for the contract.
class CKANGELSCRIPTGENERATOR_API FCkAutoTestNetStubGenerator
{
public:
    // Full, deterministic regeneration across all features. Editor-only.
    static auto
    GenerateAll() -> void;

    // Mirrors `ECk_AutoTest_NetMode` (which lives in `CkTests` — out of reach for this module).
    // Read from CDO via FByteProperty reflection; see `Read_NetModeRaw`. Used by both this
    // generator and by `FCkAutoTestWrapperGenerator` to filter classes by mode.
    enum class ENetMode : uint8
    {
        Standalone                   = 0,
        ServerAndClientsIndependent  = 1,
        Replicated                   = 2,
    };

    // Reads the `_NetMode` UPROPERTY off the entity-script class's CDO via reflection. Returns
    // `Standalone` when the field is missing (back-compat for pre-Phase-3b tests) or the CDO
    // can't be read. Both this generator and the wrapper generator route filter decisions
    // through here so they stay in lock-step.
    static auto
    Read_NetMode(const UClass* InEntityScriptClass) -> ENetMode;
};

// --------------------------------------------------------------------------------------------------------------------
