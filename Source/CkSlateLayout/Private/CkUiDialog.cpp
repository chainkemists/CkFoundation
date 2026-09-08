#include "CkSlateLayout/CkUiDialog.h"

#include "CkSlateLayout/CkFlexLayoutTypes.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Input/Events.h"
#include "Input/NavigationReply.h"
#include "InputCoreTypes.h"
#include "Layout/Children.h"
#include "Layout/WidgetPath.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"

namespace ck_ui_dialog
{
    struct FConfiguration
    {
        TAttribute<bool> Open;
        TAttribute<bool> CanDispatchEvents;
        FSimpleDelegate Dismiss;
        FString OpenBinding;
        int32 SlateUserIndex = INDEX_NONE;
        TSharedPtr<SWidget> Content;
        TSharedPtr<SWidget> Body;
        TFunction<void(const FString&)> ReleaseSlotPointerCaptures;
    };

    class FDialog;

    class SCkUiDialogBackdrop final : public SBorder
    {
    public:
        SLATE_BEGIN_ARGS(SCkUiDialogBackdrop) {}
        SLATE_END_ARGS()

        void Construct(const FArguments&, TWeakPtr<FDialog> InOwner);
        virtual auto OnMouseButtonDown(const FGeometry&, const FPointerEvent& InEvent) -> FReply override;
        virtual auto OnMouseButtonUp(const FGeometry&, const FPointerEvent& InEvent) -> FReply override;
        virtual auto OnMouseMove(const FGeometry&, const FPointerEvent& InEvent) -> FReply override;
        virtual auto OnMouseWheel(const FGeometry&, const FPointerEvent& InEvent) -> FReply override;
        virtual auto OnTouchStarted(const FGeometry&, const FPointerEvent& InEvent) -> FReply override;
        virtual auto OnTouchMoved(const FGeometry&, const FPointerEvent& InEvent) -> FReply override;
        virtual auto OnTouchEnded(const FGeometry&, const FPointerEvent& InEvent) -> FReply override;

    private:
        auto Consume(const FPointerEvent& InEvent) const -> FReply;
        TWeakPtr<FDialog> _Owner;
    };

    class SCkUiDialogRoot final : public SOverlay
    {
    public:
        SLATE_BEGIN_ARGS(SCkUiDialogRoot) {}
            SLATE_ARGUMENT(TSharedPtr<SWidget>, Content)
            SLATE_ARGUMENT(TSharedPtr<SWidget>, Body)
        SLATE_END_ARGS()

        void Construct(const FArguments& InArguments, TWeakPtr<FDialog> InOwner);
        virtual void Tick(const FGeometry& InAllottedGeometry, double InCurrentTime, float InDeltaTime) override;
        virtual auto ComputeDesiredSize(float InLayoutScaleMultiplier) const -> FVector2D override;
        virtual auto OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InEvent) -> FReply override;
        virtual auto OnNavigation(const FGeometry& InGeometry, const FNavigationEvent& InEvent) -> FNavigationReply override;
        virtual bool SupportsKeyboardFocus() const override;

