#include "CkGameSettings/UI/CkGameSettingsUI_RowWidgets.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GameSettings_UI_RowClassResolution,
    "Ck.CkGameSettings.UI.RowClassResolution",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GameSettings_UI_RowClassResolution::RunTest(const FString&)
{
    const auto NoOverrides = TMap<ECk_GameSettings_ValueType, TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>>{};
    const auto NoSelectOverride = TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{};

    const auto Resolve = [&](const FCk_GameSettings_SettingDefinition& InDefinition)
    {
        return ck::game_settings_ui::Get_ResolvedRowClass(InDefinition, NoOverrides, NoSelectOverride).Get();
    };

    // ---- Built-in defaults ----
    TestEqual(TEXT("Bool resolves to Toggle"),
        Resolve(FCk_GameSettings_SettingDefinition{FName{TEXT("spec.ui.bool")}, ECk_GameSettings_ValueType::Bool, TEXT("true")}),
        static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Toggle::StaticClass()));

    const auto RangedInt = FCk_GameSettings_SettingDefinition{FName{TEXT("spec.ui.int")}, ECk_GameSettings_ValueType::Int32, TEXT("5")}
        .Set_MinValue(TEXT("0")).Set_MaxValue(TEXT("10"));
    TestEqual(TEXT("Int32 with a full range resolves to Slider"),
        Resolve(RangedInt), static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Slider::StaticClass()));

    const auto RangedFloat = FCk_GameSettings_SettingDefinition{FName{TEXT("spec.ui.float")}, ECk_GameSettings_ValueType::Float, TEXT("0.5")}
        .Set_MinValue(TEXT("0")).Set_MaxValue(TEXT("1"));
    TestEqual(TEXT("Float with a full range resolves to Slider"),
        Resolve(RangedFloat), static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Slider::StaticClass()));

    const auto HalfRangedInt = FCk_GameSettings_SettingDefinition{FName{TEXT("spec.ui.halfint")}, ECk_GameSettings_ValueType::Int32, TEXT("5")}
        .Set_MinValue(TEXT("0"));
    TestEqual(TEXT("Int32 with only a lower bound resolves to Select (stepper)"),
        Resolve(HalfRangedInt), static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Select::StaticClass()));

    TestEqual(TEXT("Float without a range resolves to Select (stepper)"),
        Resolve(FCk_GameSettings_SettingDefinition{FName{TEXT("spec.ui.rawfloat")}, ECk_GameSettings_ValueType::Float, TEXT("1")}),
        static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Select::StaticClass()));

    TestEqual(TEXT("String without options resolves to Select (display-only)"),
        Resolve(FCk_GameSettings_SettingDefinition{FName{TEXT("spec.ui.string")}, ECk_GameSettings_ValueType::String, TEXT("abc")}),
        static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Select::StaticClass()));

    const auto WithOptions = FCk_GameSettings_SettingDefinition{FName{TEXT("spec.ui.options")}, ECk_GameSettings_ValueType::String, TEXT("a")}
        .Set_Options({FCk_GameSettings_SettingOption{FText::FromString(TEXT("A")), TEXT("a")},
                      FCk_GameSettings_SettingOption{FText::FromString(TEXT("B")), TEXT("b")}});
    TestEqual(TEXT("Options-present resolves to Select"),
        Resolve(WithOptions), static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Select::StaticClass()));

    // ---- Per-type override precedence ----
    auto Overrides = TMap<ECk_GameSettings_ValueType, TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>>{};
    Overrides.Emplace(ECk_GameSettings_ValueType::Bool,
        TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{UCk_GameSettingsUI_RowWidget_Slider::StaticClass()});

    TestEqual(TEXT("Per-type override beats the built-in default"),
        ck::game_settings_ui::Get_ResolvedRowClass(
            FCk_GameSettings_SettingDefinition{FName{TEXT("spec.ui.bool")}, ECk_GameSettings_ValueType::Bool, TEXT("true")},
            Overrides, NoSelectOverride).Get(),
        static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Slider::StaticClass()));

    TestEqual(TEXT("Per-type override for a DIFFERENT type does not apply"),
        ck::game_settings_ui::Get_ResolvedRowClass(
            FCk_GameSettings_SettingDefinition{FName{TEXT("spec.ui.string")}, ECk_GameSettings_ValueType::String, TEXT("abc")},
            Overrides, NoSelectOverride).Get(),
        static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Select::StaticClass()));

    // ---- Select override precedence (options-present case) ----
    const auto SelectOverride = TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{UCk_GameSettingsUI_RowWidget_Toggle::StaticClass()};

    TestEqual(TEXT("Select override wins for options-present settings"),
        ck::game_settings_ui::Get_ResolvedRowClass(WithOptions, Overrides, SelectOverride).Get(),
        static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Toggle::StaticClass()));

    // The shipped dropdown routes through the same hook; never a resolution default.
    const auto DropdownOverride = TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{UCk_GameSettingsUI_RowWidget_Dropdown::StaticClass()};

    TestEqual(TEXT("Dropdown as the select override resolves for options-present settings"),
        ck::game_settings_ui::Get_ResolvedRowClass(WithOptions, NoOverrides, DropdownOverride).Get(),
        static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Dropdown::StaticClass()));

    // ---- Per-SETTING override beats everything ----
    const auto PerSettingBool = FCk_GameSettings_SettingDefinition{FName{TEXT("spec.ui.rowclass.bool")}, ECk_GameSettings_ValueType::Bool, TEXT("true")}
        .Set_OptionalRowClassOverride(TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{UCk_GameSettingsUI_RowWidget_Dropdown::StaticClass()});

    TestEqual(TEXT("Definition _OptionalRowClassOverride beats the built-in default"),
        Resolve(PerSettingBool), static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Dropdown::StaticClass()));

    TestEqual(TEXT("Definition _OptionalRowClassOverride beats the per-type override"),
        ck::game_settings_ui::Get_ResolvedRowClass(PerSettingBool, Overrides, NoSelectOverride).Get(),
        static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Dropdown::StaticClass()));

    const auto PerSettingOptions = FCk_GameSettings_SettingDefinition{FName{TEXT("spec.ui.rowclass.options")}, ECk_GameSettings_ValueType::String, TEXT("a")}
        .Set_Options({FCk_GameSettings_SettingOption{FText::FromString(TEXT("A")), TEXT("a")}})
        .Set_OptionalRowClassOverride(TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{UCk_GameSettingsUI_RowWidget_Toggle::StaticClass()});

    TestEqual(TEXT("Definition _OptionalRowClassOverride beats the select override for options-present settings"),
        ck::game_settings_ui::Get_ResolvedRowClass(PerSettingOptions, Overrides, SelectOverride).Get(),
        static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Toggle::StaticClass()));

    auto OptionsTypeOverrides = TMap<ECk_GameSettings_ValueType, TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>>{};
    OptionsTypeOverrides.Emplace(ECk_GameSettings_ValueType::String,
        TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{UCk_GameSettingsUI_RowWidget_Slider::StaticClass()});

    TestEqual(TEXT("Options-present ignores the per-type override (the select override is its hook)"),
        ck::game_settings_ui::Get_ResolvedRowClass(WithOptions, OptionsTypeOverrides, NoSelectOverride).Get(),
        static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Select::StaticClass()));

    // ---- Null soft-class entries are ignored ----
    auto NullOverrides = TMap<ECk_GameSettings_ValueType, TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>>{};
    NullOverrides.Emplace(ECk_GameSettings_ValueType::Bool, TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{});

    TestEqual(TEXT("A null per-type override entry falls through to the built-in default"),
        ck::game_settings_ui::Get_ResolvedRowClass(
            FCk_GameSettings_SettingDefinition{FName{TEXT("spec.ui.bool")}, ECk_GameSettings_ValueType::Bool, TEXT("true")},
            NullOverrides, NoSelectOverride).Get(),
        static_cast<UClass*>(UCk_GameSettingsUI_RowWidget_Toggle::StaticClass()));

    // ---- Edit-condition disposition ----
    {
        using ck::game_settings_ui::EEditDisposition;
        using ck::game_settings_ui::Get_EditDisposition;

        // The function treats traits as opaque tags — any registered native tag works as a stand-in
        const auto TraitTag = FGameplayTag::RequestGameplayTag(FName{TEXT("UI.Layer.Menu")});
        const auto RequiredTraits = FGameplayTagContainer{TraitTag};

        auto NoConditions = FCk_GameSettings_EditConditions{};
        TestTrue(TEXT("No required traits => Editable"),
            Get_EditDisposition(NoConditions, FGameplayTagContainer{}) == EEditDisposition::Editable);

        auto ConditionsWithoutReason = FCk_GameSettings_EditConditions{}.Set_RequiredPlatformTraits(RequiredTraits);
        TestTrue(TEXT("Missing trait with no reason => Hidden"),
            Get_EditDisposition(ConditionsWithoutReason, FGameplayTagContainer{}) == EEditDisposition::Hidden);

        auto ConditionsWithReason = FCk_GameSettings_EditConditions{}
            .Set_RequiredPlatformTraits(RequiredTraits)
            .Set_DisabledReason(FText::FromString(TEXT("Not supported here")));
        TestTrue(TEXT("Missing trait with a reason => Disabled"),
            Get_EditDisposition(ConditionsWithReason, FGameplayTagContainer{}) == EEditDisposition::Disabled);

        TestTrue(TEXT("Trait present => Editable"),
            Get_EditDisposition(ConditionsWithReason, RequiredTraits) == EEditDisposition::Editable);
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GameSettings_UI_ToggleStateText,
    "Ck.CkGameSettings.UI.ToggleStateText",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GameSettings_UI_ToggleStateText::RunTest(const FString&)
{
    using ck::game_settings_ui::Get_ToggleStateText;

    const auto Bare = FCk_GameSettings_SettingDefinition{FName{TEXT("spec.ui.toggle")}, ECk_GameSettings_ValueType::Bool, TEXT("false")};

    TestEqual(TEXT("No options, checked => localized On"),
        Get_ToggleStateText(Bare, true).ToString(), TEXT("On"));
    TestEqual(TEXT("No options, unchecked => localized Off"),
        Get_ToggleStateText(Bare, false).ToString(), TEXT("Off"));

    // Authored options relabel the states; matching is by the option's parsed BOOL, not its
    // position, so a definition may list true first.
    const auto Relabelled = FCk_GameSettings_SettingDefinition{FName{TEXT("spec.ui.toggle.opts")}, ECk_GameSettings_ValueType::Bool, TEXT("false")}
        .Set_Options({FCk_GameSettings_SettingOption{FText::FromString(TEXT("Inverted")), TEXT("true")},
                      FCk_GameSettings_SettingOption{FText::FromString(TEXT("Normal")),   TEXT("false")}});

    TestEqual(TEXT("Authored options, checked => the true option's label"),
        Get_ToggleStateText(Relabelled, true).ToString(), TEXT("Inverted"));
    TestEqual(TEXT("Authored options, unchecked => the false option's label"),
        Get_ToggleStateText(Relabelled, false).ToString(), TEXT("Normal"));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
