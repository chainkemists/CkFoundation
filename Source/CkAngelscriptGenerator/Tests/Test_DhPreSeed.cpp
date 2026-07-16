// Tests for the boot-time DynamicHandle JSON pre-seed: the declaration parser
// (comment/string-aware textual scan for `asset X of UCkDynamic_HandleDefinition`)
// and the seeder core (canonical diff + shared sibling-stub writer). The live
// StartupModule funnel (ownership gate, default roots) isn't covered here —
// coverage there comes from the `_probe_merge_conflict` smoke run.

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_DhPreSeed.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_Dispatcher.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkDynamic/CkDynamic_AngelScript.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace ck::angelscriptgenerator::self_heal;

namespace ck_test_dh_pre_seed
{
    auto Make_TempRoot(
        const FString& InSubdir) -> FString
    {
        const auto Root = FPaths::ProjectIntermediateDir() / TEXT("CkDhPreSeedTest") / InSubdir;
        IFileManager::Get().DeleteDirectory(*Root, /*RequireExists=*/false, /*Tree=*/true);
        IFileManager::Get().MakeDirectory(*Root, /*Tree=*/true);
        return Root;
    }

    auto Load_HandleTypes(
        const FString& InJsonPath) -> TArray<TSharedPtr<FJsonValue>>
    {
        auto Content = FString{};
        if (NOT FFileHelper::LoadFileToString(Content, *InJsonPath))
        { return {}; }

        auto RootObj    = TSharedPtr<FJsonObject>{};
        auto JsonReader = TJsonReaderFactory<>::Create(Content);
        if (NOT FJsonSerializer::Deserialize(JsonReader, RootObj) || NOT RootObj.IsValid()
            || NOT RootObj->HasField(TEXT("HandleTypes")))
        { return {}; }

        return RootObj->GetArrayField(TEXT("HandleTypes"));
    }

