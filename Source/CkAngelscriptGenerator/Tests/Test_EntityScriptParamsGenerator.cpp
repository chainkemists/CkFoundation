// Regression test for Fix #2 — codegen-bug-positional-ctor-null-uobject.
//
// Bug: when the EntityScript spawn-params generator emits a struct-typed Params field whose
// CDO differs from struct default, it produces a positional-ctor expression as the field
// initializer (`<Type> Params = <Type>(arg1, ..., nullptr);`). For any struct containing a
// `UObject*` field, AS overload resolution rejects the bare nullptr arg with `<null handle>`
// and the canonical becomes unparseable.
//
// Fix #2 reshapes the emission for the trigger case: the field is declared without an inline
// initializer and per-field overrides go into the SpawnParams default ctor body, where the
// assignment is in *statement* context — the LHS slot types the RHS, so bare `nullptr` works
// for UObject* fields like the in-struct field default does today.
//
// This test pins the two helpers that drive the emission switch:
//   - Has_UObjectPointerField (the gate)
//   - Get_StructFieldOverrides (the override list)

#include "CkAngelscriptGenerator/Tests/Test_EntityScriptParamsGenerator_Fixtures.h"

#include "CkAngelscriptGenerator/CkAngelscriptEntityScriptParamsGenerator.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_SharedUtils.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"

