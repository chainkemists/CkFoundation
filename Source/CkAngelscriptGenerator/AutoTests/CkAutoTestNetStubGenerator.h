#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Emits committed C++ `IMPLEMENT_SIMPLE_AUTOMATION_TEST` stubs for every AS AutoTest whose CDO
// `_NetMode` is not `Standalone` — those need a multi-PIE harness rather than the placeable actor
// `FCkAutoTestWrapperGenerator` emits. A stub MUST land in the same git repo as the .as class it
// references (the generator prunes stale files, and git must keep the pair atomic across branch
// switches); the three-destination routing that guarantees that lives in the module's Claude.md.
//
// The module hosting an emitted stub must PUBLICLY depend on `CkTests` — the stub includes the
// harness headers and resolves `/Script/CkTests.Ck_AutoTest_NetSubject`. This generator itself
// never depends on CkTests: only the emitted runtime path crosses over.
class CKANGELSCRIPTGENERATOR_API FCkAutoTestNetStubGenerator
{
public:
    // Full, deterministic regeneration across all features. Editor-only.
    static auto
    GenerateAll() -> void;

    // Mirrors `ECk_AutoTest_NetMode`, which lives in `CkTests` — out of reach for this module.
    // The values must stay in lock-step with it; they are read off the CDO as raw bytes.
    enum class ENetMode : uint8
    {
        Standalone                   = 0,
        ServerAndClientsIndependent  = 1,
        Replicated                   = 2,
    };

    // Returns `Standalone` when the field is missing or the CDO cannot be read. Every mode filter
    // in both generators routes through here so the two stay in lock-step.
    static auto
    Read_NetMode(const UClass* InEntityScriptClass) -> ENetMode;

    // Nearest-to-leaf DIRECTORY match wins, never the filename: `Script/Ck<Feature>/` (plugin) or
    // `Tests/<Feature>/` (project). Falls back to `AS` so unconventional paths stay discoverable.
    static auto
    Derive_FeatureFromSourcePath(const FString& InSourcePath) -> FString;

    // Deliberately tests `<ProjectDir>/Script/`, NOT `<ProjectDir>/` — plugins physically live
    // under the project dir too. `InProjectDir` is a parameter so unit tests can supply their own.
    static auto
    Get_IsProjectAuthoredPath(const FString& InSourcePath, const FString& InProjectDir) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
