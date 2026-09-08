#include "CkSlateLayout/CkUiTextInput.h"

#include "InputCoreTypes.h"
#include "Widgets/Input/SEditableTextBox.h"

namespace ck_ui_text_input
{
    struct FConfiguration
    {
        TAttribute<FText> Value;
        TAttribute<FText> Placeholder;
        TAttribute<bool> ReadOnly;
        TAttribute<bool> Enabled;
        TAttribute<FText> Error;
        TAttribute<bool> CanDispatchEvents;
        FOnTextChanged Changed;
        FOnTextCommitted Committed;
        FString ValueBindingName;
    };

    class SCkUiTextInputBox final : public SEditableTextBox
    {
    public:
        SLATE_BEGIN_ARGS(SCkUiTextInputBox) {}
            SLATE_ATTRIBUTE(FText, Text)
            SLATE_ATTRIBUTE(FText, HintText)
            SLATE_ATTRIBUTE(bool, IsReadOnly)
            SLATE_ATTRIBUTE(bool, IsCaretMovedWhenGainFocus)
            SLATE_ATTRIBUTE(FSlateFontInfo, Font)
            SLATE_ATTRIBUTE(bool, RevertTextOnEscape)
            SLATE_EVENT(FOnTextChanged, OnTextChanged)
            SLATE_EVENT(FOnTextCommitted, OnTextCommitted)
            SLATE_EVENT(FOnKeyDown, OnKeyDownHandler)
        SLATE_END_ARGS()

        auto Construct(const FArguments& InArgs) -> void
        {
            SEditableTextBox::Construct(SEditableTextBox::FArguments()
                .Text(InArgs._Text)
                .HintText(InArgs._HintText)
                .IsReadOnly(InArgs._IsReadOnly)
                .IsEnabled(InArgs._IsEnabled)
                .IsCaretMovedWhenGainFocus(InArgs._IsCaretMovedWhenGainFocus)
                .Font(InArgs._Font)
                .RevertTextOnEscape(InArgs._RevertTextOnEscape)
                .OnTextChanged(InArgs._OnTextChanged)
                .OnTextCommitted(InArgs._OnTextCommitted)
                .OnKeyDownHandler(InArgs._OnKeyDownHandler));
        }

        auto SetErrorUpdateCallback(TFunction<void()> InCallback) -> void
        {
            _ErrorUpdateCallback = MoveTemp(InCallback);
        }

        virtual void Tick(const FGeometry& InAllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
        {
            SEditableTextBox::Tick(InAllottedGeometry, InCurrentTime, InDeltaTime);
            if (_ErrorUpdateCallback) { _ErrorUpdateCallback(); }
        }

    private:
        TFunction<void()> _ErrorUpdateCallback;
    };

    class FTextInput final : public ICkUiRetainedWidget, public TSharedFromThis<FTextInput>
    {
    public:
        explicit FTextInput(FConfiguration InConfiguration)
            : _Configuration(MoveTemp(InConfiguration))
        {
        }

