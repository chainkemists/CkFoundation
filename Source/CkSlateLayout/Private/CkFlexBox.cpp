#include "CkSlateLayout/CkFlexBox.h"

#include "Layout/ArrangedChildren.h"

namespace ck_slate_layout_flex_box
{
    auto IsFiniteNonNegative(const float InValue) -> bool
    {
        return FMath::IsFinite(InValue) && InValue >= 0.0f;
    }

    auto ToYogaAlign(const EHorizontalAlignment InAlignment, const EVerticalAlignment InVerticalAlignment, const EOrientation InDirection) -> YGAlign
    {
        if (InDirection == Orient_Horizontal)
        {
            switch (InVerticalAlignment)
            {
            case VAlign_Center: return YGAlignCenter;
            case VAlign_Bottom: return YGAlignFlexEnd;
            case VAlign_Fill: return YGAlignStretch;
            default: return YGAlignFlexStart;
            }
        }
        switch (InAlignment)
        {
        case HAlign_Center: return YGAlignCenter;
        case HAlign_Right: return YGAlignFlexEnd;
        case HAlign_Fill: return YGAlignStretch;
        default: return YGAlignFlexStart;
        }
    }

    auto ToYogaWrap(const ECkFlexWrap InWrap) -> YGWrap
    {
        switch (InWrap)
        {
        case ECkFlexWrap::NoWrap: return YGWrapNoWrap;
        case ECkFlexWrap::Wrap: return YGWrapWrap;
        case ECkFlexWrap::WrapReverse: return YGWrapWrapReverse;
        default: return YGWrapNoWrap;
        }
    }

    auto IsValidWrap(const ECkFlexWrap InWrap) -> bool
    {
        return InWrap == ECkFlexWrap::NoWrap || InWrap == ECkFlexWrap::Wrap || InWrap == ECkFlexWrap::WrapReverse;
    }
}

SCkFlexBox::FSlot::FSlot()
    : TBasicLayoutWidgetSlot<FSlot>(HAlign_Fill, VAlign_Fill)
    , _Grow(0.0f)
    , _Shrink(1.0f)
    , _MinWidth(0.0f)
    , _MinHeight(0.0f)
{
}

SCkFlexBox::SCkFlexBox()
    : _Children(this)
{
}

void SCkFlexBox::FSlot::Construct(const FChildren& InSlotOwner, FSlotArguments&& InArgs)
{
    TBasicLayoutWidgetSlot<FSlot>::Construct(InSlotOwner, MoveTemp(InArgs));
    _Panel = static_cast<SCkFlexBox*>(GetOwnerWidget());
    _Grow = InArgs._Grow.Get(0.0f);
    _Shrink = InArgs._Shrink.Get(1.0f);
    _MinWidth = InArgs._MinWidth.Get(0.0f);
    _MinHeight = InArgs._MinHeight.Get(0.0f);
    _MaxWidth = InArgs._MaxWidth;
    _MaxHeight = InArgs._MaxHeight;
    _Measure = MoveTemp(InArgs._Measure);
    _OnArranged = MoveTemp(InArgs._OnArranged);
}

bool SCkFlexBox::FSlot::IsStyleValid() const
{
    const auto Padding = GetPadding();
    const auto HasValidPadding = FMath::IsFinite(Padding.Left) && FMath::IsFinite(Padding.Top)
        && FMath::IsFinite(Padding.Right) && FMath::IsFinite(Padding.Bottom)
        && Padding.Left >= 0.0f && Padding.Top >= 0.0f && Padding.Right >= 0.0f && Padding.Bottom >= 0.0f;
    const auto HasValidMaximums = (!_MaxWidth.IsSet() || (FMath::IsFinite(_MaxWidth.GetValue()) && _MaxWidth.GetValue() >= _MinWidth))
        && (!_MaxHeight.IsSet() || (FMath::IsFinite(_MaxHeight.GetValue()) && _MaxHeight.GetValue() >= _MinHeight));
    return FMath::IsFinite(_Grow) && FMath::IsFinite(_Shrink)
        && FMath::IsFinite(_MinWidth) && FMath::IsFinite(_MinHeight)
        && _Grow >= 0.0f && _Shrink >= 0.0f && _MinWidth >= 0.0f && _MinHeight >= 0.0f
        && HasValidPadding && HasValidMaximums;
}

