#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AssetBlockPatcher.h"
#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AssetValueDiff.h"
#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AssetWriteBack.h"

#include "CkCore/Macros/CkMacros.h"

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptManager.h>
#include <ClassGenerator/ASClass.h>
#endif

#if WITH_DEV_AUTOMATION_TESTS && WITH_ANGELSCRIPT_CK

using namespace ck::angelscriptgenerator::write_back;

namespace ck_test_asset_write_back_integration
{
    struct FLiteralAsset
    {
        FString  Name;
        UObject* Instance = nullptr;
    };

    // Every literal asset actually declared by a loaded AngelScript module whose class is native —
    // the same population the toolbar button offers itself on.
    auto Collect_NativeLiteralAssets() -> TArray<FLiteralAsset>
    {
        auto Out = TArray<FLiteralAsset>{};

        auto* AssetsPackage = FAngelscriptManager::Get().AssetsPackage;
        if (ck::Is_NOT_Valid(AssetsPackage, ck::IsValid_Policy_NullptrOnly{}))
        { return Out; }

        for (const auto& Module : FAngelscriptManager::Get().GetActiveModules())
        {
            for (const auto& Name : Module->DeclaredLiteralAssets)
            {
                auto* Instance = FindObject<UObject>(AssetsPackage, *Name);
                if (NOT FCkAsAssetWriteBack::Get_IsWriteBackCandidate(Instance))
                { continue; }

                Out.Add(FLiteralAsset{Name, Instance});
            }
        }

        return Out;
    }

    auto Make_EmptyContext(
        const TMap<FString, FCk_ScriptAccessorEntry>& InAccessors) -> FCk_AssetValueDiffContext
    {
        auto Context = FCk_AssetValueDiffContext{};
        Context.Accessors             = &InAccessors;
        Context.AnyProviderRegistered = true;
        return Context;
    }
}

