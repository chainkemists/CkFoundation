#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Generates one AngelScript file per plugin (and one for the game module) containing one
// `A<TestName>_Actor : ACk_AutoTestRunner` wrapper class per `UCk_AutoTest_Base` subclass found
// in that plugin. The wrapper bakes in `default _TestEntityScriptClass = U<TestName>;` so the
// class is immediately pickable from the Place Actors panel without the test author having to
// hand-write a sibling class.
//
// Output convention (mirrors CkAngelscriptEntityScriptParamsGenerator):
//   <PluginBaseDir>/Script/Generated/<PluginName>_AutoTestActors.as
//   <ProjectDir>/Script/Generated/<ProjectName>_AutoTestActors.as  (game-module classes)
//
// Collision rule:
//   If a class named `A<TestName>_Actor` already exists (hand-authored by the user, or a
//   different test happens to share the name), the generator skips emission for that test.
//   This preserves existing hand-authored wrappers untouched and gives authors a simple opt-out
//   mechanism: write your own wrapper with the conventional name and the generator stays out
//   of the way.
//
// Triggers (editor-only):
//   - FCoreDelegates::OnPostEngineInit   — ensures file exists before the first manual recompile.
//   - FAngelscriptCodeModule::GetPostCompile() — picks up newly compiled AS-defined tests.
class CKANGELSCRIPTGENERATOR_API FCkAutoTestWrapperGenerator
{
public:
    // Full, deterministic regeneration across all plugins. Editor-only.
    static auto
    GenerateAll() -> void;
};

// --------------------------------------------------------------------------------------------------------------------