auto SCkFlexBox::Slot() -> FSlot::FSlotArguments
{
    return FSlot::FSlotArguments(MakeUnique<FSlot>());
}

SCkFlexBox::FScopedSlotArguments::FScopedSlotArguments(TUniquePtr<FSlot> InSlot, SCkFlexBox& InOwner, const int32 InIndex)
    : FSlot::FSlotArguments(MoveTemp(InSlot))
    , _Owner(InOwner)
    , _Index(InIndex)
{
}

SCkFlexBox::FScopedSlotArguments::~FScopedSlotArguments()
{
    if (GetSlot() == nullptr)
    {
        return;
    }
    const auto IndexIsValid = _Index == INDEX_NONE || (_Index >= 0 && _Index <= _Owner._Children.Num());
    const auto AttachedWidget = GetAttachedWidget();
    const auto HasAttachedWidget = AttachedWidget.IsValid();
    auto IsDuplicate = false;
    if (HasAttachedWidget)
    {
        for (int32 Index = 0; Index < _Owner._Children.Num(); ++Index)
        {
            if (_Owner._Children[Index].GetWidget() == AttachedWidget.ToSharedRef())
            {
                IsDuplicate = true;
                break;
            }
        }
    }
    if (!IndexIsValid || !HasAttachedWidget || IsDuplicate || AttachedWidget.Get() == &_Owner)
    {
        return;
    }
    if (_Owner._IsArranging)
    {
        return;
    }
    auto* BaseArguments = static_cast<FSlot::FSlotArguments*>(static_cast<FSlotBase::FSlotArguments*>(this));
    if (_Index == INDEX_NONE)
    {
        _Owner._Children.AddSlot(MoveTemp(*BaseArguments));
    }
    else
    {
        _Owner._Children.InsertSlot(MoveTemp(*BaseArguments), _Index);
    }
    if (!_Owner.HasValidStyle())
    {
        _Owner._Children.Remove(AttachedWidget.ToSharedRef());
        return;
    }
    _Owner.RebuildYogaTree();
}

auto SCkFlexBox::AddSlot(const int32 InIndex) -> FScopedSlotArguments
{
    return FScopedSlotArguments(MakeUnique<FSlot>(), *this, InIndex);
}

int32 SCkFlexBox::RemoveSlot(const TSharedRef<SWidget>& InWidget)
{
    if (_IsArranging)
    {
        return INDEX_NONE;
    }
    const auto RemovedIndex = _Children.Remove(InWidget);
    if (RemovedIndex != INDEX_NONE)
    {
        RebuildYogaTree();
        Invalidate(EInvalidateWidgetReason::ChildOrder);
    }
    return RemovedIndex;
}

void SCkFlexBox::Construct(const FArguments& InArgs)
{
    _Direction = InArgs._Direction;
    _Wrap = InArgs._Wrap;
    _Gap = InArgs._Gap;
    _Padding = InArgs._Padding;
    _HorizontalAlignment = InArgs._HAlign;
    _VerticalAlignment = InArgs._VAlign;
    auto& SlotArguments = const_cast<TArray<FSlot::FSlotArguments>&>(InArgs._Slots);
    TArray<FSlot::FSlotArguments> CombinedSlotArguments = MoveTemp(SlotArguments);
    CombinedSlotArguments.Reserve(CombinedSlotArguments.Num() + InArgs._Items.Num());
    for (const auto& Item : InArgs._Items)
    {
        auto SlotArgumentsForItem = Slot();
        SlotArgumentsForItem.Grow(Item.Grow).Shrink(Item.Shrink).HAlign(Item.HorizontalAlignment).VAlign(Item.VerticalAlignment)[Item.Widget];
        CombinedSlotArguments.Add(MoveTemp(SlotArgumentsForItem));
    }
    _Children.AddSlots(MoveTemp(CombinedSlotArguments));
    BuildYogaTree();
    AddMetadata(MakeShared<FCkFlexMeasureMetaData>(
        [WeakPanel = TWeakPtr<SCkFlexBox>(SharedThis(this))](const FCkFlexMeasureArgs& InMeasureArgs) -> FVector2D
        {
            const auto Panel = WeakPanel.Pin();
            return Panel.IsValid() ? Panel->MeasureForConstraints(InMeasureArgs) : FVector2D::ZeroVector;
        },
        FCkFlexMeasureMetaData::FOnArranged()));
}