    private:
        TWeakPtr<FDialog> _Owner;
        TSharedPtr<SBox> _ContentGate;
        TSharedPtr<SCkUiDialogBackdrop> _Backdrop;
        TSharedPtr<SBox> _BodyGate;
    };

    class FDialog final : public ICkUiRetainedWidget, public TSharedFromThis<FDialog>
    {
    public:
        explicit FDialog(FConfiguration InConfiguration)
            : _Configuration(MoveTemp(InConfiguration))
        {
        }

        virtual ~FDialog() override
        {
            _Active = false;
            StopPostTick();
            ReleaseLocalFocus(false);
        }

        void Initialize(const FString& InId)
        {
            const TWeakPtr<FDialog> WeakDialog = AsShared();
            SAssignNew(_Widget, SCkUiDialogRoot, WeakDialog)
                .Content(_Configuration.Content)
                .Body(_Configuration.Body);
            _Widget->SetTag(FName(*InId));
            _Widget->AddMetadata(MakeShared<FCkFlexMeasureMetaData>(
                [WeakDialog](const FCkFlexMeasureArgs& InArgs) -> FVector2D
                {
                    const TSharedPtr<FDialog> Dialog = WeakDialog.Pin();
                    return Dialog.IsValid() ? Dialog->MeasureContent(InArgs) : FVector2D::ZeroVector;
                },
                [WeakDialog](const float InWidth, const float InHeight)
                {
                    if (const TSharedPtr<FDialog> Dialog = WeakDialog.Pin()) { Dialog->NotifyArranged(InWidth, InHeight); }
                }));
        }

        virtual auto GetWidget() const -> TSharedRef<SWidget> override { return _Widget.ToSharedRef(); }
        virtual auto GetFocusTransferTarget() const -> TSharedPtr<SWidget> override { return _Configuration.Body; }
        virtual void ReleaseTransientInteraction() override { _Suppressed = true; ReleaseLocalFocus(true); }
        virtual auto PrepareReload(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) const -> TUniquePtr<ICkUiPreparedWidgetUpdate> override;

        auto IsOwner(const uint32 InUserIndex) const -> bool { return _Configuration.SlateUserIndex == static_cast<int32>(InUserIndex); }
        auto IsOpen() const -> bool { return _Active && _Configuration.Open.Get(false); }
        auto CanDispatch() const -> bool { return IsOpen() && _Configuration.CanDispatchEvents.Get(false) && HasActiveOwner(); }
        auto HasActiveOwner() const -> bool { return FSlateApplication::IsInitialized() && _Configuration.SlateUserIndex >= 0 && FSlateApplication::Get().GetUser(_Configuration.SlateUserIndex).IsValid(); }
        auto IsBodyFocusOwned() const -> bool;
        auto IsMountedVisible(const TSharedPtr<SWidget>& InWidget) const -> bool;
        auto FindInitialFocus() const -> TSharedPtr<SWidget>;
        auto RequestDismiss() -> void;
        auto Synchronize() -> void;
        static auto MakeConfiguration(const FCkUiCustomWidgetArguments& InArguments, FConfiguration& OutConfiguration, FString& OutFailure) -> bool;
        auto MeasureContent(const FCkFlexMeasureArgs& InArgs) const -> FVector2D;
        auto NotifyArranged(float InWidth, float InHeight) -> void;

    private:
        class FPreparedUpdate final : public ICkUiPreparedWidgetUpdate
        {
        public:
            FPreparedUpdate(TSharedRef<FDialog> InDialog, FConfiguration InConfiguration, TWeakPtr<SWidget> InFocusedLeaf, const bool InHasFocusedLeaf)
                : _Dialog(MoveTemp(InDialog)), _Configuration(MoveTemp(InConfiguration)), _FocusedLeaf(MoveTemp(InFocusedLeaf)), _HasFocusedLeaf(InHasFocusedLeaf) {}
            virtual void Commit() noexcept override
            {
                _Dialog->ApplyConfiguration(MoveTemp(_Configuration));
                _Dialog->QueueFocusRepair(MoveTemp(_FocusedLeaf), _HasFocusedLeaf);
            }
        private:
            TSharedRef<FDialog> _Dialog;
            FConfiguration _Configuration;
            TWeakPtr<SWidget> _FocusedLeaf;
            bool _HasFocusedLeaf = false;
        };

        static auto MeasureWidget(const TSharedPtr<SWidget>& InWidget, const FCkFlexMeasureArgs& InArgs) -> FVector2D;
        static auto NotifyWidget(const TSharedPtr<SWidget>& InWidget, float InWidth, float InHeight) -> void;
        auto ReleaseLocalFocus(bool bRestorePrevious) -> void;
        auto ReleaseSlotPointerCaptures(const FString& InSlot) -> void;
        auto EnsurePostTick() -> void;
        auto StopPostTick() -> void;
        auto ApplyConfiguration(FConfiguration InConfiguration) -> void { _Configuration = MoveTemp(InConfiguration); }
        auto QueueFocusRepair(TWeakPtr<SWidget> InFocusedLeaf, bool bHasFocusedLeaf) -> void;
        auto RepairPendingFocus() -> void;

        TSharedPtr<SCkUiDialogRoot> _Widget;
        FConfiguration _Configuration;
        TWeakPtr<SWidget> _PreviousFocus;
        TWeakPtr<SWidget> _PendingFocusedLeaf;
        FDelegateHandle _PostTickHandle;
        bool _WasOpen = false;
        bool _HasPendingFocusRepair = false;
        bool _Suppressed = false;
        bool _Active = true;
        uint32 _SessionEpoch = 0;
    };

    void SCkUiDialogBackdrop::Construct(const FArguments&, TWeakPtr<FDialog> InOwner)
    {
        _Owner = MoveTemp(InOwner);
        SBorder::Construct(SBorder::FArguments().BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f)));
    }

    auto SCkUiDialogBackdrop::Consume(const FPointerEvent& InEvent) const -> FReply
    {
        const TSharedPtr<FDialog> Dialog = _Owner.Pin();
        return Dialog.IsValid() && Dialog->CanDispatch() && Dialog->IsOwner(InEvent.GetUserIndex()) ? FReply::Handled() : FReply::Unhandled();
    }

    auto SCkUiDialogBackdrop::OnMouseButtonDown(const FGeometry&, const FPointerEvent& InEvent) -> FReply { return Consume(InEvent); }
    auto SCkUiDialogBackdrop::OnMouseButtonUp(const FGeometry&, const FPointerEvent& InEvent) -> FReply { return Consume(InEvent); }
    auto SCkUiDialogBackdrop::OnMouseMove(const FGeometry&, const FPointerEvent& InEvent) -> FReply { return Consume(InEvent); }
    auto SCkUiDialogBackdrop::OnMouseWheel(const FGeometry&, const FPointerEvent& InEvent) -> FReply { return Consume(InEvent); }
    auto SCkUiDialogBackdrop::OnTouchStarted(const FGeometry&, const FPointerEvent& InEvent) -> FReply { return Consume(InEvent); }
    auto SCkUiDialogBackdrop::OnTouchMoved(const FGeometry&, const FPointerEvent& InEvent) -> FReply { return Consume(InEvent); }
    auto SCkUiDialogBackdrop::OnTouchEnded(const FGeometry&, const FPointerEvent& InEvent) -> FReply { return Consume(InEvent); }

    void SCkUiDialogRoot::Construct(const FArguments& InArguments, TWeakPtr<FDialog> InOwner)
    {
        _Owner = MoveTemp(InOwner);
        SAssignNew(_ContentGate, SBox)[InArguments._Content.ToSharedRef()];
        SAssignNew(_Backdrop, SCkUiDialogBackdrop, _Owner);
        SAssignNew(_BodyGate, SBox)[InArguments._Body.ToSharedRef()];

        SOverlay::Construct(SOverlay::FArguments()
            + SOverlay::Slot()[_ContentGate.ToSharedRef()]
            + SOverlay::Slot()[_Backdrop.ToSharedRef()]
            + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[_BodyGate.ToSharedRef()]);
        SetCanTick(true);

        const TWeakPtr<FDialog> WeakDialog = _Owner;
        _ContentGate->SetEnabled(TAttribute<bool>::CreateLambda([WeakDialog]()
        {
            const TSharedPtr<FDialog> Dialog = WeakDialog.Pin();
            return !Dialog.IsValid() || !Dialog->IsOpen();
        }));
        _Backdrop->SetVisibility(TAttribute<EVisibility>::CreateLambda([WeakDialog]()
        {
            const TSharedPtr<FDialog> Dialog = WeakDialog.Pin();
            return Dialog.IsValid() && Dialog->IsOpen() ? EVisibility::Visible : EVisibility::Collapsed;
        }));
        _BodyGate->SetVisibility(TAttribute<EVisibility>::CreateLambda([WeakDialog]()
        {
            const TSharedPtr<FDialog> Dialog = WeakDialog.Pin();
            return Dialog.IsValid() && Dialog->IsOpen() ? EVisibility::Visible : EVisibility::Collapsed;
        }));
    }

    void SCkUiDialogRoot::Tick(const FGeometry& InAllottedGeometry, const double InCurrentTime, const float InDeltaTime)
    {
        SOverlay::Tick(InAllottedGeometry, InCurrentTime, InDeltaTime);
        if (const TSharedPtr<FDialog> Dialog = _Owner.Pin())
        {
            Dialog->NotifyArranged(InAllottedGeometry.GetLocalSize().X, InAllottedGeometry.GetLocalSize().Y);
            Dialog->Synchronize();
        }
    }

    auto SCkUiDialogRoot::ComputeDesiredSize(const float InLayoutScaleMultiplier) const -> FVector2D
    {
        const TSharedPtr<FDialog> Dialog = _Owner.Pin();
        return Dialog.IsValid() ? Dialog->MeasureContent({.LayoutScale = InLayoutScaleMultiplier}) : FVector2D::ZeroVector;
    }

    auto SCkUiDialogRoot::OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InEvent) -> FReply
    {
        const TSharedPtr<FDialog> Dialog = _Owner.Pin();
        if (!Dialog.IsValid() || !Dialog->CanDispatch() || !Dialog->IsOwner(InEvent.GetUserIndex())) { return SOverlay::OnKeyDown(InGeometry, InEvent); }
        const bool Back = FSlateApplication::IsInitialized()
            && FSlateApplication::Get().GetNavigationActionFromKey(InEvent) == EUINavigationAction::Back;
        if (Back)
        {
            if (!InEvent.IsRepeat()) { Dialog->RequestDismiss(); }
            return FReply::Handled();
        }
        return SOverlay::OnKeyDown(InGeometry, InEvent);
    }

    auto SCkUiDialogRoot::OnNavigation(const FGeometry&, const FNavigationEvent& InEvent) -> FNavigationReply
    {
        const TSharedPtr<FDialog> Dialog = _Owner.Pin();
        if (!Dialog.IsValid() || !Dialog->CanDispatch() || !Dialog->IsOwner(InEvent.GetUserIndex())) { return FNavigationReply::Escape(); }
        const EUINavigation Direction = InEvent.GetNavigationType();
        return Direction == EUINavigation::Next || Direction == EUINavigation::Previous ? FNavigationReply::Wrap() : FNavigationReply::Stop();
    }

    bool SCkUiDialogRoot::SupportsKeyboardFocus() const
    {
        const TSharedPtr<FDialog> Dialog = _Owner.Pin();
        return Dialog.IsValid() && Dialog->CanDispatch();
    }

    auto FDialog::MakeConfiguration(const FCkUiCustomWidgetArguments& InArguments, FConfiguration& OutConfiguration, FString& OutFailure) -> bool
    {
        const TAttribute<bool>* Open = InArguments.BoolBindings.Find(TEXT("open"));
        const FString* OpenBinding = InArguments.BindingNames.Find(TEXT("open"));
        const FSimpleDelegate* Dismiss = InArguments.Actions.Find(TEXT("dismiss"));
        const TSharedPtr<SWidget> Content = InArguments.Slots.FindRef(TEXT("content"));
        const TSharedPtr<SWidget> Body = InArguments.Slots.FindRef(TEXT("body"));
        if (InArguments.SlateUserIndex < 0) { OutFailure = TEXT("Dialog requires a nonnegative host Slate user."); return false; }
        if (Open == nullptr || !Open->IsSet() || OpenBinding == nullptr || OpenBinding->IsEmpty() || Dismiss == nullptr || !Dismiss->IsBound()
            || !Content.IsValid() || !Body.IsValid() || !InArguments.ReleaseSlotPointerCaptures)
        { OutFailure = TEXT("Dialog requires open and dismiss bindings plus content and body slots."); return false; }
        OutConfiguration.Open = *Open;
        OutConfiguration.CanDispatchEvents = InArguments.CanDispatchEvents;
        OutConfiguration.Dismiss = *Dismiss;
        OutConfiguration.OpenBinding = *OpenBinding;
        OutConfiguration.SlateUserIndex = InArguments.SlateUserIndex;
        OutConfiguration.Content = Content;
        OutConfiguration.Body = Body;
        OutConfiguration.ReleaseSlotPointerCaptures = InArguments.ReleaseSlotPointerCaptures;
        return true;
    }

    auto FDialog::PrepareReload(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) const -> TUniquePtr<ICkUiPreparedWidgetUpdate>
    {
        FConfiguration Configuration;
        if (!MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
        if (Configuration.SlateUserIndex != _Configuration.SlateUserIndex || Configuration.OpenBinding != _Configuration.OpenBinding
            || Configuration.Content != _Configuration.Content || Configuration.Body != _Configuration.Body)
        { OutFailure = TEXT("Dialog owner, open binding, and slot mounts cannot change for a retained id."); return {}; }
        TWeakPtr<SWidget> FocusedLeaf;
        bool HasFocusedLeaf = false;
        if (FSlateApplication::IsInitialized() && IsBodyFocusOwned())
        {
            FocusedLeaf = FSlateApplication::Get().GetUserFocusedWidget(_Configuration.SlateUserIndex);
            HasFocusedLeaf = FocusedLeaf.IsValid();
        }
        return MakeUnique<FPreparedUpdate>(ConstCastSharedRef<FDialog>(AsShared()), MoveTemp(Configuration), MoveTemp(FocusedLeaf), HasFocusedLeaf);
    }

    auto FDialog::MeasureWidget(const TSharedPtr<SWidget>& InWidget, const FCkFlexMeasureArgs& InArgs) -> FVector2D
    {
        if (!InWidget.IsValid()) { return FVector2D::ZeroVector; }
        TSharedPtr<SWidget> Target = InWidget;
        TSharedPtr<FCkFlexMeasureMetaData> Measure = Target->GetMetaData<FCkFlexMeasureMetaData>();
        if (!Measure.IsValid() && Target->GetChildren()->Num() == 1)
        {
            Target = ConstCastSharedRef<SWidget>(Target->GetChildren()->GetChildAt(0));
            Measure = Target->GetMetaData<FCkFlexMeasureMetaData>();
        }
        return Measure.IsValid() ? Measure->Measure(InArgs) : Target->GetDesiredSize();
    }

    void FDialog::NotifyWidget(const TSharedPtr<SWidget>& InWidget, const float InWidth, const float InHeight)
    {
        if (!InWidget.IsValid()) { return; }
        TSharedPtr<SWidget> Target = InWidget;
        TSharedPtr<FCkFlexMeasureMetaData> Measure = Target->GetMetaData<FCkFlexMeasureMetaData>();
        if (!Measure.IsValid() && Target->GetChildren()->Num() == 1)
        {
            Target = ConstCastSharedRef<SWidget>(Target->GetChildren()->GetChildAt(0));
            Measure = Target->GetMetaData<FCkFlexMeasureMetaData>();
        }
        if (Measure.IsValid())
        { Measure->NotifyArranged(InWidth, InHeight); }
    }

    auto FDialog::MeasureContent(const FCkFlexMeasureArgs& InArgs) const -> FVector2D
    {
        return MeasureWidget(_Configuration.Content, InArgs);
    }

    void FDialog::NotifyArranged(const float InWidth, const float InHeight)
    {
        NotifyWidget(_Configuration.Content, InWidth, InHeight);
        if (_Configuration.Body.IsValid())
        {
            auto BodyArgs = FCkFlexMeasureArgs{.AvailableWidth = InWidth, .WidthMode = YGMeasureModeAtMost,
                .AvailableHeight = InHeight, .HeightMode = YGMeasureModeAtMost, .LayoutScale = 1.0f};
            const FVector2D BodySize = MeasureWidget(_Configuration.Body, BodyArgs);
            NotifyWidget(_Configuration.Body, FMath::Min(InWidth, BodySize.X), FMath::Min(InHeight, BodySize.Y));
        }
    }

    auto FDialog::IsMountedVisible(const TSharedPtr<SWidget>& InWidget) const -> bool
    {
        if (!FSlateApplication::IsInitialized() || !InWidget.IsValid()) { return false; }
        FWidgetPath Path;
        if (!FSlateApplication::Get().GeneratePathToWidgetUnchecked(InWidget.ToSharedRef(), Path, EVisibility::Visible)) { return false; }
        for (int32 Index = 0; Index < Path.Widgets.Num(); ++Index)
        {
            const FArrangedWidget& Item = Path.Widgets[Index];
            if (!Item.Widget->GetVisibility().IsVisible() || !Item.Widget->IsEnabled()) { return false; }
        }
        return true;
    }

    auto FDialog::FindInitialFocus() const -> TSharedPtr<SWidget>
    {
        const auto Find = [this](const TSharedPtr<SWidget>& InWidget, const auto& Self) -> TSharedPtr<SWidget>
        {
            if (!InWidget.IsValid()) { return {}; }
            if (InWidget->SupportsKeyboardFocus() && IsMountedVisible(InWidget)) { return InWidget; }
            const FChildren* Children = InWidget->GetChildren();
            for (int32 Index = 0; Children != nullptr && Index < Children->Num(); ++Index)
            {
                if (const TSharedPtr<SWidget> Found = Self(ConstCastSharedRef<SWidget>(Children->GetChildAt(Index)), Self)) { return Found; }
            }
            return {};
        };
        if (const TSharedPtr<SWidget> Focusable = Find(_Configuration.Body, Find)) { return Focusable; }
        return IsMountedVisible(_Widget) && _Widget->SupportsKeyboardFocus() ? _Widget : TSharedPtr<SWidget>{};
    }

    auto FDialog::IsBodyFocusOwned() const -> bool
    {
        if (!FSlateApplication::IsInitialized() || _Configuration.SlateUserIndex < 0) { return false; }
        TSharedPtr<SWidget> Current = FSlateApplication::Get().GetUserFocusedWidget(_Configuration.SlateUserIndex);
        if (Current.Get() == _Widget.Get()) { return true; }
        while (Current.IsValid())
        {
            if (Current == _Configuration.Body) { return true; }
            Current = Current->GetParentWidget();
        }
        return false;
    }

    void FDialog::RequestDismiss()
    {
        if (!CanDispatch()) { return; }
        const FSimpleDelegate Dismiss = _Configuration.Dismiss;
        Dismiss.ExecuteIfBound();
    }

    void FDialog::ReleaseLocalFocus(const bool bRestorePrevious)
    {
        const bool HadOpenSession = _WasOpen;
        const uint32 ReleaseEpoch = ++_SessionEpoch;
        const TSharedPtr<SWidget> Previous = _PreviousFocus.Pin();
        _PreviousFocus.Reset();
        _PendingFocusedLeaf.Reset();
        _HasPendingFocusRepair = false;
        _WasOpen = false;
        if (!HadOpenSession) { return; }
        if (!FSlateApplication::IsInitialized() || _Configuration.SlateUserIndex < 0) { return; }
        ReleaseSlotPointerCaptures(TEXT("body"));
        if (_SessionEpoch != ReleaseEpoch || _WasOpen || !FSlateApplication::IsInitialized() || !HasActiveOwner()) { return; }
        FSlateApplication& Slate = FSlateApplication::Get();
        const bool OwnedFocus = IsBodyFocusOwned();
        if (OwnedFocus) { Slate.ClearUserFocus(_Configuration.SlateUserIndex, EFocusCause::SetDirectly); }
        if (_SessionEpoch != ReleaseEpoch || _WasOpen) { return; }
        if (bRestorePrevious && OwnedFocus && Previous.IsValid() && IsMountedVisible(Previous)
            && Previous->SupportsKeyboardFocus() && !Slate.GetUserFocusedWidget(_Configuration.SlateUserIndex).IsValid())
        { Slate.SetUserFocus(_Configuration.SlateUserIndex, Previous, EFocusCause::SetDirectly); }
    }

    void FDialog::ReleaseSlotPointerCaptures(const FString& InSlot)
    {
        const TFunction<void(const FString&)> Release = _Configuration.ReleaseSlotPointerCaptures;
        if (Release) { Release(InSlot); }
    }

    void FDialog::EnsurePostTick()
    {
        if (_PostTickHandle.IsValid() || !FSlateApplication::IsInitialized()) { return; }
        const TWeakPtr<FDialog> WeakDialog = AsShared();
        _PostTickHandle = FSlateApplication::Get().OnPostTick().AddLambda([WeakDialog](const float)
        {
            if (const TSharedPtr<FDialog> Dialog = WeakDialog.Pin()) { Dialog->Synchronize(); }
        });
    }

    void FDialog::StopPostTick()
    {
        if (_PostTickHandle.IsValid() && FSlateApplication::IsInitialized())
        { FSlateApplication::Get().OnPostTick().Remove(_PostTickHandle); }
        _PostTickHandle.Reset();
    }

    void FDialog::QueueFocusRepair(TWeakPtr<SWidget> InFocusedLeaf, const bool bHasFocusedLeaf)
    {
        _PendingFocusedLeaf = MoveTemp(InFocusedLeaf);
        _HasPendingFocusRepair = bHasFocusedLeaf;
    }

    void FDialog::RepairPendingFocus()
    {
        if (!_HasPendingFocusRepair || !CanDispatch() || !_WasOpen) { return; }
        const TSharedPtr<SWidget> PreviousLeaf = _PendingFocusedLeaf.Pin();
        _PendingFocusedLeaf.Reset();
        _HasPendingFocusRepair = false;
        FSlateApplication& Slate = FSlateApplication::Get();
        const TSharedPtr<SWidget> Current = Slate.GetUserFocusedWidget(_Configuration.SlateUserIndex);
        const bool CurrentIsStalePrevious = Current.IsValid() && Current == PreviousLeaf && !IsMountedVisible(Current);
        if (Current.IsValid() && !CurrentIsStalePrevious) { return; }
        if (const TSharedPtr<SWidget> Initial = FindInitialFocus())
        { Slate.SetUserFocus(_Configuration.SlateUserIndex, Initial, EFocusCause::SetDirectly); }
    }

    void FDialog::Synchronize()
    {
        // Transaction staging/publication temporarily gates dispatch. It must not synthesize focus loss
        // or restoration before the accepted configuration is visible to the consumer.
        if (!_Configuration.CanDispatchEvents.Get(false)) { StopPostTick(); return; }
        const bool ConfiguredOpen = IsOpen();
        const bool ShouldOpen = CanDispatch() && IsMountedVisible(_Widget) && IsMountedVisible(_Configuration.Body);
        if (!ConfiguredOpen)
        {
            ReleaseLocalFocus(true);
            _Suppressed = false;
            StopPostTick();
            return;
        }
        if (!ShouldOpen)
        {
            ReleaseLocalFocus(true);
            _Suppressed = false;
            StopPostTick();
            return;
        }
        if (_Suppressed) { EnsurePostTick(); return; }
        RepairPendingFocus();
        if (!_WasOpen)
        {
            FSlateApplication& Slate = FSlateApplication::Get();
            _PreviousFocus = Slate.GetUserFocusedWidget(_Configuration.SlateUserIndex);
            _WasOpen = true;
            const uint32 OpeningEpoch = ++_SessionEpoch;
            ReleaseSlotPointerCaptures(TEXT("content"));
            if (_SessionEpoch != OpeningEpoch || !_WasOpen || !CanDispatch()) { return; }
            if (const TSharedPtr<SWidget> Initial = FindInitialFocus())
            { Slate.SetUserFocus(_Configuration.SlateUserIndex, Initial, EFocusCause::SetDirectly); }
            if (_WasOpen && CanDispatch()) { EnsurePostTick(); }
        }
    }
}

auto FCkUiDialog::Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult
{
    FCkUiCustomWidgetRegistration Registration;
    Registration.Schema.Tag = TEXT("dialog");
    Registration.Schema.Properties = {
        {TEXT("open"), ECkUiCustomPropertyKind::BoolBinding, true},
        {TEXT("dismiss"), ECkUiCustomPropertyKind::Action, true},
    };
    Registration.Schema.Slots = {{TEXT("content"), true}, {TEXT("body"), true}};
    Registration.RetainedFactory = [](const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) -> TSharedPtr<ICkUiRetainedWidget>
    {
        auto Configuration = ck_ui_dialog::FConfiguration{};
        if (!ck_ui_dialog::FDialog::MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
        const TSharedRef<ck_ui_dialog::FDialog> Dialog = MakeShared<ck_ui_dialog::FDialog>(MoveTemp(Configuration));
        Dialog->Initialize(InArguments.Id);
        return Dialog;
    };
    return InRegistry.Register(MoveTemp(Registration));
}
