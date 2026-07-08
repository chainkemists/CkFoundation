// Tests for the AS recovery dispatcher's classification / action-plan logic.
// The live OnAngelscriptReloadHadErrors path isn't covered here (engine-state
// side effects); coverage there comes from `_probe_*.bat` smoke runs.

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_Dispatcher.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

#include "CkCore/Macros/CkMacros.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace ck::angelscriptgenerator::self_heal;

namespace
{
    auto Make_NoMatchingSignatures(
        const TCHAR* InNamespace,
        const TCHAR* InFunctionName,
        const TCHAR* InArgsList = TEXT("")) -> FCk_AsParsedError
    {
        auto E = FCk_AsParsedError{};
        E.Kind             = ECk_AsParsedError_Kind::NoMatchingSignatures;
        E.TargetNamespace  = InNamespace;
        E.FunctionName     = InFunctionName;
        E.ArgsList         = InArgsList;
        E.FilePath         = TEXT("D:/Test/Caller.as");
        E.Line             = 10;
        E.Column           = 5;
        return E;
    }

    auto Make_IdentifierNotADataType(
        const TCHAR* InMissingIdentifier,
        const TCHAR* InLookupScope = TEXT(""),
        const TCHAR* InFilePath    = TEXT("D:/Test/Caller.as")) -> FCk_AsParsedError
    {
        auto E = FCk_AsParsedError{};
        E.Kind              = ECk_AsParsedError_Kind::IdentifierNotADataType;
        E.MissingIdentifier = InMissingIdentifier;
        E.LookupScope       = InLookupScope;
        E.FilePath          = InFilePath;
        E.Line              = 10;
        E.Column            = 5;
        return E;
    }

    auto Make_AdjacentStringLiteral() -> FCk_AsParsedError
    {
        auto E      = FCk_AsParsedError{};
        E.Kind      = ECk_AsParsedError_Kind::AdjacentStringLiteral;
        E.FilePath  = TEXT("D:/Test/Caller.as");
        E.Line      = 44;
        E.Column    = 13;
        return E;
    }

    auto Make_BareCtor(
        const TCHAR* InMissingIdentifier,
        const TCHAR* InArgsList = TEXT("")) -> FCk_AsParsedError
    {
        auto E              = FCk_AsParsedError{};
        E.Kind              = ECk_AsParsedError_Kind::BareCtorNoMatchingSignatures;
        E.MissingIdentifier = InMissingIdentifier;
        E.ArgsList          = InArgsList;
        E.FilePath          = TEXT("D:/Test/Caller.as");
        E.Line              = 10;
        E.Column            = 5;
        return E;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Classify: NoMatchingSignatures from a U-prefixed entity-script namespace
// calling Params(...) routes to SynthesizeStub_EntitySpawnParams.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Dispatcher_Classify_EntitySpawnParams,
    "CkAngelscriptGenerator.UnitTests.Dispatcher.Classify_EntitySpawnParams",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Dispatcher_Classify_EntitySpawnParams::RunTest(const FString&)
{
    TestEqual(TEXT("UBb_X::Params(...) -> SynthesizeStub"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_NoMatchingSignatures(TEXT("UBb_DeliveryTruck_EntityScript"), TEXT("Params"), TEXT("const FTransform")))),
        static_cast<int32>(ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams));

    TestEqual(TEXT("UCk_X::Params() -> SynthesizeStub"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_NoMatchingSignatures(TEXT("UCk_Foo_EntityScript"), TEXT("Params")))),
        static_cast<int32>(ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Classify: 'assets::X(...)' routes to KickGenerator_AssetRegistry.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Dispatcher_Classify_AssetRegistry,
    "CkAngelscriptGenerator.UnitTests.Dispatcher.Classify_AssetRegistry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Dispatcher_Classify_AssetRegistry::RunTest(const FString&)
{
    TestEqual(TEXT("assets::MALE_SKEL_NEW() -> KickGenerator_AssetRegistry"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_NoMatchingSignatures(TEXT("assets"), TEXT("MALE_SKEL_NEW")))),
        static_cast<int32>(ECk_RecoveryStrategy::KickGenerator_AssetRegistry));

    TestEqual(TEXT("assets::load::X() -> KickGenerator_AssetRegistry"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_NoMatchingSignatures(TEXT("assets::load"), TEXT("Foo")))),
        static_cast<int32>(ECk_RecoveryStrategy::KickGenerator_AssetRegistry));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Classify: 'FCk_Handle_X' missing -> KickGenerator_DynamicHandle.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Dispatcher_Classify_DynamicHandle,
    "CkAngelscriptGenerator.UnitTests.Dispatcher.Classify_DynamicHandle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Dispatcher_Classify_DynamicHandle::RunTest(const FString&)
{
    TestEqual(TEXT("FCk_Handle_CheckoutCounter (global) -> DynamicHandle"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("FCk_Handle_CheckoutCounter")))),
        static_cast<int32>(ECk_RecoveryStrategy::KickGenerator_DynamicHandle));

