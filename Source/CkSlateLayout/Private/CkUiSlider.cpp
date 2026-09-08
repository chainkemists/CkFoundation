#include "CkSlateLayout/CkUiSlider.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Input/Events.h"
#include "Layout/WidgetPath.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SSlider.h"

namespace ck_ui_slider
{
    struct FConfiguration
    {
        TAttribute<float> Value;
        TAttribute<bool> Enabled;
        TAttribute<bool> ReadOnly;
        TAttribute<bool> CanDispatchEvents;
        FCkUiOnNumberChanged Changed;
        FCkUiOnNumberInteraction Interaction;
        FString ValueBinding;
        float Min = 0.0f;
        float Max = 1.0f;
        float Step = 0.01f;
        EOrientation Orientation = Orient_Horizontal;
        FSliderStyle NativeStyle;
    };

    struct FInteraction
    {
        ECkUiInteractionSource Source = ECkUiInteractionSource::Pointer;
        int32 UserIndex = INDEX_NONE;
        uint32 PointerIndex = 0;
        float Initial = 0.0f;
        float Draft = 0.0f;
        bool NormalRelease = false;
    };

    class FSlider;

    class SCkUiSlider final : public SSlider
    {
    public:
        SLATE_BEGIN_ARGS(SCkUiSlider) {}
            SLATE_ARGUMENT(EOrientation, Orientation)
            SLATE_ARGUMENT(FSliderStyle, NativeStyle)
            SLATE_ATTRIBUTE(float, Value)
            SLATE_ATTRIBUTE(float, StepSize)
        SLATE_END_ARGS()

        void Construct(const FArguments& InArguments, TWeakPtr<FSlider> InOwner)
        {
            _Owner = MoveTemp(InOwner);
            _NativeStyle = InArguments._NativeStyle;
            SSlider::Construct(SSlider::FArguments()
                .Style(&_NativeStyle)
                .Orientation(InArguments._Orientation)
                .Value(InArguments._Value).StepSize(InArguments._StepSize)
                .MinValue(0.0f).MaxValue(1.0f).IsFocusable(true).RequiresControllerLock(false)
                .OnMouseCaptureBegin(FSimpleDelegate::CreateSP(this, &SCkUiSlider::OnPointerBegin))
                .OnValueChanged(FOnFloatValueChanged::CreateSP(this, &SCkUiSlider::OnValueChanged)));
        }

        auto HasPendingTouch() const -> bool { return _TouchPending; }
        auto ApplyNativeStyle(const FSliderStyle& InStyle) -> void
        {
            _NativeStyle = InStyle;
            SetStyle(&_NativeStyle);
        }

        virtual auto OnMouseButtonDown(const FGeometry&, const FPointerEvent&) -> FReply override;
        virtual auto OnMouseMove(const FGeometry&, const FPointerEvent&) -> FReply override;
        virtual auto OnMouseButtonUp(const FGeometry&, const FPointerEvent&) -> FReply override;
        virtual auto OnTouchStarted(const FGeometry&, const FPointerEvent&) -> FReply override;
        virtual auto OnTouchMoved(const FGeometry&, const FPointerEvent&) -> FReply override;
        virtual auto OnTouchEnded(const FGeometry&, const FPointerEvent&) -> FReply override;
        virtual auto OnKeyDown(const FGeometry&, const FKeyEvent&) -> FReply override;
        virtual auto OnNavigation(const FGeometry&, const FNavigationEvent&) -> FNavigationReply override;
        virtual void OnMouseCaptureLost(const FCaptureLostEvent&) override;
        virtual void OnFocusLost(const FFocusEvent&) override;
        virtual void Tick(const FGeometry&, double, float) override;

    private:
        auto CanEdit() const -> bool;
        auto IsInputBlocked() const -> bool;
        auto IsPointer(const FPointerEvent&) const -> bool;
        auto IsForeignUser(uint32 InUser) const -> bool;
        auto Cancel() -> void;
        auto OnPointerBegin() -> void;
        auto OnValueChanged(float InValue) -> void;
        auto KeepPointerReply(FReply InReply, const FPointerEvent&) -> FReply;
        TWeakPtr<FSlider> _Owner;
        // SSlider borrows its style pointer. Keep storage alive with the native widget,
        // including when a caller holds it after the retained component is released.
        FSliderStyle _NativeStyle;
        int32 _PendingUser = INDEX_NONE;
        uint32 _PendingPointer = 0;
        bool _TouchPending = false;
        bool _FinishingPointerInteraction = false;
    };

