#include "CkParticlesEditor/Generator/CkParticles_TuningGenerator.h"

#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"
#include "CkParticles/ScriptDefinition/CkParticles_TuningDefinition.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------
// Headless top-up of the per-behavior tuning DataAssets. Env-gated so the default test pass never mutates plugin
// content: set CK_PARTICLES_GENERATE_TUNING=1 in the spawning shell, then run the toolbox with
// --test --test-pattern GenerateTuningAssets --discover-fresh. Unlike RebuildTemplateAssets this is plain asset
// authoring, so it has no CK_WITH_PARTICLES branch and runs on a stock engine.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_Particles_GenerateTuningAssets_Test,
    "Ck.Particles.GenerateTuningAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_Particles_GenerateTuningAssets_Test::RunTest(const FString& Parameters)
{
    if (FPlatformMisc::GetEnvironmentVariable(TEXT("CK_PARTICLES_GENERATE_TUNING")) != TEXT("1"))
    {
        AddInfo(TEXT("Skipped — set CK_PARTICLES_GENERATE_TUNING=1 to create the missing CkParticles tuning assets."));
        return true;
    }

    ck::particles_editor::Generate_AllTuningAssets();

    // Driven off the roster, so adding a behavior does not need this test edited.
    for (auto BehaviorId = 0; BehaviorId < ck::particles::NumBehaviors; ++BehaviorId)
    {
        const auto Path = ck::particles::Get_BehaviorTuningAssetObjectPath(BehaviorId);
        auto* Tuning = LoadObject<UCkParticles_TuningDefinition>(nullptr, *Path);
        TestNotNull(*FString::Printf(TEXT("tuning asset for behavior %d loads [%s]"), BehaviorId, *Path), Tuning);
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
