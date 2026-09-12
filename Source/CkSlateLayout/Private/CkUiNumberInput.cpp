#include "CkSlateLayout/CkUiNumberInput.h"

#include "CkSlateLayout/CkUiTextInput.h"

#include <charconv>

namespace ck_ui_number_input
{
    struct FConfiguration
    {
        TAttribute<float> Value;
        TAttribute<FText> Placeholder;
        TAttribute<FText> Error;
        TAttribute<bool> Enabled;
        TAttribute<bool> ReadOnly;
        TAttribute<bool> CanDispatchEvents;
        FCkUiOnNumberChanged Changed;
        FCkUiOnNumberCommitted Committed;
        FString ValueBindingName;
        FString Kind;
        float Min = -TNumericLimits<float>::Max();
        float Max = TNumericLimits<float>::Max();
        int32 FractionalDigits = -1;
    };

    class FNumberInput final : public ICkUiRetainedWidget, public TSharedFromThis<FNumberInput>
    {
    public:
        explicit FNumberInput(FConfiguration InConfiguration)
            : _Configuration(MoveTemp(InConfiguration))
        {
        }

        auto Initialize(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) -> bool
        {
            const FCkUiCustomWidgetArguments TextInputArguments = MakeTextInputArguments(_Configuration, InArguments.Id, InArguments.BaseFont);
            _Inner = FCkUiTextInput::Create(TextInputArguments, OutFailure);
            return _Inner.IsValid();
        }

        virtual ~FNumberInput() override
        {
            _Active = false;
        }

        virtual auto GetWidget() const -> TSharedRef<SWidget> override
        {
            return _Inner->GetWidget();
        }

        virtual auto GetFocusTransferTarget() const -> TSharedPtr<SWidget> override
        {
            return _Inner.IsValid() ? _Inner->GetFocusTransferTarget() : nullptr;
        }

        virtual void BeginFocusTransfer() noexcept override
        {
            if (_Inner.IsValid()) { _Inner->BeginFocusTransfer(); }
        }

        virtual void EndFocusTransfer() noexcept override
        {
            if (_Inner.IsValid()) { _Inner->EndFocusTransfer(); }
        }

        virtual auto PrepareReload(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) const -> TUniquePtr<ICkUiPreparedWidgetUpdate> override;

    private:
        class FPreparedUpdate final : public ICkUiPreparedWidgetUpdate
        {
        public:
            FPreparedUpdate(TSharedRef<FNumberInput> InInput, FConfiguration InConfiguration, TUniquePtr<ICkUiPreparedWidgetUpdate> InChildUpdate)
                : _Input(MoveTemp(InInput))
                , _Configuration(MoveTemp(InConfiguration))
                , _ChildUpdate(MoveTemp(InChildUpdate))
            {
            }

            virtual void Commit() noexcept override
            {
                _Input->_Configuration = MoveTemp(_Configuration);
                _ChildUpdate->Commit();
            }

        private:
            TSharedRef<FNumberInput> _Input;
            FConfiguration _Configuration;
            TUniquePtr<ICkUiPreparedWidgetUpdate> _ChildUpdate;
        };