    TestEqual(TEXT("FCk_Handle_X (namespaced scope) -> DynamicHandle"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("FCk_Handle_X"), TEXT("bb_checkout_cheats")))),
        static_cast<int32>(ECk_RecoveryStrategy::KickGenerator_DynamicHandle));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Classify: direct-construction shapes of a generated F<X>_SpawnParams route
// to SynthesizeStub_EntitySpawnParams (the source-derived full-shape path) —
// both as a missing declared type (IdentifierNotADataType) and as a bare
// ctor call (BareCtorNoMatchingSignatures). Non-SpawnParams shapes stay
// Unrecognized.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Dispatcher_Classify_SpawnParamsDirectConstruction,
    "CkAngelscriptGenerator.UnitTests.Dispatcher.Classify_SpawnParamsDirectConstruction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Dispatcher_Classify_SpawnParamsDirectConstruction::RunTest(const FString&)
{
    TestEqual(TEXT("F<X>_SpawnParams as missing declared type -> SynthesizeStub"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("FBb_DayCycle_EntityScript_SpawnParams")))),
        static_cast<int32>(ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams));

    TestEqual(TEXT("F<X>_SpawnParams bare ctor (no args) -> SynthesizeStub"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_BareCtor(TEXT("FBb_CombatReceiver_DamageReceiver_SpawnParams")))),
        static_cast<int32>(ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams));

    TestEqual(TEXT("F<X>_SpawnParams bare ctor (typed args) -> SynthesizeStub"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_BareCtor(TEXT("FBb_Npc_EntityScript_SpawnParams"), TEXT("const FTransform")))),
        static_cast<int32>(ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams));

    TestEqual(TEXT("bare ctor on a non-SpawnParams type -> Unrecognized (authoring error)"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_BareCtor(TEXT("FVector"), TEXT("float32")))),
        static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    TestEqual(TEXT("non-F-prefixed *_SpawnParams -> Unrecognized"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_BareCtor(TEXT("Bb_SpawnParams")))),
        static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Classify: AdjacentStringLiteral -> Author_FixupRequired_AdjacentStringLiteral.