        auto Initialize(const FString& InId, const FSlateFontInfo& InBaseFont) -> void
        {
            const TWeakPtr<FTextInput> WeakInput = AsShared();
            TSharedPtr<SCkUiTextInputBox> TextBox;
            SAssignNew(TextBox, SCkUiTextInputBox).Tag(FName(*InId))
                .Text(TAttribute<FText>::CreateLambda([WeakInput]()
                {
                    const TSharedPtr<FTextInput> Input = WeakInput.Pin();
                    return Input.IsValid() ? Input->GetDisplayText() : FText::GetEmpty();
                }))
                .HintText(TAttribute<FText>::CreateLambda([WeakInput]()
                {
                    const TSharedPtr<FTextInput> Input = WeakInput.Pin();
                    return Input.IsValid() ? Input->_Configuration.Placeholder.Get(FText::GetEmpty()) : FText::GetEmpty();
                }))
                .IsReadOnly(TAttribute<bool>::CreateLambda([WeakInput]()
                {
                    const TSharedPtr<FTextInput> Input = WeakInput.Pin();
                    return !Input.IsValid() || Input->_Configuration.ReadOnly.Get(false);
                }))
                .IsEnabled(TAttribute<bool>::CreateLambda([WeakInput]()
                {
                    const TSharedPtr<FTextInput> Input = WeakInput.Pin();
                    return Input.IsValid() && Input->_Configuration.Enabled.Get(true);
                }))
                .IsCaretMovedWhenGainFocus(TAttribute<bool>::CreateLambda([WeakInput]()
                {
                    const TSharedPtr<FTextInput> Input = WeakInput.Pin();
                    return Input.IsValid() && Input->CanDispatch();
                }))
                .Font(InBaseFont)
                .RevertTextOnEscape(false)
                .OnTextChanged_Lambda([WeakInput](const FText& InText)
                {
                    if (const TSharedPtr<FTextInput> Input = WeakInput.Pin()) { Input->OnTextChanged(InText); }
                })
                .OnTextCommitted_Lambda([WeakInput](const FText& InText, const ETextCommit::Type InReason)
                {
                    if (const TSharedPtr<FTextInput> Input = WeakInput.Pin()) { Input->OnTextCommitted(InText, InReason); }
                })
                .OnKeyDownHandler_Lambda([WeakInput](const FGeometry&, const FKeyEvent& InEvent)
                {
                    if (const TSharedPtr<FTextInput> Input = WeakInput.Pin()) { return Input->OnKeyDown(InEvent); }
                    return FReply::Unhandled();
                });
            _Widget = TextBox;
            UpdateErrorIfChanged();
            TextBox->SetErrorUpdateCallback([WeakInput]()
            {
                const TSharedPtr<FTextInput> Input = WeakInput.Pin();
                if (Input.IsValid() && Input->_Active) { Input->UpdateErrorIfChanged(); }
            });
        }

        virtual ~FTextInput() override
        {
            _Active = false;
        }

        virtual auto GetWidget() const -> TSharedRef<SWidget> override
        {
            return _Widget.ToSharedRef();
        }

        virtual auto PrepareReload(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) const -> TUniquePtr<ICkUiPreparedWidgetUpdate> override;

    private:
        class FPreparedUpdate final : public ICkUiPreparedWidgetUpdate
        {
        public:
            FPreparedUpdate(TSharedRef<FTextInput> InInput, FConfiguration InConfiguration)
                : _Input(MoveTemp(InInput))
                , _Configuration(MoveTemp(InConfiguration))
            {
            }

            virtual void Commit() noexcept override
            {
                _Input->ApplyConfiguration(MoveTemp(_Configuration));
            }

        private:
            TSharedRef<FTextInput> _Input;
            FConfiguration _Configuration;
        };

        auto GetDisplayText() const -> FText
        {
            return _Editing ? FText::FromString(_Draft) : _Configuration.Value.Get(FText::GetEmpty());
        }

        auto CanDispatch() const -> bool
        {
            return _Active && _Configuration.CanDispatchEvents.Get(false);
        }

        auto OnTextChanged(const FText& InText) -> void
        {
            if (_ApplyingInternalText || !CanDispatch()) { return; }
            const FString Text = InText.ToString();
            // Enter closes its native transaction after normalization. That model-equal echo is not a new draft.
            if (!_Editing && Text == _Configuration.Value.Get(FText::GetEmpty()).ToString()) { return; }
            _Editing = true;
            _Draft = Text;
            const FOnTextChanged Changed = _Configuration.Changed;
            Changed.ExecuteIfBound(InText);
        }

        auto OnTextCommitted(const FText& InText, const ETextCommit::Type InReason) -> void
        {
            if (_ApplyingInternalText || !CanDispatch()) { return; }
            // Enter is followed by native focus loss; only a new draft needs another commit.
            if (!_Editing && InReason != ETextCommit::OnEnter) { return; }
            const FOnTextCommitted Committed = _Configuration.Committed;
            _Editing = false;
            _Draft.Reset();
            Committed.ExecuteIfBound(InText, InReason);
            RefreshAuthoritativeText();
        }

        auto OnKeyDown(const FKeyEvent& InEvent) -> FReply
        {
            if (InEvent.GetKey() != EKeys::Escape || !CanDispatch()) { return FReply::Unhandled(); }
            _Editing = false;
            _Draft.Reset();
            RefreshAuthoritativeText();
            return FReply::Handled();
        }

        auto RefreshAuthoritativeText() -> void
        {
            if (!_Widget.IsValid()) { return; }
            TGuardValue<bool> Guard{_ApplyingInternalText, true};
            _Widget->SetText(TAttribute<FText>::CreateLambda([WeakInput = TWeakPtr<FTextInput>(AsShared())]()
            {
                const TSharedPtr<FTextInput> Input = WeakInput.Pin();
                return Input.IsValid() ? Input->GetDisplayText() : FText::GetEmpty();
            }));
        }

