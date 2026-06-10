// Snapshot tests for the AS error parser (Rev 10 dispatcher input layer).
//
// Inputs are verbatim from the 2026-05-11 corruption probes. These tests
// are the engine-upgrade canary — if they go red after a Hazelight upgrade,
// the AS compile-error format has changed and the parser must be updated
// before the dispatcher can resume working.

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

#include "CkCore/Macros/CkMacros.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace ck::angelscriptgenerator::self_heal;

// --------------------------------------------------------------------------------------------------------------------
// Probe 1: EntitySpawnParams drift — single "No matching signatures" with typed arg.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsErrorParser_EntitySpawnParamsProbe,
    "CkAngelscriptGenerator.UnitTests.AsErrorParser.EntitySpawnParamsProbe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsErrorParser_EntitySpawnParamsProbe::RunTest(const FString&)
{
    const auto Input = FString{TEXT(
        "D:/Repos/BusterBlock/Script/StoreDriver/BB_StoreDriver_EntityScript.as:\n"
        "(464:5) Compiling void UBb_StoreDriver_EntityScript::Drive_TruckLifecycle(TArray<FBb_StoreManager_OrderEntry>)\n"
        "(491:13) No matching signatures to 'UBb_DeliveryTruck_EntityScript::Params(const FTransform)'\n")};

    const auto Errors = FCkAsErrorParser::ParseErrors(Input);

    TestEqual(TEXT("error count"), Errors.Num(), 1);
    if (Errors.Num() < 1) { return false; }

    const auto& E = Errors[0];
    TestEqual(TEXT("Kind"),            static_cast<int32>(E.Kind), static_cast<int32>(ECk_AsParsedError_Kind::NoMatchingSignatures));
    TestEqual(TEXT("FilePath"),        E.FilePath,                  FString{TEXT("D:/Repos/BusterBlock/Script/StoreDriver/BB_StoreDriver_EntityScript.as")});
    TestEqual(TEXT("Line"),            E.Line,                      491);
    TestEqual(TEXT("Column"),          E.Column,                    13);
    TestEqual(TEXT("TargetNamespace"), E.TargetNamespace,           FString{TEXT("UBb_DeliveryTruck_EntityScript")});
    TestEqual(TEXT("FunctionName"),    E.FunctionName,              FString{TEXT("Params")});
    TestEqual(TEXT("ArgsList"),        E.ArgsList,                  FString{TEXT("const FTransform")});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Probe 2: AssetRegistry drift — single "No matching signatures" with empty args.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsErrorParser_AssetRegistryProbe,
    "CkAngelscriptGenerator.UnitTests.AsErrorParser.AssetRegistryProbe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsErrorParser_AssetRegistryProbe::RunTest(const FString&)
{
    const auto Input = FString{TEXT(
        "D:/Repos/BusterBlock/Script/PlayerCharacter/BB_PlayerCharacter_Assets.as:\n"
        "(9:679) Compiling void __Init_Asset_PlayerCharacter_Config(UBb_PlayerCharacter_Config)\n"
        "(13:33) No matching signatures to 'assets::MALE_SKEL_NEW()'\n")};

    const auto Errors = FCkAsErrorParser::ParseErrors(Input);

    TestEqual(TEXT("error count"), Errors.Num(), 1);
    if (Errors.Num() < 1) { return false; }

    const auto& E = Errors[0];
    TestEqual(TEXT("Kind"),            static_cast<int32>(E.Kind), static_cast<int32>(ECk_AsParsedError_Kind::NoMatchingSignatures));
    TestEqual(TEXT("FilePath"),        E.FilePath,                  FString{TEXT("D:/Repos/BusterBlock/Script/PlayerCharacter/BB_PlayerCharacter_Assets.as")});
    TestEqual(TEXT("Line"),            E.Line,                      13);
    TestEqual(TEXT("Column"),          E.Column,                    33);
    TestEqual(TEXT("TargetNamespace"), E.TargetNamespace,           FString{TEXT("assets")});
    TestEqual(TEXT("FunctionName"),    E.FunctionName,              FString{TEXT("MALE_SKEL_NEW")});
    TestEqual(TEXT("ArgsList"),        E.ArgsList,                  FString{});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Probe 3: DynamicHandle drift — "Identifier not a data type" with both namespace
// and global-namespace scopes, plus cascade noise that must be dropped.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsErrorParser_DynamicHandleProbe,
    "CkAngelscriptGenerator.UnitTests.AsErrorParser.DynamicHandleProbe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsErrorParser_DynamicHandleProbe::RunTest(const FString&)
{
    const auto Input = FString{TEXT(
        "D:/Repos/BusterBlock/Script/ECS/CheckoutCounter/BB_CheckoutCounter_DebugCheats.as:\n"
        "(73:23) Identifier 'FCk_Handle_CheckoutCounter' is not a data type in namespace 'bb_checkout_cheats' or parent\n"
        "(13:5) Compiling void ForceScanning_All(FCk_Handle)\n"
        "(18:101) '_Iterator' is not declared\n"
        "(19:50) Illegal operation on 'Unknown'\n"
        "D:/Repos/BusterBlock/Script/ECS/CheckoutCounter/BB_CheckoutCounter_Feature.as:\n"
        "(232:502) Identifier 'FCk_Handle_CheckoutCounter' is not a data type in global namespace\n")};

    const auto Errors = FCkAsErrorParser::ParseErrors(Input);

    // Two recognized roots; everything else (Compiling context, "_Iterator not declared",
    // "Illegal operation on Unknown") is cascade noise and must be dropped.
    TestEqual(TEXT("error count"), Errors.Num(), 2);
    if (Errors.Num() < 2) { return false; }

    // First: namespace-scoped lookup failure in DebugCheats.as
    {
        const auto& E = Errors[0];
        TestEqual(TEXT("[0] Kind"),              static_cast<int32>(E.Kind), static_cast<int32>(ECk_AsParsedError_Kind::IdentifierNotADataType));
        TestEqual(TEXT("[0] FilePath"),          E.FilePath,                  FString{TEXT("D:/Repos/BusterBlock/Script/ECS/CheckoutCounter/BB_CheckoutCounter_DebugCheats.as")});
        TestEqual(TEXT("[0] Line"),              E.Line,                      73);
        TestEqual(TEXT("[0] Column"),            E.Column,                    23);
        TestEqual(TEXT("[0] MissingIdentifier"), E.MissingIdentifier,         FString{TEXT("FCk_Handle_CheckoutCounter")});
        TestEqual(TEXT("[0] LookupScope"),       E.LookupScope,               FString{TEXT("bb_checkout_cheats")});
    }

    // Second: global-namespace lookup failure in Feature.as — LookupScope must be empty.
    {
        const auto& E = Errors[1];
        TestEqual(TEXT("[1] Kind"),              static_cast<int32>(E.Kind), static_cast<int32>(ECk_AsParsedError_Kind::IdentifierNotADataType));
        TestEqual(TEXT("[1] FilePath"),          E.FilePath,                  FString{TEXT("D:/Repos/BusterBlock/Script/ECS/CheckoutCounter/BB_CheckoutCounter_Feature.as")});
        TestEqual(TEXT("[1] Line"),              E.Line,                      232);
        TestEqual(TEXT("[1] Column"),            E.Column,                    502);
        TestEqual(TEXT("[1] MissingIdentifier"), E.MissingIdentifier,         FString{TEXT("FCk_Handle_CheckoutCounter")});
        TestEqual(TEXT("[1] LookupScope"),       E.LookupScope,               FString{});
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Multi-arg "No matching signatures" — confirms the args-list regex tolerates
// commas and template angle brackets without prematurely terminating.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsErrorParser_NoMatchingSignatures_MultiArg,
    "CkAngelscriptGenerator.UnitTests.AsErrorParser.NoMatchingSignatures_MultiArg",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsErrorParser_NoMatchingSignatures_MultiArg::RunTest(const FString&)
{
    const auto Input = FString{TEXT(
        "D:/Repos/BusterBlock/Script/Foo/Bar.as:\n"
        "(42:7) No matching signatures to 'UBb_Foo_EntityScript::Params(const FTransform, UClass, int32)'\n")};

    const auto Errors = FCkAsErrorParser::ParseErrors(Input);

    TestEqual(TEXT("error count"), Errors.Num(), 1);
    if (Errors.Num() < 1) { return false; }

    const auto& E = Errors[0];
    TestEqual(TEXT("ArgsList"), E.ArgsList, FString{TEXT("const FTransform, UClass, int32")});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Dedup: same root cause across multiple cascade sites collapses to one entry.
// Source file/line/column are intentionally NOT part of the dedup key.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsErrorParser_DeduplicateRoots,
    "CkAngelscriptGenerator.UnitTests.AsErrorParser.DeduplicateRoots",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsErrorParser_DeduplicateRoots::RunTest(const FString&)
{
    // Same missing identifier referenced from three different sites + one
    // distinct "No matching signatures" error. Expect 2 unique roots.
    const auto Input = FString{TEXT(
        "D:/Repos/BusterBlock/Script/A.as:\n"
        "(10:5) Identifier 'FCk_Handle_X' is not a data type in global namespace\n"
        "(20:5) Identifier 'FCk_Handle_X' is not a data type in namespace 'foo' or parent\n"
        "D:/Repos/BusterBlock/Script/B.as:\n"
        "(30:5) Identifier 'FCk_Handle_X' is not a data type in global namespace\n"
        "(40:5) No matching signatures to 'UBb_Y_EntityScript::Params()'\n")};

    const auto Raw  = FCkAsErrorParser::ParseErrors(Input);
    TestEqual(TEXT("raw error count (pre-dedup)"), Raw.Num(), 4);

    const auto Deduped = FCkAsErrorParser::DeduplicateRoots(Raw);
    TestEqual(TEXT("unique root count"), Deduped.Num(), 2);
    if (Deduped.Num() < 2) { return false; }

    // Preserves order of first appearance.
    TestEqual(TEXT("[0] MissingIdentifier"), Deduped[0].MissingIdentifier, FString{TEXT("FCk_Handle_X")});
    TestEqual(TEXT("[1] FunctionName"),      Deduped[1].FunctionName,      FString{TEXT("Params")});
    TestEqual(TEXT("[1] TargetNamespace"),   Deduped[1].TargetNamespace,   FString{TEXT("UBb_Y_EntityScript")});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Adjacent string-literal splice — Hazelight emits as a TWO-line pair:
//   (L:C) Expected ')' or ','
//   (L:C) Instead found '<string constant>'
// The "Expected ')' or ','" line is generic (any parse error of that shape
// fires it), so the parser keys on the unique second line.
//
// Captured 2026-05-17 from a CkAggro AS test that broke a string across lines
// C-style; recognized so the dispatcher's actionable banner replaces the
// generic "no recognized roots" terminal that wedges headless test runs.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsErrorParser_AdjacentStringLiteralProbe,
    "CkAngelscriptGenerator.UnitTests.AsErrorParser.AdjacentStringLiteralProbe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsErrorParser_AdjacentStringLiteralProbe::RunTest(const FString&)
{
    const auto Input = FString{TEXT(
        "D:/Repos/CkPlugins/Plugins/CkTests/Script/CkAggro/CkAutoTest_Aggro_GetBestAggroSingle.as:\n"
        "(37:5) Compiling void UCk_AutoTest_Aggro_GetBestAggroSingle::OnSettled(FCk_Handle_Timer, FCk_Chrono, FCk_Time)\n"
        "(44:13) Expected ')' or ','\n"
        "(44:13) Instead found '<string constant>'\n")};

    const auto Errors = FCkAsErrorParser::ParseErrors(Input);

    // Exactly one parsed root: the "Instead found '<string constant>'" line.
    // The generic "Expected ')' or ','" companion is intentionally dropped —
    // it isn't a unique enough signature to act on.
    TestEqual(TEXT("error count"), Errors.Num(), 1);
    if (Errors.Num() < 1) { return false; }

    const auto& E = Errors[0];
    TestEqual(TEXT("Kind"),     static_cast<int32>(E.Kind), static_cast<int32>(ECk_AsParsedError_Kind::AdjacentStringLiteral));
    TestEqual(TEXT("FilePath"), E.FilePath,                  FString{TEXT("D:/Repos/CkPlugins/Plugins/CkTests/Script/CkAggro/CkAutoTest_Aggro_GetBestAggroSingle.as")});
    TestEqual(TEXT("Line"),     E.Line,                      44);
    TestEqual(TEXT("Column"),   E.Column,                    13);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Cascade-only input (no recognized roots) yields an empty result. This is the
// signal the dispatcher uses to show a terminal banner instead of looping.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsErrorParser_UnrecognizedNoise,
    "CkAngelscriptGenerator.UnitTests.AsErrorParser.UnrecognizedNoise",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsErrorParser_UnrecognizedNoise::RunTest(const FString&)
{
    const auto Input = FString{TEXT(
        "D:/Repos/BusterBlock/Script/Foo.as:\n"
        "(10:5) '_Iterator' is not declared\n"
        "(11:5) Illegal operation on 'Unknown'\n"
        "(12:5) 'State' is not declared\n"
        "(13:5) Can't implicitly convert from 'Unknown' to 'AActor'.\n")};

    const auto Errors = FCkAsErrorParser::ParseErrors(Input);

    TestEqual(TEXT("error count"), Errors.Num(), 0);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Empty / whitespace-only input yields empty result without crashing.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsErrorParser_EmptyInput,
    "CkAngelscriptGenerator.UnitTests.AsErrorParser.EmptyInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsErrorParser_EmptyInput::RunTest(const FString&)
{
    TestEqual(TEXT("empty string"),       FCkAsErrorParser::ParseErrors(FString{}).Num(),                       0);
    TestEqual(TEXT("whitespace only"),    FCkAsErrorParser::ParseErrors(FString{TEXT("   \n\n  \t  \n")}).Num(), 0);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Cascade filter: AS errors whose argument list contains "Unknown" are
// downstream of a real root cause (typically a missing handle type the
// IdentifierNotADataType path will catch). The parser drops them so the
// dispatcher doesn't classify them as Unrecognized and emit terminal-banner
// warnings for symptoms rather than the root.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsErrorParser_CascadeUnknownFilter,
    "CkAngelscriptGenerator.UnitTests.AsErrorParser.CascadeUnknownFilter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsErrorParser_CascadeUnknownFilter::RunTest(const FString&)
{
    // Same shape as the handle-drift probe cascade: the IdentifierNotADataType
    // is the root (FCk_Handle_CheckoutCounter missing); the As_X / ck::IsValid
    // calls are downstream symptoms with "Unknown" in their args.
    const auto Input = FString{TEXT(
        "D:/Repos/BusterBlock/Script/Sample.as:\n"
        "(10:5) Identifier 'FCk_Handle_X' is not a data type in global namespace\n"
        "(11:5) No matching signatures to 'FCk_Handle::As_X(Unknown)'\n"
        "(12:5) No matching signatures to 'ck::IsValid(Unknown)'\n"
        "(13:5) No matching signatures to 'ck::Is_NOT_Valid(Unknown)'\n"
        "(14:5) No matching signatures to 'FCk_Handle_Y::As_Z(const ECk_SanityCheck, Unknown)'\n")};

    const auto Errors = FCkAsErrorParser::ParseErrors(Input);

    // Only the IdentifierNotADataType root should survive. All four
    // NoMatchingSignatures entries have "Unknown" in their args -> filtered.
    TestEqual(TEXT("filtered to root only"), Errors.Num(), 1);
    if (Errors.Num() != 1) { return false; }

    TestEqual(TEXT("survivor is IdentifierNotADataType"),
        static_cast<int32>(Errors[0].Kind),
        static_cast<int32>(ECk_AsParsedError_Kind::IdentifierNotADataType));
    TestEqual(TEXT("survivor missing identifier"),
        Errors[0].MissingIdentifier,
        FString{TEXT("FCk_Handle_X")});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Probe: bare constructor-style call — `No matching signatures to '<Ident>(<args>)'`
// with NO namespace qualifier. The direct-construction shape from the 2026-06
// fresh-clone ESP boot test: a caller constructs a generated F<X>_SpawnParams
// whose canonical file is missing; the subsequent field accesses cascade as
// "'<Field>' is not a member of 'Unknown'" lines that must be dropped.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsErrorParser_BareCtorProbe,
    "CkAngelscriptGenerator.UnitTests.AsErrorParser.BareCtorProbe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsErrorParser_BareCtorProbe::RunTest(const FString&)
{
    const auto Input = FString{TEXT(
        "D:/Repos/BusterBlock/Script/ECS/CombatReceiver/BB_CombatReceiver_Utils.as:\n"
        "(63:5) Compiling FCk_Handle_CombatReceiver utils_combat_receiver::Add(FCk_Handle, FBb_Fragment_CombatReceiver_Params)\n"
        "(85:30) No matching signatures to 'FBb_CombatReceiver_DamageReceiver_SpawnParams()'\n"
        "(87:24) 'Phase' is not a member of 'Unknown'\n"
        "D:/Repos/BusterBlock/Script/ECS/NpcPortal/BB_NpcPortal_Cheats.as:\n"
        "(70:28) No matching signatures to 'FBb_Npc_EntityScript_SpawnParams(const FTransform)'\n")};

    const auto Errors = FCkAsErrorParser::ParseErrors(Input);

    // Two bare-ctor roots; the Compiling context and the 'not a member of
    // Unknown' cascade are dropped.
    TestEqual(TEXT("error count"), Errors.Num(), 2);
    if (Errors.Num() < 2) { return false; }

    {
        const auto& E = Errors[0];
        TestEqual(TEXT("[0] Kind"),              static_cast<int32>(E.Kind), static_cast<int32>(ECk_AsParsedError_Kind::BareCtorNoMatchingSignatures));
        TestEqual(TEXT("[0] FilePath"),          E.FilePath,                  FString{TEXT("D:/Repos/BusterBlock/Script/ECS/CombatReceiver/BB_CombatReceiver_Utils.as")});
        TestEqual(TEXT("[0] Line"),              E.Line,                      85);
        TestEqual(TEXT("[0] Column"),            E.Column,                    30);
        TestEqual(TEXT("[0] MissingIdentifier"), E.MissingIdentifier,         FString{TEXT("FBb_CombatReceiver_DamageReceiver_SpawnParams")});
        TestEqual(TEXT("[0] ArgsList"),          E.ArgsList,                  FString{});
    }

    {
        const auto& E = Errors[1];
        TestEqual(TEXT("[1] Kind"),              static_cast<int32>(E.Kind), static_cast<int32>(ECk_AsParsedError_Kind::BareCtorNoMatchingSignatures));
        TestEqual(TEXT("[1] FilePath"),          E.FilePath,                  FString{TEXT("D:/Repos/BusterBlock/Script/ECS/NpcPortal/BB_NpcPortal_Cheats.as")});
        TestEqual(TEXT("[1] MissingIdentifier"), E.MissingIdentifier,         FString{TEXT("FBb_Npc_EntityScript_SpawnParams")});
        TestEqual(TEXT("[1] ArgsList"),          E.ArgsList,                  FString{TEXT("const FTransform")});
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Bare-ctor cascade filter + dedup: 'Unknown' args are dropped; identical
// roots across cascade sites collapse to one.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsErrorParser_BareCtor_CascadeAndDedup,
    "CkAngelscriptGenerator.UnitTests.AsErrorParser.BareCtor_CascadeAndDedup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsErrorParser_BareCtor_CascadeAndDedup::RunTest(const FString&)
{
    const auto Input = FString{TEXT(
        "D:/Repos/BusterBlock/Script/Sample.as:\n"
        "(10:5) No matching signatures to 'FBb_Foo_SpawnParams()'\n"
        "(20:5) No matching signatures to 'FBb_Foo_SpawnParams()'\n"
        "(30:5) No matching signatures to 'SomeHelper(Unknown)'\n")};

    const auto Errors  = FCkAsErrorParser::ParseErrors(Input);
    TestEqual(TEXT("Unknown-args bare ctor filtered"), Errors.Num(), 2);

    const auto Deduped = FCkAsErrorParser::DeduplicateRoots(Errors);
    TestEqual(TEXT("identical bare-ctor roots dedup to one"), Deduped.Num(), 1);
    if (Deduped.Num() < 1) { return false; }

    TestEqual(TEXT("survivor identifier"),
        Deduped[0].MissingIdentifier, FString{TEXT("FBb_Foo_SpawnParams")});

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