        auto MakeTextInputArguments(const FConfiguration& InConfiguration, const FString& InId, const FSlateFontInfo& InBaseFont) const -> FCkUiCustomWidgetArguments
        {
            const TWeakPtr<FNumberInput> WeakInput = ConstCastSharedRef<FNumberInput>(AsShared());
            auto Arguments = FCkUiCustomWidgetArguments{};
            Arguments.Id = InId;
            Arguments.BaseFont = InBaseFont;
            Arguments.BindingNames.Add(TEXT("value"), InConfiguration.ValueBindingName);
            Arguments.TextProperties.Add(TEXT("placeholder"), InConfiguration.Placeholder.Get(FText::GetEmpty()));
            Arguments.TextBindings.Add(TEXT("value"), TAttribute<FText>::CreateLambda([WeakInput]()
            {
                const TSharedPtr<FNumberInput> Input = WeakInput.Pin();
                return Input.IsValid() ? Input->GetDisplayText() : FText::GetEmpty();
            }));
            Arguments.TextBindings.Add(TEXT("error"), TAttribute<FText>::CreateLambda([WeakInput]()
            {
                const TSharedPtr<FNumberInput> Input = WeakInput.Pin();
                return Input.IsValid() ? Input->GetPresentationError() : FText::GetEmpty();
            }));
            Arguments.BoolBindings.Add(TEXT("enabled"), TAttribute<bool>::CreateLambda([WeakInput]()
            {
                const TSharedPtr<FNumberInput> Input = WeakInput.Pin();
                return Input.IsValid() && Input->_Configuration.Enabled.Get(true);
            }));
            Arguments.BoolBindings.Add(TEXT("read-only"), TAttribute<bool>::CreateLambda([WeakInput]()
            {
                const TSharedPtr<FNumberInput> Input = WeakInput.Pin();
                return !Input.IsValid() || Input->_Configuration.ReadOnly.Get(false);
            }));
            Arguments.CanDispatchEvents = TAttribute<bool>::CreateLambda([WeakInput]()
            {
                const TSharedPtr<FNumberInput> Input = WeakInput.Pin();
                return Input.IsValid() && Input->CanDispatch();
            });
            Arguments.TextChanged.Add(TEXT("changed"), FOnTextChanged::CreateLambda([WeakInput](const FText& InText)
            {
                const TSharedPtr<FNumberInput> Input = WeakInput.Pin();
                if (Input.IsValid()) { Input->OnTextChanged(InText); }
            }));
            Arguments.TextCommitted.Add(TEXT("committed"), FOnTextCommitted::CreateLambda([WeakInput](const FText& InText, const ETextCommit::Type InReason)
            {
                const TSharedPtr<FNumberInput> Input = WeakInput.Pin();
                if (Input.IsValid()) { Input->OnTextCommitted(InText, InReason); }
            }));
            return Arguments;
        }

        auto ParseFiniteNumber(const FText& InText, float& OutValue) const -> bool
        {
            const FString Value = InText.ToString().TrimStartAndEnd();
            if (Value.IsEmpty()) { return false; }
            TArray<char, TInlineAllocator<64>> Ascii;
            for (const TCHAR Character : Value)
            {
                const bool IsAsciiNumberCharacter = (Character >= TEXT('0') && Character <= TEXT('9'))
                    || Character == TEXT('+') || Character == TEXT('-') || Character == TEXT('.')
                    || Character == TEXT('e') || Character == TEXT('E');
                if (!IsAsciiNumberCharacter) { return false; }
                Ascii.Add(static_cast<char>(Character));
            }
            const char* Begin = Ascii.GetData();
            const char* End = Begin + Ascii.Num();
            // from_chars deliberately excludes a leading plus sign.
            if (*Begin == '+')
            {
                ++Begin;
                if (Begin != End && *Begin == '-') { return false; }
            }
            if (Begin == End) { return false; }
            float Parsed = 0.0f;
            const std::from_chars_result Result = std::from_chars(Begin, End, Parsed, std::chars_format::general);
            if (Result.ec != std::errc{} || Result.ptr != End || !FMath::IsFinite(Parsed)) { return false; }
            OutValue = Parsed;
            return true;
        }

        auto GetDisplayText() const -> FText
        {
            const float Value = _Configuration.Value.Get(0.0f);
            if (!FMath::IsFinite(Value)) { return FText::GetEmpty(); }
            return FText::FromString(_Configuration.FractionalDigits >= 0
                ? FString::Printf(TEXT("%.*f"), _Configuration.FractionalDigits, static_cast<double>(Value))
                : FString::Printf(TEXT("%.9g"), static_cast<double>(Value)));
        }

        auto GetPresentationError() const -> FText
        {
            const FText ExternalError = _Configuration.Error.Get(FText::GetEmpty());
            FString LocalError = _LocalError;
            if (!FMath::IsFinite(_Configuration.Value.Get(0.0f)))
            {
                LocalError = LocalError.IsEmpty()
                    ? TEXT("Model value is not finite.")
                    : LocalError + TEXT("\nModel value is not finite.");
            }
            if (LocalError.IsEmpty()) { return ExternalError; }
            return ExternalError.IsEmpty()
                ? FText::FromString(LocalError)
                : FText::FromString(ExternalError.ToString() + TEXT("\n") + LocalError);
        }