    auto Find_EntryByTypeName(
        const TArray<TSharedPtr<FJsonValue>>& InHandleTypes,
        const FString& InTypeName) -> TSharedPtr<FJsonObject>
    {
        for (const auto& Entry : InHandleTypes)
        {
            const auto Obj = Entry->AsObject();
            if (Obj.IsValid() && Obj->GetStringField(TEXT("TypeName")) == InTypeName)
            { return Obj; }
        }
        return nullptr;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Parser: basic real-world-shaped block — TypeName extracted, ShortName derived.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DhPreSeed_Parse_BasicBlock,
    "CkAngelscriptGenerator.UnitTests.DhPreSeed.Parse_BasicBlock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_DhPreSeed_Parse_BasicBlock::RunTest(const FString&)
{
    const auto Contents = FString{TEXT(
        "asset StoreDriverHandle of UCkDynamic_HandleDefinition\n"
        "{\n"
        "    TypeName = \"FCk_Handle_StoreDriver\";\n"
        "    RequiredFragments.Add(FBb_Feature_StoreDriver);\n"
        "    Description = \"Top-level director entity.\";\n"
        "}\n")};

    const auto Declarations = FCkAsDhPreSeed::Parse_HandleDefinitions(Contents, TEXT("D:/Test/Feature.as"));

    TestEqual(TEXT("one declaration found"), Declarations.Num(), 1);
    if (Declarations.Num() == 1)
    {
        TestEqual(TEXT("TypeName"),  Declarations[0].TypeName,  TEXT("FCk_Handle_StoreDriver"));
        TestEqual(TEXT("ShortName derived"), Declarations[0].ShortName, TEXT("StoreDriver"));
        TestEqual(TEXT("SourceFilePath"), Declarations[0].SourceFilePath, TEXT("D:/Test/Feature.as"));
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Parser: explicit ShortName in the block wins over derivation.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DhPreSeed_Parse_ExplicitShortName,
    "CkAngelscriptGenerator.UnitTests.DhPreSeed.Parse_ExplicitShortName",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_DhPreSeed_Parse_ExplicitShortName::RunTest(const FString&)
{
    const auto Contents = FString{TEXT(
        "asset CustomHandle of UCkDynamic_HandleDefinition\n"
        "{\n"
        "    TypeName = \"MyCustomHandle\";\n"
        "    ShortName = \"Custom\";\n"
        "}\n")};

    const auto Declarations = FCkAsDhPreSeed::Parse_HandleDefinitions(Contents, TEXT("D:/Test/Feature.as"));

    TestEqual(TEXT("one declaration found"), Declarations.Num(), 1);
    if (Declarations.Num() == 1)
    {
        TestEqual(TEXT("TypeName"),  Declarations[0].TypeName,  TEXT("MyCustomHandle"));
        TestEqual(TEXT("explicit ShortName honored"), Declarations[0].ShortName, TEXT("Custom"));
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Parser: multiple blocks in one file (the BB_StoreDriver_Feature.as shape).
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DhPreSeed_Parse_MultipleBlocks,
    "CkAngelscriptGenerator.UnitTests.DhPreSeed.Parse_MultipleBlocks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_DhPreSeed_Parse_MultipleBlocks::RunTest(const FString&)
{
    const auto Contents = FString{TEXT(
        "asset AHandle of UCkDynamic_HandleDefinition\n"
        "{\n"
        "    TypeName = \"FCk_Handle_A\";\n"
        "}\n"
        "\n"
        "class UBb_SomeUnrelated_EntityScript : UCk_EntityScript_UE\n"
        "{\n"
        "}\n"
        "\n"
        "asset BHandle of UCkDynamic_HandleDefinition\n"
        "{\n"
        "    TypeName = \"FCk_Handle_B\";\n"
        "}\n")};

    const auto Declarations = FCkAsDhPreSeed::Parse_HandleDefinitions(Contents, TEXT("D:/Test/Feature.as"));

    TestEqual(TEXT("two declarations found"), Declarations.Num(), 2);
    if (Declarations.Num() == 2)
    {
        TestEqual(TEXT("first TypeName"),  Declarations[0].TypeName, TEXT("FCk_Handle_A"));
        TestEqual(TEXT("second TypeName"), Declarations[1].TypeName, TEXT("FCk_Handle_B"));
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Parser: commented-out blocks (// and /* */) must NOT seed phantom entries.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DhPreSeed_Parse_CommentedOutBlocksIgnored,
    "CkAngelscriptGenerator.UnitTests.DhPreSeed.Parse_CommentedOutBlocksIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_DhPreSeed_Parse_CommentedOutBlocksIgnored::RunTest(const FString&)
{
    const auto Contents = FString{TEXT(
        "// asset OldHandle of UCkDynamic_HandleDefinition\n"
        "// {\n"
        "//     TypeName = \"FCk_Handle_Old\";\n"
        "// }\n"
        "\n"
        "/*\n"
        "asset OlderHandle of UCkDynamic_HandleDefinition\n"
        "{\n"
        "    TypeName = \"FCk_Handle_Older\";\n"
        "}\n"
        "*/\n"
        "\n"
        "asset LiveHandle of UCkDynamic_HandleDefinition\n"
        "{\n"
        "    TypeName = \"FCk_Handle_Live\";\n"
        "}\n")};

    const auto Declarations = FCkAsDhPreSeed::Parse_HandleDefinitions(Contents, TEXT("D:/Test/Feature.as"));

    TestEqual(TEXT("only the live declaration found"), Declarations.Num(), 1);
    if (Declarations.Num() == 1)
    { TestEqual(TEXT("TypeName"), Declarations[0].TypeName, TEXT("FCk_Handle_Live")); }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Parser: ShortName derivation parity with the registry-load fallback
// (FCkDynamic_HandleTypeRegistry::ExtractShortNameFromTypeName).
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DhPreSeed_Parse_ShortNameDerivationParity,
    "CkAngelscriptGenerator.UnitTests.DhPreSeed.Parse_ShortNameDerivationParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_DhPreSeed_Parse_ShortNameDerivationParity::RunTest(const FString&)
{
    const auto TypeNames = TArray<FString>{
        TEXT("FCk_Handle_Weapon"),
        TEXT("Handle_Weapon"),
        TEXT("Weapon"),
    };

    for (const auto& TypeName : TypeNames)
    {
        const auto Contents = FString::Printf(TEXT(
            "asset TheHandle of UCkDynamic_HandleDefinition\n"
            "{\n"
            "    TypeName = \"%s\";\n"
            "}\n"), *TypeName);

        const auto Declarations = FCkAsDhPreSeed::Parse_HandleDefinitions(Contents, TEXT("D:/Test/Feature.as"));

        TestEqual(FString::Printf(TEXT("declaration found for '%s'"), *TypeName), Declarations.Num(), 1);
        if (Declarations.Num() == 1)
        {
            TestEqual(FString::Printf(TEXT("ShortName parity for '%s'"), *TypeName),
                Declarations[0].ShortName,
                FCkDynamic_HandleTypeRegistry::ExtractShortNameFromTypeName(TypeName));
        }
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Parser: tricky string contents (braces, //, a fake TypeName assignment inside
// a Description literal) must not fool the structural scan.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DhPreSeed_Parse_TrickyStringsDontFoolScan,
    "CkAngelscriptGenerator.UnitTests.DhPreSeed.Parse_TrickyStringsDontFoolScan",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_DhPreSeed_Parse_TrickyStringsDontFoolScan::RunTest(const FString&)
{
    const auto Contents = FString{TEXT(
        "asset TrickyHandle of UCkDynamic_HandleDefinition\n"
        "{\n"
        "    Description = \"See http://example.com — braces { } and a decoy: TypeName = \\\"FCk_Handle_Evil\\\"\";\n"
        "    TypeName = \"FCk_Handle_Real\";\n"
        "}\n")};

    const auto Declarations = FCkAsDhPreSeed::Parse_HandleDefinitions(Contents, TEXT("D:/Test/Feature.as"));

    TestEqual(TEXT("one declaration found"), Declarations.Num(), 1);
    if (Declarations.Num() == 1)
    {
        TestEqual(TEXT("real TypeName wins, decoy in string ignored"),
            Declarations[0].TypeName, TEXT("FCk_Handle_Real"));
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Seeder: appends only the TypeNames missing from the canonical; entry shape
// matches the heal-path writer (single shared writer, full field set).
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DhPreSeed_Seeder_AppendsMissingOnly,
    "CkAngelscriptGenerator.UnitTests.DhPreSeed.Seeder_AppendsMissingOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_DhPreSeed_Seeder_AppendsMissingOnly::RunTest(const FString&)
{
    const auto TempRoot  = ck_test_dh_pre_seed::Make_TempRoot(TEXT("AppendsMissing"));
    const auto Canonical = TempRoot / TEXT("DynamicHandleTypes.json");

    FFileHelper::SaveStringToFile(
        TEXT("{\"HandleTypes\":[{\"TypeName\":\"FCk_Handle_A\",\"ShortName\":\"A\"}]}"),
        *Canonical, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const auto Declarations = TArray<FCk_DhDeclaration>{
        FCk_DhDeclaration{TEXT("FCk_Handle_A"), TEXT("A"), TEXT("D:/Test/A.as")},
        FCk_DhDeclaration{TEXT("FCk_Handle_B"), TEXT("B"), TEXT("D:/Test/B.as")},
    };

    const auto SeededCount = FCkAsDhPreSeed::PreSeed_MissingDynamicHandles(Canonical, Declarations);
    TestEqual(TEXT("exactly one entry seeded"), SeededCount, 1);

    const auto StubPath    = FCkAsRecoveryDispatcher::Derive_DynamicHandleStubPath(Canonical);
    const auto HandleTypes = ck_test_dh_pre_seed::Load_HandleTypes(StubPath);
    TestEqual(TEXT("sibling holds one entry"), HandleTypes.Num(), 1);

    const auto Entry = ck_test_dh_pre_seed::Find_EntryByTypeName(HandleTypes, TEXT("FCk_Handle_B"));
    TestTrue(TEXT("entry is FCk_Handle_B"), Entry.IsValid());
    if (Entry.IsValid())
    {
        TestEqual(TEXT("ShortName"), Entry->GetStringField(TEXT("ShortName")), TEXT("B"));
        TestTrue(TEXT("Description present"),        Entry->HasField(TEXT("Description")));
        TestTrue(TEXT("SourceAsset present"),        Entry->HasField(TEXT("SourceAsset")));
        TestTrue(TEXT("RequiredFragments present"),  Entry->HasField(TEXT("RequiredFragments")));
        TestEqual(TEXT("RequiredFragments empty (permissive validator contract)"),
            Entry->GetArrayField(TEXT("RequiredFragments")).Num(), 0);
    }

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Seeder: canonical already covers everything — returns 0, writes NO sibling.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DhPreSeed_Seeder_NoOpWhenCanonicalCovers,
    "CkAngelscriptGenerator.UnitTests.DhPreSeed.Seeder_NoOpWhenCanonicalCovers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_DhPreSeed_Seeder_NoOpWhenCanonicalCovers::RunTest(const FString&)
{
    const auto TempRoot  = ck_test_dh_pre_seed::Make_TempRoot(TEXT("NoOp"));
    const auto Canonical = TempRoot / TEXT("DynamicHandleTypes.json");

    FFileHelper::SaveStringToFile(
        TEXT("{\"HandleTypes\":[{\"TypeName\":\"FCk_Handle_A\",\"ShortName\":\"A\"}]}"),
        *Canonical, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const auto Declarations = TArray<FCk_DhDeclaration>{
        FCk_DhDeclaration{TEXT("FCk_Handle_A"), TEXT("A"), TEXT("D:/Test/A.as")},
    };

    const auto SeededCount = FCkAsDhPreSeed::PreSeed_MissingDynamicHandles(Canonical, Declarations);
    TestEqual(TEXT("nothing seeded"), SeededCount, 0);

    const auto StubPath = FCkAsRecoveryDispatcher::Derive_DynamicHandleStubPath(Canonical);
    TestFalse(TEXT("no sibling file written"), IFileManager::Get().FileExists(*StubPath));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Seeder: dedups against entries already in the sibling (heal-path parity —
// the pre-existing entry is written through the SAME shared writer).
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DhPreSeed_Seeder_DedupAgainstExistingSibling,
    "CkAngelscriptGenerator.UnitTests.DhPreSeed.Seeder_DedupAgainstExistingSibling",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_DhPreSeed_Seeder_DedupAgainstExistingSibling::RunTest(const FString&)
{
    const auto TempRoot  = ck_test_dh_pre_seed::Make_TempRoot(TEXT("SiblingDedup"));
    const auto Canonical = TempRoot / TEXT("DynamicHandleTypes.json");
    const auto StubPath  = FCkAsRecoveryDispatcher::Derive_DynamicHandleStubPath(Canonical);

    FFileHelper::SaveStringToFile(
        TEXT("{\"HandleTypes\":[]}"),
        *Canonical, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    // Simulate a heal-path synthesis that already landed FCk_Handle_B.
    const auto HealEntry = FCk_DynamicHandleStubEntry{TEXT("FCk_Handle_B"), TEXT("B")};
    const auto HealAppended = FCkAsRecoveryDispatcher::Append_DynamicHandleStubEntries(
        StubPath, MakeArrayView(&HealEntry, 1));
    TestTrue(TEXT("heal-path append succeeded"), HealAppended.IsSet() && *HealAppended == 1);

    const auto Declarations = TArray<FCk_DhDeclaration>{
        FCk_DhDeclaration{TEXT("FCk_Handle_B"), TEXT("B"), TEXT("D:/Test/B.as")},
        FCk_DhDeclaration{TEXT("FCk_Handle_C"), TEXT("C"), TEXT("D:/Test/C.as")},
    };

    const auto SeededCount = FCkAsDhPreSeed::PreSeed_MissingDynamicHandles(Canonical, Declarations);
    TestEqual(TEXT("only the new entry seeded"), SeededCount, 1);

    const auto HandleTypes = ck_test_dh_pre_seed::Load_HandleTypes(StubPath);
    TestEqual(TEXT("sibling holds exactly two entries (no duplicate B)"), HandleTypes.Num(), 2);
    TestTrue(TEXT("B present once"),
        ck_test_dh_pre_seed::Find_EntryByTypeName(HandleTypes, TEXT("FCk_Handle_B")).IsValid());
    TestTrue(TEXT("C appended"),
        ck_test_dh_pre_seed::Find_EntryByTypeName(HandleTypes, TEXT("FCk_Handle_C")).IsValid());

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Seeder: missing canonical (fresh clone before first regen) seeds everything.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DhPreSeed_Seeder_MissingCanonicalSeedsAll,
    "CkAngelscriptGenerator.UnitTests.DhPreSeed.Seeder_MissingCanonicalSeedsAll",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_DhPreSeed_Seeder_MissingCanonicalSeedsAll::RunTest(const FString&)
{
    const auto TempRoot  = ck_test_dh_pre_seed::Make_TempRoot(TEXT("MissingCanonical"));
    const auto Canonical = TempRoot / TEXT("DynamicHandleTypes.json"); // deliberately never written

    const auto Declarations = TArray<FCk_DhDeclaration>{
        FCk_DhDeclaration{TEXT("FCk_Handle_A"), TEXT("A"), TEXT("D:/Test/A.as")},
        FCk_DhDeclaration{TEXT("FCk_Handle_B"), TEXT("B"), TEXT("D:/Test/B.as")},
    };

    const auto SeededCount = FCkAsDhPreSeed::PreSeed_MissingDynamicHandles(Canonical, Declarations);
    TestEqual(TEXT("all declarations seeded"), SeededCount, 2);

    const auto StubPath    = FCkAsRecoveryDispatcher::Derive_DynamicHandleStubPath(Canonical);
    const auto HandleTypes = ck_test_dh_pre_seed::Load_HandleTypes(StubPath);
    TestEqual(TEXT("sibling holds both entries"), HandleTypes.Num(), 2);

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