void SCkFlexBox::BuildYogaTree()
{
    if (!HasValidStyle())
    {
        return;
    }
    _YogaConfig = YGConfigNew();
    if (_YogaConfig == nullptr)
    {
        return;
    }
    YGConfigSetUseWebDefaults(_YogaConfig, true);
    _YogaRoot = YGNodeNewWithConfig(_YogaConfig);
    if (_YogaRoot == nullptr)
    {
        YGConfigFree(_YogaConfig);
        _YogaConfig = nullptr;
        return;
    }
    ++_YogaNodeAllocationCount;
    YGNodeStyleSetFlexDirection(_YogaRoot, _Direction == Orient_Horizontal ? YGFlexDirectionRow : YGFlexDirectionColumn);
    YGNodeStyleSetFlexWrap(_YogaRoot, ck_slate_layout_flex_box::ToYogaWrap(_Wrap));
    YGNodeStyleSetGap(_YogaRoot, YGGutterAll, _Gap);
    YGNodeStyleSetPadding(_YogaRoot, YGEdgeLeft, _Padding.Left);
    YGNodeStyleSetPadding(_YogaRoot, YGEdgeTop, _Padding.Top);
    YGNodeStyleSetPadding(_YogaRoot, YGEdgeRight, _Padding.Right);
    YGNodeStyleSetPadding(_YogaRoot, YGEdgeBottom, _Padding.Bottom);
    YGNodeStyleSetAlignItems(_YogaRoot, ck_slate_layout_flex_box::ToYogaAlign(_HorizontalAlignment, _VerticalAlignment, _Direction));
    _YogaChildren.Reserve(_Children.Num());
    for (int32 Index = 0; Index < _Children.Num(); ++Index)
    {
        auto Node = YGNodeNewWithConfig(_YogaConfig);
        if (Node == nullptr)
        {
            YGNodeFreeRecursive(_YogaRoot);
            YGConfigFree(_YogaConfig);
            _YogaRoot = nullptr;
            _YogaConfig = nullptr;
            _YogaChildren.Reset();
            return;
        }
        ++_YogaNodeAllocationCount;
        auto& SlotRef = _Children[Index];
        ApplySlotStyle(Node, SlotRef);
        YGNodeSetContext(Node, &SlotRef);
        YGNodeSetMeasureFunc(Node, &SCkFlexBox::MeasureLeaf);
        YGNodeInsertChild(_YogaRoot, Node, _YogaChildren.Num());
        _YogaChildren.Add(Node);
    }
}

void SCkFlexBox::RebuildYogaTree()
{
    if (_YogaRoot != nullptr)
    {
        YGNodeFreeRecursive(_YogaRoot);
        _YogaRoot = nullptr;
    }
    if (_YogaConfig != nullptr)
    {
        YGConfigFree(_YogaConfig);
        _YogaConfig = nullptr;
    }
    _YogaChildren.Reset();
    BuildYogaTree();
}