    class FSlider final : public ICkUiRetainedWidget, public TSharedFromThis<FSlider>
    {
    public:
        explicit FSlider(FConfiguration InConfiguration) : _Configuration(MoveTemp(InConfiguration)) {}

        auto Initialize(const FString& InId) -> void
        {
            const TWeakPtr<FSlider> WeakSlider = AsShared();
            SAssignNew(_Widget, SCkUiSlider, WeakSlider)
                .Orientation(_Configuration.Orientation)
                .NativeStyle(_Configuration.NativeStyle)
                .Value_Lambda([WeakSlider]
                {
                    const TSharedPtr<FSlider> Slider = WeakSlider.Pin();
                    return Slider.IsValid() ? Slider->NormalizedValue() : 0.0f;
                })
                .StepSize_Lambda([WeakSlider]
                {
                    const TSharedPtr<FSlider> Slider = WeakSlider.Pin();
                    return Slider.IsValid() ? Slider->NormalizedStep() : 0.01f;
                })
                .IsEnabled_Lambda([WeakSlider]
                {
                    const TSharedPtr<FSlider> Slider = WeakSlider.Pin();
                    return Slider.IsValid() && Slider->_Configuration.Enabled.Get(true);
                });
            _Widget->SetTag(FName(*InId));
        }

        virtual auto GetWidget() const -> TSharedRef<SWidget> override { return _Widget.ToSharedRef(); }
        virtual auto PrepareReload(const FCkUiCustomWidgetArguments&, FString&) const -> TUniquePtr<ICkUiPreparedWidgetUpdate> override;
        virtual auto GetPointerCaptures() const -> TArray<FCkUiPointerCapture> override
        {
            if (!_Interaction.IsSet() || _Interaction->Source != ECkUiInteractionSource::Pointer) { return {}; }
            return {{_Interaction->UserIndex, _Interaction->PointerIndex, _Widget}};
        }
        virtual void BeginPointerCaptureTransfer(const FCkUiPointerCapture& InCapture) noexcept override
        { _Transfer = InCapture; }
        virtual void EndPointerCaptureTransfer(const FCkUiPointerCapture&, bool InRestored) noexcept override
        {
            _Transfer.Reset();
            if (!InRestored) { _Interaction.Reset(); }
        }
        auto IsTransfer(const FCaptureLostEvent& InEvent) const -> bool
        {
            return _Transfer.IsSet() && _Transfer->UserIndex == InEvent.UserIndex
                && _Transfer->PointerIndex == static_cast<uint32>(InEvent.PointerIndex);
        }
        auto CanDispatch() const -> bool { return _Configuration.CanDispatchEvents.Get(false); }
        auto CanEdit() const -> bool
        {
            return CanDispatch() && _Configuration.Enabled.Get(true) && !_Configuration.ReadOnly.Get(false)
                && FMath::IsFinite(_Configuration.Value.Get(0.0f));
        }
        auto IsHorizontal() const -> bool { return _Configuration.Orientation == Orient_Horizontal; }
        auto IsActive() const -> bool { return _Interaction.IsSet(); }
        auto IsForeignUser(uint32 InUser) const -> bool
        { return _Interaction.IsSet() && _Interaction->UserIndex != static_cast<int32>(InUser); }
        auto IsDispatching() const -> bool { return _Dispatching; }
        auto IsController() const -> bool
        { return _Interaction.IsSet() && _Interaction->Source == ECkUiInteractionSource::Controller; }
        auto IsPointer(int32 InUser, uint32 InPointer) const -> bool
        {
            return _Interaction.IsSet() && _Interaction->Source == ECkUiInteractionSource::Pointer
                && _Interaction->UserIndex == InUser && _Interaction->PointerIndex == InPointer;
        }
        auto MarkNormalRelease() -> void { if (_Interaction.IsSet()) { _Interaction->NormalRelease = true; } }
        auto IsNormalRelease() const -> bool { return _Interaction.IsSet() && _Interaction->NormalRelease; }
        auto Normalize(float InValue) const -> float
        {
            if (!FMath::IsFinite(InValue)) { return 0.0f; }
            const double Span = static_cast<double>(_Configuration.Max) - _Configuration.Min;
            return static_cast<float>(FMath::Clamp((static_cast<double>(InValue) - _Configuration.Min) / Span, 0.0, 1.0));
        }
        auto NormalizedValue() const -> float
        { return Normalize(_Interaction.IsSet() ? _Interaction->Draft : _Configuration.Value.Get(_Configuration.Min)); }
        auto NormalizedStep() const -> float
        { return static_cast<float>(_Configuration.Step / (static_cast<double>(_Configuration.Max) - _Configuration.Min)); }
        auto Begin(ECkUiInteractionSource InSource, int32 InUser, uint32 InPointer = 0) -> void
        {
            if (_Interaction.IsSet() || !CanEdit()) { return; }
            const float Initial = _Configuration.Value.Get(_Configuration.Min);
            _Interaction = FInteraction{InSource, InUser, InPointer, Initial, FMath::Clamp(Initial, _Configuration.Min, _Configuration.Max)};
            const FCkUiOnNumberInteraction Callback = _Configuration.Interaction;
            {
                TGuardValue<bool> DispatchGuard(_Dispatching, true);
                Callback.ExecuteIfBound({ECkUiInteractionPhase::Begin, InSource, Initial});
            }
            FinishDeferredCancellation();
        }
        auto Change(float InNormalized) -> void
        {
            if (!_Interaction.IsSet() || !CanEdit() || !FMath::IsFinite(InNormalized)) { return; }
            const double Span = static_cast<double>(_Configuration.Max) - _Configuration.Min;
            const float Value = static_cast<float>(_Configuration.Min + FMath::Clamp(static_cast<double>(InNormalized), 0.0, 1.0) * Span);
            if (_Interaction->Draft == Value) { return; }
            _Interaction->Draft = Value;
            const FCkUiOnNumberChanged Callback = _Configuration.Changed;
            {
                TGuardValue<bool> DispatchGuard(_Dispatching, true);
                Callback.ExecuteIfBound(Value);
            }
            FinishDeferredCancellation();
        }
        auto End(bool InCommit) -> void
        {
            if (!_Interaction.IsSet()) { return; }
            if (_Dispatching) { _CancelAfterDispatch = true; _SuppressDeferredCancellation |= !CanDispatch(); return; }
            const FInteraction Finished = _Interaction.GetValue();
            _Interaction.Reset();
            const bool Commit = InCommit && CanEdit();
            if (!CanDispatch()) { return; }
            const FCkUiOnNumberInteraction Callback = _Configuration.Interaction;
            TGuardValue<bool> DispatchGuard(_Dispatching, true);
            Callback.ExecuteIfBound({Commit ? ECkUiInteractionPhase::Commit : ECkUiInteractionPhase::Cancel,
                Finished.Source, Commit ? Finished.Draft : Finished.Initial});
        }

