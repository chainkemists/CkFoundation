#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AssetBlockPatcher.h"

#include "CkCore/Macros/CkMacros.h"

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace ck::angelscriptgenerator::write_back;

namespace ck_test_asset_block_patcher
{
    auto Assign(
        const FString& InProperty,
        const FString& InExpression) -> FCk_AssetBlockPatchEntry
    {
        return FCk_AssetBlockPatchEntry::Make_Assign(InProperty, {FCk_AssetBlockAssignment{FString{}, InExpression}});
    }

    auto AssignField(
        const FString& InProperty,
        const FString& InSubPath,
        const FString& InExpression) -> FCk_AssetBlockPatchEntry
    {
        return FCk_AssetBlockPatchEntry::Make_Assign(InProperty, {FCk_AssetBlockAssignment{InSubPath, InExpression}});
    }

    // FString::Find CLAMPS StartPosition to Len()-1 (Core/Private/Containers/String.cpp.inl:444), so a
    // needle that matches at the very end of the string is returned again on every call. The cursor
    // bound — not the INDEX_NONE result — is what terminates this loop.
    auto Count_Occurrences(
        const FString& InHaystack,
        const FString& InNeedle) -> int32
    {
        if (InNeedle.IsEmpty())
        { return 0; }

        const auto LastViableStart = InHaystack.Len() - InNeedle.Len();

        auto Hits   = 0;
        auto Cursor = 0;

        while (Cursor <= LastViableStart)
        {
            const auto Found = InHaystack.Find(InNeedle, ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor);
            if (Found == INDEX_NONE)
            { break; }

            ++Hits;
            Cursor = Found + InNeedle.Len();
        }

        return Hits;
    }

    auto Write_Fixture(
        const FString&                     InPath,
        const FString&                     InContents,
        FFileHelper::EEncodingOptions      InEncoding) -> void
    {
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(InPath), /*Tree=*/true);
        FFileHelper::SaveStringToFile(InContents, *InPath, InEncoding);
    }

    auto Get_TempRoot(
        const TCHAR* InLeaf) -> FString
    {
        return FPaths::ConvertRelativePathToFull(FPaths::ProjectIntermediateDir() / InLeaf);
    }
}

