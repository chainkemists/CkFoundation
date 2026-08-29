#include "CkAngelscriptGenerator/Tests/Test_AssetValueDiff_Fixtures.h"
#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AssetValueDiff.h"

#include "CkCore/Macros/CkMacros.h"

#include "Misc/AutomationTest.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptManager.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

UCkTest_WriteBack_CustomisedCdo::UCkTest_WriteBack_CustomisedCdo()
{
    _Bearing._Depth       = 7;
    _Bearing._Leaf._Scale = 3.0f;
    _Pod._Count           = 5;
    _Ref                  = UCkTest_WriteBack_Host::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_DEV_AUTOMATION_TESTS

using namespace ck::angelscriptgenerator::write_back;

namespace ck_test_asset_value_diff
{
    const auto IconPath = FString{TEXT("/CkTests/UI/T_Icon.T_Icon")};

    auto Make_Accessors() -> TMap<FString, FCk_ScriptAccessorEntry>
    {
        auto File = FCk_GeneratedAccessorFile{};
        File.AbsolutePath      = TEXT("D:/Fake/Generated/TestAssets.as");
        File.FallbackNamespace = TEXT("assets");
        File.Contents = FString::Printf(TEXT(
            "namespace assets\n"
            "{\n"
            "    TSoftObjectPtr<UObject> T_Icon() { return TSoftObjectPtr<UObject>(FSoftObjectPath(\"%s\")); }\n"
            "}\n"), *IconPath);

        return FCkAsAccessorResolver::Build_Index(FCkAsAccessorResolver::Parse_GeneratedAccessorFile(File));
    }

    auto Make_Context(
        const TMap<FString, FCk_ScriptAccessorEntry>& InAccessors) -> FCk_AssetValueDiffContext
    {
        auto Context = FCk_AssetValueDiffContext{};
        Context.Accessors             = &InAccessors;
        Context.AnyProviderRegistered = true;
        return Context;
    }

    auto Find_Entry(
        const FCk_AssetValueDiffResult& InResult,
        const TCHAR*                    InPropertyName) -> const FCk_AssetBlockPatchEntry*
    {
        return InResult.Entries.FindByPredicate([&](const FCk_AssetBlockPatchEntry& InEntry)
        {
            return InEntry.PropertyName == InPropertyName;
        });
    }
}

using namespace ck_test_asset_value_diff;

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetValueDiff_ScalarLeaves,
    "CkAngelscriptGenerator.UnitTests.AssetValueDiff.ScalarLeaves",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetValueDiff_ScalarLeaves::RunTest(const FString&)
{
    const auto Accessors = Make_Accessors();
    const auto Context   = Make_Context(Accessors);

    auto* Baseline = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Live     = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Defaults = UCkTest_WriteBack_Host::StaticClass()->GetDefaultObject();

    Live->_Count       = 5;
    Live->_Label       = TEXT("hello");
    Live->_Tag         = TEXT("Hat");
    Live->_Scratch     = 99; // transient — never authored, so never written back
    Live->_NotEditable = 99; // no CPF_Edit — the details panel cannot change it
    Live->_ReadOnly    = 99; // VisibleAnywhere — shown but not editable

    const auto Result = FCkAsAssetValueDiff::Compute(Live, Baseline, Defaults, Context);

    TestTrue (TEXT("everything resolved"), Result.Success);
    TestEqual(TEXT("no unresolved"),       Result.Unresolved.Num(), 0);
    TestEqual(TEXT("three entries"),       Result.Entries.Num(), 3);

    const auto* Count = Find_Entry(Result, TEXT("_Count"));
    TestNotNull(TEXT("_Count entry"), Count);
    if (Count != nullptr)
    {
        TestFalse(TEXT("_Count is an assign"), Count->Delete);
        TestEqual(TEXT("_Count assignment count"), Count->Assignments.Num(), 1);
        TestEqual(TEXT("_Count sub-path is empty"), Count->Assignments[0].SubPath, FString{});
        TestEqual(TEXT("_Count expression"), Count->Assignments[0].Expression, FString{TEXT("5")});
    }

    const auto* Label = Find_Entry(Result, TEXT("_Label"));
    TestNotNull(TEXT("_Label entry"), Label);
    if (Label != nullptr)
    { TestEqual(TEXT("string is quoted"), Label->Assignments[0].Expression, FString{TEXT("\"hello\"")}); }

    const auto* Tag = Find_Entry(Result, TEXT("_Tag"));
    TestNotNull(TEXT("_Tag entry"), Tag);
    if (Tag != nullptr)
    { TestEqual(TEXT("name uses the n\"\" form"), Tag->Assignments[0].Expression, FString{TEXT("n\"Hat\"")}); }

    TestNull(TEXT("transient property is not in the patch set"), Find_Entry(Result, TEXT("_Scratch")));
    TestNull(TEXT("non-editable property is not in the patch set"), Find_Entry(Result, TEXT("_NotEditable")));
    TestNull(TEXT("read-only property is not in the patch set"), Find_Entry(Result, TEXT("_ReadOnly")));
    TestFalse(TEXT("no lossy FText involved"), Result.HasLossyText);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetValueDiff_UnchangedProducesNothing,
    "CkAngelscriptGenerator.UnitTests.AssetValueDiff.UnchangedProducesNothing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetValueDiff_UnchangedProducesNothing::RunTest(const FString&)
{
    const auto Accessors = Make_Accessors();
    const auto Context   = Make_Context(Accessors);

    // The baseline stands in for what the file text produces. A property that matches it is not in
    // the patch set even when it differs wildly from the class CDO — that is the whole point of the
    // predicate: a hand-authored accessor line must survive untouched.
    auto* Baseline = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Live     = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Defaults = UCkTest_WriteBack_Host::StaticClass()->GetDefaultObject();

    Baseline->_SoftRef = FSoftObjectPath{IconPath};
    Live->_SoftRef     = FSoftObjectPath{IconPath};
    Baseline->_Numbers = {1, 2, 3};
    Live->_Numbers     = {1, 2, 3};
    Baseline->_Count   = 12;
    Live->_Count       = 12;

    const auto Result = FCkAsAssetValueDiff::Compute(Live, Baseline, Defaults, Context);

    TestTrue (TEXT("succeeds"),      Result.Success);
    TestEqual(TEXT("no entries"),    Result.Entries.Num(), 0);
    TestEqual(TEXT("no unresolved"), Result.Unresolved.Num(), 0);

    // ...and the container matching the baseline is exactly why containers are deferrable rather
    // than blocking: it never enters the patch set, so it never has to resolve.
    TestTrue(TEXT("the same object differs from its class defaults"),
        FCkAsAssetValueDiff::Get_DiffersFromClassDefaults(Live));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetValueDiff_ObjectLeafKinds,
    "CkAngelscriptGenerator.UnitTests.AssetValueDiff.ObjectLeafKinds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetValueDiff_ObjectLeafKinds::RunTest(const FString&)
{
    const auto Accessors = Make_Accessors();
    const auto Context   = Make_Context(Accessors);

    auto* Baseline = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Live     = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Defaults = UCkTest_WriteBack_Host::StaticClass()->GetDefaultObject();

    Live->_SoftRef  = FSoftObjectPath{IconPath};
    Live->_ClassRef = UCkTest_WriteBack_Host::StaticClass();

    const auto Result = FCkAsAssetValueDiff::Compute(Live, Baseline, Defaults, Context);

    TestTrue(TEXT("everything resolved"), Result.Success);

    const auto* Soft = Find_Entry(Result, TEXT("_SoftRef"));
    TestNotNull(TEXT("_SoftRef entry"), Soft);
    if (Soft != nullptr)
    {
        // Must be the accessor call, NOT `nullptr` — Get_PropertyDefaultValueLiteral returns a
        // literal nullptr for every object property, so reaching it here would silently erase the
        // reference the user is trying to save.
        TestEqual(TEXT("soft ref emits the generated accessor"),
            Soft->Assignments[0].Expression, FString{TEXT("assets::T_Icon()")});
    }

    const auto* ClassRef = Find_Entry(Result, TEXT("_ClassRef"));
    TestNotNull(TEXT("_ClassRef entry"), ClassRef);
    if (ClassRef != nullptr)
    {
        TestEqual(TEXT("a non-Blueprint class is its bare prefixed name"),
            ClassRef->Assignments[0].Expression, FString{TEXT("UCkTest_WriteBack_Host")});
    }

    // A deliberate override back to null is expressible and is NOT the same as unresolvable.
    auto* NullingLive     = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* NullingBaseline = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    NullingBaseline->_SoftRef = FSoftObjectPath{IconPath};
    // ...but only when it still differs from the class CDO, else the line is deleted instead.
    NullingBaseline->_Count = 3;
    NullingLive->_Count     = 4;

    const auto Nulled = FCkAsAssetValueDiff::Compute(NullingLive, NullingBaseline, Defaults, Context);
    TestTrue(TEXT("nulling resolves"), Nulled.Success);

    const auto* NulledSoft = Find_Entry(Nulled, TEXT("_SoftRef"));
    TestNotNull(TEXT("_SoftRef entry present"), NulledSoft);
    if (NulledSoft != nullptr)
    { TestTrue(TEXT("cleared soft ref becomes a delete"), NulledSoft->Delete); }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetValueDiff_NestedStructScalarLeaf,
    "CkAngelscriptGenerator.UnitTests.AssetValueDiff.NestedStructScalarLeaf",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetValueDiff_NestedStructScalarLeaf::RunTest(const FString&)
{
    const auto Accessors = Make_Accessors();
    const auto Context   = Make_Context(Accessors);

    auto* Baseline = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Live     = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Defaults = UCkTest_WriteBack_Host::StaticClass()->GetDefaultObject();

    // `_Bearing` carries an object field, so the walk descends into it one assignment per changed
    // leaf. `_Leaf` does NOT carry one, so the walk stops there and emits the whole sub-struct as a
    // constructor literal rather than descending further — the same rule Get_StructFieldOverrides
    // documents, and the reason a pod sub-struct never needs per-field statements.
    Live->_Bearing._Leaf._Scale = 4.0f;
    // `_Pod` carries none either, so it emits a single constructor literal at the top level.
    Live->_Pod._Count = 3;

    const auto Result = FCkAsAssetValueDiff::Compute(Live, Baseline, Defaults, Context);

    TestTrue(TEXT("everything resolved"), Result.Success);

    const auto* Bearing = Find_Entry(Result, TEXT("_Bearing"));
    TestNotNull(TEXT("_Bearing entry"), Bearing);
    if (Bearing != nullptr)
    {
        TestEqual(TEXT("only the changed leaf is emitted"), Bearing->Assignments.Num(), 1);
        TestEqual(TEXT("the walk stops at the pod sub-struct"),
            Bearing->Assignments[0].SubPath, FString{TEXT("._Leaf")});
        TestEqual(TEXT("and emits its whole constructor"),
            Bearing->Assignments[0].Expression, FString{TEXT("FCkTest_WriteBack_PodLeaf(4.0f, n\"None\")")});
    }

    const auto* Pod = Find_Entry(Result, TEXT("_Pod"));
    TestNotNull(TEXT("_Pod entry"), Pod);
    if (Pod != nullptr)
    {
        TestEqual(TEXT("pod struct is a single whole-value assignment"), Pod->Assignments.Num(), 1);
        TestEqual(TEXT("pod sub-path is empty"), Pod->Assignments[0].SubPath, FString{});
        TestTrue (TEXT("pod emits a constructor literal"),
            Pod->Assignments[0].Expression.StartsWith(TEXT("FCkTest_WriteBack_PodOnly(")));
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetValueDiff_NestedStructObjectLeaf,
    "CkAngelscriptGenerator.UnitTests.AssetValueDiff.NestedStructObjectLeaf",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetValueDiff_NestedStructObjectLeaf::RunTest(const FString&)
{
    const auto Accessors = Make_Accessors();
    const auto Context   = Make_Context(Accessors);

    auto* Baseline = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Live     = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Defaults = UCkTest_WriteBack_Host::StaticClass()->GetDefaultObject();

    Live->_Bearing._Icon  = FSoftObjectPath{IconPath};
    Live->_Bearing._Depth = 2;

    const auto Result = FCkAsAssetValueDiff::Compute(Live, Baseline, Defaults, Context);

    TestTrue(TEXT("everything resolved"), Result.Success);

    const auto* Bearing = Find_Entry(Result, TEXT("_Bearing"));
    TestNotNull(TEXT("_Bearing entry"), Bearing);
    if (Bearing == nullptr)
    { return false; }

    TestEqual(TEXT("two changed leaves"), Bearing->Assignments.Num(), 2);

    const auto* IconLeaf = Bearing->Assignments.FindByPredicate([](const FCk_AssetBlockAssignment& InAssignment)
    {
        return InAssignment.SubPath == TEXT("._Icon");
    });
    TestNotNull(TEXT("icon leaf"), IconLeaf);
    if (IconLeaf != nullptr)
    {
        // Same hazard one level down: Get_StructLiteral recurses through the same helper, so a
        // struct-literal emission would have written `nullptr` here.
        TestEqual(TEXT("nested object leaf emits the accessor"),
            IconLeaf->Expression, FString{TEXT("assets::T_Icon()")});
    }

    // An unreachable object inside a struct aborts the whole write rather than silently nulling.
    auto* Unreachable = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    Unreachable->_Bearing._Icon = FSoftObjectPath{TEXT("/Game/Nope/T_Missing.T_Missing")};

    const auto Failed = FCkAsAssetValueDiff::Compute(Unreachable, Baseline, Defaults, Context);
    TestFalse(TEXT("write is refused"), Failed.Success);
    TestEqual(TEXT("one unresolved"),   Failed.Unresolved.Num(), 1);
    if (Failed.Unresolved.Num() == 1)
    {
        TestEqual(TEXT("path names the leaf"), Failed.Unresolved[0].PropertyPath, FString{TEXT("_Bearing._Icon")});
        TestTrue (TEXT("reason is NoAccessorFound"),
            Failed.Unresolved[0].FailReason == ECk_AccessorResolve_FailReason::NoAccessorFound);
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetValueDiff_CustomisedCdoProducesNoPhantomEdit,
    "CkAngelscriptGenerator.UnitTests.AssetValueDiff.CustomisedCdoProducesNoPhantomEdit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetValueDiff_CustomisedCdoProducesNoPhantomEdit::RunTest(const FString&)
{
    const auto Accessors = Make_Accessors();
    const auto Context   = Make_Context(Accessors);

    auto* Baseline = NewObject<UCkTest_WriteBack_CustomisedCdo>(GetTransientPackage());
    auto* Live     = NewObject<UCkTest_WriteBack_CustomisedCdo>(GetTransientPackage());
    auto* Defaults = UCkTest_WriteBack_CustomisedCdo::StaticClass()->GetDefaultObject();

    TestEqual(TEXT("fixture precondition: the class constructor customised a struct field"),
        Live->_Bearing._Depth, 7);
    TestEqual(TEXT("fixture precondition: and a nested one"), Live->_Bearing._Leaf._Scale, 3.0f);

    const auto Untouched = FCkAsAssetValueDiff::Compute(Live, Baseline, Defaults, Context);

    TestTrue (TEXT("succeeds"), Untouched.Success);
    TestEqual(TEXT("a CDO-customised struct field is NOT a phantom edit"), Untouched.Entries.Num(), 0);

    // One real edit must still surface — and only that one.
    Live->_Bearing._Icon = FSoftObjectPath{IconPath};

    const auto Edited = FCkAsAssetValueDiff::Compute(Live, Baseline, Defaults, Context);
    TestTrue (TEXT("succeeds"), Edited.Success);
    TestEqual(TEXT("one entry"), Edited.Entries.Num(), 1);

    const auto* Bearing = Find_Entry(Edited, TEXT("_Bearing"));
    TestNotNull(TEXT("_Bearing entry"), Bearing);
    if (Bearing != nullptr)
    {
        TestEqual(TEXT("only the edited leaf, not the CDO-customised siblings"), Bearing->Assignments.Num(), 1);
        TestEqual(TEXT("and it is the icon"), Bearing->Assignments[0].SubPath, FString{TEXT("._Icon")});
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetValueDiff_ClearedRefEmitsExplicitNullptr,
    "CkAngelscriptGenerator.UnitTests.AssetValueDiff.ClearedRefEmitsExplicitNullptr",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetValueDiff_ClearedRefEmitsExplicitNullptr::RunTest(const FString&)
{
    const auto Accessors = Make_Accessors();
    const auto Context   = Make_Context(Accessors);

    auto* Baseline = NewObject<UCkTest_WriteBack_CustomisedCdo>(GetTransientPackage());
    auto* Live     = NewObject<UCkTest_WriteBack_CustomisedCdo>(GetTransientPackage());
    auto* Defaults = UCkTest_WriteBack_CustomisedCdo::StaticClass()->GetDefaultObject();

    TestNotNull(TEXT("fixture precondition: the CDO's reference is non-null"), Live->_Ref.Get());

    Live->_Ref = nullptr;

    const auto Result = FCkAsAssetValueDiff::Compute(Live, Baseline, Defaults, Context);

    TestTrue (TEXT("resolves"),  Result.Success);
    TestEqual(TEXT("one entry"), Result.Entries.Num(), 1);

    const auto* Ref = Find_Entry(Result, TEXT("_Ref"));
    TestNotNull(TEXT("_Ref entry"), Ref);
    if (Ref != nullptr)
    {
        // A cleared reference that still differs from the CDO is a deliberate override, so it is
        // written as `nullptr` — never confused with "could not resolve", and never deleted.
        TestFalse(TEXT("it is an assignment, not a delete"), Ref->Delete);
        TestEqual(TEXT("explicit nullptr"), Ref->Assignments[0].Expression, FString{TEXT("nullptr")});
    }

    // ...and it is reported as a cleared reference, because write-back cannot tell a deliberate
    // clear from an asset whose initializer failed to resolve the reference at load time.
    TestTrue(TEXT("reported as a cleared reference for the confirmation dialog"),
        Result.ClearedObjectReferences.Contains(TEXT("_Ref")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetValueDiff_LiteralAssetReferences,
    "CkAngelscriptGenerator.UnitTests.AssetValueDiff.LiteralAssetReferences",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetValueDiff_LiteralAssetReferences::RunTest(const FString&)
{
#if WITH_ANGELSCRIPT_CK
    auto* AssetsPackage = FAngelscriptManager::Get().AssetsPackage;
    if (ck::Is_NOT_Valid(AssetsPackage, ck::IsValid_Policy_NullptrOnly{}))
    {
        AddInfo(TEXT("No AngelScript assets package in this run; skipping literal-asset reference coverage."));
        return true;
    }

    const auto Accessors = Make_Accessors();

    // A literal asset lives in the AngelScript assets package. One declared in the SAME .as file is
    // referenceable by bare name; one from another file is not, and must abort loudly rather than
    // guess an expression.
    auto* Referenced = NewObject<UCkTest_WriteBack_Host>(AssetsPackage, TEXT("CkTest_WriteBack_LiteralTarget"),
        RF_Transient);

    auto* Baseline = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Live     = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Defaults = UCkTest_WriteBack_Host::StaticClass()->GetDefaultObject();

    Live->_HardRef = Referenced;

    {
        auto SameFile = Make_Context(Accessors);
        SameFile.SameFileLiteralAssetNames.Add(TEXT("CkTest_WriteBack_LiteralTarget"));

        const auto Result = FCkAsAssetValueDiff::Compute(Live, Baseline, Defaults, SameFile);

        TestTrue(TEXT("a same-file literal asset resolves"), Result.Success);

        const auto* Entry = Find_Entry(Result, TEXT("_HardRef"));
        TestNotNull(TEXT("_HardRef entry"), Entry);
        if (Entry != nullptr)
        {
            TestEqual(TEXT("emitted as the bare asset name"),
                Entry->Assignments[0].Expression, FString{TEXT("CkTest_WriteBack_LiteralTarget")});
        }
    }

    {
        // Same object, but not declared in this file — bare names are not visible across .as files.
        const auto CrossFile = Make_Context(Accessors);

        const auto Result = FCkAsAssetValueDiff::Compute(Live, Baseline, Defaults, CrossFile);

        TestFalse(TEXT("a cross-file literal asset aborts the write"), Result.Success);
        TestEqual(TEXT("one unresolved"), Result.Unresolved.Num(), 1);
        if (Result.Unresolved.Num() == 1)
        {
            TestTrue(TEXT("reported as CrossFileLiteralAsset, distinctly from a missing accessor"),
                Result.Unresolved[0].FailReason == ECk_AccessorResolve_FailReason::CrossFileLiteralAsset);
            TestTrue(TEXT("and the message hints at the namespace-wrapper idiom"),
                Result.Unresolved[0].Message.Contains(TEXT("namespace wrapper")));
        }
    }
#endif

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetValueDiff_SameNamedDistinctObjectsAreNotEqual,
    "CkAngelscriptGenerator.UnitTests.AssetValueDiff.SameNamedDistinctObjectsAreNotEqual",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetValueDiff_SameNamedDistinctObjectsAreNotEqual::RunTest(const FString&)
{
    const auto Accessors = Make_Accessors();
    const auto Context   = Make_Context(Accessors);

    auto* Baseline = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Live     = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Defaults = UCkTest_WriteBack_Host::StaticClass()->GetDefaultObject();

    // Two DISTINCT objects of the same class and the same FName, in different outers — the shape of
    // `/Engine/EngineMeshes/Cube` vs `/Engine/BasicShapes/Cube`. FObjectPropertyBase::StaticIdentical
    // deep-compares exactly this pair under PPF_DeepComparison, so a deep compare here would report
    // "unchanged", drop the edit from the patch set, and let the reload reset it from the file.
    auto* OuterA = NewObject<UPackage>(nullptr, TEXT("/Temp/CkWriteBackTest_A"), RF_Transient);
    auto* OuterB = NewObject<UPackage>(nullptr, TEXT("/Temp/CkWriteBackTest_B"), RF_Transient);

    const auto SharedName = FName{TEXT("Cube")};
    auto* RefA = NewObject<UCkTest_WriteBack_Host>(OuterA, SharedName, RF_Transient);
    auto* RefB = NewObject<UCkTest_WriteBack_Host>(OuterB, SharedName, RF_Transient);

    TestNotEqual(TEXT("fixture precondition: distinct objects"), (void*)RefA, (void*)RefB);
    TestEqual(TEXT("fixture precondition: same class"), RefA->GetClass(), RefB->GetClass());
    TestEqual(TEXT("fixture precondition: same name"), RefA->GetFName(), RefB->GetFName());

    Baseline->_HardRef = RefA;
    Live->_HardRef     = RefB;

    const auto Result = FCkAsAssetValueDiff::Compute(Live, Baseline, Defaults, Context);

    const auto Mentions_HardRef = Result.Entries.ContainsByPredicate(
        [](const FCk_AssetBlockPatchEntry& InEntry) { return InEntry.PropertyName == TEXT("_HardRef"); })
        || Result.Unresolved.ContainsByPredicate(
        [](const FCk_WriteBackUnresolved& InUnresolved) { return InUnresolved.PropertyPath == TEXT("_HardRef"); });

    TestTrue(TEXT("re-pointing at a same-named object in another outer is seen as a change"),
        Mentions_HardRef);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetValueDiff_UnloadedSoftRefIsNotAClear,
    "CkAngelscriptGenerator.UnitTests.AssetValueDiff.UnloadedSoftRefIsNotAClear",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetValueDiff_UnloadedSoftRefIsNotAClear::RunTest(const FString&)
{
    const auto Accessors = Make_Accessors();
    const auto Context   = Make_Context(Accessors);

    auto* Baseline = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Live     = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Defaults = UCkTest_WriteBack_Host::StaticClass()->GetDefaultObject();

    // The baseline's soft ref is loaded; the live one is set but not loaded. Resolving either
    // pointer (rather than reading the path) would make the live side look null and report a clear
    // that never happened.
    Baseline->_SoftRef = UCkTest_WriteBack_Host::StaticClass();
    Live->_SoftRef     = FSoftObjectPath{TEXT("/Game/NotLoaded/T_Absent.T_Absent")};

    TestNull(TEXT("fixture precondition: the live soft ref does not resolve"), Live->_SoftRef.Get());
    TestFalse(TEXT("but its path is set"), Live->_SoftRef.IsNull());

    const auto Result = FCkAsAssetValueDiff::Compute(Live, Baseline, Defaults, Context);

    TestFalse(TEXT("a set-but-unloaded reference is NOT a clear"),
        Result.ClearedObjectReferences.Contains(TEXT("_SoftRef")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetValueDiff_DeferredKindsAbortLoudly,
    "CkAngelscriptGenerator.UnitTests.AssetValueDiff.DeferredKindsAbortLoudly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetValueDiff_DeferredKindsAbortLoudly::RunTest(const FString&)
{
    const auto Accessors = Make_Accessors();
    const auto Context   = Make_Context(Accessors);

    auto* Baseline = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Live     = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Defaults = UCkTest_WriteBack_Host::StaticClass()->GetDefaultObject();

    // A container the USER edited does enter the patch set, cannot resolve, and must abort the whole
    // write — a partial write would let the reload stomp this edit from the CDO.
    Live->_Numbers = {7, 8};
    Live->_WeakRef = Live;

    const auto Result = FCkAsAssetValueDiff::Compute(Live, Baseline, Defaults, Context);

    TestFalse(TEXT("write is refused"), Result.Success);
    TestEqual(TEXT("both offenders reported"), Result.Unresolved.Num(), 2);

    const auto HasPath = [&](const TCHAR* InPath)
    {
        return Result.Unresolved.ContainsByPredicate([&](const FCk_WriteBackUnresolved& InUnresolved)
        {
            return InUnresolved.PropertyPath == InPath;
        });
    };

    TestTrue(TEXT("container reported"), HasPath(TEXT("_Numbers")));
    TestTrue(TEXT("weak ref reported"),  HasPath(TEXT("_WeakRef")));

    for (const auto& Unresolved : Result.Unresolved)
    {
        TestTrue(TEXT("every offender carries its own message"), NOT Unresolved.Message.IsEmpty());
        TestTrue(TEXT("and the message names the property"), Unresolved.Message.Contains(Unresolved.PropertyPath));
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetValueDiff_LossyTextIsFlagged,
    "CkAngelscriptGenerator.UnitTests.AssetValueDiff.LossyTextIsFlagged",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetValueDiff_LossyTextIsFlagged::RunTest(const FString&)
{
    const auto Accessors = Make_Accessors();
    const auto Context   = Make_Context(Accessors);

    auto* Baseline = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Live     = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    auto* Defaults = UCkTest_WriteBack_Host::StaticClass()->GetDefaultObject();

    Live->_Caption = FText::FromString(TEXT("Hello"));

    const auto Result = FCkAsAssetValueDiff::Compute(Live, Baseline, Defaults, Context);

    TestTrue(TEXT("resolves"), Result.Success);
    TestTrue(TEXT("flagged as lossy so the dialog can say so"), Result.HasLossyText);

    const auto* Caption = Find_Entry(Result, TEXT("_Caption"));
    TestNotNull(TEXT("_Caption entry"), Caption);
    if (Caption != nullptr)
    {
        TestEqual(TEXT("emitted as a culture-invariant literal"),
            Caption->Assignments[0].Expression, FString{TEXT("FText::FromString(\"Hello\")")});
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetValueDiff_DiffersFromClassDefaults,
    "CkAngelscriptGenerator.UnitTests.AssetValueDiff.DiffersFromClassDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetValueDiff_DiffersFromClassDefaults::RunTest(const FString&)
{
    auto* Pristine = NewObject<UCkTest_WriteBack_Host>(GetTransientPackage());
    TestFalse(TEXT("a fresh instance matches its class defaults"),
        FCkAsAssetValueDiff::Get_DiffersFromClassDefaults(Pristine));

    Pristine->_Scratch = 12;
    TestFalse(TEXT("a transient-only change does not count"),
        FCkAsAssetValueDiff::Get_DiffersFromClassDefaults(Pristine));

    Pristine->_NotEditable = 7;
    Pristine->_ReadOnly    = 7;
    TestFalse(TEXT("a change to a property the details panel cannot edit does not count"),
        FCkAsAssetValueDiff::Get_DiffersFromClassDefaults(Pristine));

    Pristine->_Count = 1;
    TestTrue(TEXT("an authored change counts"),
        FCkAsAssetValueDiff::Get_DiffersFromClassDefaults(Pristine));

    TestFalse(TEXT("null is safe"), FCkAsAssetValueDiff::Get_DiffersFromClassDefaults(nullptr));

    return true;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
