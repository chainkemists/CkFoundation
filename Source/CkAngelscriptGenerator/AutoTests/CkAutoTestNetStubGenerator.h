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
// Output convention (one file per "feature" derived from the AS source path; falls back to `AS`
// when no convention matches):
//   * Plugin-authored tests (`Plugins/<X>/Script/Ck<Feature>/...`):
//       Plugins/CkTests/Source/CkTests/Private/Net/Generated/<Feature>_NetAutoTestStubs.spec.cpp
//   * Project-authored tests (`<ProjectDir>/Script/Tests/<Feature>/...`, or anywhere under the
//     project's `Script/` tree):
//       <ProjectDir>/Source/<ProjectName>/Tests/Net/Generated/<Feature>_NetAutoTestStubs.spec.cpp
//
// The split is load-bearing, not cosmetic: these stubs are COMMITTED (UBT must compile them
// before the editor can ever boot to regenerate them), and the generator actively prunes stale
// files. A stub must therefore live in the SAME repo as the .as test class it references — git
// keeps the pair atomic across branch switches. Before the split, a project-authored test's stub
// landed in the CkTests submodule; any machine whose project checkout lacked that class (older
// branch, different host project) had the committed file pruned, surfacing as a mysterious
// deletion in the submodule on every boot.
//
// Plugin-authored output is rooted under `CkTests` so the emitted C++ sees the harness API
// (`CkTests/Net/CkAutoTest_NetSubject.h`, `CkTests/Net/CkNetAutomation_Common.h`) and the
// generator does not need a build-dependency on `CkTests` itself — only the runtime path is
// crossed. Project-authored output requires the project's primary module to depend on `CkTests`
// (the harness headers are public and the latent commands are `CKTESTS_API`-exported).
// `_NetMode` and `_TimeoutSeconds` are read off the CDO via reflection (same pattern the wrapper
// generator uses), so this generator's module deps stay unchanged.
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

    // Derives the per-test "feature" bucket segment from an AS source file path. Pure path
    // logic, exposed for unit testing. Recognizes (nearest-to-leaf match wins, the matched
    // segment must be a directory, never the filename):
    //   * `Script/Ck<Feature>/` — plugin convention -> `<Feature>`
    //   * `Tests/<Feature>/`    — project convention (`<ProjectDir>/Script/Tests/<Feature>/`)
    // Falls back to `AS` when neither applies, keeping unconventionally-located tests
    // discoverable.
    static auto
    Derive_FeatureFromSourcePath(const FString& InSourcePath) -> FString;

    // True when the AS source path lives under the host project's own `Script/` tree (as
    // opposed to a plugin's). Drives the output-root split documented above. `InProjectDir`
    // is parameterized for unit testing; production callers pass `FPaths::ProjectDir()`.
    // Note this deliberately checks `<ProjectDir>/Script/` and not `<ProjectDir>/` — plugins
    // physically live under the project dir too.
    static auto
    Get_IsProjectAuthoredPath(const FString& InSourcePath, const FString& InProjectDir) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
