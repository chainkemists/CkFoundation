#include "CkSlateLayout/CkUiSelect.h"

#include "CkSlateLayout/CkUiCollection.h"

#include "Containers/ObservableArray.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"

namespace ck_ui_select
{
    struct FOption final
    {
        FString Key;
        FText Label;
    };
    using FOptionPtr = TSharedPtr<FOption>;
    using FOptionSource = UE::Slate::Containers::TObservableArray<FOptionPtr>;

    struct FConfiguration
    {
        TAttribute<FString> Value;
        TSharedPtr<FCkUiCollection> Options;
        TAttribute<FText> Placeholder;
        TAttribute<bool> Enabled;
        TAttribute<bool> ReadOnly;
        TAttribute<bool> CanDispatchEvents;
        FCkUiOnStringChanged Changed;
        FString ValueBindingName;
        FString OptionsBindingName;
    };

    class SCkUiSelect final : public SComboBox<FOptionPtr>
    {
    public:
        SLATE_BEGIN_ARGS(SCkUiSelect) {}
            SLATE_ATTRIBUTE(FText, DisplayText)
            SLATE_ATTRIBUTE(FSlateFontInfo, Font)
            SLATE_EVENT(TSlateDelegates<FOptionPtr>::FOnSelectionChanged, OnSelectionChanged)
        SLATE_END_ARGS()

        auto Construct(const FArguments& InArgs) -> void
        {
            SComboBox<FOptionPtr>::Construct(SComboBox<FOptionPtr>::FArguments()
                .ComboBoxStyle(&FCoreStyle::Get().GetWidgetStyle<FComboBoxStyle>(TEXT("ComboBox")))
                .ItemStyle(&FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>(TEXT("ComboBox.Row")))
                .OptionsSource(_Options)
                .OnSelectionChanged(InArgs._OnSelectionChanged)
                .OnGenerateWidget_Lambda([Font = InArgs._Font](FOptionPtr InOption)
                {
                    return SNew(STextBlock)
                        .Text_Lambda([InOption]() { return InOption.IsValid() ? InOption->Label : FText::GetEmpty(); })
                        .Font(Font);
                })
                .IsEnabled(InArgs._IsEnabled)
                [
                    SNew(STextBlock)
                    .Text(InArgs._DisplayText)
                    .Font(InArgs._Font)
                ]);
        }

        auto ReplaceOptions(const TArray<TSharedPtr<const FCkUiRecord>>& InRecords) -> void
        {
            auto Next = TArray<FOptionPtr>{};
            Next.Reserve(InRecords.Num());
            for (const TSharedPtr<const FCkUiRecord>& Record : InRecords)
            {
                if (!Record.IsValid()) { continue; }
                const FCkUiFieldValue* Label = Record->FindField(TEXT("label"));
                if (Label == nullptr || Label->Kind != ECkUiFieldKind::Text) { continue; }
                FOptionPtr Option;
                for (const FOptionPtr& Existing : *_Options)
                {
                    if (Existing.IsValid() && Existing->Key.Equals(Record->GetKey(), ESearchCase::CaseSensitive))
                    { Option = Existing; break; }
                }
                if (!Option.IsValid())
                {
                    Option = MakeShared<FOption>();
                    Option->Key = Record->GetKey();
                }
                Option->Label = Label->Text;
                Next.Add(MoveTemp(Option));
            }
            TGuardValue<bool> SelectionGuard(_ApplyingSelection, true);
            _Options->Reset(Next.Num());
            _Options->Append(MoveTemp(Next));
        }

        auto FindLabel(const FString& InKey) const -> FText
        {
            for (const FOptionPtr& Option : *_Options)
            {
                if (Option.IsValid() && Option->Key.Equals(InKey, ESearchCase::CaseSensitive)) { return Option->Label; }
            }
            return FText::GetEmpty();
        }

        auto SetSynchronizeCallback(TFunction<void()> InCallback) -> void
        {
            _SynchronizeCallback = MoveTemp(InCallback);
        }