bool SCkFlexBox::HasValidStyle() const
{
    const auto PaddingIsValid = FMath::IsFinite(_Padding.Left) && FMath::IsFinite(_Padding.Top)
        && FMath::IsFinite(_Padding.Right) && FMath::IsFinite(_Padding.Bottom)
        && _Padding.Left >= 0.0f && _Padding.Top >= 0.0f && _Padding.Right >= 0.0f && _Padding.Bottom >= 0.0f;
    if (!ck_slate_layout_flex_box::IsValidWrap(_Wrap) || !FMath::IsFinite(_Gap) || _Gap < 0.0f || !PaddingIsValid)
    {
        return false;
    }
    for (int32 Index = 0; Index < _Children.Num(); ++Index)
    {
        if (!_Children[Index].IsStyleValid())
        {
            return false;
        }
    }
    return true;
}

SCkFlexBox::~SCkFlexBox()
{
    if (_YogaRoot != nullptr)
    {
        YGNodeFreeRecursive(_YogaRoot);
    }
    if (_YogaConfig != nullptr)
    {
        YGConfigFree(_YogaConfig);
    }
}

void SCkFlexBox::ApplySlotStyle(const YGNodeRef InNode, const FSlot& InSlot) const
{
    YGNodeStyleSetFlexGrow(InNode, InSlot.Get_Grow());
    YGNodeStyleSetFlexShrink(InNode, InSlot.Get_Shrink());
    YGNodeStyleSetMinWidth(InNode, InSlot.Get_MinWidth());
    YGNodeStyleSetMinHeight(InNode, InSlot.Get_MinHeight());
    if (InSlot.Has_MaxWidth()) { YGNodeStyleSetMaxWidth(InNode, InSlot.Get_MaxWidth()); }
    if (InSlot.Has_MaxHeight()) { YGNodeStyleSetMaxHeight(InNode, InSlot.Get_MaxHeight()); }
    const auto Padding = InSlot.GetPadding();
    YGNodeStyleSetPadding(InNode, YGEdgeLeft, Padding.Left);
    YGNodeStyleSetPadding(InNode, YGEdgeTop, Padding.Top);
    YGNodeStyleSetPadding(InNode, YGEdgeRight, Padding.Right);
    YGNodeStyleSetPadding(InNode, YGEdgeBottom, Padding.Bottom);
    YGNodeStyleSetAlignSelf(InNode, ck_slate_layout_flex_box::ToYogaAlign(InSlot.GetHorizontalAlignment(), InSlot.GetVerticalAlignment(), _Direction));
}

void SCkFlexBox::SynchronizeSlotStyles() const
{
    for (int32 Index = 0; Index < _YogaChildren.Num(); ++Index)
    {
        ApplySlotStyle(_YogaChildren[Index], _Children[Index]);
        YGNodeStyleSetDisplay(_YogaChildren[Index], _Children[Index].GetWidget()->GetVisibility() == EVisibility::Collapsed ? YGDisplayNone : YGDisplayFlex);
        YGNodeMarkDirty(_YogaChildren[Index]);
    }
}

YGSize SCkFlexBox::MeasureLeaf(const YGNodeConstRef InNode, const float InWidth, const YGMeasureMode InWidthMode, const float InHeight, const YGMeasureMode InHeightMode)
{
    const auto* Slot = static_cast<const FSlot*>(YGNodeGetContext(InNode));
    const auto PanelIsValid = Slot != nullptr && Slot->Get_Panel() != nullptr;
    if (!PanelIsValid)
    {
        return YGSize{0.0f, 0.0f};
    }

    const auto MeasureArgs = FCkFlexMeasureArgs{
        .AvailableWidth = InWidth,
        .WidthMode = InWidthMode,
        .AvailableHeight = InHeight,
        .HeightMode = InHeightMode,
        .LayoutScale = Slot->Get_Panel()->_ActiveScale,
    };
    if (!IsValid_CkFlexMeasureArgs(MeasureArgs))
    {
        return YGSize{0.0f, 0.0f};
    }
    const auto Size = Slot->Get_Panel()->MeasureSlot(*Slot, MeasureArgs);
    return YGSize{static_cast<float>(Size.X), static_cast<float>(Size.Y)};
}

