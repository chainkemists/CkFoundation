#include "CkAssetExporter/VfxCorpusExporter/CkVfxCorpusExporter.h"

#include "CkCore/Macros/CkMacros.h"

#include "HAL/PlatformMisc.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------
// Batch-exports the VFX corpus (every Niagara + Cascade system under /Game) to Saved/CkVfxCorpus.
//
// Gating: this is a content-dependent, whole-project load that wipes/rewrites the corpus dir. StressFilter would
// keep it out of default passes, but the automation commandline's "Standard" filter (Smoke|Engine|Product|Perf —
// AutomationCommandline.cpp) makes Stress tests invisible to ListAllTests/RunTests entirely, so the toolbox could
// never run it. It therefore ships as ProductFilter with an explicit opt-in: set CK_VFX_CORPUS_EXPORT=1 in the
// environment to actually export; anything else (default suite passes, consumer projects) is an instant skip.
// Per-asset export failures are DATA (recorded in index.json); the test only fails when a requested run over
// existing content couldn't produce a corpus.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_AssetExporter_ExportVfxCorpus_Test,
    "Ck.AssetExporter.ExportVfxCorpus",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_AssetExporter_ExportVfxCorpus_Test::RunTest(const FString& InParameters)
{
    if (FPlatformMisc::GetEnvironmentVariable(TEXT("CK_VFX_CORPUS_EXPORT")) != TEXT("1"))
    {
        AddInfo(TEXT("Skipped: set CK_VFX_CORPUS_EXPORT=1 to run the corpus export (heavyweight; wipes Saved/CkVfxCorpus)"));
        return true;
    }

    const auto Roots = TArray<FString>{ TEXT("/Game") };
    const auto Summary = FCk_VfxCorpusExporter::ExportCorpus(Roots, FCk_VfxCorpusExporter::Get_DefaultCorpusRoot());

    if (Summary.Systems == 0)
    {
        AddInfo(TEXT("No particle systems under /Game — nothing to export (skipped)"));
        return true;
    }

    AddInfo(FString::Printf(
        TEXT("Corpus: %d systems (%d niagara, %d cascade, %d failed), %d materials (%d failed), %d textures (%d failed) -> %s"),
        Summary.Systems, Summary.Niagara, Summary.Cascade, Summary.FailedSystems,
        Summary.Materials, Summary.FailedMaterials, Summary.Textures, Summary.FailedTextures, *Summary.CorpusRoot));

    TestTrue(TEXT("Corpus export ran to completion"), Summary.Succeeded);

    if (Summary.FailedSystems > 0)
    { AddWarning(FString::Printf(TEXT("%d systems failed to export - see index.json for reasons"), Summary.FailedSystems)); }

    return true;
}

#endif
