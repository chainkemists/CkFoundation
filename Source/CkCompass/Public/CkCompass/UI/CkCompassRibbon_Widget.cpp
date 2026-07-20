#include "CkCompassRibbon_Widget.h"

#include "CkCompass/CkCompass_Log.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

#include "Materials/MaterialInstanceDynamic.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_compass_ribbon_widget
{
    constexpr int32 NumCardinalLabels = 8;

    const TCHAR* CardinalLabelTexts[NumCardinalLabels] = { TEXT("N"), TEXT("NE"), TEXT("E"), TEXT("SE"), TEXT("S"), TEXT("SW"), TEXT("W"), TEXT("NW") };
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CompassRibbon_Widget::
    Set_Compass(
        FCk_Handle_Compass& InCompass)
    -> void
{
    auto AppearedDelegate = FCk_Delegate_Compass_EntryAppeared{};
    AppearedDelegate.BindDynamic(this, &UCk_CompassRibbon_Widget::HandleEntryAppeared);

    auto DisappearedDelegate = FCk_Delegate_Compass_EntryDisappeared{};
    DisappearedDelegate.BindDynamic(this, &UCk_CompassRibbon_Widget::HandleEntryDisappeared);

    if (ck::IsValid(_Compass))
    {
        UCk_Utils_Compass_UE::UnbindFrom_OnEntryAppeared(_Compass, AppearedDelegate);
        UCk_Utils_Compass_UE::UnbindFrom_OnEntryDisappeared(_Compass, DisappearedDelegate);
    }

    _Compass = InCompass;

    CK_ENSURE_IF_NOT(ck::IsValid(_Compass), TEXT("Invalid Compass Handle passed to CompassRibbon Widget [{}]"), this)
    { return; }

    // Seed-then-delta contract: initial state comes from the pull API; the signals only carry future
    // membership deltas (their replay stashes at most the LAST payload — never rely on it for seeding)
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

auto
    UCk_CompassRibbon_Widget::
    Get_Compass() const
    -> FCk_Handle_Compass
{
    return _Compass;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CompassRibbon_Widget::
    NativeOnInitialized()
    -> void
{
    Super::NativeOnInitialized();

    DoBuildWidgetTree();
}

auto
    UCk_CompassRibbon_Widget::
    NativeDestruct()
    -> void
{
    if (ck::IsValid(_Compass))
    {
        auto AppearedDelegate = FCk_Delegate_Compass_EntryAppeared{};
        AppearedDelegate.BindDynamic(this, &UCk_CompassRibbon_Widget::HandleEntryAppeared);

        auto DisappearedDelegate = FCk_Delegate_Compass_EntryDisappeared{};
        DisappearedDelegate.BindDynamic(this, &UCk_CompassRibbon_Widget::HandleEntryDisappeared);

        UCk_Utils_Compass_UE::UnbindFrom_OnEntryAppeared(_Compass, AppearedDelegate);
        UCk_Utils_Compass_UE::UnbindFrom_OnEntryDisappeared(_Compass, DisappearedDelegate);

        _Compass = {};
    }

    Super::NativeDestruct();
}

auto
    UCk_CompassRibbon_Widget::
    NativeTick(
        const FGeometry& MyGeometry,
        float InDeltaTime)
    -> void
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    DoRefreshLayout(MyGeometry);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CompassRibbon_Widget::
    HandleEntryAppeared(
        FCk_Handle_Compass InCompass,
        FCk_Compass_Entry InEntry)
    -> void
{
    OnCompassEntryAppeared(InEntry);
}

auto
    UCk_CompassRibbon_Widget::
    HandleEntryDisappeared(
        FCk_Handle_Compass InCompass,
        FCk_Handle_Poi InPoi)
    -> void
{
    OnCompassEntryDisappeared(InPoi);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CompassRibbon_Widget::
    DoBuildWidgetTree()
    -> void
{
    using namespace ck_compass_ribbon_widget;

    // Only build the code-authored tree when no designer tree exists (pure-code usage). A UMG subclass
    // that authors its own hierarchy owns its layout entirely and skips the built-in presentation.
    if (ck::IsValid(WidgetTree->RootWidget))
    { return; }

    _RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CompassRibbon_RootCanvas"));
    WidgetTree->RootWidget = _RootCanvas;

    if (ck::IsValid(_RibbonMaterial))
    {
        _RibbonImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CompassRibbon_Ribbon"));
        _RibbonMID = UMaterialInstanceDynamic::Create(_RibbonMaterial, this);
        _RibbonImage->SetBrushFromMaterial(_RibbonMID);

        auto RibbonSlot = _RootCanvas->AddChildToCanvas(_RibbonImage);
        RibbonSlot->SetAnchors(FAnchors{0.0f, 0.0f, 1.0f, 1.0f});
        RibbonSlot->SetOffsets(FMargin{0.0f});
    }

    _CardinalLabels.Reset(NumCardinalLabels);

    for (auto Index = 0; Index < NumCardinalLabels; ++Index)
    {
        auto Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        Label->SetText(FText::FromString(CardinalLabelTexts[Index]));

        auto LabelSlot = _RootCanvas->AddChildToCanvas(Label);
        LabelSlot->SetAutoSize(true);
        LabelSlot->SetAlignment(FVector2D{0.5, 0.5});

        _CardinalLabels.Emplace(Label);
    }
}

auto
    UCk_CompassRibbon_Widget::
    DoRefreshLayout(
        const FGeometry& InGeometry)
    -> void
{
    using namespace ck_compass_ribbon_widget;

    // The widget may be a designer-authored subclass with its own tree — nothing to lay out then
    if (ck::Is_NOT_Valid(_RootCanvas))
    { return; }

    const auto CompassIsValid = ck::IsValid(_Compass);

    if (NOT CompassIsValid)
    {
        for (const auto& Label : _CardinalLabels)
        { Label->SetVisibility(ESlateVisibility::Hidden); }

        for (const auto& Icon : _IconPool)
        { Icon->SetVisibility(ESlateVisibility::Hidden); }

        return;
    }

    const auto Heading = UCk_Utils_Compass_UE::Get_Heading(_Compass);
    const auto ArcDegrees = UCk_Utils_Compass_UE::Get_ArcDegrees(_Compass);
    const auto HalfArc = ArcDegrees * 0.5f;
    const auto PanelSize = InGeometry.GetLocalSize();
    const auto VerticalCenter = static_cast<float>(PanelSize.Y) * 0.5f;

    if (ck::IsValid(_RibbonMID))
    {
        _RibbonMID->SetScalarParameterValue(_HeadingParameterName, Heading);
    }

    // ---- Cardinal letters: pure function of heading, no POIs involved ----
    for (auto Index = 0; Index < _CardinalLabels.Num(); ++Index)
    {
        const auto& Label = _CardinalLabels[Index];
        const auto CardinalWorldYaw = static_cast<float>(Index) * 45.0f;
        const auto SignedDelta = FRotator::NormalizeAxis(CardinalWorldYaw - Heading);

        if (FMath::Abs(SignedDelta) > HalfArc)
        {
            Label->SetVisibility(ESlateVisibility::Hidden);
            continue;
        }

        Label->SetVisibility(ESlateVisibility::HitTestInvisible);
        DoPositionChildAtOffset(Label, SignedDelta / HalfArc, PanelSize, VerticalCenter * 0.6f);
    }

    // ---- POI icons: index-pooled against this frame's entries (ONE loop; membership signals are for
    //      consumers that keep per-POI widgets — this reference presentation re-derives from the pull API) ----
    const auto Entries = UCk_Utils_Compass_UE::Get_Entries(_Compass);

    for (auto Index = _IconPool.Num(); Index < Entries.Num(); ++Index)
    {
        auto Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        Icon->SetDesiredSizeOverride(_IconSize);

        auto IconSlot = _RootCanvas->AddChildToCanvas(Icon);
        IconSlot->SetAutoSize(true);
        IconSlot->SetAlignment(FVector2D{0.5, 0.5});

        _IconPool.Emplace(Icon);
    }

    for (auto Index = 0; Index < _IconPool.Num(); ++Index)
    {
        const auto& Icon = _IconPool[Index];

        if (Index >= Entries.Num())
        {
            Icon->SetVisibility(ESlateVisibility::Hidden);
            continue;
        }

        const auto& Entry = Entries[Index];
        const auto IsClamped = Entry.Get_ArcState() == ECk_Compass_EntryArcState::ClampedToEdge;

        auto Tint = _IconTint;
        Tint.A *= IsClamped ? _ClampedEntryOpacity : 1.0f;

        Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
        Icon->SetColorAndOpacity(Tint);
        DoPositionChildAtOffset(Icon, Entry.Get_NormalizedOffset(), PanelSize, VerticalCenter * 1.4f);
    }
}

auto
    UCk_CompassRibbon_Widget::
    DoPositionChildAtOffset(
        UWidget* InChild,
        float InNormalizedOffset,
        const FVector2D& InPanelSize,
        float InVerticalCenter) const
    -> void
{
    const auto CanvasSlot = Cast<UCanvasPanelSlot>(InChild->Slot);

    if (ck::Is_NOT_Valid(CanvasSlot, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    const auto HorizontalPosition = (InNormalizedOffset * 0.5f + 0.5f) * static_cast<float>(InPanelSize.X);

    CanvasSlot->SetPosition(FVector2D{HorizontalPosition, InVerticalCenter});
}

// --------------------------------------------------------------------------------------------------------------------