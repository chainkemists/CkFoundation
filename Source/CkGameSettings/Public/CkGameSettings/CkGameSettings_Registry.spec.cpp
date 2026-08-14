#include "CkGameSettings/CkGameSettings_Common.h"
#include "CkGameSettings/Subsystem/CkGameSettings_Subsystem.h"

#include "CkGameSettings_SpecSupport.h"

#include "Misc/AutomationTest.h"

#include <Engine/Engine.h>
#include <Engine/GameInstance.h>

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

namespace ck_game_settings_registry_spec
{
    auto Make_Subsystem() -> UCk_GameSettings_Subsystem_UE*
    {
        auto* GameInstance = NewObject<UGameInstance>(GEngine);
        return NewObject<UCk_GameSettings_Subsystem_UE>(GameInstance);
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GameSettings_Registry_RegisterAndQuery,
    "Ck.CkGameSettings.Registry.RegisterAndQuery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GameSettings_Registry_RegisterAndQuery::RunTest(const FString&)
{
    auto* Subsystem = ck_game_settings_registry_spec::Make_Subsystem();

    TestTrue(TEXT("register Bool"), Subsystem->Request_RegisterSetting(
        FCk_GameSettings_SettingDefinition{TEXT("test.bool"), ECk_GameSettings_ValueType::Bool, TEXT("true")}));
    TestTrue(TEXT("register Int32"), Subsystem->Request_RegisterSetting(
        FCk_GameSettings_SettingDefinition{TEXT("test.int"), ECk_GameSettings_ValueType::Int32, TEXT("5")}));
    TestTrue(TEXT("register Float"), Subsystem->Request_RegisterSetting(
        FCk_GameSettings_SettingDefinition{TEXT("test.float"), ECk_GameSettings_ValueType::Float, TEXT("0.5")}));
    TestTrue(TEXT("register String"), Subsystem->Request_RegisterSetting(
        FCk_GameSettings_SettingDefinition{TEXT("test.string"), ECk_GameSettings_ValueType::String, TEXT("hello")}));

    TestTrue(TEXT("registered key is registered"), Subsystem->Get_IsSettingRegistered(TEXT("test.bool")));
    TestFalse(TEXT("unknown key is not registered"), Subsystem->Get_IsSettingRegistered(TEXT("test.missing")));

    TestTrue(TEXT("Bool default readable"), Subsystem->Get_SettingValue_Bool(TEXT("test.bool"), false));
    TestEqual(TEXT("Int32 default readable"), Subsystem->Get_SettingValue_Int32(TEXT("test.int"), 0), 5);
    TestEqual(TEXT("Float default readable"), Subsystem->Get_SettingValue_Float(TEXT("test.float"), 0.0f), 0.5f);
    TestEqual(TEXT("String default readable"), Subsystem->Get_SettingValue_String(TEXT("test.string")), TEXT("hello"));

    const auto AllKeys = Subsystem->Get_AllSettingKeys();
    TestEqual(TEXT("four keys registered"), AllKeys.Num(), 4);
    if (AllKeys.Num() == 4)
    {
        TestEqual(TEXT("registration order preserved [0]"), AllKeys[0], FName{TEXT("test.bool")});
        TestEqual(TEXT("registration order preserved [1]"), AllKeys[1], FName{TEXT("test.int")});
        TestEqual(TEXT("registration order preserved [2]"), AllKeys[2], FName{TEXT("test.float")});
        TestEqual(TEXT("registration order preserved [3]"), AllKeys[3], FName{TEXT("test.string")});
    }

    auto Definition = FCk_GameSettings_SettingDefinition{};
    TestTrue(TEXT("definition retrievable"), Subsystem->Get_SettingDefinition(TEXT("test.int"), Definition));
    TestEqual(TEXT("retrieved definition has the right value type"), Definition.Get_ValueType(), ECk_GameSettings_ValueType::Int32);
    TestFalse(TEXT("unknown definition not retrievable"), Subsystem->Get_SettingDefinition(TEXT("test.missing"), Definition));

    TestEqual(TEXT("empty category query matches every registered setting"),
        Subsystem->Get_SettingKeysByCategory(FGameplayTagQuery{}).Num(), 4);

    const auto BatchDefinitions = TArray<FCk_GameSettings_SettingDefinition>
    {
        FCk_GameSettings_SettingDefinition{TEXT("batch.a"), ECk_GameSettings_ValueType::Bool, TEXT("false")},
        FCk_GameSettings_SettingDefinition{TEXT("batch.b"), ECk_GameSettings_ValueType::Int32, TEXT("0")},
    };
    TestTrue(TEXT("valid batch registers"), Subsystem->Request_RegisterSettings(BatchDefinitions));
    TestEqual(TEXT("six keys after batch"), Subsystem->Get_AllSettingKeys().Num(), 6);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GameSettings_Registry_DuplicateKeyRejected,
    "Ck.CkGameSettings.Registry.DuplicateKeyRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GameSettings_Registry_DuplicateKeyRejected::RunTest(const FString&)
{
    auto* Subsystem = ck_game_settings_registry_spec::Make_Subsystem();

    TestTrue(TEXT("first registration succeeds"), Subsystem->Request_RegisterSetting(
        FCk_GameSettings_SettingDefinition{TEXT("dup.key"), ECk_GameSettings_ValueType::Int32, TEXT("1")}));

    AddExpectedError(TEXT("already registered"), EAutomationExpectedErrorFlags::Contains, 0);
    TestFalse(TEXT("duplicate registration rejected"), Subsystem->Request_RegisterSetting(
        FCk_GameSettings_SettingDefinition{TEXT("dup.key"), ECk_GameSettings_ValueType::Int32, TEXT("999")}));

    TestEqual(TEXT("still exactly one key"), Subsystem->Get_AllSettingKeys().Num(), 1);
    TestEqual(TEXT("original default untouched"), Subsystem->Get_SettingValue_Int32(TEXT("dup.key"), -1), 1);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GameSettings_Registry_AtomicBatchRejectsAll,
    "Ck.CkGameSettings.Registry.AtomicBatchRejectsAll",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GameSettings_Registry_AtomicBatchRejectsAll::RunTest(const FString&)
{
    auto* Subsystem = ck_game_settings_registry_spec::Make_Subsystem();

    AddExpectedError(TEXT("registration rejected"), EAutomationExpectedErrorFlags::Contains, 0);

    const auto BatchWithUnparseableDefault = TArray<FCk_GameSettings_SettingDefinition>
    {
        FCk_GameSettings_SettingDefinition{TEXT("atomic.a"), ECk_GameSettings_ValueType::Bool, TEXT("true")},
        FCk_GameSettings_SettingDefinition{TEXT("atomic.b"), ECk_GameSettings_ValueType::Int32, TEXT("7")},
        FCk_GameSettings_SettingDefinition{TEXT("atomic.bad"), ECk_GameSettings_ValueType::Int32, TEXT("abc")},
    };
    TestFalse(TEXT("batch with unparseable default rejected"), Subsystem->Request_RegisterSettings(BatchWithUnparseableDefault));
    TestEqual(TEXT("nothing registered from rejected batch"), Subsystem->Get_AllSettingKeys().Num(), 0);
    TestFalse(TEXT("valid member of rejected batch not registered"), Subsystem->Get_IsSettingRegistered(TEXT("atomic.a")));

    const auto BatchWithUnsetCVarName = TArray<FCk_GameSettings_SettingDefinition>
    {
        FCk_GameSettings_SettingDefinition{TEXT("atomic.c"), ECk_GameSettings_ValueType::Float, TEXT("1.0")},
        FCk_GameSettings_SettingDefinition{TEXT("atomic.cvar"), ECk_GameSettings_ValueType::Float, TEXT("1.0")}
            .Set_ApplyBindingType(ECk_GameSettings_ApplyBindingType::CVar),
    };
    TestFalse(TEXT("batch with unset CVar name rejected"), Subsystem->Request_RegisterSettings(BatchWithUnsetCVarName));
    TestEqual(TEXT("nothing registered from CVar batch"), Subsystem->Get_AllSettingKeys().Num(), 0);

    const auto BatchWithRangeOnBool = TArray<FCk_GameSettings_SettingDefinition>
    {
        FCk_GameSettings_SettingDefinition{TEXT("atomic.d"), ECk_GameSettings_ValueType::String, TEXT("x")},
        FCk_GameSettings_SettingDefinition{TEXT("atomic.rangedbool"), ECk_GameSettings_ValueType::Bool, TEXT("true")}
            .Set_MinValue(TEXT("0")),
    };
    TestFalse(TEXT("batch with Min/Max on non-numeric type rejected"), Subsystem->Request_RegisterSettings(BatchWithRangeOnBool));
    TestEqual(TEXT("nothing registered from ranged-bool batch"), Subsystem->Get_AllSettingKeys().Num(), 0);

    const auto BatchWithNoneKey = TArray<FCk_GameSettings_SettingDefinition>
    {
        FCk_GameSettings_SettingDefinition{TEXT("atomic.e"), ECk_GameSettings_ValueType::Bool, TEXT("false")},
        FCk_GameSettings_SettingDefinition{FName{}, ECk_GameSettings_ValueType::Bool, TEXT("false")},
    };
    TestFalse(TEXT("batch with None key rejected"), Subsystem->Request_RegisterSettings(BatchWithNoneKey));
    TestEqual(TEXT("nothing registered from None-key batch"), Subsystem->Get_AllSettingKeys().Num(), 0);

    const auto BatchWithUnparseableMin = TArray<FCk_GameSettings_SettingDefinition>
    {
        FCk_GameSettings_SettingDefinition{TEXT("atomic.f"), ECk_GameSettings_ValueType::Int32, TEXT("1")},
        FCk_GameSettings_SettingDefinition{TEXT("atomic.badmin"), ECk_GameSettings_ValueType::Int32, TEXT("1")}
            .Set_MinValue(TEXT("abc")),
    };
    TestFalse(TEXT("batch with unparseable Min rejected"), Subsystem->Request_RegisterSettings(BatchWithUnparseableMin));
    TestEqual(TEXT("nothing registered from bad-min batch"), Subsystem->Get_AllSettingKeys().Num(), 0);

    AddExpectedError(TEXT("Invalid GameSettings Collection"), EAutomationExpectedErrorFlags::Contains, 0);
    TestFalse(TEXT("null collection rejected"), Subsystem->Request_RegisterCollection(nullptr));
    TestEqual(TEXT("nothing registered from null collection"), Subsystem->Get_AllSettingKeys().Num(), 0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GameSettings_Registry_TypedAccessAndMismatchRejected,
    "Ck.CkGameSettings.Registry.TypedAccessAndMismatchRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GameSettings_Registry_TypedAccessAndMismatchRejected::RunTest(const FString&)
{
    auto* Subsystem = ck_game_settings_registry_spec::Make_Subsystem();

    TestTrue(TEXT("register Int32"), Subsystem->Request_RegisterSetting(
        FCk_GameSettings_SettingDefinition{TEXT("typed.int"), ECk_GameSettings_ValueType::Int32, TEXT("7")}));

    TestEqual(TEXT("typed read succeeds"), Subsystem->Get_SettingValue_Int32(TEXT("typed.int"), 0), 7);

    AddExpectedError(TEXT("it is not registered"), EAutomationExpectedErrorFlags::Contains, 0);
    TestTrue(TEXT("unknown key returns the supplied fallback"), Subsystem->Get_SettingValue_Bool(TEXT("typed.missing"), true));

    AddExpectedError(TEXT("its value type is"), EAutomationExpectedErrorFlags::Contains, 0);
    TestEqual(TEXT("type-mismatched read returns the supplied fallback"),
        Subsystem->Get_SettingValue_Float(TEXT("typed.int"), 1.5f), 1.5f);

    TestFalse(TEXT("type-mismatched set rejected"), Subsystem->Request_SetSettingValue_Bool(
        FCk_Request_GameSettings_SetValue_Bool{TEXT("typed.int"), true}));
    TestEqual(TEXT("value unchanged after rejected set"), Subsystem->Get_SettingValue_Int32(TEXT("typed.int"), 0), 7);

    TestFalse(TEXT("unknown-key set rejected"), Subsystem->Request_SetSettingValue_Int32(
        FCk_Request_GameSettings_SetValue_Int32{TEXT("typed.missing"), 1}));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GameSettings_Registry_SetValueFiresChangeOnce,
    "Ck.CkGameSettings.Registry.SetValueFiresChangeOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GameSettings_Registry_SetValueFiresChangeOnce::RunTest(const FString&)
{
    auto* Subsystem = ck_game_settings_registry_spec::Make_Subsystem();

    auto* Listener = NewObject<UCk_GameSettings_SpecListener_UE>();
    auto ListenerDelegate = FCk_Delegate_GameSettings_OnSettingChanged{};
    ListenerDelegate.BindDynamic(Listener, &UCk_GameSettings_SpecListener_UE::OnSettingChanged);
    Subsystem->BindTo_OnSettingChanged(TEXT("chg.bool"), ListenerDelegate);

    TestTrue(TEXT("register after bind"), Subsystem->Request_RegisterSetting(
        FCk_GameSettings_SettingDefinition{TEXT("chg.bool"), ECk_GameSettings_ValueType::Bool, TEXT("false")}));
    TestEqual(TEXT("registration does not fire the change delegate"), Listener->Get_FireCount(), 0);

    TestTrue(TEXT("set true holds"), Subsystem->Request_SetSettingValue_Bool(
        FCk_Request_GameSettings_SetValue_Bool{TEXT("chg.bool"), true}));
    TestEqual(TEXT("change fired once"), Listener->Get_FireCount(), 1);
    TestEqual(TEXT("change carries the key"), Listener->Get_LastKey(), FName{TEXT("chg.bool")});
    TestEqual(TEXT("change carries the new value"), Listener->Get_LastValue(), TEXT("true"));

    TestTrue(TEXT("same-value set still holds"), Subsystem->Request_SetSettingValue_Bool(
        FCk_Request_GameSettings_SetValue_Bool{TEXT("chg.bool"), true}));
    TestEqual(TEXT("same-value set does NOT fire"), Listener->Get_FireCount(), 1);

    TestTrue(TEXT("set false holds"), Subsystem->Request_SetSettingValue_Bool(
        FCk_Request_GameSettings_SetValue_Bool{TEXT("chg.bool"), false}));
    TestEqual(TEXT("change fired again"), Listener->Get_FireCount(), 2);

    auto* WildcardListener = NewObject<UCk_GameSettings_SpecListener_UE>();
    auto WildcardDelegate = FCk_Delegate_GameSettings_OnSettingChanged{};
    WildcardDelegate.BindDynamic(WildcardListener, &UCk_GameSettings_SpecListener_UE::OnSettingChanged);
    Subsystem->BindTo_OnSettingChanged(NAME_None, WildcardDelegate);

    TestTrue(TEXT("set true fires both listeners"), Subsystem->Request_SetSettingValue_Bool(
        FCk_Request_GameSettings_SetValue_Bool{TEXT("chg.bool"), true}));
    TestEqual(TEXT("per-key listener fired"), Listener->Get_FireCount(), 3);
    TestEqual(TEXT("wildcard listener fired"), WildcardListener->Get_FireCount(), 1);
    TestEqual(TEXT("wildcard carries the concrete key"), WildcardListener->Get_LastKey(), FName{TEXT("chg.bool")});

    Subsystem->UnbindFrom_OnSettingChanged(TEXT("chg.bool"), ListenerDelegate);

    TestTrue(TEXT("set false after unbind holds"), Subsystem->Request_SetSettingValue_Bool(
        FCk_Request_GameSettings_SetValue_Bool{TEXT("chg.bool"), false}));
    TestEqual(TEXT("unbound listener no longer fires"), Listener->Get_FireCount(), 3);
    TestEqual(TEXT("wildcard listener still fires"), WildcardListener->Get_FireCount(), 2);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GameSettings_Registry_ResetAllRestoresDefaults,
    "Ck.CkGameSettings.Registry.ResetAllRestoresDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GameSettings_Registry_ResetAllRestoresDefaults::RunTest(const FString&)
{
    auto* Subsystem = ck_game_settings_registry_spec::Make_Subsystem();

    TestTrue(TEXT("register r.a"), Subsystem->Request_RegisterSetting(
        FCk_GameSettings_SettingDefinition{TEXT("r.a"), ECk_GameSettings_ValueType::Bool, TEXT("false")}));
    TestTrue(TEXT("register r.b"), Subsystem->Request_RegisterSetting(
        FCk_GameSettings_SettingDefinition{TEXT("r.b"), ECk_GameSettings_ValueType::Int32, TEXT("10")}));
    TestTrue(TEXT("register r.c"), Subsystem->Request_RegisterSetting(
        FCk_GameSettings_SettingDefinition{TEXT("r.c"), ECk_GameSettings_ValueType::String, TEXT("abc")}));

    TestTrue(TEXT("change r.a"), Subsystem->Request_SetSettingValue_Bool(
        FCk_Request_GameSettings_SetValue_Bool{TEXT("r.a"), true}));
    TestTrue(TEXT("change r.b"), Subsystem->Request_SetSettingValue_Int32(
        FCk_Request_GameSettings_SetValue_Int32{TEXT("r.b"), 20}));

    TestEqual(TEXT("reset-all resets exactly the two changed settings"), Subsystem->Request_ResetAllToDefaults(), 2);
    TestFalse(TEXT("r.a restored"), Subsystem->Get_SettingValue_Bool(TEXT("r.a"), true));
    TestEqual(TEXT("r.b restored"), Subsystem->Get_SettingValue_Int32(TEXT("r.b"), 0), 10);
    TestEqual(TEXT("r.c untouched"), Subsystem->Get_SettingValue_String(TEXT("r.c")), TEXT("abc"));

    TestEqual(TEXT("second reset-all is a no-op"), Subsystem->Request_ResetAllToDefaults(), 0);

    TestTrue(TEXT("change r.b again"), Subsystem->Request_SetSettingValue_Int32(
        FCk_Request_GameSettings_SetValue_Int32{TEXT("r.b"), 30}));
    TestTrue(TEXT("single reset succeeds"), Subsystem->Request_ResetToDefault(
        FCk_Request_GameSettings_ResetToDefault{TEXT("r.b")}));
    TestEqual(TEXT("r.b restored by single reset"), Subsystem->Get_SettingValue_Int32(TEXT("r.b"), 0), 10);

    AddExpectedError(TEXT("it is not registered"), EAutomationExpectedErrorFlags::Contains, 0);
    TestFalse(TEXT("resetting an unknown key is rejected"), Subsystem->Request_ResetToDefault(
        FCk_Request_GameSettings_ResetToDefault{TEXT("r.missing")}));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GameSettings_Registry_RangeViolationRejected,
    "Ck.CkGameSettings.Registry.RangeViolationRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GameSettings_Registry_RangeViolationRejected::RunTest(const FString&)
{
    auto* Subsystem = ck_game_settings_registry_spec::Make_Subsystem();

    TestTrue(TEXT("register ranged Int32"), Subsystem->Request_RegisterSetting(
        FCk_GameSettings_SettingDefinition{TEXT("rng.int"), ECk_GameSettings_ValueType::Int32, TEXT("5")}
            .Set_MinValue(TEXT("0"))
            .Set_MaxValue(TEXT("10"))));

    AddExpectedError(TEXT("outside the allowed range"), EAutomationExpectedErrorFlags::Contains, 0);
    TestFalse(TEXT("above-max set rejected"), Subsystem->Request_SetSettingValue_Int32(
        FCk_Request_GameSettings_SetValue_Int32{TEXT("rng.int"), 11}));
    TestEqual(TEXT("value unchanged after above-max"), Subsystem->Get_SettingValue_Int32(TEXT("rng.int"), -1), 5);

    TestFalse(TEXT("below-min set rejected"), Subsystem->Request_SetSettingValue_Int32(
        FCk_Request_GameSettings_SetValue_Int32{TEXT("rng.int"), -1}));
    TestEqual(TEXT("value unchanged after below-min"), Subsystem->Get_SettingValue_Int32(TEXT("rng.int"), -1), 5);

    TestTrue(TEXT("max boundary is inclusive"), Subsystem->Request_SetSettingValue_Int32(
        FCk_Request_GameSettings_SetValue_Int32{TEXT("rng.int"), 10}));
    TestEqual(TEXT("boundary value stored"), Subsystem->Get_SettingValue_Int32(TEXT("rng.int"), -1), 10);
    TestTrue(TEXT("min boundary is inclusive"), Subsystem->Request_SetSettingValue_Int32(
        FCk_Request_GameSettings_SetValue_Int32{TEXT("rng.int"), 0}));

    TestTrue(TEXT("register ranged Float"), Subsystem->Request_RegisterSetting(
        FCk_GameSettings_SettingDefinition{TEXT("rng.float"), ECk_GameSettings_ValueType::Float, TEXT("0.5")}
            .Set_MinValue(TEXT("0"))
            .Set_MaxValue(TEXT("1"))));

    TestFalse(TEXT("out-of-range float set rejected"), Subsystem->Request_SetSettingValue_Float(
        FCk_Request_GameSettings_SetValue_Float{TEXT("rng.float"), 1.5f}));
    TestEqual(TEXT("float value unchanged"), Subsystem->Get_SettingValue_Float(TEXT("rng.float"), -1.0f), 0.5f);
    TestTrue(TEXT("in-range float set holds"), Subsystem->Request_SetSettingValue_Float(
        FCk_Request_GameSettings_SetValue_Float{TEXT("rng.float"), 1.0f}));

    TestTrue(TEXT("register Select-style String"), Subsystem->Request_RegisterSetting(
        FCk_GameSettings_SettingDefinition{TEXT("opt.str"), ECk_GameSettings_ValueType::String, TEXT("low")}
            .Set_Options(TArray<FCk_GameSettings_SettingOption>
            {
                FCk_GameSettings_SettingOption{FText::FromString(TEXT("Low")), TEXT("low")},
                FCk_GameSettings_SettingOption{FText::FromString(TEXT("High")), TEXT("high")},
            })));

    AddExpectedError(TEXT("not one of the allowed options"), EAutomationExpectedErrorFlags::Contains, 0);
    TestFalse(TEXT("non-option value rejected"), Subsystem->Request_SetSettingValue_String(
        FCk_Request_GameSettings_SetValue_String{TEXT("opt.str"), TEXT("medium")}));
    TestEqual(TEXT("value unchanged after rejected option"), Subsystem->Get_SettingValue_String(TEXT("opt.str")), TEXT("low"));

    TestTrue(TEXT("allowed option accepted"), Subsystem->Request_SetSettingValue_String(
        FCk_Request_GameSettings_SetValue_String{TEXT("opt.str"), TEXT("high")}));
    TestEqual(TEXT("option value stored"), Subsystem->Get_SettingValue_String(TEXT("opt.str")), TEXT("high"));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