        auto CanDispatch() const -> bool
        {
            return _Active && _Configuration.CanDispatchEvents.Get(false)
                && _Configuration.Enabled.Get(true) && !_Configuration.ReadOnly.Get(false);
        }

        auto OnTextChanged(const FText& InText) -> void
        {
            float Value = 0.0f;
            if (!CanDispatch() || !ParseFiniteNumber(InText, Value)) { return; }
            const FCkUiOnNumberChanged Changed = _Configuration.Changed;
            Changed.ExecuteIfBound(Value);
        }

        auto OnTextCommitted(const FText& InText, const ETextCommit::Type InReason) -> void
        {
            if (!CanDispatch()) { return; }
            float Value = 0.0f;
            if (!ParseFiniteNumber(InText, Value))
            {
                _LocalError = TEXT("Enter a finite number.");
                return;
            }
            if (_Configuration.Kind == TEXT("integer"))
            {
                const float IntegerMin = FMath::CeilToFloat(_Configuration.Min);
                const float IntegerMax = FMath::FloorToFloat(_Configuration.Max);
                Value = FMath::Clamp(FMath::RoundToFloat(Value), IntegerMin, IntegerMax);
            }
            else
            {
                Value = FMath::Clamp(Value, _Configuration.Min, _Configuration.Max);
            }
            _LocalError.Reset();
            const FCkUiOnNumberCommitted Committed = _Configuration.Committed;
            Committed.ExecuteIfBound(Value, InReason);
        }

        TSharedPtr<ICkUiRetainedWidget> _Inner;
        FConfiguration _Configuration;
        FString _LocalError;
        bool _Active = true;
    };

    auto MakeConfiguration(const FCkUiCustomWidgetArguments& InArguments, FConfiguration& OutConfiguration, FString& OutFailure) -> bool
    {
        const TAttribute<float>* Value = InArguments.NumberBindings.Find(TEXT("value"));
        const FCkUiOnNumberCommitted* Committed = InArguments.NumberCommitted.Find(TEXT("committed"));
        const FString* ValueBindingName = InArguments.BindingNames.Find(TEXT("value"));
        if (Value == nullptr || !Value->IsSet() || Committed == nullptr || !Committed->IsBound()
            || ValueBindingName == nullptr || ValueBindingName->IsEmpty())
        {
            OutFailure = TEXT("Number input requires value and committed bindings.");
            return false;
        }

        OutConfiguration.Value = *Value;
        OutConfiguration.Placeholder = InArguments.TextProperties.FindRef(TEXT("placeholder"));
        OutConfiguration.Error = InArguments.TextBindings.FindRef(TEXT("error"));
        OutConfiguration.Enabled = InArguments.BoolBindings.FindRef(TEXT("enabled"));
        OutConfiguration.ReadOnly = InArguments.BoolBindings.FindRef(TEXT("read-only"));
        OutConfiguration.CanDispatchEvents = InArguments.CanDispatchEvents;
        OutConfiguration.Changed = InArguments.NumberChanged.FindRef(TEXT("changed"));
        OutConfiguration.Committed = *Committed;
        OutConfiguration.ValueBindingName = *ValueBindingName;
        OutConfiguration.Kind = InArguments.TextProperties.FindRef(TEXT("kind")).ToString();
        if (OutConfiguration.Kind.IsEmpty()) { OutConfiguration.Kind = TEXT("float"); }
        OutConfiguration.Min = InArguments.NumberProperties.Contains(TEXT("min"))
            ? InArguments.NumberProperties.FindRef(TEXT("min"))
            : -TNumericLimits<float>::Max();
        OutConfiguration.Max = InArguments.NumberProperties.Contains(TEXT("max"))
            ? InArguments.NumberProperties.FindRef(TEXT("max"))
            : TNumericLimits<float>::Max();

        const bool IsInteger = OutConfiguration.Kind == TEXT("integer");
        const bool HasFeasibleIntegerRange = !IsInteger
            || FMath::CeilToFloat(OutConfiguration.Min) <= FMath::FloorToFloat(OutConfiguration.Max);
        if ((OutConfiguration.Kind != TEXT("float") && !IsInteger)
            || !FMath::IsFinite(OutConfiguration.Min) || !FMath::IsFinite(OutConfiguration.Max)
            || OutConfiguration.Min > OutConfiguration.Max || !HasFeasibleIntegerRange)
        {
            OutFailure = TEXT("Number input configuration is invalid.");
            return false;
        }

        if (const float* FractionalDigits = InArguments.NumberProperties.Find(TEXT("fractional-digits")); FractionalDigits != nullptr)
        {
            const bool IsFiniteDigitsValue = FMath::IsFinite(*FractionalDigits);
            const bool IsWholeDigitsValue = IsFiniteDigitsValue && FMath::FloorToFloat(*FractionalDigits) == *FractionalDigits;
            if (!IsFiniteDigitsValue || !IsWholeDigitsValue || *FractionalDigits < 0.0f || *FractionalDigits > 6.0f
                || (IsInteger && *FractionalDigits != 0.0f))
            {
                OutFailure = TEXT("Number input fractional-digits must be an integer from 0 to 6; integer inputs only allow 0.");
                return false;
            }
            OutConfiguration.FractionalDigits = static_cast<int32>(*FractionalDigits);
        }
        return true;
    }