    private:
        class FPreparedUpdate final : public ICkUiPreparedWidgetUpdate
        {
        public:
            FPreparedUpdate(TSharedRef<FSlider> InSlider, FConfiguration InConfiguration)
                : _Slider(MoveTemp(InSlider)), _Configuration(MoveTemp(InConfiguration)) {}
            virtual void Commit() noexcept override
            {
                _Slider->_Widget->SetOrientation(_Configuration.Orientation);
                _Slider->_Widget->ApplyNativeStyle(_Configuration.NativeStyle);
                _Slider->_Configuration = MoveTemp(_Configuration);
            }
        private:
            TSharedRef<FSlider> _Slider;
            FConfiguration _Configuration;
        };
        auto FinishDeferredCancellation() -> void
        {
            if (_CancelAfterDispatch)
            {
                _CancelAfterDispatch = false;
                if (_SuppressDeferredCancellation) { _Interaction.Reset(); }
                else { End(false); }
                _SuppressDeferredCancellation = false;
            }
        }
        FConfiguration _Configuration;
        bool _Dispatching = false;
        bool _CancelAfterDispatch = false;
        bool _SuppressDeferredCancellation = false;
        TSharedPtr<SCkUiSlider> _Widget;
        TOptional<FInteraction> _Interaction;
        TOptional<FCkUiPointerCapture> _Transfer;
    };

