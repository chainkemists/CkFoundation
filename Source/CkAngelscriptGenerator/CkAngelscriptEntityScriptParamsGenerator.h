#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Generates one AngelScript file per plugin (and one for the game module) containing one namespace
// per UCk_EntityScript_UE subclass found in that plugin. Each namespace exposes a USTRUCT `Params`
// matching the EntityScript's UPROPERTY(ExposeOnSpawn) properties (flattened across the hierarchy,
// since AngelScript has no struct inheritance).
//
// Output convention (mirrors CkAngelscriptWrapperGenerator):
//   <PluginBaseDir>/Script/Generated/<PluginName>_EntitySpawnParams.as
//   <ProjectDir>/Script/Generated/<ProjectName>_EntitySpawnParams.as  (game-module classes)
//
// Triggers (editor-only):
//   - FCoreDelegates::OnPostEngineInit   — ensures file exists before first manual AS recompile.
//   - FAngelscriptCodeModule::GetPostCompile() — picks up newly compiled AS-defined EntityScripts.
class CKANGELSCRIPTGENERATOR_API FCkAngelscriptEntityScriptParamsGenerator
{
public:
    // Full, deterministic regeneration across all plugins. Editor-only.
    static auto
    GenerateAll() -> void;

    // The class filter GenerateAll enumerates with. Exposed for unit tests — the
    // Blueprint-generated-class exclusion is load-bearing for cross-process determinism
    // (BP classes only exist in processes that loaded the BP; see Tests/Test_EntityScriptParamsGenerator.cpp).
    static auto
    Is_IncludedEntityScriptClass(
        UClass* InClass) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
