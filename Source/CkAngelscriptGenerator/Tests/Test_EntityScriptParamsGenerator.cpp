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
    // The entity-script base is UCLASS(Abstract) — its CDO is a fully initialized reflected
    // object, which is all property injection needs, and no instance can be manufactured.
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

    const auto Overrides = UCk_Utils_Reflection_UE::Get_StructFieldOverrides(ParamsProp, Host);

    if (NOT TestEqual(TEXT("exactly one override (Offset)"), Overrides.Num(), 1))
    { return false; }

    TestEqual(TEXT("dotted path is 'Offset'"),
        Overrides[0]._DottedFieldPath, FString{TEXT("Offset")});

    const auto Expr = UCk_Utils_Reflection_UE::Get_AngelscriptDefaultExpression(Overrides[0]._Literal);
    TestFalse(TEXT("override literal is non-empty AS expression"), Expr.IsEmpty());
    // Content, not spelling: the exact literal format is owned by Get_PropertyDefaultValueLiteral.
    TestTrue(TEXT("literal references FVector"), Expr.Contains(TEXT("FVector")));
    TestTrue(TEXT("literal contains the X coord value"),  Expr.Contains(TEXT("10")));

    return true;
}

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

    // Synthetic `<X>_BP_C` shape: the filter must reject it BEFORE any flag/AS-source checks,
    // and it is auto-named so a second run in the same process can't collide.
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
