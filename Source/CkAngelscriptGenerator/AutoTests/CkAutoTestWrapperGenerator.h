#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Emits `<Plugin>_AutoTestActors.as` — one `A<TestName>_Actor : ACk_AutoTestRunner` wrapper per
// `UCk_AutoTest_Base` subclass, so every AS test is placeable without a hand-written sibling class.
// Output paths, the hand-authored-wrapper collision opt-out, and triggers: the module's Claude.md.
class CKANGELSCRIPTGENERATOR_API FCkAutoTestWrapperGenerator
{
public:
    // Full, deterministic regeneration across all plugins. Editor-only.
    static auto
    GenerateAll() -> void;

    // Answers "which `A<X>_Actor` wrappers does the .as SOURCE declare", as BARE names (no leading
    // `A` — the form `Get_WrapperBareName` produces), so a hand-authored wrapper suppresses emission
    // even while its file fails to compile and no live UClass answers for it.
    // Candidates are matched over raw file text and then CONFIRMED through
    // `FCkAsSourceScanner::Parse_ClassDeclaration`, which blanks comments and string literals first —
    // a commented-out or quoted declaration is never counted.
    // Costs one read of every .as outside `Generated/` per pass, which is cheap beside the
    // AngelScript recompile that triggers the pass.
    static auto
    Collect_SourceDeclaredWrapperNames(const TArray<FString>& InAsSourceFiles) -> TSet<FString>;
};

// --------------------------------------------------------------------------------------------------------------------
