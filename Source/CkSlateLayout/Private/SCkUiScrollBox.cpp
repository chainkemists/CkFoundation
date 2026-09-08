#include "CkSlateLayout/SCkUiScrollBox.h"

#include "CkSlateLayout/CkFlexLayoutTypes.h"

#include "Layout/ArrangedChildren.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SNullWidget.h"

namespace ck_ui_scrollbox
{
    auto IsFiniteNonNegative(const float InValue) -> bool
    {
        return FMath::IsFinite(InValue) && InValue >= 0.0f;
    }

    auto IsValidPadding(const FMargin& InPadding) -> bool
    {
        return IsFiniteNonNegative(InPadding.Left) && IsFiniteNonNegative(InPadding.Top)
            && IsFiniteNonNegative(InPadding.Right) && IsFiniteNonNegative(InPadding.Bottom);
    }

    auto IsFiniteSize(const FVector2D& InSize) -> bool
    {
        return IsFiniteNonNegative(InSize.X) && IsFiniteNonNegative(InSize.Y);
    }

    class SCkUiMeasuredContent final : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SCkUiMeasuredContent)
        {
        }
        SLATE_END_ARGS()

        void Construct(const FArguments& InArgs)
        {
            ChildSlot[SNullWidget::NullWidget];
        }

        void SetContent(TSharedRef<SWidget> InContent)
        {
            _Content = InContent;
            // A replacement still occupies the retained viewport. Keep its width so
            // the next prepass measures wrapped content before offset clamping.
            _MeasuredSize = FVector2D::ZeroVector;
            ChildSlot[InContent];
            Invalidate(EInvalidateWidgetReason::Layout);
        }

        auto Measure(const FCkFlexMeasureArgs& InArgs) -> FVector2D
        {
            if (!IsValid_CkFlexMeasureArgs(InArgs) || !_Content.IsValid())
            {
                return FVector2D::ZeroVector;
            }

            const TSharedPtr<FCkFlexMeasureMetaData> Metadata = _Content->GetMetaData<FCkFlexMeasureMetaData>();
            const FVector2D Measured = Metadata.IsValid() ? Metadata->Measure(InArgs) : _Content->GetDesiredSize();
            return IsFiniteSize(Measured) ? Measured : FVector2D::ZeroVector;
        }

        auto SetViewportWidth(const float InWidth, const float InScale) -> bool
        {
            if (!IsFiniteNonNegative(InWidth) || !FMath::IsFinite(InScale) || InScale <= 0.0f)
            {
                return false;
            }

            const FCkFlexMeasureArgs Args{
                .AvailableWidth = InWidth,
                .WidthMode = YGMeasureModeExactly,
                .AvailableHeight = YGUndefined,
                .HeightMode = YGMeasureModeUndefined,
                .LayoutScale = InScale,
            };
            const FVector2D NewSize = Measure(Args);
            const bool bChanged = YGFloatIsUndefined(_ConstraintWidth)
                || !FMath::IsNearlyEqual(_ConstraintWidth, InWidth)
                || !NewSize.Equals(_MeasuredSize);
            _ConstraintWidth = InWidth;
            _MeasuredSize = NewSize;
            return bChanged;
        }

        virtual void OnArrangeChildren(const FGeometry& InAllottedGeometry, FArrangedChildren& OutArrangedChildren) const override
        {
            SCompoundWidget::OnArrangeChildren(InAllottedGeometry, OutArrangedChildren);
            if (!_Content.IsValid())
            {
                return;
            }

            const auto LocalSize = InAllottedGeometry.GetLocalSize();
            if (!IsFiniteNonNegative(LocalSize.X) || !IsFiniteNonNegative(LocalSize.Y))
            {
                return;
            }
            if (const TSharedPtr<FCkFlexMeasureMetaData> Metadata = _Content->GetMetaData<FCkFlexMeasureMetaData>(); Metadata.IsValid())
            {
                Metadata->NotifyArranged(LocalSize.X, LocalSize.Y);
            }
        }

    protected:
        virtual FVector2D ComputeDesiredSize(const float InLayoutScaleMultiplier) const override
        {
            if (!_Content.IsValid() || !FMath::IsFinite(InLayoutScaleMultiplier) || InLayoutScaleMultiplier <= 0.0f)
            {
                return FVector2D::ZeroVector;
            }

            // Re-run the callback during every prepass. Bound text and visibility
            // can change while the viewport width remains unchanged.
            if (IsFiniteNonNegative(_ConstraintWidth))
            {
                auto* MutableThis = const_cast<SCkUiMeasuredContent*>(this);
                MutableThis->SetViewportWidth(_ConstraintWidth, InLayoutScaleMultiplier);
                return _MeasuredSize;
            }

            const FCkFlexMeasureArgs Unconstrained{
                .AvailableWidth = YGUndefined,
                .WidthMode = YGMeasureModeUndefined,
                .AvailableHeight = YGUndefined,
                .HeightMode = YGMeasureModeUndefined,
                .LayoutScale = InLayoutScaleMultiplier,
            };
            return const_cast<SCkUiMeasuredContent*>(this)->Measure(Unconstrained);
        }

    private:
        TSharedPtr<SWidget> _Content;
        float _ConstraintWidth = YGUndefined;
        FVector2D _MeasuredSize = FVector2D::ZeroVector;
    };
}

