#include "CkCompassUI_RibbonWidget.h"

#include "CkCompass/CkCompass_Log.h"
#include "CkCompass/UI/CkCompassUI_MarkerWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"

#include "Materials/MaterialInstanceDynamic.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CompassUI_RibbonWidget::
    InjectCompass(
        FCk_Handle_Compass& InCompass)
    -> void
{
    auto AppearedDelegate = FCk_Delegate_Compass_EntryAppeared{};
    AppearedDelegate.BindDynamic(this, &UCk_CompassUI_RibbonWidget::HandleEntryAppeared);

    auto DisappearedDelegate = FCk_Delegate_Compass_EntryDisappeared{};
    DisappearedDelegate.BindDynamic(this, &UCk_CompassUI_RibbonWidget::HandleEntryDisappeared);

    if (ck::IsValid(_Compass))
    {
        UCk_Utils_Compass_UE::UnbindFrom_OnEntryAppeared(_Compass, AppearedDelegate);
        UCk_Utils_Compass_UE::UnbindFrom_OnEntryDisappeared(_Compass, DisappearedDelegate);
    }

    _Compass = InCompass;
    _PendingShownPois.Reset();

    CK_ENSURE_IF_NOT(ck::IsValid(_Compass), TEXT("Invalid Compass Handle passed to CompassRibbon Widget [{}]"), this)
    { return; }

    for (const auto& Entry : UCk_Utils_Compass_UE::Get_Entries(_Compass))
    {
        OnCompassEntryAppeared(Entry);
    }

    UCk_Utils_Compass_UE::BindTo_OnEntryAppeared(_Compass, AppearedDelegate,
        ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior::DoNothing);

    UCk_Utils_Compass_UE::BindTo_OnEntryDisappeared(_Compass, DisappearedDelegate,
        ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior::DoNothing);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CompassUI_RibbonWidget::
    NativePreConstruct()
    -> void
{
    Super::NativePreConstruct();

    DoResolveBindings();

    if (IsDesignTime())
    {
        DoLayoutPreview();
    }
}

auto
    UCk_CompassUI_RibbonWidget::
    NativeDestruct()
    -> void
{
    if (ck::IsValid(_Compass))
    {
        auto AppearedDelegate = FCk_Delegate_Compass_EntryAppeared{};
        AppearedDelegate.BindDynamic(this, &UCk_CompassUI_RibbonWidget::HandleEntryAppeared);

        auto DisappearedDelegate = FCk_Delegate_Compass_EntryDisappeared{};
        DisappearedDelegate.BindDynamic(this, &UCk_CompassUI_RibbonWidget::HandleEntryDisappeared);

        UCk_Utils_Compass_UE::UnbindFrom_OnEntryAppeared(_Compass, AppearedDelegate);
        UCk_Utils_Compass_UE::UnbindFrom_OnEntryDisappeared(_Compass, DisappearedDelegate);

        _Compass = {};
    }

    Super::NativeDestruct();
}

auto
    UCk_CompassUI_RibbonWidget::
    NativeTick(
        const FGeometry& MyGeometry,
        float InDeltaTime)
    -> void
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    DoRefreshLayout();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CompassUI_RibbonWidget::
    HandleEntryAppeared(
        FCk_Handle_Compass InCompass,
        FCk_Compass_Entry InEntry)
    -> void
{
    _PendingShownPois.AddUnique(InEntry.Get_Poi());
    OnCompassEntryAppeared(InEntry);
}

auto
    UCk_CompassUI_RibbonWidget::
    HandleEntryDisappeared(
        FCk_Handle_Compass InCompass,
        FCk_Handle_Poi InPoi)
    -> void
{
    _PendingShownPois.Remove(InPoi);
    OnCompassEntryDisappeared(InPoi);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CompassUI_RibbonWidget::
    DoResolveBindings()
    -> void
{
    if (ck::IsValid(_MarkerWidgetClass) && ck::Is_NOT_Valid(_MarkerCanvas))
    {
        DoReportMisconfig(TEXT("CompassRibbon [{}] sets _MarkerWidgetClass but binds no _MarkerCanvas — "
            "markers have no injection point. Bind a CanvasPanel named _MarkerCanvas"));
        // Fall through — the ribbon material path still works
    }

    if (ck::IsValid(_RibbonImage) && ck::Is_NOT_Valid(_RibbonMID))
    {
        if (const auto BrushMaterial = Cast<UMaterialInterface>(_RibbonImage->GetBrush().GetResourceObject());
            ck::IsValid(BrushMaterial))
        {
            _RibbonMID = _RibbonImage->GetDynamicMaterial();
        }
    }
}

auto
    UCk_CompassUI_RibbonWidget::
    DoLayoutPreview()
    -> void
{
    if (ck::IsValid(_RibbonMID))
    {
        _RibbonMID->SetScalarParameterValue(_HeadingParameterName, _PreviewHeadingDegrees);
        _RibbonMID->SetScalarParameterValue(_ArcParameterName, _PreviewArcDegrees);
    }

    DoPositionCardinals(_PreviewHeadingDegrees, _PreviewArcDegrees);

    for (auto Index = 0; Index < _PreviewMarkerCount; ++Index)
    {
        const auto Marker = Get_MarkerAt(Index);

        if (ck::Is_NOT_Valid(Marker))
        { break; }

        const auto NormalizedOffset = _PreviewMarkerCount == 1
            ? 0.0f
            : FMath::Lerp(-0.8f, 0.8f, static_cast<float>(Index) / static_cast<float>(_PreviewMarkerCount - 1));

        Marker->SetVisibility(ESlateVisibility::HitTestInvisible);
        DoPositionChildAtOffset(Marker, NormalizedOffset, _MarkerBandY);

        // Invalid Poi handle on purpose — the marker leaves designer-authored icon defaults alone
        const auto PreviewEntry = FCk_Compass_Entry{
            FCk_Handle_Poi{}, FGameplayTag{}, NormalizedOffset * _PreviewArcDegrees * 0.5f, NormalizedOffset,
            ECk_Compass_EntryArcState::InsideArc, static_cast<float>(Index + 1) * 2500.0f, 0.0f, 0, 1.0f};

        Marker->InjectEntry(PreviewEntry);
    }

    DoHideMarkersFromIndex(_PreviewMarkerCount);
}

auto
    UCk_CompassUI_RibbonWidget::
    DoRefreshLayout()
    -> void
{
    const auto CompassIsValid = ck::IsValid(_Compass);

    if (NOT CompassIsValid)
    {
        for (const auto& Cardinal : Get_CardinalWidgets())
        {
            if (ck::IsValid(Cardinal))
            { Cardinal->SetVisibility(ESlateVisibility::Hidden); }
        }

        DoHideMarkersFromIndex(0);
        _PendingShownPois.Reset();

        return;
    }

    const auto Heading = UCk_Utils_Compass_UE::Get_Heading(_Compass);
    const auto ArcDegrees = UCk_Utils_Compass_UE::Get_ArcDegrees(_Compass);

    if (ck::IsValid(_RibbonMID))
    {
        _RibbonMID->SetScalarParameterValue(_HeadingParameterName, Heading);
        _RibbonMID->SetScalarParameterValue(_ArcParameterName, ArcDegrees);
    }

    DoPositionCardinals(Heading, ArcDegrees);

    // ---- POI markers: index-pooled against this frame's entries ----
    const auto Entries = UCk_Utils_Compass_UE::Get_Entries(_Compass);

    for (auto Index = 0; Index < Entries.Num(); ++Index)
    {
        const auto Marker = Get_MarkerAt(Index);

        if (ck::Is_NOT_Valid(Marker))
        { break; }

        const auto& Entry = Entries[Index];
        const auto IsClamped = Entry.Get_ArcState() == ECk_Compass_EntryArcState::ClampedToEdge;

        Marker->SetVisibility(ESlateVisibility::HitTestInvisible);
        Marker->SetRenderOpacity(Entry.Get_FadeAlpha() * (IsClamped ? _ClampedEntryOpacity : 1.0f));
        DoPositionChildAtOffset(Marker, Entry.Get_NormalizedOffset(), _MarkerBandY);

        Marker->InjectEntry(Entry);

        if (_PendingShownPois.Remove(Entry.Get_Poi()) > 0)
        { Marker->NotifyShown(); }
    }

    DoHideMarkersFromIndex(Entries.Num());
}

auto
    UCk_CompassUI_RibbonWidget::
    DoPositionCardinals(
        float InHeading,
        float InArcDegrees)
    -> void
{
    const auto HalfArc = InArcDegrees * 0.5f;
    const auto CardinalWidgets = Get_CardinalWidgets();

    for (auto Index = 0; Index < CardinalWidgets.Num(); ++Index)
    {
        const auto& Cardinal = CardinalWidgets[Index];

        if (ck::Is_NOT_Valid(Cardinal))
        { continue; }

        const auto CardinalWorldYaw = static_cast<float>(Index) * 45.0f;
        const auto SignedDelta = FRotator::NormalizeAxis(CardinalWorldYaw - InHeading);

        if (FMath::Abs(SignedDelta) > HalfArc)
        {
            Cardinal->SetVisibility(ESlateVisibility::Hidden);
            continue;
        }

        Cardinal->SetVisibility(ESlateVisibility::HitTestInvisible);
        DoPositionChildAtOffset(Cardinal, SignedDelta / HalfArc, _CardinalBandY);
    }
}

auto
    UCk_CompassUI_RibbonWidget::
    DoPositionChildAtOffset(
        UWidget* InChild,
        float InNormalizedOffset,
        float InBandNormalizedY) const
    -> void
{
    const auto CanvasSlot = Cast<UCanvasPanelSlot>(InChild->Slot);

    if (ck::Is_NOT_Valid(CanvasSlot))
    { return; }

    const auto U = InNormalizedOffset * 0.5f + 0.5f;

    CanvasSlot->SetAnchors(FAnchors{U, InBandNormalizedY, U, InBandNormalizedY});
    CanvasSlot->SetAlignment(FVector2D{0.5, 0.5});
    CanvasSlot->SetAutoSize(true);
    CanvasSlot->SetPosition(FVector2D::ZeroVector);
}

auto
    UCk_CompassUI_RibbonWidget::
    Get_CardinalWidgets() const
    -> TArray<UWidget*, TInlineAllocator<8>>
{
    // Index order matches world yaw: Index * 45 degrees, 0 = North
    return {_CardinalN, _CardinalNE, _CardinalE, _CardinalSE, _CardinalS, _CardinalSW, _CardinalW, _CardinalNW};
}

auto
    UCk_CompassUI_RibbonWidget::
    Get_MarkerAt(
        int32 InIndex)
    -> UCk_CompassUI_MarkerWidget*
{
    if (ck::Is_NOT_Valid(_MarkerCanvas) || ck::Is_NOT_Valid(_MarkerWidgetClass))
    { return {}; }

    while (_MarkerPool.Num() <= InIndex)
    {
        const auto Marker = CreateWidget<UCk_CompassUI_MarkerWidget>(this, _MarkerWidgetClass);

        if (ck::Is_NOT_Valid(Marker))
        { return {}; }

        _MarkerCanvas->AddChildToCanvas(Marker);
        _MarkerPool.Emplace(Marker);
    }

    return _MarkerPool[InIndex];
}

auto
    UCk_CompassUI_RibbonWidget::
    DoHideMarkersFromIndex(
        int32 InFirstHiddenIndex)
    -> void
{
    for (auto Index = InFirstHiddenIndex; Index < _MarkerPool.Num(); ++Index)
    { _MarkerPool[Index]->SetVisibility(ESlateVisibility::Hidden); }
}

auto
    UCk_CompassUI_RibbonWidget::
    DoReportMisconfig(
        const TCHAR* InMessage)
    -> void
{
    if (IsDesignTime())
    {
        if (_ReportedMisconfig)
        { return; }

        _ReportedMisconfig = true;
        ck::compass::Warning(InMessage, this);
    }
    else
    {
        CK_TRIGGER_ENSURE(InMessage, this);
    }
}

// --------------------------------------------------------------------------------------------------------------------