    auto SCkUiSlider::CanEdit() const -> bool
    {
        const TSharedPtr<FSlider> Slider = _Owner.Pin();
        return !_FinishingPointerInteraction && Slider.IsValid() && Slider->CanEdit();
    }
    auto SCkUiSlider::IsInputBlocked() const -> bool
    {
        const TSharedPtr<FSlider> Slider = _Owner.Pin();
        return _FinishingPointerInteraction || (Slider.IsValid() && Slider->IsDispatching());
    }
    auto SCkUiSlider::IsPointer(const FPointerEvent& InEvent) const -> bool
    {
        const TSharedPtr<FSlider> Slider = _Owner.Pin();
        return Slider.IsValid() && Slider->IsPointer(static_cast<int32>(InEvent.GetUserIndex()), InEvent.GetPointerIndex());
    }
    auto SCkUiSlider::IsForeignUser(uint32 InUser) const -> bool
    {
        const TSharedPtr<FSlider> Slider = _Owner.Pin();
        return (_TouchPending && _PendingUser != static_cast<int32>(InUser))
            || (Slider.IsValid() && Slider->IsForeignUser(InUser));
    }
    auto SCkUiSlider::Cancel() -> void
    {
        if (_FinishingPointerInteraction) { return; }
        TGuardValue<bool> FinishingGuard(_FinishingPointerInteraction, true);
        TArray<FCkUiPointerCapture> Captures;
        if (const TSharedPtr<FSlider> Slider = _Owner.Pin())
        {
            Captures = Slider->GetPointerCaptures();
            Slider->End(false);
        }
        _TouchPending = false;
        if (!FSlateApplication::IsInitialized()) { return; }
        for (const FCkUiPointerCapture& Capture : Captures)
        {
            const TSharedPtr<FSlateUser> User = FSlateApplication::Get().GetUser(Capture.UserIndex);
            if (User.IsValid() && User->GetPointerCaptor(Capture.PointerIndex).Get() == this)
            { User->ReleaseCapture(Capture.PointerIndex); }
        }
    }
    auto SCkUiSlider::OnPointerBegin() -> void
    {
        if (const TSharedPtr<FSlider> Slider = _Owner.Pin())
        { Slider->Begin(ECkUiInteractionSource::Pointer, _PendingUser, _PendingPointer); }
    }
    auto SCkUiSlider::OnValueChanged(float InValue) -> void
    {
        if (const TSharedPtr<FSlider> Slider = _Owner.Pin()) { Slider->Change(InValue); }
    }
    auto SCkUiSlider::KeepPointerReply(FReply InReply, const FPointerEvent& InEvent) -> FReply
    {
        if (!CanEdit() || !IsPointer(InEvent))
        {
            Cancel();
            return FReply::Handled();
        }
        if (InReply.GetMouseCaptor().Get() == this && FSlateApplication::IsInitialized())
        {
            FSlateApplication& Slate = FSlateApplication::Get();
            const TSharedPtr<FSlateUser> User = Slate.GetUser(static_cast<int32>(InEvent.GetUserIndex()));
            const TSharedPtr<SWidget> CurrentCaptor = User.IsValid() ? User->GetPointerCaptor(InEvent.GetPointerIndex()) : nullptr;
            FWidgetPath CurrentPath;
            if ((CurrentCaptor.IsValid() && CurrentCaptor.Get() != this)
                || !Slate.GeneratePathToWidgetUnchecked(SharedThis(this), CurrentPath))
            {
                Cancel();
                return FReply::Handled();
            }
            // Begin/Changed can reload the layout before Slate consumes the outer event reply.
            // Acquire through the current mounted path now, then return a reply with no stale capture request.
            Slate.ProcessReply(CurrentPath, InReply, &CurrentPath, &InEvent, InEvent.GetUserIndex());
            return InReply.ShouldThrottle() ? FReply::Handled() : FReply::Handled().PreventThrottling();
        }
        return InReply;
    }
    auto SCkUiSlider::OnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InEvent) -> FReply
    {
        if (IsForeignUser(InEvent.GetUserIndex())) { return FReply::Unhandled(); }
        if (IsInputBlocked()) { return FReply::Unhandled(); }
        if (!CanEdit() || InEvent.GetEffectingButton() != EKeys::LeftMouseButton) { return FReply::Unhandled(); }
        Cancel();
        if (!CanEdit()) { return FReply::Handled(); }
        _PendingUser = static_cast<int32>(InEvent.GetUserIndex());
        _PendingPointer = InEvent.GetPointerIndex();
        return KeepPointerReply(SSlider::OnMouseButtonDown(InGeometry, InEvent), InEvent);
    }
    auto SCkUiSlider::OnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InEvent) -> FReply
    {
        if (IsInputBlocked()) { return FReply::Unhandled(); }
        if (!CanEdit()) { Cancel(); return FReply::Unhandled(); }
        if (!IsPointer(InEvent)) { return FReply::Unhandled(); }
        return KeepPointerReply(SSlider::OnMouseMove(InGeometry, InEvent), InEvent);
    }
    auto SCkUiSlider::OnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InEvent) -> FReply
    {
        if (IsInputBlocked()) { return FReply::Unhandled(); }
        if (!IsPointer(InEvent) || InEvent.GetEffectingButton() != EKeys::LeftMouseButton) { return FReply::Unhandled(); }
        if (const TSharedPtr<FSlider> Slider = _Owner.Pin()) { Slider->MarkNormalRelease(); }
        return SSlider::OnMouseButtonUp(InGeometry, InEvent);
    }
    auto SCkUiSlider::OnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InEvent) -> FReply
    {
        if (IsForeignUser(InEvent.GetUserIndex())) { return FReply::Unhandled(); }
        if (IsInputBlocked()) { return FReply::Unhandled(); }
        if (!CanEdit()) { return FReply::Unhandled(); }
        bool ControllerActive = false;
        if (const TSharedPtr<FSlider> Slider = _Owner.Pin())
        {
            ControllerActive = Slider->IsController();
            if (Slider->IsActive() && !ControllerActive) { return FReply::Unhandled(); }
        }
        if (ControllerActive) { Cancel(); }
        if (!CanEdit()) { return FReply::Unhandled(); }
        // Cancellation may reenter through the consumer and establish another interaction.
        if (const TSharedPtr<FSlider> Slider = _Owner.Pin(); Slider.IsValid() && Slider->IsActive()) { return FReply::Unhandled(); }
        _PendingUser = static_cast<int32>(InEvent.GetUserIndex());
        _PendingPointer = InEvent.GetPointerIndex();
        _TouchPending = true;
        return SSlider::OnTouchStarted(InGeometry, InEvent);
    }
    auto SCkUiSlider::OnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InEvent) -> FReply
    {
        if (IsInputBlocked()) { return FReply::Unhandled(); }
        if (!_TouchPending || _PendingUser != static_cast<int32>(InEvent.GetUserIndex()) || _PendingPointer != InEvent.GetPointerIndex()) { return FReply::Unhandled(); }
        if (!CanEdit()) { Cancel(); return FReply::Unhandled(); }
        const FReply Reply = SSlider::OnTouchMoved(InGeometry, InEvent);
        return Reply.IsEventHandled() ? KeepPointerReply(Reply, InEvent) : Reply;
    }
    auto SCkUiSlider::OnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InEvent) -> FReply
    {
        if (IsInputBlocked()) { return FReply::Unhandled(); }
        if (!_TouchPending || _PendingUser != static_cast<int32>(InEvent.GetUserIndex()) || _PendingPointer != InEvent.GetPointerIndex()) { return FReply::Unhandled(); }
        _TouchPending = false;
        if (!IsPointer(InEvent)) { return FReply::Handled(); }
        if (const TSharedPtr<FSlider> Slider = _Owner.Pin()) { Slider->MarkNormalRelease(); }
        return SSlider::OnTouchEnded(InGeometry, InEvent);
    }
    void SCkUiSlider::OnMouseCaptureLost(const FCaptureLostEvent& InEvent)
    {
        // Slate erases the old pointer entry after this callback; nested input cannot reacquire it yet.
        TGuardValue<bool> FinishingGuard(_FinishingPointerInteraction, true);
        if (const TSharedPtr<FSlider> Slider = _Owner.Pin())
        {
            if (Slider->IsTransfer(InEvent)) { return; }
            if (Slider->IsPointer(InEvent.UserIndex, static_cast<uint32>(InEvent.PointerIndex)))
            {
                _TouchPending = false;
                Slider->End(Slider->IsNormalRelease());
            }
        }
        SSlider::OnMouseCaptureLost(InEvent);
    }
    auto SCkUiSlider::OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InEvent) -> FReply
    {
        if (IsForeignUser(InEvent.GetUserIndex())) { return FReply::Unhandled(); }
        if (IsInputBlocked()) { return FReply::Unhandled(); }
        if (InEvent.GetKey() == EKeys::Escape || InEvent.GetKey() == EKeys::Gamepad_FaceButton_Right)
        {
            const TSharedPtr<FSlider> Slider = _Owner.Pin();
            if (!_TouchPending && (!Slider.IsValid() || !Slider->IsActive())) { return FReply::Unhandled(); }
            Cancel();
            return FReply::Handled();
        }
        if (!CanEdit()) { Cancel(); return FReply::Unhandled(); }
        if (InEvent.GetKey().IsGamepadKey()
            && FSlateApplication::Get().GetNavigationActionFromKey(InEvent) == EUINavigationAction::Accept)
        {
            if (const TSharedPtr<FSlider> Slider = _Owner.Pin())
            {
                if (Slider->IsController()) { Slider->End(true); }
                else if (!Slider->IsActive()) { Slider->Begin(ECkUiInteractionSource::Controller, static_cast<int32>(InEvent.GetUserIndex())); }
            }
            return FReply::Handled();
        }
        return SWidget::OnKeyDown(InGeometry, InEvent);
    }
    auto SCkUiSlider::OnNavigation(const FGeometry& InGeometry, const FNavigationEvent& InEvent) -> FNavigationReply
    {
        if (IsForeignUser(InEvent.GetUserIndex())) { return FNavigationReply::Escape(); }
        if (IsInputBlocked()) { return FNavigationReply::Escape(); }
        if (!CanEdit()) { Cancel(); return FNavigationReply::Escape(); }
        const EUINavigation Direction = InEvent.GetNavigationType();
        bool Controller = false;
        if (const TSharedPtr<FSlider> Slider = _Owner.Pin())
        {
            Controller = Slider->IsController();
            if (InEvent.GetNavigationGenesis() == ENavigationGenesis::Controller && !Controller) { return FNavigationReply::Escape(); }
            if (Slider->IsActive() && !Controller) { return FNavigationReply::Stop(); }
            const bool AlongAxis = Slider->IsHorizontal()
                ? Direction == EUINavigation::Left || Direction == EUINavigation::Right
                : Direction == EUINavigation::Up || Direction == EUINavigation::Down;
            if (!AlongAxis) { return Controller ? FNavigationReply::Stop() : FNavigationReply::Escape(); }
            if (!Controller) { Slider->Begin(ECkUiInteractionSource::Keyboard, static_cast<int32>(InEvent.GetUserIndex())); }
        }
        if (!CanEdit()) { Cancel(); return FNavigationReply::Stop(); }
        const FNavigationReply Reply = SSlider::OnNavigation(InGeometry, InEvent);
        if (!Controller)
        {
            if (const TSharedPtr<FSlider> Slider = _Owner.Pin()) { Slider->End(true); }
        }
        return Reply;
    }
    void SCkUiSlider::OnFocusLost(const FFocusEvent& InEvent)
    {
        const TSharedPtr<FSlider> Slider = _Owner.Pin();
        if (!IsForeignUser(InEvent.GetUser()) && Slider.IsValid() && Slider->CanDispatch()) { Cancel(); }
        SSlider::OnFocusLost(InEvent);
    }
    void SCkUiSlider::Tick(const FGeometry& InGeometry, double InTime, float InDeltaTime)
    {
        if (IsInputBlocked()) { return; }
        SSlider::Tick(InGeometry, InTime, InDeltaTime);
        if (!CanEdit()) { Cancel(); }
    }

    auto MakeConfiguration(const FCkUiCustomWidgetArguments& InArguments, FConfiguration& OutConfiguration, FString& OutFailure) -> bool
    {
        OutConfiguration.NativeStyle = FCoreStyle::Get().GetWidgetStyle<FSliderStyle>(TEXT("Slider"));
        const auto SetColor = [&InArguments](const TCHAR* InName, FSlateBrush& InOutBrush)
        {
            if (const FCkUiCustomStyleValue* Value = InArguments.Style.CustomProperties.Find(InName))
            { InOutBrush.TintColor = FSlateColor(Value->Color); }
        };
        SetColor(TEXT("-ck-slider-bar-color"), OutConfiguration.NativeStyle.NormalBarImage);
        SetColor(TEXT("-ck-slider-bar-hover-color"), OutConfiguration.NativeStyle.HoveredBarImage);
        SetColor(TEXT("-ck-slider-bar-disabled-color"), OutConfiguration.NativeStyle.DisabledBarImage);
        SetColor(TEXT("-ck-slider-thumb-color"), OutConfiguration.NativeStyle.NormalThumbImage);
        SetColor(TEXT("-ck-slider-thumb-hover-color"), OutConfiguration.NativeStyle.HoveredThumbImage);
        SetColor(TEXT("-ck-slider-thumb-disabled-color"), OutConfiguration.NativeStyle.DisabledThumbImage);
        if (const FCkUiCustomStyleValue* Thickness = InArguments.Style.CustomProperties.Find(TEXT("-ck-slider-bar-thickness")))
        { OutConfiguration.NativeStyle.BarThickness = Thickness->Number; }
        for (FSlateBrush* Thumb : {&OutConfiguration.NativeStyle.NormalThumbImage,
            &OutConfiguration.NativeStyle.HoveredThumbImage, &OutConfiguration.NativeStyle.DisabledThumbImage})
        {
            if (const FCkUiCustomStyleValue* Width = InArguments.Style.CustomProperties.Find(TEXT("-ck-slider-thumb-width")))
            { Thumb->ImageSize.X = Width->Number; }
            if (const FCkUiCustomStyleValue* Height = InArguments.Style.CustomProperties.Find(TEXT("-ck-slider-thumb-height")))
            { Thumb->ImageSize.Y = Height->Number; }
        }
        const TAttribute<float>* Value = InArguments.NumberBindings.Find(TEXT("value"));
        const FCkUiOnNumberInteraction* Interaction = InArguments.NumberInteraction.Find(TEXT("interaction"));
        const FString* Binding = InArguments.BindingNames.Find(TEXT("value"));
        if (Value == nullptr || !Value->IsSet() || Interaction == nullptr || !Interaction->IsBound() || Binding == nullptr || Binding->IsEmpty())
        { OutFailure = TEXT("Slider requires value and interaction bindings."); return false; }
        if (const FText* Orientation = InArguments.TextProperties.Find(TEXT("orientation")))
        {
            const FString Name = Orientation->ToString();
            if (!Name.Equals(TEXT("horizontal"), ESearchCase::CaseSensitive)
                && !Name.Equals(TEXT("vertical"), ESearchCase::CaseSensitive))
            { OutFailure = TEXT("Slider orientation must be horizontal or vertical."); return false; }
            OutConfiguration.Orientation = Name == TEXT("vertical") ? Orient_Vertical : Orient_Horizontal;
        }
        OutConfiguration.Value = *Value;
        OutConfiguration.ValueBinding = *Binding;
        OutConfiguration.Interaction = *Interaction;
        OutConfiguration.Changed = InArguments.NumberChanged.FindRef(TEXT("changed"));
        OutConfiguration.Enabled = InArguments.BoolBindings.FindRef(TEXT("enabled"));
        OutConfiguration.ReadOnly = InArguments.BoolBindings.FindRef(TEXT("read-only"));
        OutConfiguration.CanDispatchEvents = InArguments.CanDispatchEvents;
        if (const float* Min = InArguments.NumberProperties.Find(TEXT("min"))) { OutConfiguration.Min = *Min; }
        if (const float* Max = InArguments.NumberProperties.Find(TEXT("max"))) { OutConfiguration.Max = *Max; }
        if (const float* Step = InArguments.NumberProperties.Find(TEXT("step"))) { OutConfiguration.Step = *Step; }
        const double Span = static_cast<double>(OutConfiguration.Max) - OutConfiguration.Min;
        const bool RangeValid = FMath::IsFinite(OutConfiguration.Min) && FMath::IsFinite(OutConfiguration.Max)
            && FMath::IsFinite(OutConfiguration.Step) && Span > 0.0 && OutConfiguration.Step > 0.0f && OutConfiguration.Step <= Span;
        if (!RangeValid)
        { OutFailure = TEXT("Slider requires a finite increasing range and a positive step no larger than that range."); return false; }
        const float NormalizedStep = static_cast<float>(OutConfiguration.Step / Span);
        if (1.0f + NormalizedStep == 1.0f || OutConfiguration.Min + OutConfiguration.Step == OutConfiguration.Min
            || OutConfiguration.Max - OutConfiguration.Step == OutConfiguration.Max)
        { OutFailure = TEXT("Slider step is too small for the range's float precision."); return false; }
        return true;
    }
    auto FSlider::PrepareReload(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) const -> TUniquePtr<ICkUiPreparedWidgetUpdate>
    {
        FConfiguration Configuration;
        if (!MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
        if (Configuration.ValueBinding != _Configuration.ValueBinding)
        { OutFailure = TEXT("Slider value binding cannot change for a retained id."); return {}; }
        if (Configuration.Orientation != _Configuration.Orientation
            && (_Interaction.IsSet() || _Widget->HasPendingTouch()))
        { OutFailure = TEXT("Slider orientation cannot change during an interaction or pending touch."); return {}; }
        const bool GeometryChanged = Configuration.NativeStyle.BarThickness != _Configuration.NativeStyle.BarThickness
            || Configuration.NativeStyle.NormalThumbImage.ImageSize != _Configuration.NativeStyle.NormalThumbImage.ImageSize
            || Configuration.NativeStyle.HoveredThumbImage.ImageSize != _Configuration.NativeStyle.HoveredThumbImage.ImageSize
            || Configuration.NativeStyle.DisabledThumbImage.ImageSize != _Configuration.NativeStyle.DisabledThumbImage.ImageSize;
        if (GeometryChanged && (_Interaction.IsSet() || _Widget->HasPendingTouch()))
        { OutFailure = TEXT("Slider style dimensions cannot change during an interaction or pending touch."); return {}; }
        if (_Interaction.IsSet() && (Configuration.Min != _Configuration.Min || Configuration.Max != _Configuration.Max || Configuration.Step != _Configuration.Step))
        { OutFailure = TEXT("Slider range and step cannot change during an interaction."); return {}; }
        return MakeUnique<FPreparedUpdate>(ConstCastSharedRef<FSlider>(AsShared()), MoveTemp(Configuration));
    }
}