        auto SetCanEditCallback(TFunction<bool()> InCallback) -> void
        {
            _CanEditCallback = MoveTemp(InCallback);
        }

        auto ApplySelection(const FString& InSelectedKey) -> void
        {
            const FOptionPtr Target = FindOption(InSelectedKey);
            const FOptionPtr Current = GetSelectedItem();
            if (Current == Target) { return; }
            TGuardValue<bool> Guard(_ApplyingSelection, true);
            SetSelectedItem(Target);
        }

        auto GetFocusTransferTarget() const -> TSharedPtr<SWidget> { return IsOpen() ? MenuContent : nullptr; }

        auto ReleasePopup() -> void
        {
            if (FSlateApplication::IsInitialized())
            {
                FSlateApplication& Slate = FSlateApplication::Get();
                Slate.ForEachUser([this, &Slate](FSlateUser& User)
                {
                    // Closing a detached anchor cannot restore focus to it.
                    if (MenuContent.IsValid() && User.IsWidgetInFocusPath(MenuContent))
                    {
                        Slate.ClearUserFocus(User.GetUserIndex(), EFocusCause::SetDirectly);
                    }
                }, true);
            }
            if (IsOpen()) { SetIsOpen(false); }
        }

        auto IsApplyingSelection() const -> bool { return _ApplyingSelection; }
        auto IsProcessingInput() const -> bool { return _ProcessingInput; }

        virtual void SetIsOpen(bool InIsOpen, const bool InFocusMenu = true, const int32 InFocusUserIndex = 0) override
        {
            // Native selection synchronization closes menus; live model updates preserve an open menu.
            if (!InIsOpen && _ApplyingSelection) { return; }
            SComboBox<FOptionPtr>::SetIsOpen(InIsOpen, InFocusMenu, InFocusUserIndex);
        }
        virtual void Tick(const FGeometry& InAllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
        {
            SComboBox<FOptionPtr>::Tick(InAllottedGeometry, InCurrentTime, InDeltaTime);
            if (_SynchronizeCallback) { _SynchronizeCallback(); }
        }

        virtual FReply OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InEvent) override
        {
            if (!_CanEditCallback || !_CanEditCallback()) { return FReply::Unhandled(); }
            TGuardValue<bool> InputGuard(_ProcessingInput, true);
            return SComboBox<FOptionPtr>::OnKeyDown(InGeometry, InEvent);
        }

        virtual FReply OnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InEvent) override
        {
            return !_CanEditCallback || !_CanEditCallback()
                ? FReply::Unhandled()
                : SComboBox<FOptionPtr>::OnMouseButtonDown(InGeometry, InEvent);
        }

    private:
        auto FindOption(const FString& InKey) const -> FOptionPtr
        {
            for (const FOptionPtr& Option : *_Options)
            {
                if (Option.IsValid() && Option->Key.Equals(InKey, ESearchCase::CaseSensitive)) { return Option; }
            }
            return {};
        }

