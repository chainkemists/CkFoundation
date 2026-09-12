#include "CkSlateLayout/CkUiColorPicker.h"

#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Layout/WidgetPath.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SWindow.h"

namespace ck_ui_color_picker
{
    struct FConfiguration
    {
        TAttribute<FLinearColor> Value;
        TAttribute<bool> Enabled;
        TAttribute<bool> ReadOnly;
        TAttribute<bool> CanDispatchEvents;
        FCkUiOnColorCommitted Committed;
        FString ValueBindingName;
        int32 SlateUserIndex = INDEX_NONE;
        bool UseAlpha = false;
    };

    auto IsFinite(const FLinearColor InColor) -> bool
    {
        return FMath::IsFinite(InColor.R) && FMath::IsFinite(InColor.G)
            && FMath::IsFinite(InColor.B) && FMath::IsFinite(InColor.A);
    }

    auto Clamp(const FLinearColor InColor, const bool bUseAlpha) -> FLinearColor
    {
        return FLinearColor(FMath::Clamp(InColor.R, 0.0f, 1.0f), FMath::Clamp(InColor.G, 0.0f, 1.0f),
            FMath::Clamp(InColor.B, 0.0f, 1.0f), bUseAlpha ? FMath::Clamp(InColor.A, 0.0f, 1.0f) : 1.0f);
    }

    class FColorPicker final : public ICkUiRetainedWidget, public TSharedFromThis<FColorPicker>
    {
    public:
        explicit FColorPicker(FConfiguration InConfiguration)
            : _Configuration(MoveTemp(InConfiguration))
        {
        }

        virtual ~FColorPicker() override
        {
            _Active = false;
            ReleasePopup();
        }

        void Initialize(const FString& InId)
        {
            const TWeakPtr<FColorPicker> WeakPicker = AsShared();
            SAssignNew(_Widget, SButton)
                .Tag(FName(*InId))
                .IsEnabled(TAttribute<bool>::CreateLambda([WeakPicker]()
                {
                    const TSharedPtr<FColorPicker> Picker = WeakPicker.Pin();
                    return Picker.IsValid() && Picker->CanOpen();
                }))
                .OnClicked_Lambda([WeakPicker]()
                {
                    if (const TSharedPtr<FColorPicker> Picker = WeakPicker.Pin()) { Picker->Open(); }
                    return FReply::Handled();
                })
                [
                    SNew(SColorBlock)
                        .Color_Lambda([WeakPicker]()
                        {
                            const TSharedPtr<FColorPicker> Picker = WeakPicker.Pin();
                            return Picker.IsValid() ? Picker->CurrentColor() : FLinearColor::Transparent;
                        })
                        .Size(FVector2D(64.0f, 18.0f))
                        .CornerRadius(FVector4(3.0f))
                        .ShowBackgroundForAlpha(false)
                ];
        }

        virtual auto GetWidget() const -> TSharedRef<SWidget> override { return _Widget.ToSharedRef(); }
        virtual auto GetFocusTransferTarget() const -> TSharedPtr<SWidget> override
        { return _PopupReady ? _Window : nullptr; }
        virtual void ReleaseTransientInteraction() override { ReleasePopup(); }
        virtual void ReleaseOwnerInteraction() override { ReleasePopup(); }
        virtual auto PrepareReload(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) const -> TUniquePtr<ICkUiPreparedWidgetUpdate> override;

    private:
        class FPreparedUpdate final : public ICkUiPreparedWidgetUpdate
        {
        public:
            FPreparedUpdate(TSharedRef<FColorPicker> InPicker, FConfiguration InConfiguration)
                : _Picker(MoveTemp(InPicker)), _Configuration(MoveTemp(InConfiguration))
            {
            }

            virtual void Commit() noexcept override { _Picker->ApplyConfiguration(MoveTemp(_Configuration)); }

        private:
            TSharedRef<FColorPicker> _Picker;
            FConfiguration _Configuration;
        };

        auto CanDispatch() const -> bool
        {
            return _Active && _Configuration.CanDispatchEvents.Get(false) && _Configuration.Enabled.Get(true)
                && !_Configuration.ReadOnly.Get(false) && _Configuration.SlateUserIndex >= 0
                && FSlateApplication::IsInitialized() && FSlateApplication::Get().GetUser(_Configuration.SlateUserIndex).IsValid();
        }

        auto CanOpen() const -> bool
        {
            return CanDispatch() && IsFinite(_Configuration.Value.Get(FLinearColor::White));
        }

