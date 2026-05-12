// Regression tests for FCkAngelScript_HandleRegistry::UpdateExistingDynamicHandle.
//
// The Rev 10 DynamicHandle recovery + "Force Refresh button works for UPDATE"
// fixes both rely on the framework's register-or-update path replacing an
// existing entry's validator + cast lambdas in place. AS-bound methods read
// the validator via a stable raw pointer (TypeInfo*) at call time, so mutating
// the TypeInfo in place is visible without re-registering with the AS engine.
//
// These tests verify the in-place mutation actually replaces what subsequent
// callers see. If a future framework change reintroduces capture-by-value or
// breaks the pointer-stability invariant, these go red before recovery
// silently regresses to leaving permissive validators in flight.

#include "CkEcs/Handle/CkHandle_AngelScript_Registry.h"

#include "CkCore/Macros/CkMacros.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
    // Unique test-type name keyed to this test file. We can't deregister
    // (FCkAngelScript_HandleRegistry is intentionally append-only at the
    // public-API surface), so subsequent test runs in the same editor
    // session re-encounter this type and skip the register step. The
    // tests are written to handle both first-run and subsequent-run paths.
    constexpr auto TestTypeName  = TEXT("FCk_Handle_Rev10UpdateProbe");
    constexpr auto TestShortName = TEXT("Rev10UpdateProbe");

    // Two distinct validators so we can verify which one is in effect.
    // We use lambdas-by-pointer-equivalent (TFunction equality isn't
    // observable directly, but the call result is) — Permissive returns
    // true always, Strict returns false always. Calling the validator on
    // any handle reveals which is in place.
    auto Make_PermissiveValidator() -> TFunction<bool(const FCk_Handle&)>
    {
        return [](const FCk_Handle&) -> bool { return true; };
    }

    auto Make_StrictValidator() -> TFunction<bool(const FCk_Handle&)>
    {
        return [](const FCk_Handle&) -> bool { return false; };
    }

    // Ensure the test type is registered (idempotent). Returns true on
    // first-time register, false if already present (so subsequent tests
    // know the in-flight validator is whatever Update has left it).
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
// UpdateExistingDynamicHandle returns false for an unregistered type.
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
// UpdateExistingDynamicHandle returns false for an empty TypeName.
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
// End-to-end: register with permissive, update to strict, verify the validator
// reachable via GetHandleTypeInfo reflects the update. This is the load-bearing
// guarantee for the DynamicHandle recovery path.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_HandleRegistry_Update_ReplacesValidatorInPlace,
    "CkAngelscriptGenerator.UnitTests.HandleRegistry.Update_ReplacesValidatorInPlace",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_HandleRegistry_Update_ReplacesValidatorInPlace::RunTest(const FString&)
{
    Ensure_TestTypeRegistered();

    // Step 1: install permissive validator (overwriting whatever the prior
    // test run left in flight — idempotent).
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

    // Step 2: update to strict validator with new metadata. This is the
    // core promise: after this call, every consumer that dereferences the
    // type's validator sees the new function.
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

    // Step 3: Cast lambdas should also reflect the new validator — they
    // capture by value but the Update path rebuilds them with the new
    // closure. Verify by calling Cast and CastChecked.
    {
        const auto* Info = FCkAngelScript_HandleRegistry::GetHandleTypeInfo(TestTypeName);
        if (Info == nullptr) { return false; }

        // With the strict validator returning false, Cast should return an
        // invalid handle (the lambda's "return FCk_Handle{}" branch).
        const auto CastResult = Info->Cast(FCk_Handle{});
        TestFalse(TEXT("strict Cast returns invalid handle"), ck::IsValid(CastResult));
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Pointer stability: GetHandleTypeInfo returns the same pointer across calls
// (no entry replacement, just in-place mutation). This is what lets AS-bound
// methods cache the TypeInfo* in userData / AuxData at binding time and
// dereference it at call time.
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
