// Classification / action-plan logic only. The live OnAngelscriptReloadHadErrors
// path has engine-state side effects; its coverage is the `_probe_*.bat` smoke runs.

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

    auto Make_NotAMemberOfStruct(
        const TCHAR* InMissingMember,
        const TCHAR* InOwningStruct,
        const TCHAR* InFilePath = TEXT("D:/Test/Caller.as")) -> FCk_AsParsedError
    {
        auto E              = FCk_AsParsedError{};
        E.Kind              = ECk_AsParsedError_Kind::NotAMemberOfStruct;
        E.MissingIdentifier = InMissingMember;
        E.LookupScope       = InOwningStruct;
        E.FilePath          = InFilePath;
        E.Line              = 6948;
        E.Column            = 15;
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
// LOCATION is the discriminator for a deleted type that is neither a dynamic
// handle nor a SpawnParams struct: only gitignored *_EntitySpawnParams.as
// canonicals are delete-safe, so only they quarantine.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Dispatcher_Classify_StaleEspCanonical,
    "CkAngelscriptGenerator.UnitTests.Dispatcher.Classify_StaleEspCanonical",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Dispatcher_Classify_StaleEspCanonical::RunTest(const FString&)
{
    TestEqual(TEXT("deleted enum inside project ESP canonical -> Quarantine"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("EBb_NamedNpc"), TEXT(""),
                TEXT("D:/Repos/BusterBlock/Script/Generated/BusterBlock_EntitySpawnParams.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::Quarantine_StaleEspCanonical));

    TestEqual(TEXT("deleted enum inside plugin ESP canonical (backslashes) -> Quarantine"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("EBb_Employee"), TEXT("UBb_Npc_EntityScript"),
                TEXT("D:\\Repos\\BusterBlock\\Plugins\\BusterBlockTests\\Script\\Generated\\BusterBlockTests_EntitySpawnParams.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::Quarantine_StaleEspCanonical));

    // The identical error in AUTHOR source is a real authoring bug — never
    // quarantine user code.
    TestEqual(TEXT("deleted type in author source -> Unrecognized"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("EBb_NamedNpc"), TEXT(""),
                TEXT("D:/Repos/BusterBlock/Script/Npc/BB_Npc_EntityScript.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    // Tracked generated files are not ESP canonicals — deleting them would
    // destroy committed state.
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

    // A sibling recovery stub carries the canonical suffix but is our OWN output.
    // Pinned for THIS kind too: the guard lives in the shared path predicate, so
    // moving it into either Classify arm alone must fail here.
    TestEqual(TEXT("deleted type in _StubRecovery_ sibling -> Unrecognized"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_IdentifierNotADataType(TEXT("EBb_NamedNpc"), TEXT(""),
                TEXT("D:/Repos/BusterBlock/Script/Generated/_StubRecovery_BusterBlock_EntitySpawnParams.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    // The location check is a FALLBACK after the handle / SpawnParams checks.
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
// Same LOCATION discriminator for a deleted FIELD (the 2026-08-07 OpenSign wedge):
// `LocalRotationOffset` was dropped from the params struct while machine-local
// canonicals kept assigning it, and the boot had no recognized root cause.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Dispatcher_Classify_StaleEspCanonical_DeletedField,
    "CkAngelscriptGenerator.UnitTests.Dispatcher.Classify_StaleEspCanonical_DeletedField",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Dispatcher_Classify_StaleEspCanonical_DeletedField::RunTest(const FString&)
{
    TestEqual(TEXT("deleted field inside project ESP canonical -> Quarantine"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_NotAMemberOfStruct(TEXT("LocalRotationOffset"), TEXT("FBb_Fragment_OpenSign_ParamsData"),
                TEXT("D:/Repos/BusterBlock/Script/Generated/BusterBlock_EntitySpawnParams.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::Quarantine_StaleEspCanonical));

    TestEqual(TEXT("deleted field inside plugin ESP canonical (backslashes) -> Quarantine"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_NotAMemberOfStruct(TEXT("LocalRotationOffset"), TEXT("FBb_Fragment_OpenSign_ParamsData"),
                TEXT("D:\\Repos\\BusterBlock\\Plugins\\BusterBlockTests\\Script\\Generated\\BusterBlockTests_EntitySpawnParams.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::Quarantine_StaleEspCanonical));

    // The identical error in AUTHOR source is a real authoring bug.
    TestEqual(TEXT("deleted field in author source -> Unrecognized"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_NotAMemberOfStruct(TEXT("LocalRotationOffset"), TEXT("FBb_Fragment_OpenSign_ParamsData"),
                TEXT("D:/Repos/BusterBlock/Script/ECS/OpenSign/BB_OpenSign_EntityScript.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    // Tracked generated files are never delete-safe.
    TestEqual(TEXT("deleted field in tracked generated Assets file -> Unrecognized"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_NotAMemberOfStruct(TEXT("DeadField"), TEXT("FBb_Fragment_Foo_ParamsData"),
                TEXT("D:/Repos/BusterBlock/Script/Generated/BusterBlockAssets.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    // A sibling recovery stub carries the canonical suffix but is our OWN output —
    // quarantining it would derive a doubly-prefixed sibling from a stub.
    TestEqual(TEXT("deleted field in _StubRecovery_ sibling -> Unrecognized"),
        static_cast<int32>(FCkAsRecoveryDispatcher::Classify(
            Make_NotAMemberOfStruct(TEXT("DeadField"), TEXT("FBb_Fragment_Foo_ParamsData"),
                TEXT("D:/Repos/BusterBlock/Script/Generated/_StubRecovery_BusterBlock_EntitySpawnParams.as")))),
        static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    return true;
}

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
    if (Plan.Num() < 4)
    { return false; }

    TestEqual(TEXT("[0] strategy"), static_cast<int32>(Plan[0].Strategy), static_cast<int32>(ECk_RecoveryStrategy::SynthesizeStub_EntitySpawnParams));
    TestEqual(TEXT("[1] strategy"), static_cast<int32>(Plan[1].Strategy), static_cast<int32>(ECk_RecoveryStrategy::KickGenerator_DynamicHandle));
    TestEqual(TEXT("[2] strategy"), static_cast<int32>(Plan[2].Strategy), static_cast<int32>(ECk_RecoveryStrategy::KickGenerator_AssetRegistry));
    TestEqual(TEXT("[3] strategy"), static_cast<int32>(Plan[3].Strategy), static_cast<int32>(ECk_RecoveryStrategy::Unrecognized));

    TestEqual(TEXT("[0] error preserved"), Plan[0].Error.TargetNamespace, FString{TEXT("UBb_Foo_EntityScript")});
    TestEqual(TEXT("[1] error preserved"), Plan[1].Error.MissingIdentifier, FString{TEXT("FCk_Handle_X")});

    return true;
}

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
// NOTE: mutates global session state (sBootstrapComplete + sCyclesRun) and does
// NOT restore it — the bootstrap flag has editor-session lifetime by design.
// Safe only while no other test reads it.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Dispatcher_BootstrapMode_FlipFlop,
    "CkAngelscriptGenerator.UnitTests.Dispatcher.BootstrapMode_FlipFlop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Dispatcher_BootstrapMode_FlipFlop::RunTest(const FString&)
{
    // Only the transition is asserted, not the initial value — another test may
    // already have called Mark_BootstrapComplete.
    const auto WasBootstrap = FCkAsRecoveryDispatcher::Is_BootstrapMode();

    FCkAsRecoveryDispatcher::Mark_BootstrapComplete();
    TestFalse(TEXT("after Mark_BootstrapComplete, Is_BootstrapMode returns false"),
        FCkAsRecoveryDispatcher::Is_BootstrapMode());

    TestEqual(TEXT("cycle counter reset to 0 at bootstrap->mid-session transition"),
        FCkAsRecoveryDispatcher::Get_CyclesRun(), 0);

    (void)WasBootstrap;
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