        auto CurrentColor() const -> FLinearColor
        {
            const FLinearColor Value = _Configuration.Value.Get(FLinearColor::White);
            return IsFinite(Value) ? Clamp(Value, _Configuration.UseAlpha) : FLinearColor::White;
        }

        auto Open() -> void
        {
            if (_Opening || !CanOpen() || !_Widget.IsValid()) { return; }
            TGuardValue<bool> OpeningGuard(_Opening, true);
            ReleasePopup();
            if (!CanOpen()) { return; }

            FSlateApplication& Slate = FSlateApplication::Get();
            FWidgetPath OwnerPath;
            if (!Slate.GeneratePathToWidgetUnchecked(_Widget.ToSharedRef(), OwnerPath)) { return; }

            const uint64 Generation = _Generation;
            const FVector2D Cursor = Slate.GetCursorPos();
            const FSlateRect Anchor(Cursor.X, Cursor.Y, Cursor.X, Cursor.Y);
            const FVector2D EstimatedSize = SColorPicker::DEFAULT_WINDOW_SIZE + FVector2D(0.0f, 130.0f);
            const FVector2D Position = Slate.CalculatePopupWindowPosition(Anchor, EstimatedSize, true, FVector2D::ZeroVector, Orient_Horizontal);
            const TSharedRef<SWindow> Window = SNew(SWindow)
                .AutoCenter(EAutoCenter::None)
                .ScreenPosition(Position)
                .SupportsMaximize(false)
                .SupportsMinimize(false)
                .SizingRule(ESizingRule::Autosized)
                .FocusWhenFirstShown(false)
                .FocusUserIndex(_Configuration.SlateUserIndex)
                .Title(NSLOCTEXT("CkSlateLayout", "ColorPickerTitle", "Color Picker"));

            const TWeakPtr<FColorPicker> WeakPicker = AsShared();
            const TSharedRef<SColorPicker> Picker = SNew(SColorPicker)
                .TargetColorAttribute(TAttribute<FLinearColor>::CreateLambda([WeakPicker]()
                {
                    if (const TSharedPtr<FColorPicker> Existing = WeakPicker.Pin()) { return Existing->CurrentColor(); }
                    return FLinearColor::White;
                }))
                .UseAlpha(_Configuration.UseAlpha)
                .OnlyRefreshOnMouseUp(true)
                .OnlyRefreshOnOk(false)
                .ClampValue(true)
                .ParentWindow(Window)
                .OnColorCommitted(FOnLinearColorValueChanged::CreateLambda([WeakPicker, Generation](const FLinearColor InColor)
                {
                    if (const TSharedPtr<FColorPicker> Picker = WeakPicker.Pin()) { Picker->Commit(InColor, Generation); }
                }))
                .OnColorPickerWindowClosed(FOnWindowClosed::CreateLambda([WeakPicker, Generation](const TSharedRef<SWindow>&)
                {
                    if (const TSharedPtr<FColorPicker> Picker = WeakPicker.Pin()) { Picker->Dismissed(Generation); }
                }));
            Window->SetContent(Picker);
            if (Generation != _Generation || !CanOpen()) { return; }
            _PopupReady = false;
            _Window = Window;
            _Picker = Picker;
            Slate.AddWindowAsNativeChild(Window, OwnerPath.GetWindow());
            if (Generation != _Generation || !CanOpen() || _Window != Window)
            {
                if (_Window == Window) { ReleasePopup(); }
                else { Slate.DestroyWindowImmediately(Window); }
                return;
            }
            Slate.SetUserFocus(_Configuration.SlateUserIndex, Window, EFocusCause::SetDirectly);
            if (Generation != _Generation || !CanOpen() || _Window != Window)
            {
                if (_Window == Window) { ReleasePopup(); }
                else { Slate.DestroyWindowImmediately(Window); }
                return;
            }
            _PopupReady = true;
        }

        auto Commit(const FLinearColor InColor, const uint64 InGeneration) -> void
        {
            if (InGeneration != _Generation || !_PopupReady || !_Window.IsValid() || _Updating || !CanDispatch() || !IsFinite(InColor)) { return; }
            const FCkUiOnColorCommitted Committed = _Configuration.Committed;
            Committed.ExecuteIfBound(Clamp(InColor, _Configuration.UseAlpha));
        }

