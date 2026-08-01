#include "CkParticlesEditor/Generator/CkParticles_TemplateBuilder.h"

#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"

#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------
// Headless regeneration of every code-built CkParticles asset. Env-gated so the default test pass never mutates
// plugin content: set CK_PARTICLES_REBUILD_TEMPLATES=1 in the spawning shell, then run the toolbox with
// --test --test-pattern RebuildTemplateAssets --discover-fresh. StressFilter is invisible to the automation
// commandline, so ProductFilter + an env gate is the sanctioned shape for an opt-in heavy test.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_Particles_RebuildTemplateAssets_Test,
    "Ck.Particles.RebuildTemplateAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_Particles_RebuildTemplateAssets_Test::RunTest(const FString& Parameters)
{
    if (FPlatformMisc::GetEnvironmentVariable(TEXT("CK_PARTICLES_REBUILD_TEMPLATES")) != TEXT("1"))
    {
        AddInfo(TEXT("Skipped — set CK_PARTICLES_REBUILD_TEMPLATES=1 to regenerate the CkParticles template assets."));
        return true;
    }

    TestTrue(TEXT("Build_AllTemplateSystems (textures + materials + meshes + every cadence template)"),
        ck::particles_editor::Build_AllTemplateSystems());

    // Driven off the cadence table, so adding a cadence row does not need this test edited.
    for (const auto& Spec : ck::particles::Get_TemplateSpecs())
    {
        const auto Path = ck::particles::Get_TemplateSystemObjectPath(Spec.AssetName);
        TestNotNull(*FString::Printf(TEXT("template [%s] loads"), Spec.AssetName),
            LoadObject<UNiagaraSystem>(nullptr, *Path));
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
