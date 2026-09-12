#include "CkSlateLayout/CkUiInt32Input.h"

#include "CkSlateLayout/CkUiTextInput.h"

#include <charconv>

namespace ck_ui_int32_input
{
    struct FConfiguration
    {
        TAttribute<int32> Value;
        TAttribute<FText> Placeholder;
        TAttribute<FText> Error;
        TAttribute<bool> Enabled;
        TAttribute<bool> ReadOnly;
        TAttribute<bool> CanDispatchEvents;
        FCkUiOnIntegerCommitted Committed;
        FString ValueBindingName;
        int32 Min = TNumericLimits<int32>::Lowest();
        int32 Max = TNumericLimits<int32>::Max();
    };

    auto TryParseExactInt32(const FText& InText, int32& OutValue) -> bool
    {
        const FString Value = InText.ToString().TrimStartAndEnd();
        if (Value.IsEmpty()) { return false; }

        TArray<char, TInlineAllocator<16>> Ascii;
        for (const TCHAR Character : Value)
        {
            if ((Character < TEXT('0') || Character > TEXT('9')) && Character != TEXT('-')) { return false; }
            Ascii.Add(static_cast<char>(Character));
        }

        const char* Begin = Ascii.GetData();
        const char* End = Begin + Ascii.Num();
        if (*Begin == '-')
        {
            ++Begin;
            if (Begin == End) { return false; }
        }

        int64 Parsed = 0;
        const std::from_chars_result Result = std::from_chars(Ascii.GetData(), End, Parsed);
        if (Result.ec != std::errc{} || Result.ptr != End
            || Parsed < TNumericLimits<int32>::Lowest() || Parsed > TNumericLimits<int32>::Max())
        {
            return false;
        }

        OutValue = static_cast<int32>(Parsed);
        return true;
    }

    auto ParseFiniteDouble(const FText& InText, double& OutValue) -> bool
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