#include "Engine/BlueprintGeneratedClass.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
    auto FindStructProp(
        const UClass* InOwner,
        const TCHAR*  InPropName) -> const FStructProperty*
    {
        return CastField<FStructProperty>(InOwner->FindPropertyByName(FName{InPropName}));
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Get_DetailedPropertyType: weak/soft UObject wrappers are lifetime semantics,
// not presentation detail. They must survive reflection-driven regeneration of
// EntitySpawnParams fields and both Params() signatures.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ParamsGen_PreservesWeakAndSoftObjectWrappers,
    "CkAngelscriptGenerator.UnitTests.ParamsGenerator.PreservesWeakAndSoftObjectWrappers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ParamsGen_PreservesWeakAndSoftObjectWrappers::RunTest(const FString&)
{
    const auto* HostClass = UCkTest_ParamsGenerator_Host::StaticClass();

    auto CheckPropertyType = [this, HostClass](
        const TCHAR* InPropertyName,
        const FString& InExpectedType,
        const FFieldClass* InExpectedPropertyClass) -> bool
    {
        auto* Property = HostClass->FindPropertyByName(FName{InPropertyName});
        if (NOT TestNotNull(
                *FString::Printf(TEXT("%s property reflected"), InPropertyName),
                Property))
        { return false; }

        TestTrue(
            *FString::Printf(TEXT("%s retains reflected property kind"), InPropertyName),
            Property->IsA(InExpectedPropertyClass));
        TestEqual(
            *FString::Printf(TEXT("%s retains AngelScript wrapper type"), InPropertyName),
            FCkAngelscriptGenerator_SharedUtils::Get_DetailedPropertyType(Property),
            InExpectedType);
        return true;
    };

    auto Result = true;
    auto* StrongProperty = HostClass->FindPropertyByName(TEXT("StrongSound"));
    Result &= TestNotNull(TEXT("StrongSound property reflected"), StrongProperty);
    if (StrongProperty != nullptr)
    {
        Result &= TestEqual(
            TEXT("Params call keeps the source strong UObject type"),
            FCkAngelscriptGenerator_SharedUtils::Get_DetailedPropertyType(StrongProperty),
            FString{TEXT("TObjectPtr<USoundBase>")});
        Result &= TestEqual(
            TEXT("EntitySpawnParams weakens direct strong UObject retention"),
            FCkAngelscriptEntityScriptParamsGenerator::Get_RetainedPropertyType(StrongProperty),
            FString{TEXT("TWeakObjectPtr<USoundBase>")});
    }
    Result &= CheckPropertyType(
        TEXT("WeakSound"),
        TEXT("TWeakObjectPtr<USoundBase>"),
        FWeakObjectProperty::StaticClass());
    Result &= CheckPropertyType(
        TEXT("SoftSound"),
        TEXT("TSoftObjectPtr<USoundBase>"),
        FSoftObjectProperty::StaticClass());
    Result &= CheckPropertyType(
        TEXT("SoftSoundClass"),
        TEXT("TSoftClassPtr<USoundBase>"),
        FSoftClassProperty::StaticClass());
    Result &= CheckPropertyType(
        TEXT("WeakSounds"),
        TEXT("TArray<TWeakObjectPtr<USoundBase>>"),
        FArrayProperty::StaticClass());
    return Result;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ParamsGen_InjectsWeakMirrorIntoStrongOwner,
    "CkAngelscriptGenerator.UnitTests.ParamsGenerator.InjectsWeakMirrorIntoStrongOwner",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ParamsGen_InjectsWeakMirrorIntoStrongOwner::RunTest(const FString&)
{
    // The production base is intentionally UCLASS(Abstract). Its CDO is still a fully initialized reflected object,
    // which is all this property-injection test needs; avoid manufacturing an instance of an abstract fixture.
    auto* Target = GetMutableDefault<UCkTest_ParamsGenerator_WeakInjectionTarget>();
    auto* Value = NewObject<UCkTest_ParamsGenerator_Host>(GetTransientPackage());
    if (NOT TestNotNull(TEXT("target allocated"), Target)
        || NOT TestNotNull(TEXT("value allocated"), Value))
    { return false; }

    const auto PreviousValue = Target->InjectedObject;
    Target->InjectedObject = nullptr;
    ON_SCOPE_EXIT
    {
        Target->InjectedObject = PreviousValue;
    };

    auto Params = FCkTest_ParamsGenerator_WeakInjectionParams{};
    Params.InjectedObject = Value;
    UCk_Utils_EntityScript_UE::TryInjectEntityScriptSpawnParams(Target, FInstancedStruct::Make(Params));

    TestTrue(TEXT("weak retained identity resolves into the traced strong owner"), Target->InjectedObject.Get() == Value);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Has_UObjectPointerField: returns true for structs containing a UObject*/interface field
// (recursively), false for pure POD structs.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ParamsGen_Has_UObjectPointerField,
    "CkAngelscriptGenerator.UnitTests.ParamsGenerator.Has_UObjectPointerField",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ParamsGen_Has_UObjectPointerField::RunTest(const FString&)
{
    TestTrue(TEXT("struct with TObjectPtr<USoundBase> -> true"),
        UCk_Utils_Reflection_UE::Has_UObjectPointerField(
            FCkTest_ParamsGenerator_MixedFields::StaticStruct()));

    TestFalse(TEXT("POD-only struct -> false"),
        UCk_Utils_Reflection_UE::Has_UObjectPointerField(
            FCkTest_ParamsGenerator_PodOnly::StaticStruct()));

    TestFalse(TEXT("null UScriptStruct -> false (no crash)"),
        UCk_Utils_Reflection_UE::Has_UObjectPointerField(nullptr));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Get_StructFieldOverrides: returns empty when struct is fully at default; returns a
// dotted-path entry per leaf field that differs from InitializeStruct default.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ParamsGen_Get_StructFieldOverrides_AtDefault,
    "CkAngelscriptGenerator.UnitTests.ParamsGenerator.Get_StructFieldOverrides_AtDefault",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ParamsGen_Get_StructFieldOverrides_AtDefault::RunTest(const FString&)
{
    const auto* HostClass = UCkTest_ParamsGenerator_Host::StaticClass();
    const auto* ParamsProp = FindStructProp(HostClass, TEXT("Params"));
    if (NOT TestNotNull(TEXT("Params property reflected"), ParamsProp))
    { return false; }

    const auto* CDO = HostClass->GetDefaultObject();
    if (NOT TestNotNull(TEXT("CDO available"), CDO))
    { return false; }

    const auto Overrides = UCk_Utils_Reflection_UE::Get_StructFieldOverrides(ParamsProp, CDO);

    TestEqual(TEXT("no overrides on a CDO matching struct default"), Overrides.Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ParamsGen_Get_StructFieldOverrides_PodDiff,
    "CkAngelscriptGenerator.UnitTests.ParamsGenerator.Get_StructFieldOverrides_PodDiff",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ParamsGen_Get_StructFieldOverrides_PodDiff::RunTest(const FString&)
{
    const auto* HostClass = UCkTest_ParamsGenerator_Host::StaticClass();
    const auto* ParamsProp = FindStructProp(HostClass, TEXT("Params"));
    if (NOT TestNotNull(TEXT("Params property reflected"), ParamsProp))
    { return false; }

    // NewObject (not CDO) so we can mutate without affecting other tests.
    auto* Host = NewObject<UCkTest_ParamsGenerator_Host>();
    if (NOT TestNotNull(TEXT("host instance allocated"), Host))
    { return false; }

    Host->Params.Offset = FVector{10.0, 0.0, 0.0};
    // Sound stays at default nullptr; Flag/Rotation stay at struct default.

    const auto Overrides = UCk_Utils_Reflection_UE::Get_StructFieldOverrides(ParamsProp, Host);

    if (NOT TestEqual(TEXT("exactly one override (Offset)"), Overrides.Num(), 1))
    { return false; }

    TestEqual(TEXT("dotted path is 'Offset'"),
        Overrides[0]._DottedFieldPath, FString{TEXT("Offset")});

    const auto Expr = UCk_Utils_Reflection_UE::Get_AngelscriptDefaultExpression(Overrides[0]._Literal);
    TestFalse(TEXT("override literal is non-empty AS expression"), Expr.IsEmpty());
    // Sanity: the literal should reference the FVector form. We don't pin the exact format
    // (FVector(10.0, 0.0, 0.0) or similar) because Get_PropertyDefaultValueLiteral controls
    // it — but it should mention FVector and 10.
    TestTrue(TEXT("literal references FVector"), Expr.Contains(TEXT("FVector")));
    TestTrue(TEXT("literal contains the X coord value"),  Expr.Contains(TEXT("10")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Is_IncludedEntityScriptClass: Blueprint-generated entity-script classes are EXCLUDED.
//
// Regression pin for the 2026-06-12 two-instance hot-reload ping-pong: BP classes exist only in
// processes that happen to have the BP loaded, so emitting them makes the generated file's
// content depend on per-process load state — two editors of the same project then rewrite the
// file back and forth forever. The original exclusion (e55fe07df) used
// `InClass->IsChildOf(UBlueprintGeneratedClass)` — a wrong-level check (a BP class is an
// INSTANCE of UBlueprintGeneratedClass, never a subclass of it) that never fired.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ParamsGen_ClassFilter_ExcludesBlueprintGeneratedClasses,
    "CkAngelscriptGenerator.UnitTests.ParamsGenerator.ClassFilter_ExcludesBlueprintGeneratedClasses",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ParamsGen_ClassFilter_ExcludesBlueprintGeneratedClasses::RunTest(const FString&)
{
    TestTrue(TEXT("concrete native entity-script subclass -> included"),
        FCkAngelscriptEntityScriptParamsGenerator::Is_IncludedEntityScriptClass(
            UCkTest_ParamsGenerator_NativeEntityScript::StaticClass()));

    TestFalse(TEXT("UCk_EntityScript_UE base itself -> excluded"),
        FCkAngelscriptEntityScriptParamsGenerator::Is_IncludedEntityScriptClass(
            UCk_EntityScript_UE::StaticClass()));

    TestFalse(TEXT("non-entity-script class -> excluded"),
        FCkAngelscriptEntityScriptParamsGenerator::Is_IncludedEntityScriptClass(
            UCkTest_ParamsGenerator_Host::StaticClass()));

    // Synthetic UBlueprintGeneratedClass parented to an entity script — the shape of every
    // `<X>_BP_C` class. The filter must reject it BEFORE any flag/AS-source checks. (Auto-named
    // so a second run in the same process can't collide.)
    auto* Bpgc = NewObject<UBlueprintGeneratedClass>(GetTransientPackage());
    if (NOT TestNotNull(TEXT("synthetic BPGC allocated"), Bpgc))
    { return false; }

    Bpgc->SetSuperStruct(UCkTest_ParamsGenerator_NativeEntityScript::StaticClass());

    TestTrue(TEXT("sanity: synthetic BPGC IS an entity-script subclass"),
        Bpgc->IsChildOf(UCk_EntityScript_UE::StaticClass()));

    TestFalse(TEXT("Blueprint-generated entity-script class -> excluded"),
        FCkAngelscriptEntityScriptParamsGenerator::Is_IncludedEntityScriptClass(Bpgc));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