void SCkUiScrollBox::Construct(const FArguments& InArgs)
{
    SScrollBox::FArguments ScrollArguments;
    ScrollArguments._Orientation = Orient_Vertical;
    ScrollArguments._ConsumeMouseWheel = EConsumeMouseWheel::WhenScrollingPossible;
    ScrollArguments._AllowOverscroll = EAllowOverscroll::No;
    ScrollArguments._AllowContentToShrink = false;
    SScrollBox::Construct(ScrollArguments);

    _MeasuredContent = SNew(ck_ui_scrollbox::SCkUiMeasuredContent);
    AddMetadata(MakeShared<FCkFlexMeasureMetaData>(
        [WeakScroll = TWeakPtr<SCkUiScrollBox>(SharedThis(this))](const FCkFlexMeasureArgs& InMeasureArgs) -> FVector2D
        {
            const TSharedPtr<SCkUiScrollBox> Scroll = WeakScroll.Pin();
            if (!Scroll.IsValid() || !Scroll->_MeasuredContent.IsValid() || !IsValid_CkFlexMeasureArgs(InMeasureArgs))
            {
                return FVector2D::ZeroVector;
            }

            FCkFlexMeasureArgs ContentArgs = InMeasureArgs;
            if (ContentArgs.WidthMode != YGMeasureModeUndefined)
            {
                ContentArgs.AvailableWidth = FMath::Max(0.0f, ContentArgs.AvailableWidth
                    - Scroll->_ContentPadding.GetTotalSpaceAlong<Orient_Horizontal>());
            }
            // A vertical scroll's content axis stays intrinsic. Its parent owns
            // the finite viewport through the returned constrained size.
            ContentArgs.AvailableHeight = YGUndefined;
            ContentArgs.HeightMode = YGMeasureModeUndefined;

            FVector2D Result = Scroll->_MeasuredContent->Measure(ContentArgs) + Scroll->_ContentPadding.GetDesiredSize();
            if (!ck_ui_scrollbox::IsFiniteSize(Result)) { return FVector2D::ZeroVector; }
            if (InMeasureArgs.WidthMode == YGMeasureModeExactly) { Result.X = InMeasureArgs.AvailableWidth; }
            if (InMeasureArgs.WidthMode == YGMeasureModeAtMost) { Result.X = FMath::Min(Result.X, InMeasureArgs.AvailableWidth); }
            if (InMeasureArgs.HeightMode == YGMeasureModeExactly) { Result.Y = InMeasureArgs.AvailableHeight; }
            if (InMeasureArgs.HeightMode == YGMeasureModeAtMost) { Result.Y = FMath::Min(Result.Y, InMeasureArgs.AvailableHeight); }
            return Result;
        },
        FCkFlexMeasureMetaData::FOnArranged()));
}

void SCkUiScrollBox::SetAuthoredContent(TSharedRef<SWidget> InContent, const FMargin InPadding)
{
    if (!_MeasuredContent.IsValid() || !ck_ui_scrollbox::IsValidPadding(InPadding))
    {
        return;
    }

    const float ScrollOffset = GetScrollOffset();
    _ContentPadding = InPadding;
    _MeasuredContent->SetContent(InContent);
    ClearChildren();
    AddSlot().AutoSize().Padding(InPadding)[_MeasuredContent.ToSharedRef()];
    if (FMath::IsFinite(ScrollOffset) && ScrollOffset >= 0.0f)
    {
        SetScrollOffset(ScrollOffset);
    }
}