        double Parsed = 0.0;
        const std::from_chars_result Result = std::from_chars(Begin, End, Parsed, std::chars_format::general);
        if (Result.ec != std::errc{} || Result.ptr != End || !FMath::IsFinite(Parsed)) { return false; }
        OutValue = Parsed;
        return true;
    }

    class FInt32Input final : public ICkUiRetainedWidget, public TSharedFromThis<FInt32Input>
    {
    public:
        explicit FInt32Input(FConfiguration InConfiguration)
            : _Configuration(MoveTemp(InConfiguration))
        {
        }

        auto Initialize(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) -> bool
        {
            _Inner = FCkUiTextInput::Create(MakeTextInputArguments(_Configuration, InArguments.Id, InArguments.BaseFont), OutFailure);
            return _Inner.IsValid();
        }

        virtual ~FInt32Input() override { _Active = false; }

        virtual auto GetWidget() const -> TSharedRef<SWidget> override { return _Inner->GetWidget(); }
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
            FPreparedUpdate(TSharedRef<FInt32Input> InInput, FConfiguration InConfiguration, TUniquePtr<ICkUiPreparedWidgetUpdate> InChildUpdate)
                : _Input(MoveTemp(InInput)), _Configuration(MoveTemp(InConfiguration)), _ChildUpdate(MoveTemp(InChildUpdate)) {}
            virtual void Commit() noexcept override
            {
                _Input->_Configuration = MoveTemp(_Configuration);
                _ChildUpdate->Commit();
            }
        private:
            TSharedRef<FInt32Input> _Input;
            FConfiguration _Configuration;
            TUniquePtr<ICkUiPreparedWidgetUpdate> _ChildUpdate;
        };

        auto MakeTextInputArguments(const FConfiguration& InConfiguration, const FString& InId, const FSlateFontInfo& InBaseFont) const -> FCkUiCustomWidgetArguments
        {
            const TWeakPtr<FInt32Input> WeakInput = ConstCastSharedRef<FInt32Input>(AsShared());
            auto Arguments = FCkUiCustomWidgetArguments{};
            Arguments.Id = InId;
            Arguments.BaseFont = InBaseFont;
            Arguments.BindingNames.Add(TEXT("value"), InConfiguration.ValueBindingName);
            Arguments.TextProperties.Add(TEXT("placeholder"), InConfiguration.Placeholder.Get(FText::GetEmpty()));
            Arguments.TextBindings.Add(TEXT("value"), TAttribute<FText>::CreateLambda([WeakInput]()
            {
                const TSharedPtr<FInt32Input> Input = WeakInput.Pin();
                return Input.IsValid() ? FText::FromString(FString::FromInt(Input->_Configuration.Value.Get(0))) : FText::GetEmpty();
            }));
            Arguments.TextBindings.Add(TEXT("error"), TAttribute<FText>::CreateLambda([WeakInput]()
            {
                const TSharedPtr<FInt32Input> Input = WeakInput.Pin();
                if (!Input.IsValid()) { return FText::GetEmpty(); }
                const FText ExternalError = Input->_Configuration.Error.Get(FText::GetEmpty());
                return Input->_LocalError.IsEmpty() ? ExternalError
                    : ExternalError.IsEmpty() ? FText::FromString(Input->_LocalError)
                    : FText::FromString(ExternalError.ToString() + TEXT("\n") + Input->_LocalError);
            }));
            Arguments.BoolBindings.Add(TEXT("enabled"), TAttribute<bool>::CreateLambda([WeakInput]()
            {
                const TSharedPtr<FInt32Input> Input = WeakInput.Pin();
                return Input.IsValid() && Input->_Configuration.Enabled.Get(true);
            }));
            Arguments.BoolBindings.Add(TEXT("read-only"), TAttribute<bool>::CreateLambda([WeakInput]()
            {
                const TSharedPtr<FInt32Input> Input = WeakInput.Pin();
                return !Input.IsValid() || Input->_Configuration.ReadOnly.Get(false);
            }));
            Arguments.CanDispatchEvents = TAttribute<bool>::CreateLambda([WeakInput]()
            {
                const TSharedPtr<FInt32Input> Input = WeakInput.Pin();
                return Input.IsValid() && Input->CanDispatch();
            });
            Arguments.TextCommitted.Add(TEXT("committed"), FOnTextCommitted::CreateLambda([WeakInput](const FText& InText, const ETextCommit::Type InReason)
            {
                if (const TSharedPtr<FInt32Input> Input = WeakInput.Pin()) { Input->OnTextCommitted(InText, InReason); }
            }));
            return Arguments;
        }

        auto CanDispatch() const -> bool
        {
            return _Active && _Configuration.CanDispatchEvents.Get(false)
                && _Configuration.Enabled.Get(true) && !_Configuration.ReadOnly.Get(false);
        }

        auto OnTextCommitted(const FText& InText, const ETextCommit::Type InReason) -> void
        {
            if (!CanDispatch()) { return; }
            if (InReason != ETextCommit::OnEnter && InReason != ETextCommit::OnUserMovedFocus) { return; }
            double Parsed = 0.0;
            if (!ParseFiniteDouble(InText, Parsed))
            {
                _LocalError = TEXT("Enter a finite number.");
                return;
            }

            const double Rounded = FMath::RoundToDouble(Parsed);
            const double Clamped = FMath::Clamp(Rounded, static_cast<double>(_Configuration.Min), static_cast<double>(_Configuration.Max));
            _LocalError.Reset();
            const FCkUiOnIntegerCommitted Committed = _Configuration.Committed;
            Committed.ExecuteIfBound(static_cast<int32>(Clamped), InReason);
        }

        TSharedPtr<ICkUiRetainedWidget> _Inner;
        FConfiguration _Configuration;
        FString _LocalError;
        bool _Active = true;
    };

    auto MakeConfiguration(const FCkUiCustomWidgetArguments& InArguments, FConfiguration& OutConfiguration, FString& OutFailure) -> bool
    {
        const TAttribute<int32>* Value = InArguments.IntegerBindings.Find(TEXT("value"));
        const FCkUiOnIntegerCommitted* Committed = InArguments.IntegerCommitted.Find(TEXT("committed"));
        const FString* ValueBindingName = InArguments.BindingNames.Find(TEXT("value"));
        if (Value == nullptr || !Value->IsSet() || Committed == nullptr || !Committed->IsBound()
            || ValueBindingName == nullptr || ValueBindingName->IsEmpty())
        {
            OutFailure = TEXT("Int32 input requires value and committed bindings.");
            return false;
        }

        OutConfiguration.Value = *Value;
        OutConfiguration.Placeholder = InArguments.TextProperties.FindRef(TEXT("placeholder"));
        OutConfiguration.Error = InArguments.TextBindings.FindRef(TEXT("error"));
        OutConfiguration.Enabled = InArguments.BoolBindings.FindRef(TEXT("enabled"));
        OutConfiguration.ReadOnly = InArguments.BoolBindings.FindRef(TEXT("read-only"));
        OutConfiguration.CanDispatchEvents = InArguments.CanDispatchEvents;
        OutConfiguration.Committed = *Committed;
        OutConfiguration.ValueBindingName = *ValueBindingName;

        const FText MinText = InArguments.TextProperties.Contains(TEXT("min"))
            ? InArguments.TextProperties.FindRef(TEXT("min")) : FText::FromString(FString::FromInt(TNumericLimits<int32>::Lowest()));
        const FText MaxText = InArguments.TextProperties.Contains(TEXT("max"))
            ? InArguments.TextProperties.FindRef(TEXT("max")) : FText::FromString(FString::FromInt(TNumericLimits<int32>::Max()));
        if (!TryParseExactInt32(MinText, OutConfiguration.Min) || !TryParseExactInt32(MaxText, OutConfiguration.Max)
            || OutConfiguration.Min > OutConfiguration.Max)
        {
            OutFailure = TEXT("Int32 input min and max must be exact int32 decimal literals with min less than or equal to max.");
            return false;
        }
        return true;
    }

    auto FInt32Input::PrepareReload(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) const -> TUniquePtr<ICkUiPreparedWidgetUpdate>
    {
        auto Configuration = FConfiguration{};
        if (!MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
        if (Configuration.ValueBindingName != _Configuration.ValueBindingName)
        {
            OutFailure = TEXT("Int32 input value binding cannot change for a retained id.");
            return {};
        }
        if (!_Inner.IsValid())
        {
            OutFailure = TEXT("Int32 input has no retained text input.");
            return {};
        }

        TUniquePtr<ICkUiPreparedWidgetUpdate> ChildUpdate = _Inner->PrepareReload(
            MakeTextInputArguments(Configuration, InArguments.Id, InArguments.BaseFont), OutFailure);
        if (!ChildUpdate.IsValid()) { return {}; }
        return MakeUnique<FPreparedUpdate>(ConstCastSharedRef<FInt32Input>(AsShared()), MoveTemp(Configuration), MoveTemp(ChildUpdate));
    }
}

