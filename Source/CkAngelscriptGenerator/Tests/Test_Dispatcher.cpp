// Tests for the AS recovery dispatcher's classification / action-plan logic.
//
// The live OnAngelscriptReloadHadErrors path is not covered here — it pulls
// diagnostics from FAngelscriptManager and applies side-effecting strategies
// to disk. Coverage of that path comes from the end-to-end §8.16 / §8.17
// verifications run from the editor against the corruption probes.

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
        const TCHAR* InLookupScope = TEXT("")) -> FCk_AsParsedError
    {
        auto E = FCk_AsParsedError{};
        E.Kind              = ECk_AsParsedError_Kind::IdentifierNotADataType;
        E.MissingIdentifier = InMissingIdentifier;
        E.LookupScope       = InLookupScope;
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
// Classify: anything that doesn't fit the three above routes to Unrecognized.
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