using namespace ck_test_asset_block_patcher;

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_Find_SimpleBlock,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.Find_SimpleBlock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_Find_SimpleBlock::RunTest(const FString&)
{
    const auto Source = FString{TEXT(
        "// Language=angelscript\n"
        "\n"
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _Count = 1;\n"
        "}\n")};

    const auto Location = FCkAsAssetBlockPatcher::Find_AssetBlock(Source, TEXT("Asset_Foo"));

    TestTrue (TEXT("Found"),                    Location.Found);
    TestEqual(TEXT("TypeName"),                 Location.TypeName, FString{TEXT("UCk_Thing_Data")});
    TestEqual(TEXT("DeclLine"),                 Location.DeclLine, 3);
    TestEqual(TEXT("body opens on `{`"),        Source.Mid(Location.BodyOpen, 1), FString{TEXT("{")});
    TestEqual(TEXT("body closes on `}`"),       Source.Mid(Location.BodyClose, 1), FString{TEXT("}")});
    TestEqual(TEXT("DeclIndent is empty"),      Location.DeclIndent, FString{});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_Find_SkipsCommentedDeclaration,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.Find_SkipsCommentedDeclaration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_Find_SkipsCommentedDeclaration::RunTest(const FString&)
{
    const auto Source = FString{TEXT(
        "// asset Asset_Foo of UDecoy_Data\n"
        "// {\n"
        "//     _Count = 99;\n"
        "// }\n"
        "/* asset Asset_Foo of UBlockDecoy_Data */\n"
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _Count = 1;\n"
        "}\n")};

    const auto Location = FCkAsAssetBlockPatcher::Find_AssetBlock(Source, TEXT("Asset_Foo"));

    TestTrue (TEXT("Found"),         Location.Found);
    TestEqual(TEXT("real type won"), Location.TypeName, FString{TEXT("UCk_Thing_Data")});
    TestEqual(TEXT("DeclLine is the real one"), Location.DeclLine, 6);

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"), {Assign(TEXT("_Count"), TEXT("7"))});

    TestTrue (TEXT("patch succeeded"),          Patched.Success);
    TestTrue (TEXT("commented decoy untouched"), Patched.PatchedContents.Contains(TEXT("//     _Count = 99;")));
    TestTrue (TEXT("real value replaced"),      Patched.PatchedContents.Contains(TEXT("    _Count = 7;")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_Find_BracesInStringsAndComments,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.Find_BracesInStringsAndComments",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_Find_BracesInStringsAndComments::RunTest(const FString&)
{
    const auto Source = FString{TEXT(
        "asset Asset_Braces of UCk_Thing_Data\n"
        "{\n"
        "    _Label = \"a } brace { inside a string\";\n"
        "    // a } dangling brace in a line comment\n"
        "    /* and a { in a block comment } */\n"
        "    _Count = 1;\n"
        "}\n"
        "\n"
        "void AfterTheBlock() {}\n")};

    const auto Location = FCkAsAssetBlockPatcher::Find_AssetBlock(Source, TEXT("Asset_Braces"));

    TestTrue(TEXT("Found"), Location.Found);
    TestEqual(TEXT("closing brace is the block's own, not one in a string/comment"),
        FCkAsAssetBlockPatcher::Blank_CommentsAndStrings(Source).Mid(Location.BodyClose, 1), FString{TEXT("}")});
    TestEqual(TEXT("BodyClose is on line 7, past every decoy brace"),
        Count_Occurrences(Source.Left(Location.BodyClose), TEXT("\n")) + 1, 7);

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Braces"), {Assign(TEXT("_Count"), TEXT("42"))});

    TestTrue(TEXT("patch succeeded"),        Patched.Success);
    TestTrue(TEXT("string literal untouched"), Patched.PatchedContents.Contains(TEXT("\"a } brace { inside a string\"")));
    TestTrue(TEXT("value replaced"),         Patched.PatchedContents.Contains(TEXT("_Count = 42;")));
    TestTrue(TEXT("trailing function untouched"), Patched.PatchedContents.EndsWith(TEXT("void AfterTheBlock() {}\n")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_Find_MissingDeclaration,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.Find_MissingDeclaration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_Find_MissingDeclaration::RunTest(const FString&)
{
    const auto Source = FString{TEXT("asset Asset_Other of UCk_Thing_Data\n{\n}\n")};

    const auto Missing = FCkAsAssetBlockPatcher::Find_AssetBlock(Source, TEXT("Asset_Foo"));
    TestFalse(TEXT("not found"), Missing.Found);
    TestTrue (TEXT("reason is DeclarationNotFound"),
        Missing.FailReason == ECk_AssetBlockPatch_FailReason::DeclarationNotFound);

    const auto Unterminated = FCkAsAssetBlockPatcher::Find_AssetBlock(
        FString{TEXT("asset Asset_Foo of UCk_Thing_Data\n{\n    _Count = 1;\n")}, TEXT("Asset_Foo"));
    TestFalse(TEXT("unterminated not found"), Unterminated.Found);
    TestTrue (TEXT("reason is BodyUnterminated"),
        Unterminated.FailReason == ECk_AssetBlockPatch_FailReason::BodyUnterminated);

    const auto NoBody = FCkAsAssetBlockPatcher::Find_AssetBlock(
        FString{TEXT("asset Asset_Foo of UCk_Thing_Data\nint Stray = 1;\n")}, TEXT("Asset_Foo"));
    TestFalse(TEXT("no body not found"), NoBody.Found);
    TestTrue (TEXT("reason is BodyOpenNotFound"),
        NoBody.FailReason == ECk_AssetBlockPatch_FailReason::BodyOpenNotFound);

    const auto Failed = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"), {Assign(TEXT("_Count"), TEXT("1"))});
    TestFalse(TEXT("patch fails"),            Failed.Success);
    TestTrue (TEXT("patched contents empty"), Failed.PatchedContents.IsEmpty());

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_Replace_PreservesTrailingComment,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.Replace_PreservesTrailingComment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_Replace_PreservesTrailingComment::RunTest(const FString&)
{
    const auto Source = FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _Count = 1; // slot 0 must be valid\n"
        "    _Scale  =  2.0f;\n"
        "}\n")};

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"),
        {Assign(TEXT("_Count"), TEXT("9")), Assign(TEXT("_Scale"), TEXT("3.5f"))});

    TestTrue (TEXT("succeeded"), Patched.Success);
    TestTrue (TEXT("trailing comment survives"),
        Patched.PatchedContents.Contains(TEXT("    _Count = 9; // slot 0 must be valid\n")));
    TestTrue (TEXT("author's spacing around `=` survives"),
        Patched.PatchedContents.Contains(TEXT("    _Scale  =  3.5f;\n")));
    TestEqual(TEXT("two diff rows"), Patched.Diff.Num(), 2);
    TestTrue (TEXT("both are replacements"),
        Patched.Diff[0].Op == ECk_AssetBlockPatch_Op::ReplaceValue
        && Patched.Diff[1].Op == ECk_AssetBlockPatch_Op::ReplaceValue);
    TestEqual(TEXT("first diff line number"), Patched.Diff[0].LineNumber, 3);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_Replace_PropertyNamePrefixCollision,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.Replace_PropertyNamePrefixCollision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_Replace_PropertyNamePrefixCollision::RunTest(const FString&)
{
    const auto Source = FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _MeshScale = 1.0f;\n"
        "    _Mesh = nullptr;\n"
        "    _MeshScaleOverride = 2.0f;\n"
        "}\n")};

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"),
        {Assign(TEXT("_Mesh"), TEXT("assets::load::SKM_Manny()"))});

    TestTrue(TEXT("succeeded"), Patched.Success);
    TestTrue(TEXT("_Mesh replaced"),
        Patched.PatchedContents.Contains(TEXT("    _Mesh = assets::load::SKM_Manny();\n")));
    TestTrue(TEXT("_MeshScale untouched"),
        Patched.PatchedContents.Contains(TEXT("    _MeshScale = 1.0f;\n")));
    TestTrue(TEXT("_MeshScaleOverride untouched"),
        Patched.PatchedContents.Contains(TEXT("    _MeshScaleOverride = 2.0f;\n")));
    TestEqual(TEXT("exactly one diff row"), Patched.Diff.Num(), 1);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_Replace_DuplicateAssignmentTakesLast,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.Replace_DuplicateAssignmentTakesLast",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_Replace_DuplicateAssignmentTakesLast::RunTest(const FString&)
{
    // AngelScript runs the body top-down, so only the LAST assignment decides the produced value.
    const auto Source = FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _Count = 1;\n"
        "    _Other = 5;\n"
        "    _Count = 2;\n"
        "}\n")};

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"),
        {Assign(TEXT("_Count"), TEXT("99"))});

    TestTrue (TEXT("succeeded"), Patched.Success);
    TestTrue (TEXT("first assignment untouched"), Patched.PatchedContents.Contains(TEXT("    _Count = 1;\n")));
    TestTrue (TEXT("last assignment patched"),    Patched.PatchedContents.Contains(TEXT("    _Count = 99;\n")));
    TestEqual(TEXT("exactly one diff row"),       Patched.Diff.Num(), 1);
    TestEqual(TEXT("diff points at the last line"), Patched.Diff[0].LineNumber, 5);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_Insert_NewProperty,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.Insert_NewProperty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_Insert_NewProperty::RunTest(const FString&)
{
    const auto Source = FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _Count = 1;\n"
        "}\n")};

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"),
        {Assign(TEXT("_Scale"), TEXT("2.5f"))});

    TestTrue (TEXT("succeeded"), Patched.Success);
    TestEqual(TEXT("inserted before the closing brace"), Patched.PatchedContents, FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _Count = 1;\n"
        "    _Scale = 2.5f;\n"
        "}\n")});
    TestEqual(TEXT("one diff row"), Patched.Diff.Num(), 1);
    TestTrue (TEXT("diff op is InsertLine"), Patched.Diff[0].Op == ECk_AssetBlockPatch_Op::InsertLine);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_Insert_EmptyBody,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.Insert_EmptyBody",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_Insert_EmptyBody::RunTest(const FString&)
{
    const auto Source = FString{TEXT(
        "namespace things\n"
        "{\n"
        "    asset Asset_Empty of UCk_Thing_Data\n"
        "    {\n"
        "    }\n"
        "}\n")};

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Empty"),
        {Assign(TEXT("_Count"), TEXT("3"))});

    TestTrue (TEXT("succeeded"), Patched.Success);
    TestEqual(TEXT("indent derives from the declaration when the body is empty"),
        Patched.PatchedContents, FString{TEXT(
        "namespace things\n"
        "{\n"
        "    asset Asset_Empty of UCk_Thing_Data\n"
        "    {\n"
        "        _Count = 3;\n"
        "    }\n"
        "}\n")});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_NamespaceAndEditorGuardedBlock,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.NamespaceAndEditorGuardedBlock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_NamespaceAndEditorGuardedBlock::RunTest(const FString&)
{
    const auto Source = FString{TEXT(
        "#if EDITOR\n"
        "namespace gym\n"
        "{\n"
        "    asset Asset_Guarded of UCk_Thing_Data\n"
        "    {\n"
        "        _Count = 1;\n"
        "    }\n"
        "}\n"
        "#endif\n")};

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Guarded"),
        {Assign(TEXT("_Count"), TEXT("4")), Assign(TEXT("_Extra"), TEXT("true"))});

    TestTrue (TEXT("succeeded"), Patched.Success);
    TestEqual(TEXT("preprocessor guards and namespace preserved"), Patched.PatchedContents, FString{TEXT(
        "#if EDITOR\n"
        "namespace gym\n"
        "{\n"
        "    asset Asset_Guarded of UCk_Thing_Data\n"
        "    {\n"
        "        _Count = 4;\n"
        "        _Extra = true;\n"
        "    }\n"
        "}\n"
        "#endif\n")});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_Delete_RevertedProperty,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.Delete_RevertedProperty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_Delete_RevertedProperty::RunTest(const FString&)
{
    const auto Source = FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _Count = 1;\n"
        "    _Scale = 2.0f; // trailing note goes with the line\n"
        "    _Keep = 3;\n"
        "}\n")};

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"),
        {FCk_AssetBlockPatchEntry::Make_Delete(TEXT("_Scale"))});

    TestTrue (TEXT("succeeded"), Patched.Success);
    TestEqual(TEXT("whole line removed, neighbours intact"), Patched.PatchedContents, FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _Count = 1;\n"
        "    _Keep = 3;\n"
        "}\n")});
    TestEqual(TEXT("one diff row"), Patched.Diff.Num(), 1);
    TestTrue (TEXT("diff op is DeleteLine"), Patched.Diff[0].Op == ECk_AssetBlockPatch_Op::DeleteLine);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_Delete_RemovesEveryDuplicate,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.Delete_RemovesEveryDuplicate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_Delete_RemovesEveryDuplicate::RunTest(const FString&)
{
    // Leaving one behind would still produce a non-default value, so a revert must take them all.
    const auto Source = FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _Count = 1;\n"
        "    _Keep = 3;\n"
        "    _Count = 2;\n"
        "}\n")};

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"),
        {FCk_AssetBlockPatchEntry::Make_Delete(TEXT("_Count"))});

    TestTrue (TEXT("succeeded"), Patched.Success);
    TestEqual(TEXT("no `_Count` assignment survives"), Count_Occurrences(Patched.PatchedContents, TEXT("_Count")), 0);
    TestTrue (TEXT("neighbour intact"), Patched.PatchedContents.Contains(TEXT("    _Keep = 3;\n")));
    TestEqual(TEXT("two diff rows"), Patched.Diff.Num(), 2);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_PreservesUnrecognisedLines,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.PreservesUnrecognisedLines",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_PreservesUnrecognisedLines::RunTest(const FString&)
{
    // Modelled on CkIskmRenderer_Assets.as. A container populated by `.Add()`, plus the locals it is
    // built from, must come out byte-identical — those lines are arbitrary AngelScript, not
    // assignments the patcher owns.
    const auto Source = FString{TEXT(
        "asset Asset_RendererData_Demo of UCk_IskmRenderer_Data\n"
        "{\n"
        "    _AnimCollection = Asset_AnimCollection_Demo;\n"
        "\n"
        "    // OutfitSwap station + Q3 OutfitAttach test require a \"Hat\" entry.\n"
        "    FCk_IskmRenderer_MeshDesc HatDesc;\n"
        "    HatDesc._Name = n\"Hat\";\n"
        "    HatDesc._Mesh = assets::load::SKM_Manny_Simple();\n"
        "    _Submeshes.Add(HatDesc);\n"
        "\n"
        "    _NumCustomDataFloat = 1;\n"
        "}\n")};

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_RendererData_Demo"),
        {Assign(TEXT("_NumCustomDataFloat"), TEXT("3"))});

    TestTrue (TEXT("succeeded"), Patched.Success);
    TestEqual(TEXT("exactly one line changed"), Patched.Diff.Num(), 1);
    TestEqual(TEXT("only the scalar moved"), Patched.PatchedContents,
        Source.Replace(TEXT("_NumCustomDataFloat = 1;"), TEXT("_NumCustomDataFloat = 3;")));

    // `_Submeshes.Add(...)` is a call, not an assignment: a delete request must find nothing.
    const auto NoTouch = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_RendererData_Demo"),
        {FCk_AssetBlockPatchEntry::Make_Delete(TEXT("_Submeshes"))});

    TestTrue (TEXT("delete on an Add()-only container succeeds"), NoTouch.Success);
    TestEqual(TEXT("and changes nothing"), NoTouch.PatchedContents, Source);
    TestEqual(TEXT("with no diff rows"),   NoTouch.Diff.Num(), 0);

    // `HatDesc._Name` is a LOCAL's field, never the asset's — the patcher must not adopt it.
    const auto Locals = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_RendererData_Demo"),
        {AssignField(TEXT("_Submeshes"), TEXT("._Name"), TEXT("n\"Boot\""))});

    TestTrue(TEXT("local field is not adopted"),
        Locals.PatchedContents.Contains(TEXT("    HatDesc._Name = n\"Hat\";\n")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_NestedStructLiteralInBody,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.NestedStructLiteralInBody",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_NestedStructLiteralInBody::RunTest(const FString&)
{
    const auto Source = FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _Bounds = FCk_ValueRange(0.0f, 1.0f);\n"
        "    if (true)\n"
        "    {\n"
        "        _Count = 1;\n"
        "    }\n"
        "    _Tail = 2;\n"
        "}\n")};

    const auto Location = FCkAsAssetBlockPatcher::Find_AssetBlock(Source, TEXT("Asset_Foo"));
    TestTrue(TEXT("found past the nested scope"), Location.Found);

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"),
        {Assign(TEXT("_Bounds"), TEXT("FCk_ValueRange(2.0f, 3.0f)")), Assign(TEXT("_Tail"), TEXT("5"))});

    TestTrue (TEXT("succeeded"), Patched.Success);
    TestTrue (TEXT("struct literal RHS replaced whole"),
        Patched.PatchedContents.Contains(TEXT("    _Bounds = FCk_ValueRange(2.0f, 3.0f);\n")));
    TestTrue (TEXT("tail after the nested scope replaced"),
        Patched.PatchedContents.Contains(TEXT("    _Tail = 5;\n")));
    TestTrue (TEXT("assignment nested inside a scope is left alone"),
        Patched.PatchedContents.Contains(TEXT("        _Count = 1;\n")));

    // A nested-scope assignment is not at body depth 0, so it is invisible to the patcher: a patch
    // for it inserts a new statement rather than editing the nested one.
    const auto Nested = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"),
        {Assign(TEXT("_Count"), TEXT("8"))});

    TestTrue(TEXT("nested assignment untouched"), Nested.PatchedContents.Contains(TEXT("        _Count = 1;\n")));
    TestTrue(TEXT("new statement inserted at body level"), Nested.PatchedContents.Contains(TEXT("    _Count = 8;\n")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_StructSubPathAssignments,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.StructSubPathAssignments",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_StructSubPathAssignments::RunTest(const FString&)
{
    const auto Source = FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _Display._Tint = FLinearColor(1, 0, 0, 1);\n"
        "    _Display._Icon = nullptr;\n"
        "}\n")};

    const auto Entry = FCk_AssetBlockPatchEntry::Make_Assign(TEXT("_Display"), {
        FCk_AssetBlockAssignment{TEXT("._Icon"), TEXT("assets::load::T_Icon()")},
        FCk_AssetBlockAssignment{TEXT("._Depth"), TEXT("2")},
    });

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"), {Entry});

    TestTrue (TEXT("succeeded"), Patched.Success);
    TestEqual(TEXT("changed leaf replaced, new leaf inserted, untouched leaf left alone"),
        Patched.PatchedContents, FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _Display._Tint = FLinearColor(1, 0, 0, 1);\n"
        "    _Display._Icon = assets::load::T_Icon();\n"
        "    _Display._Depth = 2;\n"
        "}\n")});

    // Delete is root-scoped: every statement whose left-hand root is `_Display` goes.
    const auto Deleted = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"),
        {FCk_AssetBlockPatchEntry::Make_Delete(TEXT("_Display"))});

    TestEqual(TEXT("all sub-path statements removed"), Deleted.PatchedContents, FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "}\n")});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_MultiLineRhsAndSharedLine,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.MultiLineRhsAndSharedLine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_MultiLineRhsAndSharedLine::RunTest(const FString&)
{
    const auto Wrapped = FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _Bounds = FCk_ValueRange(\n"
        "        0.0f,\n"
        "        1.0f);\n"
        "}\n")};

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Wrapped, TEXT("Asset_Foo"),
        {Assign(TEXT("_Bounds"), TEXT("FCk_ValueRange(2.0f, 3.0f)"))});

    TestTrue (TEXT("succeeded"), Patched.Success);
    TestEqual(TEXT("the whole wrapped RHS is replaced, not just its first line"),
        Patched.PatchedContents, FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    _Bounds = FCk_ValueRange(2.0f, 3.0f);\n"
        "}\n")});

    const auto Shared = FString{TEXT(
        "asset Asset_Bar of UCk_Thing_Data\n"
        "{\n"
        "    _A = 1; _B = 2;\n"
        "}\n")};

    const auto SharedDelete = FCkAsAssetBlockPatcher::Apply_Patch(Shared, TEXT("Asset_Bar"),
        {FCk_AssetBlockPatchEntry::Make_Delete(TEXT("_A"))});

    TestTrue(TEXT("shared-line delete keeps the neighbour"),
        SharedDelete.PatchedContents.Contains(TEXT("_B = 2;")));
    TestEqual(TEXT("shared-line delete removes only its own span"),
        Count_Occurrences(SharedDelete.PatchedContents, TEXT("_A")), 0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_LineEndings_CrlfPreserved,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.LineEndings_CrlfPreserved",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_LineEndings_CrlfPreserved::RunTest(const FString&)
{
    const auto Crlf = FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\r\n"
        "{\r\n"
        "    _Count = 1;\r\n"
        "}\r\n")};

    TestEqual(TEXT("CRLF detected"), FCkAsAssetBlockPatcher::Get_LineTerminator(Crlf), FString{TEXT("\r\n")});

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Crlf, TEXT("Asset_Foo"),
        {Assign(TEXT("_Count"), TEXT("2")), Assign(TEXT("_New"), TEXT("true"))});

    TestTrue (TEXT("succeeded"), Patched.Success);
    TestEqual(TEXT("no bare LF introduced"),
        Count_Occurrences(Patched.PatchedContents, TEXT("\n")),
        Count_Occurrences(Patched.PatchedContents, TEXT("\r\n")));
    TestEqual(TEXT("inserted line uses CRLF"), Patched.PatchedContents, FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\r\n"
        "{\r\n"
        "    _Count = 2;\r\n"
        "    _New = true;\r\n"
        "}\r\n")});

    const auto Lf = Crlf.Replace(TEXT("\r\n"), TEXT("\n"));
    TestEqual(TEXT("LF detected"), FCkAsAssetBlockPatcher::Get_LineTerminator(Lf), FString{TEXT("\n")});

    const auto LfPatched = FCkAsAssetBlockPatcher::Apply_Patch(Lf, TEXT("Asset_Foo"),
        {Assign(TEXT("_New"), TEXT("true"))});
    TestEqual(TEXT("no CR introduced into an LF file"),
        Count_Occurrences(LfPatched.PatchedContents, TEXT("\r")), 0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_Io_BomAndTerminatorRoundTrip,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.Io_BomAndTerminatorRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_Io_BomAndTerminatorRoundTrip::RunTest(const FString&)
{
    const auto TempRoot = Get_TempRoot(TEXT("CkAssetBlockPatcherTest_Io"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

    const auto Body = FString{TEXT(
        "asset Asset_Io of UCk_Thing_Data\r\n"
        "{\r\n"
        "    _Count = 1;\r\n"
        "}\r\n")};

    const auto BomPath   = TempRoot / TEXT("WithBom.as");
    const auto PlainPath = TempRoot / TEXT("NoBom.as");

    Write_Fixture(BomPath,   Body, FFileHelper::EEncodingOptions::ForceUTF8);
    Write_Fixture(PlainPath, Body, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    auto BomSnapshot = FCk_AsFileSnapshot{};
    TestTrue (TEXT("BOM file loads"),        FCkAsAssetBlockPatcher::Try_ReadSnapshot(BomPath, BomSnapshot));
    TestTrue (TEXT("BOM detected"),          BomSnapshot.HadUtf8Bom);
    TestEqual(TEXT("BOM stripped from text"), BomSnapshot.Contents, Body);
    TestEqual(TEXT("CRLF recorded"),         BomSnapshot.LineTerminator, FString{TEXT("\r\n")});

    auto PlainSnapshot = FCk_AsFileSnapshot{};
    TestTrue (TEXT("plain file loads"), FCkAsAssetBlockPatcher::Try_ReadSnapshot(PlainPath, PlainSnapshot));
    TestFalse(TEXT("no BOM detected"),  PlainSnapshot.HadUtf8Bom);

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(BomSnapshot.Contents, TEXT("Asset_Io"),
        {Assign(TEXT("_Count"), TEXT("2"))});
    TestTrue(TEXT("patch succeeded"), Patched.Success);
    TestTrue(TEXT("atomic write succeeded"),
        FCkAsAssetBlockPatcher::Try_AtomicWrite(BomSnapshot, Patched.PatchedContents));

    auto RawBytes = TArray<uint8>{};
    TestTrue(TEXT("re-read raw"), FFileHelper::LoadFileToArray(RawBytes, *BomPath));
    TestTrue(TEXT("BOM survives the round trip"),
        RawBytes.Num() >= 3 && RawBytes[0] == 0xEF && RawBytes[1] == 0xBB && RawBytes[2] == 0xBF);

    auto ReRead = FCk_AsFileSnapshot{};
    TestTrue (TEXT("re-read as snapshot"), FCkAsAssetBlockPatcher::Try_ReadSnapshot(BomPath, ReRead));
    TestEqual(TEXT("content is the patched text, CRLF intact"), ReRead.Contents, FString{TEXT(
        "asset Asset_Io of UCk_Thing_Data\r\n"
        "{\r\n"
        "    _Count = 2;\r\n"
        "}\r\n")});

    TestTrue(TEXT("no temp file left behind"),
        NOT IFileManager::Get().FileExists(*(BomPath + TEXT(".writebacktmp"))));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_Delete_UnmatchedIsReported,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.Delete_UnmatchedIsReported",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_Delete_UnmatchedIsReported::RunTest(const FString&)
{
    // `_Count` is assigned inside a nested scope, which the patcher deliberately never edits. A
    // revert therefore has no line to remove — and must say so rather than reporting success, since
    // writing an unchanged file still reloads the script and puts the old value back.
    const auto Source = FString{TEXT(
        "asset Asset_Foo of UCk_Thing_Data\n"
        "{\n"
        "    if (true)\n"
        "    {\n"
        "        _Count = 1;\n"
        "    }\n"
        "    _Keep = 2;\n"
        "}\n")};

    const auto Patched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"),
        {FCk_AssetBlockPatchEntry::Make_Delete(TEXT("_Count"))});

    TestTrue (TEXT("the patch itself does not error"), Patched.Success);
    TestEqual(TEXT("no line was changed"), Patched.Diff.Num(), 0);
    TestEqual(TEXT("the unmatched delete is reported"), Patched.UnmatchedDeletes.Num(), 1);
    if (Patched.UnmatchedDeletes.Num() == 1)
    { TestEqual(TEXT("and names the property"), Patched.UnmatchedDeletes[0], FString{TEXT("_Count")}); }

    // A delete that DOES match reports nothing unmatched.
    const auto Matched = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"),
        {FCk_AssetBlockPatchEntry::Make_Delete(TEXT("_Keep"))});

    TestEqual(TEXT("matched delete reports nothing unmatched"), Matched.UnmatchedDeletes.Num(), 0);
    TestEqual(TEXT("and changes one line"), Matched.Diff.Num(), 1);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetBlockPatcher_EmptyEntryList,
    "CkAngelscriptGenerator.UnitTests.AssetBlockPatcher.EmptyEntryList",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetBlockPatcher_EmptyEntryList::RunTest(const FString&)
{
    const auto Source = FString{TEXT("asset Asset_Foo of UCk_Thing_Data\n{\n}\n")};
    const auto Result = FCkAsAssetBlockPatcher::Apply_Patch(Source, TEXT("Asset_Foo"), {});

    TestFalse(TEXT("refused"), Result.Success);
    TestTrue (TEXT("reason is NothingToPatch"),
        Result.FailReason == ECk_AssetBlockPatch_FailReason::NothingToPatch);

    return true;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