auto FCkUiInt32Input::Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult
{
    auto Registration = FCkUiCustomWidgetRegistration{};
    Registration.Schema.Tag = TEXT("int32-input");
    Registration.Schema.Properties = {
        {TEXT("value"), ECkUiCustomPropertyKind::IntegerBinding, true},
        {TEXT("committed"), ECkUiCustomPropertyKind::IntegerCommitted, true},
        {TEXT("min"), ECkUiCustomPropertyKind::Text, false},
        {TEXT("max"), ECkUiCustomPropertyKind::Text, false},
        {TEXT("placeholder"), ECkUiCustomPropertyKind::Text, false},
        {TEXT("enabled"), ECkUiCustomPropertyKind::BoolBinding, false},
        {TEXT("read-only"), ECkUiCustomPropertyKind::BoolBinding, false},
        {TEXT("error"), ECkUiCustomPropertyKind::TextBinding, false},
    };
    Registration.RetainedFactory = [](const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) -> TSharedPtr<ICkUiRetainedWidget>
    {
        auto Configuration = ck_ui_int32_input::FConfiguration{};
        if (!ck_ui_int32_input::MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
        const TSharedRef<ck_ui_int32_input::FInt32Input> Input = MakeShared<ck_ui_int32_input::FInt32Input>(MoveTemp(Configuration));
        if (!Input->Initialize(InArguments, OutFailure)) { return {}; }
        return Input;
    };
    return InRegistry.Register(MoveTemp(Registration));
}
