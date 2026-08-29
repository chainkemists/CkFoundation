#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AccessorResolver.h"
#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AssetBlockPatcher.h"

#include "CkCore/Macros/CkMacros.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace ck::angelscriptgenerator::write_back;

namespace ck_test_accessor_resolver
{
    // Mirrors the real emitted shape (CkAssetRegistrySubsystem.cpp:692-757 / the on-disk
    // Plugins/CkTests/Script/Generated/CkTestsAssets.as): a soft `namespace <ns>` section of
    // one-line accessors, then a `namespace <ns>::load` section of blocking-load wrappers.
    auto Make_GeneratedFile(
        const TCHAR* InPath = TEXT("D:/Fake/Generated/TestAssets.as")) -> FCk_GeneratedAccessorFile
    {
        auto File = FCk_GeneratedAccessorFile{};
        File.AbsolutePath      = InPath;
        File.FallbackNamespace = TEXT("fallback_ns");
        File.Contents = FString{TEXT(
            "// Auto-generated Asset Registry\n"
            "// DO NOT EDIT - This file is automatically regenerated\n"
            "\n"
            "// Soft references - for deferred loading\n"
            "namespace assets\n"
            "{\n"
            "    TSoftObjectPtr<USkeleton> SK_Mannequin() { return TSoftObjectPtr<USkeleton>(FSoftObjectPath(\"/CkTests/Meshes/SK_Mannequin.SK_Mannequin\")); }\n"
            "    TSoftObjectPtr<AActor> BP_DemoRoom() { return TSoftObjectPtr<AActor>(FSoftObjectPath(\"/CkTests/BP/BP_DemoRoom.BP_DemoRoom\")); }\n"
            "    TSoftClassPtr<AActor> BP_DemoRoom_Class() { return TSoftClassPtr<AActor>(FSoftObjectPath(\"/CkTests/BP/BP_DemoRoom.BP_DemoRoom_C\")); }\n"
            "    TSoftObjectPtr<UTexture2D> T_Icon_DUP1() { return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(\"/CkTests/UI/Second/T_Icon.T_Icon\")); }\n"
            "#if Editor\n"
            "    TSoftObjectPtr<UTexture2D> T_EditorOnly() { return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(\"/CkTests/Editor/T_EditorOnly.T_EditorOnly\")); }\n"
            "#endif\n"
            "}\n"
            "\n"
            "namespace assets::load\n"
            "{\n"
            "    USkeleton SK_Mannequin()\n"
            "    {\n"
            "        return System::LoadAsset_Blocking(assets::SK_Mannequin());\n"
            "    }\n"
            "}\n")};
        return File;
    }

    auto Make_Index(
        const FCk_GeneratedAccessorFile& InFile) -> TMap<FString, FCk_ScriptAccessorEntry>
    {
        return FCkAsAccessorResolver::Build_Index(FCkAsAccessorResolver::Parse_GeneratedAccessorFile(InFile));
    }
}

