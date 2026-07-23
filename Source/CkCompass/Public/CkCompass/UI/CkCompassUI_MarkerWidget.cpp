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
    DoResolveDisplay()
    -> void
{
    if (ck::Is_NOT_Valid(_Icon))
    { return; }

    // Preview / dead-POI entries keep whatever the designer authored on the Icon
    if (ck::Is_NOT_Valid(_AssignedPoi))
    { return; }

    // Resolve the compass-consumer display definition for this POI; unset (no definition for this consumer)
    // -> null soft ptr, the same "keep authored icon" path as an invalid POI above.
    const auto DisplayDefinition = UCk_Utils_PoiDisplayDefinition_UE::TryGet_PoiDisplayDefinition_ByConsumer(
        _AssignedPoi, Tag_PoiConsumer_Compass);

    const auto DisplaySoft = ck::IsValid(DisplayDefinition)
        ? UCk_Utils_PoiDisplayDefinition_UE::Get_DisplayAsset(DisplayDefinition)
        : TSoftObjectPtr<UCk_Poi_DisplayDefinition_PDA>{};

    if (DisplaySoft.IsNull())
    { return; }

    // Bounded hitch: a tiny PDA + one icon texture, once per POI assignment
    const auto Display = DisplaySoft.LoadSynchronous();

    if (ck::Is_NOT_Valid(Display))
    { return; }

    if (NOT Display->Get_Icon().IsNull())
    {
        if (const auto IconTexture = Display->Get_Icon().LoadSynchronous();
            ck::IsValid(IconTexture))
        { _Icon->SetBrushFromTexture(IconTexture, false); }
    }

    _Icon->SetColorAndOpacity(Display->Get_Tint());
    _Icon->SetDesiredSizeOverride(Display->Get_SizeHint());
}

// --------------------------------------------------------------------------------------------------------------------