using namespace ck_test_asset_write_back_integration;

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetWriteBack_ScratchBaselineReproducesEveryLiveAsset,
    "CkAngelscriptGenerator.Integration.AssetWriteBack.ScratchBaselineReproducesEveryLiveAsset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetWriteBack_ScratchBaselineReproducesEveryLiveAsset::RunTest(const FString&)
{
    // The whole design rests on one claim: re-running `__Init_<Name>` onto a fresh instance yields
    // exactly what the .as file produces. This asserts it against EVERY real literal asset in the
    // project rather than a fixture — which is also the runtime evidence that re-running those
    // bodies (some of which call global `assets::` accessors) has no side effect that changes the
    // result.
    const auto Assets = Collect_NativeLiteralAssets();

    TestTrue(TEXT("the project declares at least one native-class literal asset to check"),
        Assets.Num() > 0);

    const auto Accessors = TMap<FString, FCk_ScriptAccessorEntry>{};
    const auto Context   = Make_EmptyContext(Accessors);

    auto Checked          = 0;
    auto UnresolvedAtLoad = TArray<FString>{};

    for (const auto& Asset : Assets)
    {
        const auto Baseline = FCkAsAssetValueDiff::Build_ScratchBaseline(Asset.Instance->GetClass(), Asset.Name);

        if (ck::Is_NOT_Valid(Baseline.Get()))
        {
            AddError(FString::Printf(
                TEXT("Could not build a scratch baseline for literal asset '%s' — `__Init_%s` did not run."),
                *Asset.Name, *Asset.Name));
            continue;
        }

        // No accessor index is supplied, so any entry here would have to be a difference the diff
        // found before resolution — i.e. a genuine live-vs-text divergence, not a lookup failure.
        const auto Diff = FCkAsAssetValueDiff::Compute(
            Asset.Instance, Baseline.Get(), Asset.Instance->GetClass()->GetDefaultObject(false), Context);

        // A divergence on a non-null object reference never reaches Entries: it is reported in
        // Unresolved, because this runs with an empty accessor index. Reading only Entries would
        // make this gate blind to exactly the case it exists to catch — a baseline that resolves a
        // DIFFERENT object than the live one.
        for (const auto& Unresolved : Diff.Unresolved)
        {
            AddError(FString::Printf(
                TEXT("Literal asset '%s': re-running `__Init_%s` did NOT reproduce the live object at '%s' ")
                TEXT("(%s). The write-back baseline cannot be trusted for this asset."),
                *Asset.Name, *Asset.Name, *Unresolved.PropertyPath, *Unresolved.Detail));
        }

        // The one divergence write-back does not own: an asset whose initializer reached the asset
        // registry before it was scanned holds null where the file text produces a real reference.
        // Compute() reports exactly those, and the confirmation dialog warns on them rather than
        // erasing the assignment. Anything ELSE diverging would mean `__Init_` is not reproducible,
        // which invalidates the baseline the whole design rests on — so that fails.
        for (const auto& Entry : Diff.Entries)
        {
            if (Diff.ClearedObjectReferences.Contains(Entry.PropertyName))
            {
                UnresolvedAtLoad.Add(FString::Printf(TEXT("%s.%s"), *Asset.Name, *Entry.PropertyName));
                continue;
            }

            const auto* Property = Asset.Instance->GetClass()->FindPropertyByName(*Entry.PropertyName);

            auto LiveText     = FString{};
            auto BaselineText = FString{};
            if (Property != nullptr)
            {
                Property->ExportText_InContainer(0, LiveText,     Asset.Instance, nullptr, nullptr, PPF_None);
                Property->ExportText_InContainer(0, BaselineText, Baseline.Get(), nullptr, nullptr, PPF_None);
            }

            AddError(FString::Printf(
                TEXT("Literal asset '%s': re-running `__Init_%s` did NOT reproduce the live object at '%s' ")
                TEXT("(live=[%s] text-produced=[%s]), and it is not a load-time-unresolved reference. ")
                TEXT("The write-back baseline cannot be trusted for this asset."),
                *Asset.Name, *Asset.Name, *Entry.PropertyName, *LiveText, *BaselineText));
        }

        ++Checked;
    }

    AddInfo(FString::Printf(TEXT("Re-ran `__Init_` against %d native-class literal asset(s)."), Checked));

    // A literal asset holding null where its own source assigns a reference means its initializer
    // asked the asset registry before it was scanned and nothing healed it afterwards. The deferred
    // heal now covers that path (UCk_Utils_IO_UE::DoLoadAssetsByName notes the deferral), so this is
    // a gate rather than a report: if it goes red, an initializer has found a new way to resolve a
    // reference that the heal does not know about, and write-back would offer to erase that line.
    TestEqual(FString::Printf(
        TEXT("no literal asset holds null where its source assigns a reference [%s]"),
        *FString::Join(UnresolvedAtLoad, TEXT(", "))),
        UnresolvedAtLoad.Num(), 0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetWriteBack_ContainerBodySurvivesAScalarEdit,
    "CkAngelscriptGenerator.Integration.AssetWriteBack.ContainerBodySurvivesAScalarEdit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetWriteBack_ContainerBodySurvivesAScalarEdit::RunTest(const FString&)
{
    // `Asset_RendererData_Demo` populates `_Submeshes` with `.Add()`. Diffing against the class CDO
    // would put that container in the patch set, fail to express it, and abort every write. Diffing
    // against the text-produced baseline must leave it out entirely, so a scalar edit writes exactly
    // one line and the `.Add()` block comes out byte-identical.
    static const auto AssetName = FString{TEXT("Asset_RendererData_Demo")};

    const auto Assets = Collect_NativeLiteralAssets();
    const auto* Specimen = Assets.FindByPredicate([](const FLiteralAsset& InAsset)
    {
        return InAsset.Name == AssetName;
    });

    if (Specimen == nullptr)
    {
        AddInfo(FString::Printf(
            TEXT("'%s' is not present in this project (it ships with CkTests); container preservation ")
            TEXT("is covered by the AssetBlockPatcher.PreservesUnrecognisedLines unit test instead."), *AssetName));
        return true;
    }

    auto* Live = Specimen->Instance;

    const auto Baseline = FCkAsAssetValueDiff::Build_ScratchBaseline(Live->GetClass(), AssetName);
    TestTrue(TEXT("scratch baseline built"), ck::IsValid(Baseline.Get()));
    if (ck::Is_NOT_Valid(Baseline.Get()))
    { return false; }

    const auto* IntScalar = CastField<FIntProperty>(
        Live->GetClass()->FindPropertyByName(TEXT("_NumCustomDataFloat")));

    if (IntScalar == nullptr)
    {
        AddInfo(TEXT("'_NumCustomDataFloat' is no longer an int property on this specimen; ")
                TEXT("skipping the scalar-edit half of the check."));
        return true;
    }

    // Move the BASELINE off the live value rather than editing the real asset — the diff is the
    // same shape a details-panel edit produces, and nothing outside this test observes a change.
    const auto LiveValue = IntScalar->GetPropertyValue_InContainer(Live);
    IntScalar->SetPropertyValue_InContainer(Baseline.Get(), LiveValue + 7);

    const auto Accessors = TMap<FString, FCk_ScriptAccessorEntry>{};
    const auto Context   = Make_EmptyContext(Accessors);

    const auto Diff = FCkAsAssetValueDiff::Compute(
        Live, Baseline.Get(), Live->GetClass()->GetDefaultObject(false), Context);

    TestTrue (TEXT("the diff resolves completely"), Diff.Success);
    TestEqual(TEXT("exactly one property is in the patch set"), Diff.Entries.Num(), 1);
    if (Diff.Entries.Num() != 1)
    {
        for (const auto& Entry : Diff.Entries)
        { AddError(FString::Printf(TEXT("unexpected patch-set property: %s"), *Entry.PropertyName)); }
        return false;
    }

    TestEqual(TEXT("and it is the scalar, NOT the `.Add()` container"),
        Diff.Entries[0].PropertyName, FString{TEXT("_NumCustomDataFloat")});
    TestEqual(TEXT("the emitted expression is the live value"),
        Diff.Entries[0].Assignments[0].Expression, FString::FromInt(LiveValue));

    // Now patch the real file text in memory and prove the container body is untouched.
    auto SourcePath = FString{};
    {
        auto* Package = Live->GetPackage();
        TestTrue(TEXT("the asset has a package"), ck::IsValid(Package, ck::IsValid_Policy_NullptrOnly{}));
        if (ck::Is_NOT_Valid(Package, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }

        SourcePath = Package->GetMetaData().GetValue(Live, TEXT("ScriptAssetFilename"));
    }

    if (SourcePath.IsEmpty() || NOT FPaths::FileExists(SourcePath))
    {
        AddInfo(TEXT("The asset's package metadata carries no readable .as path in this run; ")
                TEXT("skipping the text half of the check."));
        return true;
    }

    auto Snapshot = FCk_AsFileSnapshot{};
    TestTrue(TEXT("the declaring .as reads back"), FCkAsAssetBlockPatcher::Try_ReadSnapshot(SourcePath, Snapshot));
    if (NOT Snapshot.Loaded)
    { return false; }

    const auto Patch = FCkAsAssetBlockPatcher::Apply_Patch(Snapshot.Contents, AssetName, Diff.Entries);

    TestTrue (TEXT("the patch applies"), Patch.Success);
    TestEqual(TEXT("exactly one line changes"), Patch.Diff.Num(), 1);

    // The file is never written by this test — only the patched string is inspected. Each line is
    // asserted PRESENT in the source first: silently skipping a drifted fixture line would let all
    // three checks pass without proving anything.
    auto CheckedLines = 0;
    for (const auto& Line : {FString{TEXT("_Submeshes.Add(HatDesc);")},
                             FString{TEXT("HatDesc._Name = n\"Hat\";")},
                             FString{TEXT("HatDesc._Mesh = assets::load::SKM_Manny_Simple();")}})
    {
        if (NOT TestTrue(FString::Printf(TEXT("fixture still contains: %s"), *Line),
                Snapshot.Contents.Contains(Line)))
        { continue; }

        ++CheckedLines;
        TestTrue(FString::Printf(TEXT("container body line survives byte-identical: %s"), *Line),
            Patch.PatchedContents.Contains(Line));
    }

    TestEqual(TEXT("all three container-body lines were actually checked"), CheckedLines, 3);

    TestEqual(TEXT("the only textual difference is the scalar line"),
        Patch.PatchedContents.Len() - Snapshot.Contents.Len(),
        Patch.Diff[0].After.Len() - Patch.Diff[0].Before.Len());

    return true;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
