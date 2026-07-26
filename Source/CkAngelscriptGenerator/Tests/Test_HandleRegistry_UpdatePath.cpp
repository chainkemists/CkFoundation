#include "CkEcs/Handle/CkHandle_AngelScript_Registry.h"

#include "CkCore/Macros/CkMacros.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
    // The registry is intentionally append-only at its public surface — there is no
    // deregister — so a second run in the same editor session re-encounters this type
    // and skips the register step. Every test below handles both paths.
    constexpr auto TestTypeName  = TEXT("FCk_Handle_Rev10UpdateProbe");
    constexpr auto TestShortName = TEXT("Rev10UpdateProbe");

    // TFunction equality isn't observable — the call result is what identifies
    // which validator is currently in place.
    auto Make_PermissiveValidator() -> TFunction<bool(const FCk_Handle&)>
    {
        return [](const FCk_Handle&) -> bool { return true; };
    }

    auto Make_StrictValidator() -> TFunction<bool(const FCk_Handle&)>
    {
        return [](const FCk_Handle&) -> bool { return false; };
    }

    // Idempotent. Returns true only on a first-time register — false means the
    // in-flight validator is whatever a prior Update left behind.
    auto Ensure_TestTypeRegistered() -> bool
    {
        if (FCkAngelScript_HandleRegistry::IsHandleTypeRegistered(TestTypeName))
        { return false; }

        FCkAngelScript_HandleRegistry::RegisterDynamicHandle(
            TestTypeName,
            TestShortName,
            Make_PermissiveValidator(),
            TArray<FString>{},
            TEXT("Rev10 update-path regression test"),
            TEXT("/Script/Test.Rev10UpdateProbe"));

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_HandleRegistry_Update_RejectsUnregistered,
    "CkAngelscriptGenerator.UnitTests.HandleRegistry.Update_RejectsUnregistered",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_HandleRegistry_Update_RejectsUnregistered::RunTest(const FString&)
{
    const auto Updated = FCkAngelScript_HandleRegistry::UpdateExistingDynamicHandle(
        TEXT("FCk_Handle_NeverRegistered_DefinitelyNotReal"),
        Make_StrictValidator(),
        TArray<FString>{},
        TEXT(""),
        TEXT(""));

    TestFalse(TEXT("unregistered type -> false"), Updated);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_HandleRegistry_Update_RejectsEmptyName,
    "CkAngelscriptGenerator.UnitTests.HandleRegistry.Update_RejectsEmptyName",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_HandleRegistry_Update_RejectsEmptyName::RunTest(const FString&)
{
    const auto Updated = FCkAngelScript_HandleRegistry::UpdateExistingDynamicHandle(
        FString{},
        Make_StrictValidator(),
        TArray<FString>{},
        TEXT(""),
        TEXT(""));

    TestFalse(TEXT("empty TypeName -> false"), Updated);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_HandleRegistry_Update_ReplacesValidatorInPlace,
    "CkAngelscriptGenerator.UnitTests.HandleRegistry.Update_ReplacesValidatorInPlace",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_HandleRegistry_Update_ReplacesValidatorInPlace::RunTest(const FString&)
{
    Ensure_TestTypeRegistered();

    {
        const auto UpdatedToPermissive = FCkAngelScript_HandleRegistry::UpdateExistingDynamicHandle(
            TestTypeName,
            Make_PermissiveValidator(),
            TArray<FString>{},
            TEXT("permissive state"),
            TEXT("/Script/Test.Rev10UpdateProbe"));
        TestTrue(TEXT("update to permissive succeeded"), UpdatedToPermissive);

        const auto* Info = FCkAngelScript_HandleRegistry::GetHandleTypeInfo(TestTypeName);
        TestNotNull(TEXT("type still registered after permissive update"), Info);
        if (Info == nullptr) { return false; }
        TestTrue(TEXT("permissive validator returns true"), Info->IsValidAsType(FCk_Handle{}));
        TestEqual(TEXT("RequiredFragments is empty"), Info->RequiredFragments.Num(), 0);
        TestEqual(TEXT("Description == 'permissive state'"), Info->Description, FString{TEXT("permissive state")});
    }

    {
        const auto UpdatedToStrict = FCkAngelScript_HandleRegistry::UpdateExistingDynamicHandle(
            TestTypeName,
            Make_StrictValidator(),
            TArray<FString>{TEXT("test_fragment_1"), TEXT("test_fragment_2")},
            TEXT("strict state"),
            TEXT("/Script/Test.Rev10UpdateProbe.Updated"));
        TestTrue(TEXT("update to strict succeeded"), UpdatedToStrict);

        const auto* Info = FCkAngelScript_HandleRegistry::GetHandleTypeInfo(TestTypeName);
        TestNotNull(TEXT("type still registered after strict update"), Info);
        if (Info == nullptr) { return false; }
        TestFalse(TEXT("strict validator returns false"), Info->IsValidAsType(FCk_Handle{}));
        TestEqual(TEXT("RequiredFragments has 2 entries"), Info->RequiredFragments.Num(), 2);
        TestEqual(TEXT("RequiredFragments[0]"), Info->RequiredFragments[0], FString{TEXT("test_fragment_1")});
        TestEqual(TEXT("RequiredFragments[1]"), Info->RequiredFragments[1], FString{TEXT("test_fragment_2")});
        TestEqual(TEXT("Description == 'strict state'"), Info->Description, FString{TEXT("strict state")});
        TestEqual(TEXT("SourceAsset updated"), Info->SourceAsset, FString{TEXT("/Script/Test.Rev10UpdateProbe.Updated")});
    }

    // The Cast lambdas capture the validator by value; Update rebuilds them with the new closure.
    {
        const auto* Info = FCkAngelScript_HandleRegistry::GetHandleTypeInfo(TestTypeName);
        if (Info == nullptr) { return false; }

        const auto CastResult = Info->Cast(FCk_Handle{});
        TestFalse(TEXT("strict Cast returns invalid handle"), ck::IsValid(CastResult));
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// AS-bound methods cache the TypeInfo* at binding time and dereference it at call time, so
// Update must mutate the entry in place rather than replace it.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_HandleRegistry_Update_PointerStability,
    "CkAngelscriptGenerator.UnitTests.HandleRegistry.Update_PointerStability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_HandleRegistry_Update_PointerStability::RunTest(const FString&)
{
    Ensure_TestTypeRegistered();

    const auto* BeforeUpdate = FCkAngelScript_HandleRegistry::GetHandleTypeInfo(TestTypeName);
    TestNotNull(TEXT("type info before update"), BeforeUpdate);
    if (BeforeUpdate == nullptr) { return false; }

    FCkAngelScript_HandleRegistry::UpdateExistingDynamicHandle(
        TestTypeName,
        Make_StrictValidator(),
        TArray<FString>{TEXT("pointer_stability_test")},
        TEXT("ptr stability test"),
        TEXT("/Script/Test.PtrStability"));

    const auto* AfterUpdate = FCkAngelScript_HandleRegistry::GetHandleTypeInfo(TestTypeName);
    TestNotNull(TEXT("type info after update"), AfterUpdate);
    if (AfterUpdate == nullptr) { return false; }

    TestEqual(TEXT("TypeInfo pointer is stable across Update"),
        reinterpret_cast<UPTRINT>(BeforeUpdate),
        reinterpret_cast<UPTRINT>(AfterUpdate));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