    auto FNumberInput::PrepareReload(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) const -> TUniquePtr<ICkUiPreparedWidgetUpdate>
    {
        auto Configuration = FConfiguration{};
        if (!MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
        if (Configuration.ValueBindingName != _Configuration.ValueBindingName || Configuration.Kind != _Configuration.Kind)
        {
            OutFailure = TEXT("Number input value binding and kind cannot change for a retained id.");
            return {};
        }
        if (!_Inner.IsValid())
        {
            OutFailure = TEXT("Number input has no retained text input.");
            return {};
        }

        const FCkUiCustomWidgetArguments TextInputArguments = MakeTextInputArguments(Configuration, InArguments.Id, InArguments.BaseFont);
        TUniquePtr<ICkUiPreparedWidgetUpdate> ChildUpdate = _Inner->PrepareReload(TextInputArguments, OutFailure);
        if (!ChildUpdate.IsValid()) { return {}; }
        return MakeUnique<FPreparedUpdate>(
            ConstCastSharedRef<FNumberInput>(AsShared()),
            MoveTemp(Configuration),
            MoveTemp(ChildUpdate));
    }
}

auto FCkUiNumberInput::Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult
{
    auto Registration = FCkUiCustomWidgetRegistration{};
    Registration.Schema.Tag = TEXT("number-input");
    Registration.Schema.Properties = {
        {TEXT("value"), ECkUiCustomPropertyKind::NumberBinding, true},
        {TEXT("committed"), ECkUiCustomPropertyKind::NumberCommitted, true},
        {TEXT("changed"), ECkUiCustomPropertyKind::NumberChanged, false},
        {TEXT("kind"), ECkUiCustomPropertyKind::Text, false},
        {TEXT("min"), ECkUiCustomPropertyKind::Number, false},
        {TEXT("max"), ECkUiCustomPropertyKind::Number, false},
        {TEXT("fractional-digits"), ECkUiCustomPropertyKind::Number, false},
        {TEXT("placeholder"), ECkUiCustomPropertyKind::Text, false},
        {TEXT("enabled"), ECkUiCustomPropertyKind::BoolBinding, false},
        {TEXT("read-only"), ECkUiCustomPropertyKind::BoolBinding, false},
        {TEXT("error"), ECkUiCustomPropertyKind::TextBinding, false},
    };
    Registration.RetainedFactory = [](const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) -> TSharedPtr<ICkUiRetainedWidget>
    {
        auto Configuration = ck_ui_number_input::FConfiguration{};
        if (!ck_ui_number_input::MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
        const TSharedRef<ck_ui_number_input::FNumberInput> Input = MakeShared<ck_ui_number_input::FNumberInput>(MoveTemp(Configuration));
        if (!Input->Initialize(InArguments, OutFailure)) { return {}; }
        return Input;
    };
    return InRegistry.Register(MoveTemp(Registration));
}