FVector2D SCkFlexBox::MeasureSlot(const FSlot& InSlot, const FCkFlexMeasureArgs& InArgs) const
{
    const auto& ExplicitMeasure = InSlot.Get_Measure();
    auto Size = FVector2D::ZeroVector;
    if (ExplicitMeasure)
    {
        Size = ExplicitMeasure(InArgs);
    }
    else
    {
        const auto MeasureMetadata = InSlot.GetWidget()->GetMetaData<FCkFlexMeasureMetaData>();
        Size = MeasureMetadata.IsValid()
            ? MeasureMetadata->Measure(InArgs)
            : InSlot.GetWidget()->GetDesiredSize();
    }

    const auto HasFiniteResult = FMath::IsFinite(Size.X) && FMath::IsFinite(Size.Y)
        && Size.X >= 0.0f && Size.Y >= 0.0f;
    if (!HasFiniteResult)
    {
        return FVector2D::ZeroVector;
    }
    if (InArgs.WidthMode == YGMeasureModeExactly) { Size.X = InArgs.AvailableWidth; }
    if (InArgs.WidthMode == YGMeasureModeAtMost) { Size.X = FMath::Min(Size.X, InArgs.AvailableWidth); }
    if (InArgs.HeightMode == YGMeasureModeExactly) { Size.Y = InArgs.AvailableHeight; }
    if (InArgs.HeightMode == YGMeasureModeAtMost) { Size.Y = FMath::Min(Size.Y, InArgs.AvailableHeight); }
    return Size;
}

void SCkFlexBox::CalculateLayout(const float InWidth, const float InHeight, const float InScale) const
{
    const auto InputsAreValid = _YogaConfig != nullptr
        && _YogaRoot != nullptr
        && ck_slate_layout_flex_box::IsFiniteNonNegative(InWidth)
        && ck_slate_layout_flex_box::IsFiniteNonNegative(InHeight)
        && ck_slate_layout_flex_box::IsFiniteNonNegative(InScale)
        && InScale > 0.0f;
    if (!InputsAreValid)
    {
        return;
    }
    if (!HasValidStyle())
    {
        return;
    }

    _ActiveScale = InScale;
    YGConfigSetPointScaleFactor(_YogaConfig, InScale);
    YGNodeStyleSetMaxWidth(_YogaRoot, YGUndefined);
    YGNodeStyleSetMaxHeight(_YogaRoot, YGUndefined);
    SynchronizeSlotStyles();
    YGNodeCalculateLayout(_YogaRoot, InWidth, InHeight, YGDirectionLTR);
    ++_LayoutPassCount;
}

FVector2D SCkFlexBox::MeasureForConstraints(const FCkFlexMeasureArgs& InArgs) const
{
    if (_IsArranging) { return FVector2D::ZeroVector; }
    TGuardValue<bool> MeasuringGuard(_IsArranging, true);
    if (!IsValid_CkFlexMeasureArgs(InArgs) || !HasValidStyle())
    {
        return FVector2D::ZeroVector;
    }
    const auto WidthIsFinite = ck_slate_layout_flex_box::IsFiniteNonNegative(InArgs.AvailableWidth);
    const auto HeightIsFinite = ck_slate_layout_flex_box::IsFiniteNonNegative(InArgs.AvailableHeight);
    if (_YogaRoot == nullptr || _YogaConfig == nullptr)
    {
        return FVector2D::ZeroVector;
    }

    const auto WidthIsExact = WidthIsFinite && InArgs.WidthMode == YGMeasureModeExactly;
    const auto HeightIsExact = HeightIsFinite && InArgs.HeightMode == YGMeasureModeExactly;
    const auto WidthIsAtMost = WidthIsFinite && InArgs.WidthMode == YGMeasureModeAtMost;
    const auto HeightIsAtMost = HeightIsFinite && InArgs.HeightMode == YGMeasureModeAtMost;
    const auto Width = WidthIsExact ? InArgs.AvailableWidth : YGUndefined;
    const auto Height = HeightIsExact ? InArgs.AvailableHeight : YGUndefined;
    _ActiveScale = InArgs.LayoutScale;
    YGConfigSetPointScaleFactor(_YogaConfig, _ActiveScale);
    YGNodeStyleSetMaxWidth(_YogaRoot, WidthIsAtMost ? InArgs.AvailableWidth : YGUndefined);
    YGNodeStyleSetMaxHeight(_YogaRoot, HeightIsAtMost ? InArgs.AvailableHeight : YGUndefined);
    SynchronizeSlotStyles();
    YGNodeCalculateLayout(_YogaRoot, Width, Height, YGDirectionLTR);
    ++_LayoutPassCount;
    auto Size = FVector2D(YGNodeLayoutGetWidth(_YogaRoot), YGNodeLayoutGetHeight(_YogaRoot));
    if (InArgs.WidthMode == YGMeasureModeAtMost && WidthIsFinite) { Size.X = FMath::Min(Size.X, InArgs.AvailableWidth); }
    if (InArgs.HeightMode == YGMeasureModeAtMost && HeightIsFinite) { Size.Y = FMath::Min(Size.Y, InArgs.AvailableHeight); }
    return Size;
}

