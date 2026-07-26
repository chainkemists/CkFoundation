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
};

// --------------------------------------------------------------------------------------------------------------------
