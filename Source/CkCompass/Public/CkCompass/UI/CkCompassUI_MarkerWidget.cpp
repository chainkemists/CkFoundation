#include "CkCompassUI_MarkerWidget.h"

#include "CkCompass/CkCompass_Utils.h"

#include "CkPoiDisplayDefinition/CkPoiDisplayDefinition_Utils.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CompassUI_MarkerWidget::
    InjectEntry(
        const FCk_Compass_Entry& InEntry)
    -> void
{
    if (InEntry.Get_Poi() != _AssignedPoi)
    {
        _AssignedPoi = InEntry.Get_Poi();
        _LastDistanceMeters = -1;
        DoResolveDisplay();
    }

    if (ck::IsValid(_DistanceText))
    {
        if (_DistanceTextVisibleBeyondCm > 0.0f)
        {
            const auto ShowDistance = InEntry.Get_Distance() >= _DistanceTextVisibleBeyondCm;
            _DistanceText->SetVisibility(ShowDistance
                ? ESlateVisibility::HitTestInvisible
                : ESlateVisibility::Collapsed);
        }

        const auto Meters = FMath::RoundToInt(InEntry.Get_Distance() / 100.0f);

        if (Meters != _LastDistanceMeters)
        {
            _LastDistanceMeters = Meters;
            _DistanceText->SetText(FText::Format(NSLOCTEXT("CkCompass", "MarkerDistance", "{0} m"), Meters));
        }
    }

    OnEntryUpdated(InEntry);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CompassUI_MarkerWidget::
    NotifyShown()
    -> void
{
    OnMarkerShown();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CompassUI_MarkerWidget::
    NativePreConstruct()
    -> void
{
    Super::NativePreConstruct();

    if (NOT IsDesignTime())
    { return; }

    InjectEntry(FCk_Compass_Entry{
        FCk_Handle_Poi{}, FGameplayTag{}, 0.0f, 0.0f,
        ECk_Compass_EntryArcState::InsideArc, _PreviewDistanceCm, 0.0f, 0, 1.0f});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CompassUI_MarkerWidget::
    NativeDestruct()
    -> void
{
    DoUnbindDisplayChanged();

    Super::NativeDestruct();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CompassUI_MarkerWidget::
    DoResolveDisplay()
    -> void
{
    // The binding follows the POI, not the widget: drop it here so a pooled marker never keeps listening to
    // the definition of a POI it no longer represents.
    DoUnbindDisplayChanged();

    if (ck::Is_NOT_Valid(_Icon))
    { return; }

    // Preview / dead-POI entries keep whatever the designer authored on the Icon
    if (ck::Is_NOT_Valid(_AssignedPoi))
    { return; }

    auto DisplayDefinition = UCk_Utils_PoiDisplayDefinition_UE::TryGet_PoiDisplayDefinition_ByConsumer(
        _AssignedPoi, Tag_PoiConsumer_Compass);

    // No definition for this consumer -> fall back to the AUTHORED look. Restoring rather than
    // simply returning is what stops a pooled marker inheriting the last POI it displayed.
    if (ck::Is_NOT_Valid(DisplayDefinition))
    {
        DoRestoreAuthoredDisplay();
        return;
    }

    DoApplyDisplay(DisplayDefinition);

    auto Delegate = FCk_Delegate_PoiDisplayDefinition_DisplayChanged{};
    Delegate.BindDynamic(this, &UCk_CompassUI_MarkerWidget::DoOnDisplayChanged);

    UCk_Utils_PoiDisplayDefinition_UE::BindTo_OnDisplayChanged(DisplayDefinition, Delegate,
        ECk_Signal_BindingPolicy::FireIfPayloadInFlight, ECk_Signal_PostFireBehavior::DoNothing);

    _BoundDisplayDefinition = DisplayDefinition;
}

auto
    UCk_CompassUI_MarkerWidget::
    DoApplyDisplay(
        const FCk_Handle_PoiDisplayDefinition& InDefinition)
    -> void
{
    if (ck::Is_NOT_Valid(_Icon))
    { return; }

    DoCaptureAuthoredDisplay();

    const auto IconSoft = UCk_Utils_PoiDisplayDefinition_UE::Get_Icon(InDefinition);

    // A null icon means "no icon authored" — leave whatever the designer put on the brush rather than
    // blanking it, which is the same contract as having no definition at all.
    if (NOT IconSoft.IsNull())
    {
        // Bounded hitch: one icon texture, once per POI assignment. The icon is immutable on a
        // definition, so this never re-runs on a display change.
        if (const auto IconTexture = IconSoft.LoadSynchronous();
            ck::IsValid(IconTexture))
        { _Icon->SetBrushFromTexture(IconTexture, false); }
    }

    DoApplyMutableDisplay(InDefinition);
}

auto
    UCk_CompassUI_MarkerWidget::
    DoApplyMutableDisplay(
        const FCk_Handle_PoiDisplayDefinition& InDefinition)
    -> void
{
    if (ck::Is_NOT_Valid(_Icon))
    { return; }

    _Icon->SetColorAndOpacity(UCk_Utils_PoiDisplayDefinition_UE::Get_Tint(InDefinition));
    _Icon->SetDesiredSizeOverride(UCk_Utils_PoiDisplayDefinition_UE::Get_SizeHint(InDefinition));
}

auto
    UCk_CompassUI_MarkerWidget::
    DoUnbindDisplayChanged()
    -> void
{
    // The definition entity can die before the widget does (POI destroyed while this marker was pooled out),
    // so the validity check is load-bearing, not defensive.
    if (ck::Is_NOT_Valid(_BoundDisplayDefinition))
    {
        _BoundDisplayDefinition = FCk_Handle_PoiDisplayDefinition{};
        return;
    }

    auto Delegate = FCk_Delegate_PoiDisplayDefinition_DisplayChanged{};
    Delegate.BindDynamic(this, &UCk_CompassUI_MarkerWidget::DoOnDisplayChanged);

    UCk_Utils_PoiDisplayDefinition_UE::UnbindFrom_OnDisplayChanged(_BoundDisplayDefinition, Delegate);

    _BoundDisplayDefinition = FCk_Handle_PoiDisplayDefinition{};
}

auto
    UCk_CompassUI_MarkerWidget::
    DoOnDisplayChanged(
        FCk_Handle_PoiDisplayDefinition InDefinition)
    -> void
{
    // Only the mutable half: the signal cannot mean "the icon changed", so re-resolving and
    // re-loading the texture on every tint change would be pure waste.
    DoApplyMutableDisplay(InDefinition);
}

// Captured on first override rather than in Construct: the ribbon can inject an entry before
// this widget constructs, and capturing after an override would bake another POI's look in as
// "authored".
auto
    UCk_CompassUI_MarkerWidget::
    DoCaptureAuthoredDisplay()
    -> void
{
    if (_AuthoredCaptured || ck::Is_NOT_Valid(_Icon))
    { return; }

    _AuthoredCaptured = true;
    _AuthoredBrush = _Icon->GetBrush();
    _AuthoredTint = _Icon->GetColorAndOpacity();
}

auto
    UCk_CompassUI_MarkerWidget::
    DoRestoreAuthoredDisplay()
    -> void
{
    if (NOT _AuthoredCaptured || ck::Is_NOT_Valid(_Icon))
    { return; }

    _Icon->SetBrush(_AuthoredBrush);
    _Icon->SetColorAndOpacity(_AuthoredTint);
    _Icon->SetDesiredSizeOverride(_AuthoredBrush.ImageSize);
}

// --------------------------------------------------------------------------------------------------------------------