        TSharedRef<FOptionSource> _Options = MakeShared<FOptionSource>();
        TFunction<void()> _SynchronizeCallback;
        TFunction<bool()> _CanEditCallback;
        bool _ApplyingSelection = false;
        bool _ProcessingInput = false;
    };

    class FSelect final : public ICkUiRetainedWidget, public TSharedFromThis<FSelect>
    {
    public:
        explicit FSelect(FConfiguration InConfiguration)
            : _Configuration(MoveTemp(InConfiguration))
        {
        }

        auto Initialize(const FString& InId, const FSlateFontInfo& InBaseFont) -> void
        {
            const TWeakPtr<FSelect> WeakSelect = AsShared();
            TSharedPtr<SCkUiSelect> Select;
            SAssignNew(Select, SCkUiSelect).Tag(FName(*InId))
                .DisplayText(TAttribute<FText>::CreateLambda([WeakSelect]()
                {
                    const TSharedPtr<FSelect> Control = WeakSelect.Pin();
                    return Control.IsValid() ? Control->GetDisplayText() : FText::GetEmpty();
                }))
                .IsEnabled(TAttribute<bool>::CreateLambda([WeakSelect]()
                {
                    const TSharedPtr<FSelect> Control = WeakSelect.Pin();
                    return Control.IsValid() && Control->CanDispatch();
                }))
                .Font(InBaseFont)
                .OnSelectionChanged_Lambda([WeakSelect](FOptionPtr InOption, const ESelectInfo::Type InInfo)
                {
                    if (const TSharedPtr<FSelect> Control = WeakSelect.Pin()) { Control->OnSelectionChanged(MoveTemp(InOption), InInfo); }
                });
            _Widget = Select;
            BindCollection();
            RefreshOptions();
            SynchronizeSelection();
            Select->SetSynchronizeCallback([WeakSelect]()
            {
                if (const TSharedPtr<FSelect> Control = WeakSelect.Pin()) { Control->SynchronizeFromTick(); }
            });
            Select->SetCanEditCallback([WeakSelect]()
            {
                const TSharedPtr<FSelect> Control = WeakSelect.Pin();
                return Control.IsValid() && Control->CanDispatch();
            });
        }

        virtual ~FSelect() override
        {
            _Active = false;
            if (_Widget.IsValid()) { _Widget->ReleasePopup(); }
            UnbindCollection();
        }

        virtual auto GetWidget() const -> TSharedRef<SWidget> override
        {
            return _Widget.ToSharedRef();
        }

        virtual auto GetFocusTransferTarget() const -> TSharedPtr<SWidget> override
        { return _Widget->GetFocusTransferTarget(); }

        virtual void ReleaseTransientInteraction() override { _Widget->ReleasePopup(); }

        virtual auto PrepareReload(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) const -> TUniquePtr<ICkUiPreparedWidgetUpdate> override;

    private:
        class FPreparedUpdate final : public ICkUiPreparedWidgetUpdate
        {
        public:
            FPreparedUpdate(TSharedRef<FSelect> InSelect, FConfiguration InConfiguration)
                : _Select(MoveTemp(InSelect))
                , _Configuration(MoveTemp(InConfiguration))
            {
            }

            virtual void Commit() noexcept override
            {
                _Select->ApplyConfiguration(MoveTemp(_Configuration));
            }

        private:
            TSharedRef<FSelect> _Select;
            FConfiguration _Configuration;
        };

        auto CanDispatch() const -> bool
        {
            return _Active && !_Dispatching && _Configuration.CanDispatchEvents.Get(false)
                && _Configuration.Enabled.Get(true) && !_Configuration.ReadOnly.Get(false);
        }

        auto GetDisplayText() const -> FText
        {
            if (!_Widget.IsValid()) { return FText::GetEmpty(); }
            const FText Label = _Widget->FindLabel(_Configuration.Value.Get(FString{}));
            return Label.IsEmpty() ? _Configuration.Placeholder.Get(FText::GetEmpty()) : Label;
        }

        auto RefreshOptions() -> void
        {
            if (!_Widget.IsValid() || !_Configuration.Options.IsValid()) { return; }
            _Widget->ReplaceOptions(_Configuration.Options->GetRecords());
            _AppliedOptionsRevision = _Configuration.Options->GetRevision();
        }

        auto SynchronizeSelection() -> void
        {
            if (_Active && _Widget.IsValid()) { _Widget->ApplySelection(_Configuration.Value.Get(FString{})); }
        }

        auto SynchronizeFromTick() -> void
        {
            if (!_Active || !_Configuration.CanDispatchEvents.Get(false)) { return; }
            if (_Configuration.Options.IsValid() && (_OptionsDirty || _AppliedOptionsRevision != _Configuration.Options->GetRevision()))
            {
                RefreshOptions();
                _OptionsDirty = false;
            }
            SynchronizeSelection();
        }

        auto BindCollection() -> void
        {
            if (_Configuration.Options.IsValid())
            { _CollectionChangedHandle = _Configuration.Options->OnChanged().AddSP(AsShared(), &FSelect::OnOptionsChanged); }
        }

        auto UnbindCollection() -> void
        {
            if (_Configuration.Options.IsValid() && _CollectionChangedHandle.IsValid())
            { _Configuration.Options->OnChanged().Remove(_CollectionChangedHandle); }
            _CollectionChangedHandle.Reset();
        }

        auto OnOptionsChanged() -> void
        {
            _OptionsDirty = true;
        }

        auto OnSelectionChanged(FOptionPtr InOption, const ESelectInfo::Type InInfo) -> void
        {
            if (!_Active || _Dispatching || !_Widget.IsValid() || _Widget->IsApplyingSelection() || !InOption.IsValid()) { return; }
            if (InInfo == ESelectInfo::Direct && !_Widget->IsProcessingInput()) { return; }
            const TSharedPtr<const FCkUiRecord> CurrentRecord = _Configuration.Options.IsValid()
                ? _Configuration.Options->FindRecord(InOption->Key)
                : nullptr;
            const FCkUiFieldValue* CurrentLabel = CurrentRecord.IsValid() ? CurrentRecord->FindField(TEXT("label")) : nullptr;
            if (!CurrentRecord.IsValid() || !CurrentRecord->GetKey().Equals(InOption->Key, ESearchCase::CaseSensitive)
                || CurrentLabel == nullptr || CurrentLabel->Kind != ECkUiFieldKind::Text)
            {
                if (_Configuration.CanDispatchEvents.Get(false))
                {
                    if (_Configuration.Options.IsValid() && (_OptionsDirty || _AppliedOptionsRevision != _Configuration.Options->GetRevision()))
                    {
                        RefreshOptions();
                        _OptionsDirty = false;
                    }
                    SynchronizeSelection();
                }
                return;
            }
            if (!CanDispatch())
            {
                if (_Configuration.CanDispatchEvents.Get(false)) { SynchronizeSelection(); }
                return;
            }
            const FCkUiOnStringChanged Changed = _Configuration.Changed;
            {
                TGuardValue<bool> DispatchGuard(_Dispatching, true);
                Changed.ExecuteIfBound(InOption->Key);
            }
            if (!_Active || !_Configuration.CanDispatchEvents.Get(false)) { return; }
            // The consumer may reject, normalize, reload, or replace options. Re-read both authoritative sources.
            if (_Configuration.Options.IsValid() && (_OptionsDirty || _AppliedOptionsRevision != _Configuration.Options->GetRevision()))
            {
                RefreshOptions();
                _OptionsDirty = false;
            }
            SynchronizeSelection();
        }

        auto ApplyConfiguration(FConfiguration InConfiguration) -> void
        {
            const bool SameOptions = _Configuration.Options == InConfiguration.Options;
            if (!SameOptions) { UnbindCollection(); }
            _Configuration = MoveTemp(InConfiguration);
            if (!SameOptions)
            {
                BindCollection();
                _OptionsDirty = true;
            }
        }

        TSharedPtr<SCkUiSelect> _Widget;
        FConfiguration _Configuration;
        FDelegateHandle _CollectionChangedHandle;
        int64 _AppliedOptionsRevision = INDEX_NONE;
        bool _OptionsDirty = false;
        bool _Dispatching = false;
        bool _Active = true;
    };

    auto MakeConfiguration(const FCkUiCustomWidgetArguments& InArguments, FConfiguration& OutConfiguration, FString& OutFailure) -> bool
    {
        const TAttribute<FString>* Value = InArguments.StringBindings.Find(TEXT("value"));
        const TSharedPtr<FCkUiCollection>* Options = InArguments.Collections.Find(TEXT("options"));
        const FCkUiOnStringChanged* Changed = InArguments.StringChanged.Find(TEXT("changed"));
        const FString* ValueBindingName = InArguments.BindingNames.Find(TEXT("value"));
        const FString* OptionsBindingName = InArguments.BindingNames.Find(TEXT("options"));
        if (Value == nullptr || !Value->IsSet() || Options == nullptr || !Options->IsValid() || Changed == nullptr || !Changed->IsBound()
            || ValueBindingName == nullptr || ValueBindingName->IsEmpty() || OptionsBindingName == nullptr || OptionsBindingName->IsEmpty())
        {
            OutFailure = TEXT("Select requires value, options, and changed bindings.");
            return false;
        }
        const FCkUiFieldSchema* LabelSchema = (*Options)->GetSchema().FindByPredicate([](const FCkUiFieldSchema& InField)
        { return InField.Name == TEXT("label"); });
        if (LabelSchema == nullptr || LabelSchema->Kind != ECkUiFieldKind::Text || !LabelSchema->Required)
        {
            OutFailure = TEXT("Select options require a required Text field named 'label'.");
            return false;
        }
        OutConfiguration.Value = *Value;
        OutConfiguration.Options = *Options;
        OutConfiguration.Placeholder = InArguments.TextBindings.FindRef(TEXT("placeholder"));
        OutConfiguration.Enabled = InArguments.BoolBindings.FindRef(TEXT("enabled"));
        OutConfiguration.ReadOnly = InArguments.BoolBindings.FindRef(TEXT("read-only"));
        OutConfiguration.CanDispatchEvents = InArguments.CanDispatchEvents;
        OutConfiguration.Changed = *Changed;
        OutConfiguration.ValueBindingName = *ValueBindingName;
        OutConfiguration.OptionsBindingName = *OptionsBindingName;
        return true;
    }

    auto FSelect::PrepareReload(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) const -> TUniquePtr<ICkUiPreparedWidgetUpdate>
    {
        auto Configuration = FConfiguration{};
        if (!MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
        if (!Configuration.ValueBindingName.Equals(_Configuration.ValueBindingName, ESearchCase::CaseSensitive)
            || !Configuration.OptionsBindingName.Equals(_Configuration.OptionsBindingName, ESearchCase::CaseSensitive))
        {
            OutFailure = TEXT("Select value and options bindings cannot change for a retained id.");
            return {};
        }
        if (!_Widget.IsValid())
        {
            OutFailure = TEXT("Select has no retained native widget.");
            return {};
        }
        return MakeUnique<FPreparedUpdate>(ConstCastSharedRef<FSelect>(AsShared()), MoveTemp(Configuration));
    }
}

auto FCkUiSelect::Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult
{
    auto Registration = FCkUiCustomWidgetRegistration{};
    Registration.Schema.Tag = TEXT("select");
    Registration.Schema.Properties = {
        {TEXT("value"), ECkUiCustomPropertyKind::StringBinding, true},
        {TEXT("options"), ECkUiCustomPropertyKind::CollectionBinding, true},
        {TEXT("changed"), ECkUiCustomPropertyKind::StringChanged, true},
        {TEXT("enabled"), ECkUiCustomPropertyKind::BoolBinding, false},
        {TEXT("read-only"), ECkUiCustomPropertyKind::BoolBinding, false},
        {TEXT("placeholder"), ECkUiCustomPropertyKind::TextBinding, false},
    };
    Registration.RetainedFactory = [](const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) -> TSharedPtr<ICkUiRetainedWidget>
    {
        auto Configuration = ck_ui_select::FConfiguration{};
        if (!ck_ui_select::MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
        const TSharedRef<ck_ui_select::FSelect> Select = MakeShared<ck_ui_select::FSelect>(MoveTemp(Configuration));
        Select->Initialize(InArguments.Id, InArguments.BaseFont);
        return Select;
    };
    return InRegistry.Register(MoveTemp(Registration));
}