auto FCkUiSlider::Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult
{
    FCkUiCustomWidgetRegistration Registration;
    Registration.Schema.Tag = TEXT("slider");
    Registration.Schema.StyleProperties = {
        {TEXT("-ck-slider-bar-color"), ECkUiCustomStyleKind::Color},
        {TEXT("-ck-slider-bar-hover-color"), ECkUiCustomStyleKind::Color},
        {TEXT("-ck-slider-bar-disabled-color"), ECkUiCustomStyleKind::Color},
        {TEXT("-ck-slider-thumb-color"), ECkUiCustomStyleKind::Color},
        {TEXT("-ck-slider-thumb-hover-color"), ECkUiCustomStyleKind::Color},
        {TEXT("-ck-slider-thumb-disabled-color"), ECkUiCustomStyleKind::Color},
        {TEXT("-ck-slider-thumb-width"), ECkUiCustomStyleKind::Length},
        {TEXT("-ck-slider-thumb-height"), ECkUiCustomStyleKind::Length},
        {TEXT("-ck-slider-bar-thickness"), ECkUiCustomStyleKind::Length}
    };
    Registration.Schema.Properties = {
        {TEXT("value"), ECkUiCustomPropertyKind::NumberBinding, true},
        {TEXT("interaction"), ECkUiCustomPropertyKind::NumberInteraction, true},
        {TEXT("changed"), ECkUiCustomPropertyKind::NumberChanged, false},
        {TEXT("enabled"), ECkUiCustomPropertyKind::BoolBinding, false},
        {TEXT("read-only"), ECkUiCustomPropertyKind::BoolBinding, false},
        {TEXT("min"), ECkUiCustomPropertyKind::Number, false},
        {TEXT("max"), ECkUiCustomPropertyKind::Number, false},
        {TEXT("step"), ECkUiCustomPropertyKind::Number, false},
        {TEXT("orientation"), ECkUiCustomPropertyKind::Text, false}
    };
    Registration.RetainedFactory = [](const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) -> TSharedPtr<ICkUiRetainedWidget>
    {
        ck_ui_slider::FConfiguration Configuration;
        if (!ck_ui_slider::MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
        const TSharedRef<ck_ui_slider::FSlider> Slider = MakeShared<ck_ui_slider::FSlider>(MoveTemp(Configuration));
        Slider->Initialize(InArguments.Id);
        return Slider;
    };
    return InRegistry.Register(MoveTemp(Registration));
}