        auto Dismissed(const uint64 InGeneration) -> void
        {
            if (InGeneration != _Generation) { return; }
            _PopupReady = false;
            ++_Generation;
            _Window.Reset();
            _Picker.Reset();
        }

        auto ReleasePopup() -> void
        {
            _PopupReady = false;
            ++_Generation;
            const TSharedPtr<SWindow> Window = MoveTemp(_Window);
            _Picker.Reset();
            if (Window.IsValid() && FSlateApplication::IsInitialized()) { Window->RequestDestroyWindow(); }
        }

        auto ApplyConfiguration(FConfiguration InConfiguration) -> void
        {
            TGuardValue<bool> UpdatingGuard(_Updating, true);
            _Configuration = MoveTemp(InConfiguration);
        }

        TSharedPtr<SButton> _Widget;
        FConfiguration _Configuration;
        TSharedPtr<SWindow> _Window;
        TSharedPtr<SColorPicker> _Picker;
        uint64 _Generation = 0;
        bool _Active = true;
        bool _Opening = false;
        bool _PopupReady = false;
        bool _Updating = false;
    };

    auto MakeConfiguration(const FCkUiCustomWidgetArguments& InArguments, FConfiguration& OutConfiguration, FString& OutFailure) -> bool
    {
        const TAttribute<FLinearColor>* Value = InArguments.ColorBindings.Find(TEXT("value"));
        const FCkUiOnColorCommitted* Committed = InArguments.ColorCommitted.Find(TEXT("committed"));
        const FString* ValueBindingName = InArguments.BindingNames.Find(TEXT("value"));
        if (InArguments.SlateUserIndex < 0)
        { OutFailure = TEXT("Color picker requires a nonnegative host Slate user."); return false; }
        if (Value == nullptr || !Value->IsSet() || Committed == nullptr || !Committed->IsBound()
            || ValueBindingName == nullptr || ValueBindingName->IsEmpty())
        { OutFailure = TEXT("Color picker requires value and committed bindings."); return false; }
        OutConfiguration.Value = *Value;
        OutConfiguration.Committed = *Committed;
        OutConfiguration.ValueBindingName = *ValueBindingName;
        OutConfiguration.Enabled = InArguments.BoolBindings.FindRef(TEXT("enabled"));
        OutConfiguration.ReadOnly = InArguments.BoolBindings.FindRef(TEXT("read-only"));
        OutConfiguration.CanDispatchEvents = InArguments.CanDispatchEvents;
        OutConfiguration.SlateUserIndex = InArguments.SlateUserIndex;
        OutConfiguration.UseAlpha = InArguments.BoolProperties.FindRef(TEXT("alpha"));
        return true;
    }

    auto FColorPicker::PrepareReload(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) const -> TUniquePtr<ICkUiPreparedWidgetUpdate>
    {
        FConfiguration Configuration;
        if (!MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
        if (Configuration.ValueBindingName != _Configuration.ValueBindingName || Configuration.SlateUserIndex != _Configuration.SlateUserIndex
            || Configuration.UseAlpha != _Configuration.UseAlpha)
        { OutFailure = TEXT("Color picker value binding, host user, and alpha policy cannot change for a retained id."); return {}; }
        return MakeUnique<FPreparedUpdate>(ConstCastSharedRef<FColorPicker>(AsShared()), MoveTemp(Configuration));
    }
}

auto FCkUiColorPicker::Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult
{
    auto Registration = FCkUiCustomWidgetRegistration{};
    Registration.Schema.Tag = TEXT("color-picker");
    Registration.Schema.Properties = {
        {TEXT("value"), ECkUiCustomPropertyKind::ColorBinding, true},
        {TEXT("committed"), ECkUiCustomPropertyKind::ColorCommitted, true},
        {TEXT("enabled"), ECkUiCustomPropertyKind::BoolBinding, false},
        {TEXT("read-only"), ECkUiCustomPropertyKind::BoolBinding, false},
        {TEXT("alpha"), ECkUiCustomPropertyKind::Bool, false},
    };
    Registration.RetainedFactory = [](const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) -> TSharedPtr<ICkUiRetainedWidget>
    {
        auto Configuration = ck_ui_color_picker::FConfiguration{};
        if (!ck_ui_color_picker::MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
        const TSharedRef<ck_ui_color_picker::FColorPicker> Picker = MakeShared<ck_ui_color_picker::FColorPicker>(MoveTemp(Configuration));
        Picker->Initialize(InArguments.Id);
        return Picker;
    };
    return InRegistry.Register(MoveTemp(Registration));
}
