#include "CkGameSettingsUI_RowWidgets.h"

#include "CkGameSettings/CkGameSettings_Log.h"
#include "CkGameSettings/Settings/CkGameSettings_Settings.h"
#include "CkGameSettings/Subsystem/CkGameSettings_Utils.h"

#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

#include <CommonUISettings.h>
#include <ICommonUIModule.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_game_settings_ui_row_widgets
{
    constexpr auto FloatOptionMatchTolerance = 1.e-4f;
    constexpr auto SliderFallbackMin = 0.0f;
    constexpr auto SliderFallbackMax = 1.0f;
    constexpr auto DefaultFloatPrecision = 2;
    constexpr auto AutoStepFractionOfRange = 0.01f;

    auto
        Get_StepSize(
            const FCk_GameSettings_SettingDefinition& InDefinition,
            float InMin,
            float InMax)
        -> float
    {
        if (const auto AuthoredStep = InDefinition.Get_OptionalStepSize();
            AuthoredStep > 0.0f)
        { return AuthoredStep; }

        return InDefinition.Get_ValueType() == ECk_GameSettings_ValueType::Int32
            ? 1.0f
            : (InMax - InMin) * AutoStepFractionOfRange;
    }

    auto
        Get_SnappedValue(
            float InValue,
            const FCk_GameSettings_SettingDefinition& InDefinition)
        -> float
    {
        auto MinValue = SliderFallbackMin;
        auto MaxValue = SliderFallbackMax;
        LexTryParseString(MinValue, *InDefinition.Get_MinValue());
        LexTryParseString(MaxValue, *InDefinition.Get_MaxValue());

        const auto StepSize = Get_StepSize(InDefinition, MinValue, MaxValue);

        if (StepSize <= 0.0f)
        { return InValue; }

        return MinValue + FMath::RoundToFloat((InValue - MinValue) / StepSize) * StepSize;
    }

    /** Fixed-precision then trailing-zeros trimmed: 90.00 -> "90", 1.50 -> "1.5", 0.30000001 -> "0.3". */
    auto
        Format_Readout(
            float InValue,
            int32 InPrecision)
        -> FString
    {
        auto Formatted = FString::Printf(TEXT("%.*f"), FMath::Max(0, InPrecision), InValue);

        if (NOT Formatted.Contains(TEXT(".")))
        { return Formatted; }

        while (Formatted.EndsWith(TEXT("0")))
        { Formatted.LeftChopInline(1); }

        if (Formatted.EndsWith(TEXT(".")))
        { Formatted.LeftChopInline(1); }

        return Formatted;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_RowWidgetBase::
    InjectSetting(
        const UObject* InWorldContextObject,
        FName InKey)
    -> void
{
    auto Delegate = FCk_Delegate_GameSettings_OnSettingChanged{};
    Delegate.BindDynamic(this, &UCk_GameSettingsUI_RowWidgetBase::HandleSettingChanged);

    if (_Key != NAME_None && _WorldContextObject.IsValid())
    {
        UCk_Utils_GameSettings_UE::UnbindFrom_OnSettingChanged(_WorldContextObject.Get(), _Key, Delegate);
    }

    _WorldContextObject = InWorldContextObject;
    _Key = InKey;

    const auto ContextIsUsable = ck::IsValid(InWorldContextObject);
    CK_ENSURE_IF_NOT(ContextIsUsable, TEXT("Invalid WorldContextObject passed to GameSettings row [{}] for key [{}]"), this, InKey)
    {}
    if (NOT ContextIsUsable)
    { return; }

    const auto KeyIsRegistered = UCk_Utils_GameSettings_UE::Get_IsSettingRegistered(InWorldContextObject, InKey);
    CK_ENSURE_IF_NOT(KeyIsRegistered, TEXT("GameSettings row [{}] injected with unregistered key [{}]"), this, InKey)
    {}
    if (NOT KeyIsRegistered)
    { return; }

    UCk_Utils_GameSettings_UE::BindTo_OnSettingChanged(InWorldContextObject, InKey, Delegate);

    DoRefreshFromRegistry();
}

auto
    UCk_GameSettingsUI_RowWidgetBase::
    InjectPreview(
        const FCk_GameSettings_SettingDefinition& InDefinition,
        const FString& InValueString)
    -> void
{
    _Key = InDefinition.Get_Key();

    DoRefreshCommon(InDefinition);

    _RefreshingControl = true;
    DoRefreshControl(InDefinition, InValueString);
    _RefreshingControl = false;
}

auto
    UCk_GameSettingsUI_RowWidgetBase::
    Get_SettingKey() const
    -> FName
{
    return _Key;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_RowWidgetBase::
    NativeDestruct()
    -> void
{
    if (_Key != NAME_None && _WorldContextObject.IsValid())
    {
        auto Delegate = FCk_Delegate_GameSettings_OnSettingChanged{};
        Delegate.BindDynamic(this, &UCk_GameSettingsUI_RowWidgetBase::HandleSettingChanged);

        UCk_Utils_GameSettings_UE::UnbindFrom_OnSettingChanged(_WorldContextObject.Get(), _Key, Delegate);
    }

    Super::NativeDestruct();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_RowWidgetBase::
    DoRefreshFromRegistry()
    -> void
{
    const auto WorldContext = Get_WorldContext();

    if (ck::Is_NOT_Valid(WorldContext))
    { return; }

    auto Definition = FCk_GameSettings_SettingDefinition{};

    if (NOT UCk_Utils_GameSettings_UE::Get_SettingDefinition(WorldContext, _Key, Definition))
    { return; }

    DoRefreshCommon(Definition);

    _RefreshingControl = true;
    DoRefreshControl(Definition, Get_CurrentValueString(Definition));
    _RefreshingControl = false;
}

auto
    UCk_GameSettingsUI_RowWidgetBase::
    DoRefreshCommon(
        const FCk_GameSettings_SettingDefinition& InDefinition)
    -> void
{
    if (ck::IsValid(_DisplayNameText))
    {
        const auto& DisplayName = InDefinition.Get_DisplayName();
        _DisplayNameText->SetText(DisplayName.IsEmpty() ? FText::FromName(InDefinition.Get_Key()) : DisplayName);
    }

    const auto Disposition = ck::game_settings_ui::Get_EditDisposition(InDefinition.Get_EditConditions());
    const auto IsEditable = Disposition == ck::game_settings_ui::EEditDisposition::Editable;

    SetIsEnabled(IsEditable);
    SetToolTipText(IsEditable ? InDefinition.Get_Description() : InDefinition.Get_EditConditions().Get_DisabledReason());
}

auto
    UCk_GameSettingsUI_RowWidgetBase::
    Get_WorldContext() const
    -> const UObject*
{
    return _WorldContextObject.Get();
}

auto
    UCk_GameSettingsUI_RowWidgetBase::
    Get_CurrentValueString(
        const FCk_GameSettings_SettingDefinition& InDefinition) const
    -> FString
{
    const auto WorldContext = Get_WorldContext();

    if (ck::Is_NOT_Valid(WorldContext))
    { return InDefinition.Get_DefaultValue(); }

    switch (InDefinition.Get_ValueType())
    {
        case ECk_GameSettings_ValueType::Bool:
        {
            return UCk_Utils_GameSettings_UE::Get_SettingValue_Bool(WorldContext, _Key) ? TEXT("true") : TEXT("false");
        }
        case ECk_GameSettings_ValueType::Int32:
        {
            return FString::FromInt(UCk_Utils_GameSettings_UE::Get_SettingValue_Int32(WorldContext, _Key));
        }
        case ECk_GameSettings_ValueType::Float:
        {
            return LexToString(UCk_Utils_GameSettings_UE::Get_SettingValue_Float(WorldContext, _Key));
        }
        case ECk_GameSettings_ValueType::String:
        {
            return UCk_Utils_GameSettings_UE::Get_SettingValue_String(WorldContext, _Key);
        }
    }

    return InDefinition.Get_DefaultValue();
}

auto
    UCk_GameSettingsUI_RowWidgetBase::
    HandleSettingChanged(
        FName InKey,
        const FString& InNewValue)
    -> void
{
    if (InKey != _Key)
    { return; }

    DoRefreshFromRegistry();
}

auto
    UCk_GameSettingsUI_RowWidgetBase::
    DoCommitOptionValue(
        const FCk_GameSettings_SettingDefinition& InDefinition,
        const FString& InOptionValue)
    -> void
{
    const auto WorldContext = Get_WorldContext();

    if (ck::Is_NOT_Valid(WorldContext))
    { return; }

    switch (InDefinition.Get_ValueType())
    {
        case ECk_GameSettings_ValueType::Bool:
        {
            UCk_Utils_GameSettings_UE::Request_SetSettingValue_Bool(WorldContext,
                FCk_Request_GameSettings_SetValue_Bool{Get_Key(), InOptionValue.ToBool()});
            return;
        }
        case ECk_GameSettings_ValueType::Int32:
        {
            auto Parsed = int32{};
            const auto OptionParses = LexTryParseString(Parsed, *InOptionValue);
            CK_ENSURE_IF_NOT(OptionParses, TEXT("GameSettings option value [{}] on key [{}] does not parse as Int32"), InOptionValue, Get_Key())
            {}
            if (NOT OptionParses)
            { return; }

            UCk_Utils_GameSettings_UE::Request_SetSettingValue_Int32(WorldContext,
                FCk_Request_GameSettings_SetValue_Int32{Get_Key(), Parsed});
            return;
        }
        case ECk_GameSettings_ValueType::Float:
        {
            auto Parsed = float{};
            const auto OptionParses = LexTryParseString(Parsed, *InOptionValue);
            CK_ENSURE_IF_NOT(OptionParses, TEXT("GameSettings option value [{}] on key [{}] does not parse as Float"), InOptionValue, Get_Key())
            {}
            if (NOT OptionParses)
            { return; }

            UCk_Utils_GameSettings_UE::Request_SetSettingValue_Float(WorldContext,
                FCk_Request_GameSettings_SetValue_Float{Get_Key(), Parsed});
            return;
        }
        case ECk_GameSettings_ValueType::String:
        {
            UCk_Utils_GameSettings_UE::Request_SetSettingValue_String(WorldContext,
                FCk_Request_GameSettings_SetValue_String{Get_Key(), InOptionValue});
            return;
        }
    }
}

auto
    UCk_GameSettingsUI_RowWidgetBase::
    Get_CurrentOptionIndex(
        const FCk_GameSettings_SettingDefinition& InDefinition,
        const FString& InValueString) const
    -> int32
{
    using namespace ck_game_settings_ui_row_widgets;

    const auto& Options = InDefinition.Get_Options();

    for (auto Index = 0; Index < Options.Num(); ++Index)
    {
        const auto& OptionValue = Options[Index].Get_Value();

        switch (InDefinition.Get_ValueType())
        {
            case ECk_GameSettings_ValueType::Bool:
            {
                if (OptionValue.ToBool() == InValueString.ToBool())
                { return Index; }
                break;
            }
            case ECk_GameSettings_ValueType::Int32:
            {
                auto OptionParsed = int32{};
                auto ValueParsed = int32{};

                if (LexTryParseString(OptionParsed, *OptionValue) &&
                    LexTryParseString(ValueParsed, *InValueString) &&
                    OptionParsed == ValueParsed)
                { return Index; }
                break;
            }
            case ECk_GameSettings_ValueType::Float:
            {
                auto OptionParsed = float{};
                auto ValueParsed = float{};

                if (LexTryParseString(OptionParsed, *OptionValue) &&
                    LexTryParseString(ValueParsed, *InValueString) &&
                    FMath::IsNearlyEqual(OptionParsed, ValueParsed, FloatOptionMatchTolerance))
                { return Index; }
                break;
            }
            case ECk_GameSettings_ValueType::String:
            {
                if (OptionValue == InValueString)
                { return Index; }
                break;
            }
        }
    }

    return INDEX_NONE;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_RowWidget_Toggle::
    DoRefreshControl(
        const FCk_GameSettings_SettingDefinition& InDefinition,
        const FString& InValueString)
    -> void
{
    if (ck::Is_NOT_Valid(_ValueCheckBox))
    { return; }

    _ValueCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UCk_GameSettingsUI_RowWidget_Toggle::HandleCheckStateChanged);
    _ValueCheckBox->SetIsChecked(InValueString.ToBool());
}

auto
    UCk_GameSettingsUI_RowWidget_Toggle::
    HandleCheckStateChanged(
        bool InIsChecked)
    -> void
{
    if (_RefreshingControl)
    { return; }

    const auto WorldContext = Get_WorldContext();

    if (ck::Is_NOT_Valid(WorldContext))
    { return; }

    UCk_Utils_GameSettings_UE::Request_SetSettingValue_Bool(WorldContext,
        FCk_Request_GameSettings_SetValue_Bool{Get_Key(), InIsChecked});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_RowWidget_Slider::
    DoRefreshControl(
        const FCk_GameSettings_SettingDefinition& InDefinition,
        const FString& InValueString)
    -> void
{
    using namespace ck_game_settings_ui_row_widgets;

    if (ck::Is_NOT_Valid(_ValueSlider))
    { return; }

    _ValueSlider->OnValueChanged.AddUniqueDynamic(this, &UCk_GameSettingsUI_RowWidget_Slider::HandleValueChanged);
    _ValueSlider->OnMouseCaptureBegin.AddUniqueDynamic(this, &UCk_GameSettingsUI_RowWidget_Slider::HandleCaptureBegin);
    _ValueSlider->OnMouseCaptureEnd.AddUniqueDynamic(this, &UCk_GameSettingsUI_RowWidget_Slider::HandleCaptureEnd);
    _ValueSlider->OnControllerCaptureBegin.AddUniqueDynamic(this, &UCk_GameSettingsUI_RowWidget_Slider::HandleCaptureBegin);
    _ValueSlider->OnControllerCaptureEnd.AddUniqueDynamic(this, &UCk_GameSettingsUI_RowWidget_Slider::HandleCaptureEnd);

    auto MinValue = SliderFallbackMin;
    auto MaxValue = SliderFallbackMax;
    LexTryParseString(MinValue, *InDefinition.Get_MinValue());
    LexTryParseString(MaxValue, *InDefinition.Get_MaxValue());

    _ValueSlider->SetMinValue(MinValue);
    _ValueSlider->SetMaxValue(MaxValue);
    _ValueSlider->SetStepSize(Get_StepSize(InDefinition, MinValue, MaxValue));

    auto CurrentValue = MinValue;
    LexTryParseString(CurrentValue, *InValueString);

    _ValueSlider->SetValue(CurrentValue);
    DoUpdateReadout(CurrentValue, InDefinition);
}

auto
    UCk_GameSettingsUI_RowWidget_Slider::
    HandleValueChanged(
        float InNewValue)
    -> void
{
    if (_RefreshingControl)
    { return; }

    const auto WorldContext = Get_WorldContext();

    if (ck::Is_NOT_Valid(WorldContext))
    { return; }

    auto Definition = FCk_GameSettings_SettingDefinition{};

    if (NOT UCk_Utils_GameSettings_UE::Get_SettingDefinition(WorldContext, Get_Key(), Definition))
    { return; }

    DoUpdateReadout(InNewValue, Definition);

    if (NOT _DragInProgress)
    { DoCommitValue(InNewValue); }
}

auto
    UCk_GameSettingsUI_RowWidget_Slider::
    HandleCaptureBegin()
    -> void
{
    _DragInProgress = true;
}

auto
    UCk_GameSettingsUI_RowWidget_Slider::
    HandleCaptureEnd()
    -> void
{
    _DragInProgress = false;

    if (ck::IsValid(_ValueSlider))
    { DoCommitValue(_ValueSlider->GetValue()); }
}

auto
    UCk_GameSettingsUI_RowWidget_Slider::
    DoUpdateReadout(
        float InValue,
        const FCk_GameSettings_SettingDefinition& InDefinition)
    -> void
{
    using namespace ck_game_settings_ui_row_widgets;

    if (ck::Is_NOT_Valid(_ValueText))
    { return; }

    const auto AuthoredPrecision = InDefinition.Get_OptionalDisplayPrecision();
    const auto Precision = AuthoredPrecision >= 0
        ? AuthoredPrecision
        : (InDefinition.Get_ValueType() == ECk_GameSettings_ValueType::Int32 ? 0 : DefaultFloatPrecision);

    const auto DisplayScale = InDefinition.Get_OptionalDisplayScale();
    const auto DisplayValue = Get_SnappedValue(InValue, InDefinition) * (DisplayScale > 0.0f ? DisplayScale : 1.0f);

    _ValueText->SetText(FText::FromString(Format_Readout(DisplayValue, Precision)));
}

auto
    UCk_GameSettingsUI_RowWidget_Slider::
    DoCommitValue(
        float InValue)
    -> void
{
    const auto WorldContext = Get_WorldContext();

    if (ck::Is_NOT_Valid(WorldContext))
    { return; }

    auto Definition = FCk_GameSettings_SettingDefinition{};

    if (NOT UCk_Utils_GameSettings_UE::Get_SettingDefinition(WorldContext, Get_Key(), Definition))
    { return; }

    const auto SnappedValue = ck_game_settings_ui_row_widgets::Get_SnappedValue(InValue, Definition);

    if (Definition.Get_ValueType() == ECk_GameSettings_ValueType::Int32)
    {
        UCk_Utils_GameSettings_UE::Request_SetSettingValue_Int32(WorldContext,
            FCk_Request_GameSettings_SetValue_Int32{Get_Key(), FMath::RoundToInt32(SnappedValue)});
        return;
    }

    UCk_Utils_GameSettings_UE::Request_SetSettingValue_Float(WorldContext,
        FCk_Request_GameSettings_SetValue_Float{Get_Key(), SnappedValue});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_RowWidget_Select::
    DoRefreshControl(
        const FCk_GameSettings_SettingDefinition& InDefinition,
        const FString& InValueString)
    -> void
{
    ck::game_settings_ui::BindClick(_PrevButton, this, &UCk_GameSettingsUI_RowWidget_Select::HandlePrevClicked);
    ck::game_settings_ui::BindClick(_NextButton, this, &UCk_GameSettingsUI_RowWidget_Select::HandleNextClicked);

    const auto& Options = InDefinition.Get_Options();
    const auto IsSteppable = Options.Num() > 0 ||
        InDefinition.Get_ValueType() == ECk_GameSettings_ValueType::Int32 ||
        InDefinition.Get_ValueType() == ECk_GameSettings_ValueType::Float;

    if (ck::IsValid(_PrevButton))
    { _PrevButton->SetIsEnabled(IsSteppable); }

    if (ck::IsValid(_NextButton))
    { _NextButton->SetIsEnabled(IsSteppable); }

    if (ck::Is_NOT_Valid(_ValueText))
    { return; }

    if (Options.Num() > 0)
    {
        const auto OptionIndex = Get_CurrentOptionIndex(InDefinition, InValueString);

        _ValueText->SetText(Options.IsValidIndex(OptionIndex)
            ? Options[OptionIndex].Get_Label()
            : FText::FromString(InValueString));
        return;
    }

    _ValueText->SetText(FText::FromString(InValueString));
}

auto
    UCk_GameSettingsUI_RowWidget_Select::
    HandlePrevClicked()
    -> void
{
    DoStep(-1);
}

auto
    UCk_GameSettingsUI_RowWidget_Select::
    HandleNextClicked()
    -> void
{
    DoStep(1);
}

auto
    UCk_GameSettingsUI_RowWidget_Select::
    DoStep(
        int32 InDirection)
    -> void
{
    if (_RefreshingControl)
    { return; }

    const auto WorldContext = Get_WorldContext();

    if (ck::Is_NOT_Valid(WorldContext))
    { return; }

    auto Definition = FCk_GameSettings_SettingDefinition{};

    if (NOT UCk_Utils_GameSettings_UE::Get_SettingDefinition(WorldContext, Get_Key(), Definition))
    { return; }

    const auto& Options = Definition.Get_Options();

    if (Options.Num() > 0)
    {
        const auto CurrentIndex = Get_CurrentOptionIndex(Definition, Get_CurrentValueString(Definition));
        const auto BaseIndex = CurrentIndex == INDEX_NONE ? 0 : CurrentIndex;
        const auto NextIndex = (BaseIndex + InDirection + Options.Num()) % Options.Num();

        DoCommitOptionValue(Definition, Options[NextIndex].Get_Value());
        return;
    }

    if (Definition.Get_ValueType() == ECk_GameSettings_ValueType::Int32)
    {
        auto Target = UCk_Utils_GameSettings_UE::Get_SettingValue_Int32(WorldContext, Get_Key()) + InDirection;

        auto Bound = int32{};
        if (LexTryParseString(Bound, *Definition.Get_MinValue()))
        { Target = FMath::Max(Target, Bound); }
        if (LexTryParseString(Bound, *Definition.Get_MaxValue()))
        { Target = FMath::Min(Target, Bound); }

        UCk_Utils_GameSettings_UE::Request_SetSettingValue_Int32(WorldContext,
            FCk_Request_GameSettings_SetValue_Int32{Get_Key(), Target});
        return;
    }

    if (Definition.Get_ValueType() == ECk_GameSettings_ValueType::Float)
    {
        auto Target = UCk_Utils_GameSettings_UE::Get_SettingValue_Float(WorldContext, Get_Key()) + static_cast<float>(InDirection);

        auto Bound = float{};
        if (LexTryParseString(Bound, *Definition.Get_MinValue()))
        { Target = FMath::Max(Target, Bound); }
        if (LexTryParseString(Bound, *Definition.Get_MaxValue()))
        { Target = FMath::Min(Target, Bound); }

        UCk_Utils_GameSettings_UE::Request_SetSettingValue_Float(WorldContext,
            FCk_Request_GameSettings_SetValue_Float{Get_Key(), Target});
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_RowWidget_Dropdown::
    DoRefreshControl(
        const FCk_GameSettings_SettingDefinition& InDefinition,
        const FString& InValueString)
    -> void
{
    if (ck::Is_NOT_Valid(_ValueComboBox))
    { return; }

    // Unbound while mutating: ClearOptions/SetSelectedIndex fire OnSelectionChanged, and a refresh
    // must never read as a user commit.
    _ValueComboBox->OnSelectionChanged.RemoveDynamic(this, &UCk_GameSettingsUI_RowWidget_Dropdown::HandleSelectionChanged);

    _ValueComboBox->ClearOptions();

    const auto& Options = InDefinition.Get_Options();

    for (const auto& Option : Options)
    { _ValueComboBox->AddOption(Option.Get_Label().ToString()); }

    if (Options.Num() == 0)
    {
        _ValueComboBox->AddOption(InValueString);
        _ValueComboBox->SetSelectedIndex(0);
        _ValueComboBox->SetIsEnabled(false);
    }
    else
    {
        const auto OptionIndex = Get_CurrentOptionIndex(InDefinition, InValueString);

        if (OptionIndex == INDEX_NONE)
        { _ValueComboBox->ClearSelection(); }
        else
        { _ValueComboBox->SetSelectedIndex(OptionIndex); }

        _ValueComboBox->SetIsEnabled(true);
    }

    _ValueComboBox->OnSelectionChanged.AddDynamic(this, &UCk_GameSettingsUI_RowWidget_Dropdown::HandleSelectionChanged);
}

auto
    UCk_GameSettingsUI_RowWidget_Dropdown::
    HandleSelectionChanged(
        FString InSelectedItem,
        ESelectInfo::Type InSelectionType)
    -> void
{
    if (_RefreshingControl || InSelectionType == ESelectInfo::Direct)
    { return; }

    const auto WorldContext = Get_WorldContext();

    if (ck::Is_NOT_Valid(WorldContext))
    { return; }

    auto Definition = FCk_GameSettings_SettingDefinition{};

    if (NOT UCk_Utils_GameSettings_UE::Get_SettingDefinition(WorldContext, Get_Key(), Definition))
    { return; }

    const auto& Options = Definition.Get_Options();
    const auto SelectedIndex = ck::IsValid(_ValueComboBox) ? _ValueComboBox->GetSelectedIndex() : INDEX_NONE;

    if (NOT Options.IsValidIndex(SelectedIndex))
    { return; }

    DoCommitOptionValue(Definition, Options[SelectedIndex].Get_Value());
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::game_settings_ui
{
    auto
        Get_EditDisposition(
            const FCk_GameSettings_EditConditions& InEditConditions,
            const FGameplayTagContainer& InPlatformTraits)
        -> EEditDisposition
    {
        const auto& RequiredTraits = InEditConditions.Get_RequiredPlatformTraits();

        if (RequiredTraits.IsEmpty())
        { return EEditDisposition::Editable; }

        if (InPlatformTraits.HasAll(RequiredTraits))
        { return EEditDisposition::Editable; }

        return InEditConditions.Get_DisabledReason().IsEmpty()
            ? EEditDisposition::Hidden
            : EEditDisposition::Disabled;
    }

    auto
        Get_EditDisposition(
            const FCk_GameSettings_EditConditions& InEditConditions)
        -> EEditDisposition
    {
        return Get_EditDisposition(InEditConditions, ICommonUIModule::GetSettings().GetPlatformTraits());
    }

    auto
        Get_ResolvedRowClass(
            const FCk_GameSettings_SettingDefinition& InDefinition,
            const TMap<ECk_GameSettings_ValueType, TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>>& InRowClassOverrides,
            const TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>& InSelectRowClassOverride)
        -> TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>
    {
        if (ck::IsValid(InDefinition.Get_OptionalRowClassOverride()))
        { return InDefinition.Get_OptionalRowClassOverride(); }

        if (InDefinition.Get_Options().Num() > 0)
        {
            if (ck::IsValid(InSelectRowClassOverride))
            { return InSelectRowClassOverride; }

            return TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{UCk_GameSettingsUI_RowWidget_Select::StaticClass()};
        }

        if (const auto FoundOverride = InRowClassOverrides.Find(InDefinition.Get_ValueType());
            ck::IsValid(FoundOverride, ck::IsValid_Policy_NullptrOnly{}) && ck::IsValid(*FoundOverride))
        { return *FoundOverride; }

        switch (InDefinition.Get_ValueType())
        {
            case ECk_GameSettings_ValueType::Bool:
            {
                return TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{UCk_GameSettingsUI_RowWidget_Toggle::StaticClass()};
            }
            case ECk_GameSettings_ValueType::Int32:
            case ECk_GameSettings_ValueType::Float:
            {
                const auto HasFullRange = NOT InDefinition.Get_MinValue().IsEmpty() && NOT InDefinition.Get_MaxValue().IsEmpty();

                return HasFullRange
                    ? TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{UCk_GameSettingsUI_RowWidget_Slider::StaticClass()}
                    : TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{UCk_GameSettingsUI_RowWidget_Select::StaticClass()};
            }
            case ECk_GameSettings_ValueType::String:
            {
                return TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{UCk_GameSettingsUI_RowWidget_Select::StaticClass()};
            }
        }

        return TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{UCk_GameSettingsUI_RowWidget_Select::StaticClass()};
    }

    auto
        Get_ResolvedRowClass(
            const FCk_GameSettings_SettingDefinition& InDefinition)
        -> TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>
    {
        return Get_ResolvedRowClass(InDefinition,
            UCk_Utils_GameSettings_Settings_UE::Get_RowClassOverrides(),
            UCk_Utils_GameSettings_Settings_UE::Get_SelectRowClassOverride());
    }
}

// --------------------------------------------------------------------------------------------------------------------