using namespace ck_test_accessor_resolver;

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AccessorResolver_Parse_GeneratedFile,
    "CkAngelscriptGenerator.UnitTests.AccessorResolver.Parse_GeneratedFile",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AccessorResolver_Parse_GeneratedFile::RunTest(const FString&)
{
    const auto Entries = FCkAsAccessorResolver::Parse_GeneratedAccessorFile(Make_GeneratedFile());

    // `_C` lines are class siblings of a base entry, never entries in their own right.
    TestEqual(TEXT("four base entries"), Entries.Num(), 4);

    const auto Index = FCkAsAccessorResolver::Build_Index(Entries);

    const auto* Skeleton = Index.Find(TEXT("/CkTests/Meshes/SK_Mannequin.SK_Mannequin"));
    TestNotNull(TEXT("skeleton entry"), Skeleton);
    if (Skeleton == nullptr)
    { return false; }

    TestEqual(TEXT("namespace read from the file, not the fallback"), Skeleton->Namespace, FString{TEXT("assets")});
    TestEqual(TEXT("function name"),   Skeleton->FunctionName, FString{TEXT("SK_Mannequin")});
    TestFalse(TEXT("no class sibling"), Skeleton->HasClassAccessor);
    TestFalse(TEXT("not editor-only"),  Skeleton->IsEditorOnly);
    TestEqual(TEXT("source file recorded"), Skeleton->SourceFile, FString{TEXT("D:/Fake/Generated/TestAssets.as")});

    const auto* DemoRoom = Index.Find(TEXT("/CkTests/BP/BP_DemoRoom.BP_DemoRoom"));
    TestNotNull(TEXT("blueprint entry"), DemoRoom);
    if (DemoRoom != nullptr)
    { TestTrue(TEXT("class sibling linked from the `_C` line"), DemoRoom->HasClassAccessor); }

    const auto* EditorOnly = Index.Find(TEXT("/CkTests/Editor/T_EditorOnly.T_EditorOnly"));
    TestNotNull(TEXT("editor-only entry"), EditorOnly);
    if (EditorOnly != nullptr)
    { TestTrue(TEXT("`#if Editor` guard recorded"), EditorOnly->IsEditorOnly); }

    // `_DUP{N}` is not derivable from the asset name, so it can only come from the generated file.
    const auto* Duped = Index.Find(TEXT("/CkTests/UI/Second/T_Icon.T_Icon"));
    TestNotNull(TEXT("deduped entry"), Duped);
    if (Duped != nullptr)
    { TestEqual(TEXT("`_DUP1` name honoured verbatim"), Duped->FunctionName, FString{TEXT("T_Icon_DUP1")}); }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AccessorResolver_Emit_EveryKind,
    "CkAngelscriptGenerator.UnitTests.AccessorResolver.Emit_EveryKind",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AccessorResolver_Emit_EveryKind::RunTest(const FString&)
{
    const auto Index = Make_Index(Make_GeneratedFile());

    const auto Soft = FCkAsAccessorResolver::Resolve(Index, /*InAnyProviderRegistered=*/true,
        TEXT("/CkTests/Meshes/SK_Mannequin.SK_Mannequin"), ECk_ScriptAccessorKind::SoftObject, false);
    TestTrue (TEXT("soft resolves"), Soft.Success);
    TestEqual(TEXT("soft expression"), Soft.Expression, FString{TEXT("assets::SK_Mannequin()")});

    const auto Hard = FCkAsAccessorResolver::Resolve(Index, true,
        TEXT("/CkTests/Meshes/SK_Mannequin.SK_Mannequin"), ECk_ScriptAccessorKind::HardObject, false);
    TestTrue (TEXT("hard resolves"), Hard.Success);
    TestEqual(TEXT("hard expression uses load::"), Hard.Expression, FString{TEXT("assets::load::SK_Mannequin()")});

    const auto SoftClass = FCkAsAccessorResolver::Resolve(Index, true,
        TEXT("/CkTests/BP/BP_DemoRoom.BP_DemoRoom_C"), ECk_ScriptAccessorKind::SoftClass, false);
    TestTrue (TEXT("soft class resolves from the `_C` path"), SoftClass.Success);
    TestEqual(TEXT("soft class expression"), SoftClass.Expression, FString{TEXT("assets::BP_DemoRoom_Class()")});

    const auto HardClass = FCkAsAccessorResolver::Resolve(Index, true,
        TEXT("/CkTests/BP/BP_DemoRoom.BP_DemoRoom_C"), ECk_ScriptAccessorKind::HardClass, false);
    TestTrue (TEXT("hard class resolves"), HardClass.Success);
    TestEqual(TEXT("hard class expression"), HardClass.Expression, FString{TEXT("assets::load::BP_DemoRoom_Class()")});

    // A class request against a non-Blueprint entry has no `_Class` sibling to reach.
    const auto MissingClass = FCkAsAccessorResolver::Resolve(Index, true,
        TEXT("/CkTests/Meshes/SK_Mannequin.SK_Mannequin"), ECk_ScriptAccessorKind::HardClass, false);
    TestFalse(TEXT("refused"), MissingClass.Success);
    TestTrue (TEXT("reason is NoClassAccessor"),
        MissingClass.FailReason == ECk_AccessorResolve_FailReason::NoClassAccessor);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AccessorResolver_NoProviderVsNoAccessor,
    "CkAngelscriptGenerator.UnitTests.AccessorResolver.NoProviderVsNoAccessor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AccessorResolver_NoProviderVsNoAccessor::RunTest(const FString&)
{
    const auto Index = Make_Index(Make_GeneratedFile());

    // "Nothing is registered" and "this one asset is unreachable" are different user problems and
    // must not collapse into one message.
    const auto NoProvider = FCkAsAccessorResolver::Resolve(Index, /*InAnyProviderRegistered=*/false,
        TEXT("/CkTests/Meshes/SK_Mannequin.SK_Mannequin"), ECk_ScriptAccessorKind::SoftObject, false);
    TestFalse(TEXT("refused"), NoProvider.Success);
    TestTrue (TEXT("reason is NoProviderRegistered"),
        NoProvider.FailReason == ECk_AccessorResolve_FailReason::NoProviderRegistered);
    TestTrue (TEXT("message names the generation step"),
        NoProvider.ErrorMessage.Contains(TEXT("Generate the asset registry")));

    const auto NoAccessor = FCkAsAccessorResolver::Resolve(Index, /*InAnyProviderRegistered=*/true,
        TEXT("/Game/Elsewhere/T_Unreachable.T_Unreachable"), ECk_ScriptAccessorKind::SoftObject, false);
    TestFalse(TEXT("refused"), NoAccessor.Success);
    TestTrue (TEXT("reason is NoAccessorFound"),
        NoAccessor.FailReason == ECk_AccessorResolve_FailReason::NoAccessorFound);
    TestTrue (TEXT("message names the discovery root"),
        NoAccessor.ErrorMessage.Contains(TEXT("discovery root")));

    TestTrue(TEXT("the two messages are distinct"), NoProvider.ErrorMessage != NoAccessor.ErrorMessage);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AccessorResolver_EditorOnlyGate,
    "CkAngelscriptGenerator.UnitTests.AccessorResolver.EditorOnlyGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AccessorResolver_EditorOnlyGate::RunTest(const FString&)
{
    const auto Index = Make_Index(Make_GeneratedFile());
    const auto EditorOnlyPath = FString{TEXT("/CkTests/Editor/T_EditorOnly.T_EditorOnly")};

    const auto Rejected = FCkAsAccessorResolver::Resolve(Index, true,
        EditorOnlyPath, ECk_ScriptAccessorKind::SoftObject, /*InTargetBlockIsEditorOnly=*/false);
    TestFalse(TEXT("refused from an unguarded block"), Rejected.Success);
    TestTrue (TEXT("reason is EditorOnlyAccessorFromRuntimeBlock"),
        Rejected.FailReason == ECk_AccessorResolve_FailReason::EditorOnlyAccessorFromRuntimeBlock);

    const auto Allowed = FCkAsAccessorResolver::Resolve(Index, true,
        EditorOnlyPath, ECk_ScriptAccessorKind::SoftObject, /*InTargetBlockIsEditorOnly=*/true);
    TestTrue (TEXT("allowed from a guarded block"), Allowed.Success);
    TestEqual(TEXT("expression"), Allowed.Expression, FString{TEXT("assets::T_EditorOnly()")});

    // A non-editor accessor is legal from either kind of block.
    const auto Plain = FCkAsAccessorResolver::Resolve(Index, true,
        TEXT("/CkTests/Meshes/SK_Mannequin.SK_Mannequin"), ECk_ScriptAccessorKind::SoftObject, true);
    TestTrue(TEXT("plain accessor allowed inside a guard too"), Plain.Success);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AccessorResolver_DuplicatePathAcrossFiles,
    "CkAngelscriptGenerator.UnitTests.AccessorResolver.DuplicatePathAcrossFiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AccessorResolver_DuplicatePathAcrossFiles::RunTest(const FString&)
{
    auto Second = FCk_GeneratedAccessorFile{};
    Second.AbsolutePath      = TEXT("D:/Fake/Generated/ZZ_Later.as");
    Second.FallbackNamespace = TEXT("other");
    Second.Contents = FString{TEXT(
        "namespace other\n"
        "{\n"
        "    TSoftObjectPtr<USkeleton> SK_Mannequin_Shadow() { return TSoftObjectPtr<USkeleton>(FSoftObjectPath(\"/CkTests/Meshes/SK_Mannequin.SK_Mannequin\")); }\n"
        "}\n")};

    auto All = FCkAsAccessorResolver::Parse_GeneratedAccessorFile(Make_GeneratedFile());
    All.Append(FCkAsAccessorResolver::Parse_GeneratedAccessorFile(Second));

    const auto Index = FCkAsAccessorResolver::Build_Index(All);

    const auto Resolved = FCkAsAccessorResolver::Resolve(Index, true,
        TEXT("/CkTests/Meshes/SK_Mannequin.SK_Mannequin"), ECk_ScriptAccessorKind::SoftObject, false);

    TestTrue (TEXT("resolves"), Resolved.Success);
    TestEqual(TEXT("first file in scan order wins"), Resolved.Expression, FString{TEXT("assets::SK_Mannequin()")});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AccessorResolver_EditorGuardDetection,
    "CkAngelscriptGenerator.UnitTests.AccessorResolver.EditorGuardDetection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AccessorResolver_EditorGuardDetection::RunTest(const FString&)
{
    const auto Source = FString{TEXT(
        "asset Asset_Plain of UCk_Thing_Data\n"   // line 1
        "{\n"
        "}\n"
        "\n"
        "#if EDITOR\n"                            // line 5
        "asset Asset_Guarded of UCk_Thing_Data\n" // line 6
        "{\n"
        "}\n"
        "#endif\n"                                // line 9
        "\n"
        "asset Asset_After of UCk_Thing_Data\n"   // line 11
        "{\n"
        "}\n")};

    const auto Plain = FCkAsAssetBlockPatcher::Find_AssetBlock(Source, TEXT("Asset_Plain"));
    const auto Guarded = FCkAsAssetBlockPatcher::Find_AssetBlock(Source, TEXT("Asset_Guarded"));
    const auto After = FCkAsAssetBlockPatcher::Find_AssetBlock(Source, TEXT("Asset_After"));

    TestTrue (TEXT("all three located"), Plain.Found && Guarded.Found && After.Found);
    TestFalse(TEXT("before the guard"),  FCkAsAssetBlockPatcher::Get_IsInsideEditorGuard(Source, Plain.DeclStart));
    TestTrue (TEXT("inside the guard"),  FCkAsAssetBlockPatcher::Get_IsInsideEditorGuard(Source, Guarded.DeclStart));
    TestFalse(TEXT("after `#endif`"),    FCkAsAssetBlockPatcher::Get_IsInsideEditorGuard(Source, After.DeclStart));

    // A nested non-editor `#if` must not let its `#endif` close the editor guard.
    const auto Nested = FString{TEXT(
        "#if EDITOR\n"
        "#if SOMETHING_ELSE\n"
        "#endif\n"
        "asset Asset_Deep of UCk_Thing_Data\n"
        "{\n"
        "}\n"
        "#endif\n")};

    const auto Deep = FCkAsAssetBlockPatcher::Find_AssetBlock(Nested, TEXT("Asset_Deep"));
    TestTrue(TEXT("still inside the editor guard"),
        FCkAsAssetBlockPatcher::Get_IsInsideEditorGuard(Nested, Deep.DeclStart));

    // Lowercase `#if editor` is what CkTests_Assets.as actually uses.
    const auto Lower = FString{TEXT("#if editor\nasset Asset_Low of UCk_Thing_Data\n{\n}\n#endif\n")};
    const auto Low = FCkAsAssetBlockPatcher::Find_AssetBlock(Lower, TEXT("Asset_Low"));
    TestTrue(TEXT("case-insensitive guard token"),
        FCkAsAssetBlockPatcher::Get_IsInsideEditorGuard(Lower, Low.DeclStart));

    // The `#else` arm is the not-editor side; treating it as guarded would let an editor-only
    // accessor into code that ships.
    const auto Else = FString{TEXT(
        "#if EDITOR\n"
        "#else\n"
        "asset Asset_Runtime of UCk_Thing_Data\n"
        "{\n"
        "}\n"
        "#endif\n")};

    const auto Runtime = FCkAsAssetBlockPatcher::Find_AssetBlock(Else, TEXT("Asset_Runtime"));
    TestTrue (TEXT("located"), Runtime.Found);
    TestFalse(TEXT("the `#else` arm is NOT editor-guarded"),
        FCkAsAssetBlockPatcher::Get_IsInsideEditorGuard(Else, Runtime.DeclStart));

    // The else-arm of a NON-editor guard must not read as editor-guarded either. Flipping a boolean
    // on `#else` would report true here and let an editor-only accessor into shipping code.
    const auto OtherElse = FString{TEXT(
        "#if SOMETHING_ELSE\n"
        "#else\n"
        "asset Asset_NotEditor of UCk_Thing_Data\n"
        "{\n"
        "}\n"
        "#endif\n")};

    const auto NotEditor = FCkAsAssetBlockPatcher::Find_AssetBlock(OtherElse, TEXT("Asset_NotEditor"));
    TestTrue (TEXT("located"), NotEditor.Found);
    TestFalse(TEXT("the else-arm of a non-editor `#if` is NOT editor-guarded"),
        FCkAsAssetBlockPatcher::Get_IsInsideEditorGuard(OtherElse, NotEditor.DeclStart));

    // `#elif` also leaves the editor arm.
    const auto Elif = FString{TEXT(
        "#if EDITOR\n"
        "#elif SOMETHING\n"
        "asset Asset_Elif of UCk_Thing_Data\n"
        "{\n"
        "}\n"
        "#endif\n")};

    const auto ElifBlock = FCkAsAssetBlockPatcher::Find_AssetBlock(Elif, TEXT("Asset_Elif"));
    TestTrue (TEXT("located"), ElifBlock.Found);
    TestFalse(TEXT("the `#elif` arm is NOT editor-guarded"),
        FCkAsAssetBlockPatcher::Get_IsInsideEditorGuard(Elif, ElifBlock.DeclStart));

    // A guard mentioned inside a comment must not open anything.
    const auto Commented = FString{TEXT(
        "/*\n"
        "#if EDITOR\n"
        "*/\n"
        "asset Asset_Commented of UCk_Thing_Data\n"
        "{\n"
        "}\n")};

    const auto CommentedBlock = FCkAsAssetBlockPatcher::Find_AssetBlock(Commented, TEXT("Asset_Commented"));
    TestTrue (TEXT("located"), CommentedBlock.Found);
    TestFalse(TEXT("a commented-out `#if EDITOR` opens no guard"),
        FCkAsAssetBlockPatcher::Get_IsInsideEditorGuard(Commented, CommentedBlock.DeclStart));

    return true;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