void SCkUiScrollBox::Tick(const FGeometry& InAllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SScrollBox::Tick(InAllottedGeometry, InCurrentTime, InDeltaTime);

    if (!ScrollPanel.IsValid())
    {
        return;
    }

    const float DesiredOffset = GetScrollOffset();
    if (!FMath::IsFinite(DesiredOffset))
    {
        SetScrollOffset(0.0f);
        return;
    }

    const TSharedRef<SWidget> Panel = ScrollPanel.ToSharedRef();
    auto Targets = TSet<TSharedRef<SWidget>>{};
    Targets.Add(Panel);
    auto Geometries = TMap<TSharedRef<SWidget>, FArrangedWidget>{};
    if (!FindChildGeometries(InAllottedGeometry, Targets, Geometries))
    {
        return;
    }

    const FArrangedWidget* PanelGeometry = Geometries.Find(Panel);
    if (PanelGeometry == nullptr)
    {
        return;
    }
    const auto ContentSize = GetScrollPanelContentSize();
    const float ContentHeight = static_cast<float>(ContentSize.Y);
    const float EndOffset = FMath::Max(0.0f, ContentHeight - PanelGeometry->Geometry.GetLocalSize().Y);
    if (!FMath::IsFinite(EndOffset))
    {
        return;
    }

    const float ClampedOffset = FMath::Clamp(DesiredOffset, 0.0f, EndOffset);
    if (!FMath::IsNearlyEqual(DesiredOffset, ClampedOffset))
    {
        // SScrollBox only clamps PhysicalOffset. Keep its public desired offset
        // coherent after an authored child shrinks, so later growth cannot jump.
        SetScrollOffset(ClampedOffset);
    }
}

void SCkUiScrollBox::OnArrangeChildren(const FGeometry& InAllottedGeometry, FArrangedChildren& OutArrangedChildren) const
{
    if (_IsResolvingScrollPanelGeometry)
    {
        SCompoundWidget::OnArrangeChildren(InAllottedGeometry, OutArrangedChildren);
        return;
    }

    if (_MeasuredContent.IsValid() && ScrollPanel.IsValid())
    {
        const float Scale = InAllottedGeometry.GetAccumulatedLayoutTransform().GetScale();
        const auto LocalSize = InAllottedGeometry.GetLocalSize();
        if (ck_ui_scrollbox::IsFiniteNonNegative(LocalSize.X)
            && ck_ui_scrollbox::IsFiniteNonNegative(LocalSize.Y)
            && FMath::IsFinite(Scale) && Scale > 0.0f)
        {
            auto Targets = TSet<TSharedRef<SWidget>>{};
            const TSharedRef<SWidget> Panel = ScrollPanel.ToSharedRef();
            Targets.Add(Panel);
            auto Geometries = TMap<TSharedRef<SWidget>, FArrangedWidget>{};
            bool bFoundPanel = false;
            {
                TGuardValue<bool> ResolvingGuard(_IsResolvingScrollPanelGeometry, true);
                bFoundPanel = FindChildGeometries(InAllottedGeometry, Targets, Geometries);
            }
            const FArrangedWidget* PanelGeometry = bFoundPanel ? Geometries.Find(Panel) : nullptr;
            if (PanelGeometry == nullptr)
            {
                SCompoundWidget::OnArrangeChildren(InAllottedGeometry, OutArrangedChildren);
                return;
            }

            const float ContentWidth = FMath::Max(0.0f, PanelGeometry->Geometry.GetLocalSize().X
                - _ContentPadding.GetTotalSpaceAlong<Orient_Horizontal>());
            if (_MeasuredContent->SetViewportWidth(ContentWidth, Scale))
            {
                // The native SScrollPanel will arrange after this outer traversal.
                // Refresh only when the constrained desired size actually changed.
                _MeasuredContent->Invalidate(EInvalidateWidgetReason::Layout);
                _MeasuredContent->SlatePrepass(Scale);
            }
        }
    }

    SCompoundWidget::OnArrangeChildren(InAllottedGeometry, OutArrangedChildren);
}