void SCkFlexBox::OnArrangeChildren(const FGeometry& InAllottedGeometry, FArrangedChildren& OutArrangedChildren) const
{
    const auto LocalSize = InAllottedGeometry.GetLocalSize();
    const auto Scale = InAllottedGeometry.GetAccumulatedLayoutTransform().GetScale();
    const auto HasValidGeometry = ck_slate_layout_flex_box::IsFiniteNonNegative(LocalSize.X)
        && ck_slate_layout_flex_box::IsFiniteNonNegative(LocalSize.Y)
        && ck_slate_layout_flex_box::IsFiniteNonNegative(Scale) && Scale > 0.0f;
    if (!HasValidGeometry)
    {
        return;
    }
    if (!HasValidStyle())
    {
        return;
    }
    auto& MutableThis = const_cast<SCkFlexBox&>(*this);
    MutableThis._IsArranging = true;
    struct FArrangeGuard final
    {
        bool& Value;
        ~FArrangeGuard() { Value = false; }
    } Guard{MutableThis._IsArranging};
    CalculateLayout(LocalSize.X, LocalSize.Y, Scale);
    if (_YogaRoot == nullptr)
    {
        return;
    }

    const auto ChildCount = FMath::Min(_Children.Num(), _YogaChildren.Num());
    for (int32 Index = 0; Index < ChildCount; ++Index)
    {
        const auto& Slot = _Children[Index];
        const auto Child = Slot.GetWidget();
        if (!OutArrangedChildren.Accepts(Child->GetVisibility()))
        {
            continue;
        }

        const auto Node = _YogaChildren[Index];
        const auto Padding = Slot.GetPadding();
        const auto Offset = FVector2D(YGNodeLayoutGetLeft(Node) + Padding.Left, YGNodeLayoutGetTop(Node) + Padding.Top);
        const auto Size = FVector2D(FMath::Max(0.0f, YGNodeLayoutGetWidth(Node) - Padding.Left - Padding.Right), FMath::Max(0.0f, YGNodeLayoutGetHeight(Node) - Padding.Top - Padding.Bottom));
        const auto MeasureMetadata = Child->GetMetaData<FCkFlexMeasureMetaData>();
        if (MeasureMetadata.IsValid())
        {
            MeasureMetadata->NotifyArranged(Size.X, Size.Y);
        }
        if (Slot.Get_OnArranged())
        {
            Slot.Get_OnArranged()(Size.X, Size.Y);
        }
        OutArrangedChildren.AddWidget(InAllottedGeometry.MakeChild(Child, Offset, Size));
    }
}

FVector2D SCkFlexBox::ComputeDesiredSize(const float InLayoutScaleMultiplier) const
{
    return MeasureForConstraints(FCkFlexMeasureArgs{.LayoutScale = InLayoutScaleMultiplier});
}

FChildren* SCkFlexBox::GetChildren()
{
    return &_Children;
}
