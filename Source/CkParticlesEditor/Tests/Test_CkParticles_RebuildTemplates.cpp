#include "CkParticlesEditor/Generator/CkParticles_TemplateBuilder.h"

#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"

#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------
// Headless regeneration of every code-built CkParticles asset (textures -> master materials -> carrier meshes ->
// both template systems). Env-gated so the default test pass never mutates plugin content: set
// CK_PARTICLES_REBUILD_TEMPLATES=1 in the spawning shell, then run via the toolbox
//   --test --test-pattern RebuildTemplateAssets --discover-fresh
// (mirrors the CK_VFX_CORPUS_EXPORT gate pattern in CkAssetExporter — StressFilter is invisible to the
// automation commandline, so ProductFilter + env gate is the sanctioned shape for opt-in heavy tests).
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

    TestTrue(TEXT("Build_AllTemplateSystems (textures + materials + meshes + both templates)"),
        ck::particles_editor::Build_AllTemplateSystems());

    TestNotNull(TEXT("Continuous seed template loads"),
        LoadObject<UNiagaraSystem>(nullptr, *ck::particles::Get_DefaultTemplateSystemObjectPath()));
    TestNotNull(TEXT("Burst template loads"),
        LoadObject<UNiagaraSystem>(nullptr, *ck::particles::Get_BurstTemplateSystemObjectPath()));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