// This is a diagnose-only path (no auto-fix), but the strategy is "actionable"
// in the dispatcher sense — it short-circuits the "no recognized roots"
// terminal that wedges headless test runs without a usable diagnostic.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Dispatcher_Classify_AdjacentStringLiteral,
    "CkAngelscriptGenerator.UnitTests.Dispatcher.Classify_AdjacentStringLiteral",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Dispatcher_Classify_AdjacentStringLiteral::RunTest(const FString&)
{
    TestEqual(TEXT("AdjacentStringLiteral -> Author_FixupRequired_AdjacentStringLiteral"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(Make_AdjacentStringLiteral())),
        static_cast<int32>(ECk_RecoveryStrategy::Author_FixupRequired_AdjacentStringLiteral));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Classify: anything that doesn't fit the four above routes to Unrecognized.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Dispatcher_Classify_Unrecognized,
    "CkAngelscriptGenerator.UnitTests.Dispatcher.Classify_Unrecognized",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Dispatcher_Classify_Unrecognized::RunTest(const FString&)
{
    TestEqual(TEXT("NoMatch with non-Params function on U namespace -> Unrecognized"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_NoMatchingSignatures(TEXT("UBb_X_EntityScript"), TEXT("NotParams")))),
        static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    TestEqual(TEXT("NoMatch with non-U / non-assets namespace -> Unrecognized"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_NoMatchingSignatures(TEXT("FCk_Handle"), TEXT("As_CheckoutCounter")))),
        static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    TestEqual(TEXT("Identifier not a data type but not FCk_Handle_ -> Unrecognized"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("FBb_SomeRandomStruct")))),
        static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Classify: a deleted type ('Identifier <X> is not a data type') that is
// neither a dynamic handle nor a generated SpawnParams struct routes to
// Quarantine_StaleEspCanonical when — and ONLY when — the error is located
// inside a generated *_EntitySpawnParams.as canonical. The same error in author
// source, or in a TRACKED generated file, stays Unrecognized: the LOCATION is
// the discriminator, and only gitignored ESP canonicals are delete-safe.
//
// Regression for the 2026-07 stale-canonical boot-blocker: enums EBb_NamedNpc /
// EBb_Employee were unified into EBb_Npc; a leftover local ESP canonical still
// referenced the deleted enums and self-heal declined to act (the identifier
// matched neither FCk_Handle_ nor F<X>_SpawnParams, so Classify returned
// Unrecognized -> terminal banner -> editor wedged at the AS modal).
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Dispatcher_Classify_StaleEspCanonical,
    "CkAngelscriptGenerator.UnitTests.Dispatcher.Classify_StaleEspCanonical",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Dispatcher_Classify_StaleEspCanonical::RunTest(const FString&)
{
    // The real incident: a deleted enum referenced inside the project ESP canonical.
    TestEqual(TEXT("deleted enum inside project ESP canonical -> Quarantine"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("EBb_NamedNpc"), TEXT(""),
                TEXT("D:/Repos/BusterBlock/Script/Generated/BusterBlock_EntitySpawnParams.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::Quarantine_StaleEspCanonical));

    // A plugin ESP canonical (backslashes + a namespaced lookup scope) also qualifies.
    TestEqual(TEXT("deleted enum inside plugin ESP canonical (backslashes) -> Quarantine"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("EBb_Employee"), TEXT("UBb_Npc_EntityScript"),
                TEXT("D:\\Repos\\BusterBlock\\Plugins\\BusterBlockTests\\Script\\Generated\\BusterBlockTests_EntitySpawnParams.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::Quarantine_StaleEspCanonical));

    // Discrimination 1: the identical error in AUTHOR source is a real authoring
    // bug — stays Unrecognized (terminal banner), never quarantines user code.
    TestEqual(TEXT("deleted type in author source -> Unrecognized"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("EBb_NamedNpc"), TEXT(""),
                TEXT("D:/Repos/BusterBlock/Script/Npc/BB_Npc_EntityScript.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    // Discrimination 2: TRACKED generated files (BusterBlockAssets.as,
    // *_AutoTestActors.as) are not *_EntitySpawnParams.as canonicals — deleting
    // them would destroy committed state, so they must never quarantine.
    TestEqual(TEXT("deleted type in tracked generated Assets file -> Unrecognized"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("EBb_NamedNpc"), TEXT(""),
                TEXT("D:/Repos/BusterBlock/Script/Generated/BusterBlockAssets.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    TestEqual(TEXT("deleted type in tracked AutoTestActors file -> Unrecognized"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("EBb_NamedNpc"), TEXT(""),
                TEXT("D:/Repos/BusterBlock/Script/Generated/BusterBlock_AutoTestActors.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    // The new location check is a FALLBACK after the handle / SpawnParams
    // checks — a real handle or SpawnParams identifier still wins even when the
    // error is located inside the canonical (classification order preserved).
    TestEqual(TEXT("FCk_Handle_ inside canonical still -> DynamicHandle (not Quarantine)"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("FCk_Handle_CheckoutCounter"), TEXT(""),
                TEXT("D:/Repos/BusterBlock/Script/Generated/BusterBlock_EntitySpawnParams.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::KickGenerator_DynamicHandle));

    TestEqual(TEXT("F<X>_SpawnParams inside canonical still -> SynthesizeStub (not Quarantine)"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("FBb_Npc_EntityScript_SpawnParams"), TEXT(""),
                TEXT("D:/Repos/BusterBlock/Script/Generated/BusterBlock_EntitySpawnParams.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// BuildActionPlan: one action per deduped root, preserves order, classifies
// each correctly.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Dispatcher_BuildActionPlan,
    "CkAngelscriptGenerator.UnitTests.Dispatcher.BuildActionPlan",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Dispatcher_BuildActionPlan::RunTest(const FString&)
{
    const auto Roots = TArray<FCk_AsParsedError>{
        Make_NoMatchingSignatures(TEXT("UBb_Foo_EntityScript"), TEXT("Params"), TEXT("FTransform")),
        Make_IdentifierNotADataType(TEXT("FCk_Handle_X")),
        Make_NoMatchingSignatures(TEXT("assets"), TEXT("MALE_SKEL_NEW")),
        Make_NoMatchingSignatures(TEXT("FCk_Handle"), TEXT("As_X")),  // -> Unrecognized
    };

    const auto Plan = FCkAsRecoveryDispatcher::BuildActionPlan(Roots);

    TestEqual(TEXT("plan size matches roots"), Plan.Num(), 4);
    if (Plan.Num() < 4) { return false; }

    TestEqual(TEXT("[0] strategy"), static_cast<int32>(Plan[0].Strategy), static_cast<int32>(ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams));
    TestEqual(TEXT("[1] strategy"), static_cast<int32>(Plan[1].Strategy), static_cast<int32>(ECk_RecoveryStrategy::KickGenerator_DynamicHandle));
    TestEqual(TEXT("[2] strategy"), static_cast<int32>(Plan[2].Strategy), static_cast<int32>(ECk_RecoveryStrategy::KickGenerator_AssetRegistry));
    TestEqual(TEXT("[3] strategy"), static_cast<int32>(Plan[3].Strategy), static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    TestEqual(TEXT("[0] error preserved"), Plan[0].Error.TargetNamespace, FString{TEXT("UBb_Foo_EntityScript")});
    TestEqual(TEXT("[1] error preserved"), Plan[1].Error.MissingIdentifier, FString{TEXT("FCk_Handle_X")});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Cycle counter: Reset_CyclesRun + Get_CyclesRun round-trip. The live OnReload
// path also increments this on each applied cycle; tests for that increment
// would need diagnostics injection (deferred to integration tests).
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Dispatcher_CycleCounter_Reset,
    "CkAngelscriptGenerator.UnitTests.Dispatcher.CycleCounter_Reset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Dispatcher_CycleCounter_Reset::RunTest(const FString&)
{
    FCkAsRecoveryDispatcher::Reset_CyclesRun();
    TestEqual(TEXT("post-reset counter is 0"), FCkAsRecoveryDispatcher::Get_CyclesRun(), 0);

    TestEqual(TEXT("MaxCycles is 3 (CTO Rev 10 pushback #2)"),
        FCkAsRecoveryDispatcher::MaxCycles, 3);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Bootstrap-mode flag: Is_BootstrapMode starts true, Mark_BootstrapComplete
// flips it to false. Drives the OnReloadHadErrors routing decision (modal-tick
// pump for cold-start vs FTSTicker for mid-session hot-reload).
//
// NOTE: This test mutates global session state (sBootstrapComplete +
// sCyclesRun) and does not restore it. The dispatcher's bootstrap flag has
// editor-session lifetime by design — once flipped, it stays flipped until
// editor restart. The CycleCounter_Reset test above happens to leave the
// counter at 0, and Reset_CyclesRun is called from StartupModule, so the
// stale-state risk is low. Run order shouldn't matter for the other tests
// since they don't read this flag.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Dispatcher_BootstrapMode_FlipFlop,
    "CkAngelscriptGenerator.UnitTests.Dispatcher.BootstrapMode_FlipFlop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Dispatcher_BootstrapMode_FlipFlop::RunTest(const FString&)
{
    // Test order: if this test runs after another that already called
    // Mark_BootstrapComplete, the initial state will be `bootstrap done`. We
    // only assert the transition, not the initial value, to stay robust to
    // test ordering.
    const auto WasBootstrap = FCkAsRecoveryDispatcher::Is_BootstrapMode();

    FCkAsRecoveryDispatcher::Mark_BootstrapComplete();
    TestFalse(TEXT("after Mark_BootstrapComplete, Is_BootstrapMode returns false"),
        FCkAsRecoveryDispatcher::Is_BootstrapMode());

    TestEqual(TEXT("cycle counter reset to 0 at bootstrap→mid-session transition"),
        FCkAsRecoveryDispatcher::Get_CyclesRun(), 0);

    (void)WasBootstrap;
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