        auto UpdateErrorIfChanged() -> void
        {
            if (!_Widget.IsValid()) { return; }
            const FString Error = _Configuration.Error.Get(FText::GetEmpty()).ToString();
            // Slate clears its native error on commit even when the bound model error is unchanged.
            if (Error == _AppliedError && _Widget->HasError() == !Error.IsEmpty()) { return; }
            _AppliedError = Error;
            _Widget->SetError(Error);
        }

        auto ApplyConfiguration(FConfiguration InConfiguration) -> void
        {
            _Configuration = MoveTemp(InConfiguration);
        }

        TSharedPtr<SEditableTextBox> _Widget;
        FConfiguration _Configuration;
        FString _Draft;
        bool _Editing = false;
        bool _ApplyingInternalText = false;
        bool _Active = true;
        FString _AppliedError;
    };

    auto MakeConfiguration(const FCkUiCustomWidgetArguments& InArguments, FConfiguration& OutConfiguration, FString& OutFailure) -> bool
    {
        const TAttribute<FText>* Value = InArguments.TextBindings.Find(TEXT("value"));
        const FOnTextCommitted* Committed = InArguments.TextCommitted.Find(TEXT("committed"));
        const FString* ValueBindingName = InArguments.BindingNames.Find(TEXT("value"));
        if (Value == nullptr || !Value->IsSet() || Committed == nullptr || !Committed->IsBound() || ValueBindingName == nullptr || ValueBindingName->IsEmpty())
        {
            OutFailure = TEXT("Text input requires value and committed bindings.");
            return false;
        }
        OutConfiguration.Value = *Value;
        OutConfiguration.Placeholder = InArguments.TextProperties.FindRef(TEXT("placeholder"));
        OutConfiguration.ReadOnly = InArguments.BoolBindings.FindRef(TEXT("read-only"));
        OutConfiguration.Enabled = InArguments.BoolBindings.FindRef(TEXT("enabled"));
        OutConfiguration.Error = InArguments.TextBindings.FindRef(TEXT("error"));
        OutConfiguration.CanDispatchEvents = InArguments.CanDispatchEvents;
        OutConfiguration.Changed = InArguments.TextChanged.FindRef(TEXT("changed"));
        OutConfiguration.Committed = *Committed;
        OutConfiguration.ValueBindingName = *ValueBindingName;
        return true;
    }

    auto FTextInput::PrepareReload(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) const -> TUniquePtr<ICkUiPreparedWidgetUpdate>
    {
        auto Configuration = FConfiguration{};
        if (!MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
        if (Configuration.ValueBindingName != _Configuration.ValueBindingName)
        {
            OutFailure = TEXT("Text input value binding cannot change for a retained id.");
            return {};
        }
        return MakeUnique<FPreparedUpdate>(ConstCastSharedRef<FTextInput>(AsShared()), MoveTemp(Configuration));
    }
}

auto FCkUiTextInput::Create(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) -> TSharedPtr<ICkUiRetainedWidget>
{
    auto Configuration = ck_ui_text_input::FConfiguration{};
    if (!ck_ui_text_input::MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
    const TSharedRef<ck_ui_text_input::FTextInput> Input = MakeShared<ck_ui_text_input::FTextInput>(MoveTemp(Configuration));
    Input->Initialize(InArguments.Id, InArguments.BaseFont);
    return Input;
}

auto FCkUiTextInput::Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult
{
    auto Registration = FCkUiCustomWidgetRegistration{};
    Registration.Schema.Tag = TEXT("text-input");
    Registration.Schema.Properties = {
        {TEXT("value"), ECkUiCustomPropertyKind::TextBinding, true},
        {TEXT("committed"), ECkUiCustomPropertyKind::TextCommitted, true},
        {TEXT("changed"), ECkUiCustomPropertyKind::TextChanged, false},
        {TEXT("placeholder"), ECkUiCustomPropertyKind::Text, false},
        {TEXT("read-only"), ECkUiCustomPropertyKind::BoolBinding, false},
        {TEXT("enabled"), ECkUiCustomPropertyKind::BoolBinding, false},
        {TEXT("error"), ECkUiCustomPropertyKind::TextBinding, false},
    };
    Registration.RetainedFactory = &FCkUiTextInput::Create;
    return InRegistry.Register(MoveTemp(Registration));
}
