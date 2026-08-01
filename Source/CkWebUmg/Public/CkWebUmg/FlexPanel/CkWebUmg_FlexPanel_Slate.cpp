#include "CkWebUmg_FlexPanel_Slate.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkWebUmg_Log.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_webumg_flexpanel
{
    auto
    ToFlexDirection(
        const FString& InValue)
        -> YGFlexDirection
    {
        if (InValue == TEXT("row")) { return YGFlexDirectionRow; }
        if (InValue == TEXT("row-reverse")) { return YGFlexDirectionRowReverse; }
        if (InValue == TEXT("column-reverse")) { return YGFlexDirectionColumnReverse; }
        return YGFlexDirectionColumn;
    }

    auto
    ToJustify(
        const FString& InValue)
        -> YGJustify
    {
        if (InValue == TEXT("center")) { return YGJustifyCenter; }
        if (InValue == TEXT("flex-end") || InValue == TEXT("end")) { return YGJustifyFlexEnd; }
        if (InValue == TEXT("space-between")) { return YGJustifySpaceBetween; }
        if (InValue == TEXT("space-around")) { return YGJustifySpaceAround; }
        if (InValue == TEXT("space-evenly")) { return YGJustifySpaceEvenly; }
        return YGJustifyFlexStart;
    }

    auto
    ToAlign(
        const FString& InValue,
        YGAlign InDefault)
        -> YGAlign
    {
        if (InValue == TEXT("flex-start") || InValue == TEXT("start")) { return YGAlignFlexStart; }
        if (InValue == TEXT("flex-end") || InValue == TEXT("end")) { return YGAlignFlexEnd; }
        if (InValue == TEXT("center")) { return YGAlignCenter; }
        if (InValue == TEXT("stretch")) { return YGAlignStretch; }
        if (InValue == TEXT("baseline")) { return YGAlignBaseline; }
        if (InValue == TEXT("space-between")) { return YGAlignSpaceBetween; }
        if (InValue == TEXT("space-around")) { return YGAlignSpaceAround; }
        if (InValue == TEXT("space-evenly")) { return YGAlignSpaceEvenly; }
        if (InValue == TEXT("auto")) { return YGAlignAuto; }
        return InDefault;
    }

    auto
    ToWrap(
        const FString& InValue)
        -> YGWrap
    {
        if (InValue == TEXT("wrap")) { return YGWrapWrap; }
        if (InValue == TEXT("wrap-reverse")) { return YGWrapWrapReverse; }
        return YGWrapNoWrap;
    }

    auto
    ParsePx(
        const FString& InValue)
        -> TOptional<float>
    {
        if (InValue.IsEmpty() || InValue == TEXT("auto"))
        { return {}; }

        auto Number = 0.0f;
        if (LexTryParseString(Number, *InValue.Replace(TEXT("px"), TEXT(""))))
        { return Number; }
        return {};
    }

    // The IR records used boxes; the per-edge style inputs are recovered by differencing them.
    struct FEdges
    {
        float Top = 0.0f;
        float Right = 0.0f;
        float Bottom = 0.0f;
        float Left = 0.0f;
    };

    auto
    MarginEdges(
        const FCkWebUmg_IrBox& InBox)
        -> FEdges
    {
        return FEdges{
            InBox.Border.Y - InBox.Margin.Y,
            (InBox.Margin.X + InBox.Margin.W) - (InBox.Border.X + InBox.Border.W),
            (InBox.Margin.Y + InBox.Margin.H) - (InBox.Border.Y + InBox.Border.H),
            InBox.Border.X - InBox.Margin.X};
    }

    auto
    PaddingEdges(
        const FCkWebUmg_IrBox& InBox)
        -> FEdges
    {
        return FEdges{
            InBox.Content.Y - InBox.Padding.Y,
            (InBox.Padding.X + InBox.Padding.W) - (InBox.Content.X + InBox.Content.W),
            (InBox.Padding.Y + InBox.Padding.H) - (InBox.Content.Y + InBox.Content.H),
            InBox.Content.X - InBox.Padding.X};
    }

    auto
    BorderEdges(
        const FCkWebUmg_IrBox& InBox)
        -> FEdges
    {
        return FEdges{
            InBox.Padding.Y - InBox.Border.Y,
            (InBox.Border.X + InBox.Border.W) - (InBox.Padding.X + InBox.Padding.W),
            (InBox.Border.Y + InBox.Border.H) - (InBox.Padding.Y + InBox.Padding.H),
            InBox.Padding.X - InBox.Border.X};
    }

    auto
    SetEdges(
        YGNodeRef InNode,
        const FEdges& InEdges,
        void (*InSetter)(YGNodeRef, YGEdge, float))
        -> void
    {
        InSetter(InNode, YGEdgeTop, InEdges.Top);
        InSetter(InNode, YGEdgeRight, InEdges.Right);
        InSetter(InNode, YGEdgeBottom, InEdges.Bottom);
        InSetter(InNode, YGEdgeLeft, InEdges.Left);
    }

    // Container-level inputs (direction/justify/align/wrap/gap/padding/border). Applied to the
    // panel's own root Yoga node AND to child nodes that are themselves containers is NOT needed —
    // child containers are nested panels running their own pass; here a child is a leaf box.
    auto
    ApplyContainerStyle(
        YGNodeRef InNode,
        const FCkWebUmg_IrNode& InIr)
        -> void
    {
        YGNodeStyleSetFlexDirection(InNode, ToFlexDirection(InIr.Layout.Direction));
        YGNodeStyleSetJustifyContent(InNode, ToJustify(InIr.Layout.Justify));
        YGNodeStyleSetAlignItems(InNode, ToAlign(InIr.Layout.Align, YGAlignStretch));
        YGNodeStyleSetAlignContent(InNode, ToAlign(InIr.Layout.AlignContent, YGAlignStretch));
        YGNodeStyleSetFlexWrap(InNode, ToWrap(InIr.Layout.Wrap));
        YGNodeStyleSetGap(InNode, YGGutterColumn, InIr.Layout.Gap.X);
        YGNodeStyleSetGap(InNode, YGGutterRow, InIr.Layout.Gap.Y);
        SetEdges(InNode, PaddingEdges(InIr.Get_LayoutBox()), &YGNodeStyleSetPadding);
        SetEdges(InNode, BorderEdges(InIr.Get_LayoutBox()), &YGNodeStyleSetBorder);
    }

    // Item-level inputs. Sizing policy (v1, reference-viewport contract): explicit used size on
    // any axis Yoga is not being asked to compute — the main axis stays free when grow/shrink can
    // move it (basis carries the start point), the cross axis stays free under stretch. This keeps
    // grow/shrink/wrap/alignment as real Yoga math instead of a bake, without pretending the IR
    // still knows the author's unresolved percentages (it records used values by design).
    auto
    ApplyItemStyle(
        YGNodeRef InNode,
        const FCkWebUmg_IrNode& InIr,
        YGFlexDirection InParentDirection,
        YGAlign InParentAlignItems,
        bool InIsTextLeaf,
        bool InParentIsFlexContainer)
        -> void
    {
        const auto& Layout = InIr.Layout;
        // Transformed nodes lay out at their UNtransformed geometry; the transform reapplies at
        // paint (render transform). Feeding the transformed AABB here would teach Yoga wrong sizes.
        const auto& Box = InIr.Get_LayoutBox();

        // Computed flex-grow/shrink exist on every element, but they only have meaning inside a
        // flex formatting context — a block parent's children never flex (L7 defect).
        YGNodeStyleSetFlexGrow(InNode, InParentIsFlexContainer ? Layout.Grow : 0.0f);
        YGNodeStyleSetFlexShrink(InNode, InParentIsFlexContainer ? Layout.Shrink : 0.0f);
        YGNodeStyleSetBoxSizing(InNode, YGBoxSizingBorderBox);

        const auto MainIsRowForBasis = InParentDirection == YGFlexDirectionRow
            || InParentDirection == YGFlexDirectionRowReverse;

        if (Layout.Basis.EndsWith(TEXT("%")))
        {
            auto Percent = 0.0f;
            LexTryParseString(Percent, *Layout.Basis.LeftChop(1));
            YGNodeStyleSetFlexBasisPercent(InNode, Percent);
        }
        else if (const auto Basis = ParsePx(Layout.Basis); Basis.IsSet())
        { YGNodeStyleSetFlexBasis(InNode, *Basis); }
        else if (Layout.Grow > 0.0f && InIr.Children.Num() > 0)
        {
            // Content-driven auto basis on a nested CONTAINER: the local-decomposition model makes
            // nested panels leaves in this tree, so Yoga cannot see their content. Bake the used
            // main size as basis — positions/gaps stay live; this node's grow share is fixed at
            // the reference layout. Documented v1 limitation (module Claude.md).
            YGNodeStyleSetFlexBasis(InNode,
                MainIsRowForBasis ? Box.Border.W : Box.Border.H);
        }
        else
        { YGNodeStyleSetFlexBasisAuto(InNode); }

        // Min/max are applied only when they BOUND at extraction (used size == the clamp).
        // Yoga floors the flex-base size to min before grow distribution; Blink clamps the target
        // after — feeding a non-binding min into Yoga shifts every sibling's share (L2 defect).
        // At the reference viewport, "the clamp mattered" is exactly "Chromium let it bind".
        const auto ApplyIfBinding = [&](const TOptional<float>& InConstraint, float InUsed,
            void (*InSetter)(YGNodeRef, float)) -> void
        {
            if (InConstraint.IsSet() && FMath::IsNearlyEqual(*InConstraint, InUsed, 0.5f))
            { InSetter(InNode, *InConstraint); }
        };
        ApplyIfBinding(Layout.MinSize[0], Box.Border.W, &YGNodeStyleSetMinWidth);
        ApplyIfBinding(Layout.MinSize[1], Box.Border.H, &YGNodeStyleSetMinHeight);
        ApplyIfBinding(Layout.MaxSize[0], Box.Border.W, &YGNodeStyleSetMaxWidth);
        ApplyIfBinding(Layout.MaxSize[1], Box.Border.H, &YGNodeStyleSetMaxHeight);

        if (Layout.AlignSelf != TEXT("auto"))
        { YGNodeStyleSetAlignSelf(InNode, ToAlign(Layout.AlignSelf, YGAlignAuto)); }

        SetEdges(InNode, MarginEdges(Box), &YGNodeStyleSetMargin);

        const auto EffectiveAlign = Layout.AlignSelf != TEXT("auto")
            ? ToAlign(Layout.AlignSelf, InParentAlignItems)
            : InParentAlignItems;

        const auto MainIsRow = InParentDirection == YGFlexDirectionRow
            || InParentDirection == YGFlexDirectionRowReverse;

        // Authored sizes beat stretch, exactly as Chromium prioritized them; stretch only drives
        // axes the author left unsized (sizingAuthored — the sizing twin of inset.authored).
        const auto WidthAuthored = Layout.SizingAuthored.Contains(TEXT("width"));
        const auto HeightAuthored = Layout.SizingAuthored.Contains(TEXT("height"));
        const auto CrossAuthored = MainIsRow ? HeightAuthored : WidthAuthored;

        const auto MainSizeIsFlexDriven = Layout.Grow > 0.0f;
        const auto CrossSizeIsStretchDriven = EffectiveAlign == YGAlignStretch && NOT CrossAuthored;

        const auto BorderW = Box.Border.W;
        const auto BorderH = Box.Border.H;

        // Text leaves: authored axes are explicit like any node (an authored height beats both
        // stretch AND the measure callback — L9's stretched-card defect); only genuinely
        // auto-sized axes are measure-driven.
        const auto MainAuthored = MainIsRow ? WidthAuthored : HeightAuthored;
        const auto SetMain = NOT MainSizeIsFlexDriven && (NOT InIsTextLeaf || MainAuthored);
        const auto SetCross = NOT CrossSizeIsStretchDriven && (NOT InIsTextLeaf || CrossAuthored);
        if (MainIsRow)
        {
            if (SetMain) { YGNodeStyleSetWidth(InNode, BorderW); }
            if (SetCross) { YGNodeStyleSetHeight(InNode, BorderH); }
        }
        else
        {
            if (SetMain) { YGNodeStyleSetHeight(InNode, BorderH); }
            if (SetCross) { YGNodeStyleSetWidth(InNode, BorderW); }
        }

        if (Layout.Position == TEXT("absolute"))
        {
            YGNodeStyleSetPositionType(InNode, YGPositionTypeAbsolute);
            if (Layout.Inset.IsSet())
            {
                const auto& Inset = *Layout.Inset;
                const auto SetIfAuthored = [&](const TCHAR* InSide, YGEdge InEdge, const FString& InValue)
                {
                    if (NOT Inset.Authored.Contains(InSide))
                    { return; }
                    if (const auto Px = ParsePx(InValue); Px.IsSet())
                    { YGNodeStyleSetPosition(InNode, InEdge, *Px); }
                };
                SetIfAuthored(TEXT("top"), YGEdgeTop, Inset.Top);
                SetIfAuthored(TEXT("right"), YGEdgeRight, Inset.Right);
                SetIfAuthored(TEXT("bottom"), YGEdgeBottom, Inset.Bottom);
                SetIfAuthored(TEXT("left"), YGEdgeLeft, Inset.Left);

                // All four edges authored = stretch: size comes from the insets, not the used box.
                if (Inset.Authored.Num() == 4)
                {
                    YGNodeStyleSetWidthAuto(InNode);
                    YGNodeStyleSetHeightAuto(InNode);
                }
            }
        }
    }

    // Gate 2 contract: the measure callback answers with the IR-recorded text box (Chromium's
    // truth), which exercises the full Yoga measure plumbing without importing the Chromium-vs-
    // Slate font-metric gap into every sibling's position. The Slate-side measurement is computed
    // alongside and logged, so the §8.1 divergence becomes numbers for Gate 3 — not a silent bake.
    auto
    MeasureTextLeaf(
        YGNodeConstRef InNode,
        float InWidth,
        YGMeasureMode InWidthMode,
        float InHeight,
        YGMeasureMode InHeightMode)
        -> YGSize
    {
        const auto* Ir = static_cast<const FCkWebUmg_IrNode*>(YGNodeGetContext(InNode));
        if (Ir == nullptr || NOT Ir->Text.IsSet())
        { return YGSize{0.0f, 0.0f}; }

        const auto& Text = *Ir->Text;
        auto Result = YGSize{Ir->Get_LayoutBox().Border.W, Ir->Get_LayoutBox().Border.H};

        if (FSlateApplication::IsInitialized())
        {
            const auto FontInfo = FCoreStyle::GetDefaultFontStyle(
                Text.Weight >= 600 ? "Bold" : "Regular", FMath::RoundToInt32(Text.SizePx));
            const auto FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
            const auto SlateMeasured = FontMeasure->Measure(Text.Content, FontInfo, 1.0f);
            ck::webumg::VeryVerbose(
                TEXT("MeasureTextLeaf [{}]: modes W={} H={} avail [{} x {}] -> recorded [{} x {}], slate [{} x {}]"),
                Ir->Id, static_cast<int32>(InWidthMode), static_cast<int32>(InHeightMode),
                InWidth, InHeight, Ir->Get_LayoutBox().Border.W, Ir->Get_LayoutBox().Border.H, SlateMeasured.X, SlateMeasured.Y);
        }

        if (InWidthMode == YGMeasureModeExactly)
        { Result.width = InWidth; }
        else if (InWidthMode == YGMeasureModeAtMost)
        { Result.width = FMath::Min(Result.width, InWidth); }
        if (InHeightMode == YGMeasureModeExactly)
        { Result.height = InHeight; }
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------

SCk_WebUmgFlexPanel::SCk_WebUmgFlexPanel()
    : _Children(this)
{
    _YogaConfig = YGConfigNew();
    YGConfigSetUseWebDefaults(_YogaConfig, true);
    YGConfigSetPointScaleFactor(_YogaConfig, 1.0f);
}

SCk_WebUmgFlexPanel::~SCk_WebUmgFlexPanel()
{
    if (_YogaRoot != nullptr)
    { YGNodeFreeRecursive(_YogaRoot); }
    if (_YogaConfig != nullptr)
    { YGConfigFree(_YogaConfig); }
}

auto
    SCk_WebUmgFlexPanel::
    Construct(
        const FArguments& InArgs)
    -> void
{
    _IrNode = InArgs._IrNode;
}

auto
    SCk_WebUmgFlexPanel::
    AddIrChild(
        TSharedPtr<const FCkWebUmg_IrNode> InChildIr,
        const TSharedRef<SWidget>& InWidget)
    -> void
{
    auto SlotArgs = FSlot::FSlotArguments{MakeUnique<FSlot>()};
    _Children.AddSlot(MoveTemp(SlotArgs));
    auto& Slot = _Children[_Children.Num() - 1];
    Slot.Set_IrNode(MoveTemp(InChildIr));
    Slot.AttachWidget(InWidget);
    _YogaTreeDirty = true;
}

auto
    SCk_WebUmgFlexPanel::
    DoRebuildYogaTree() const
    -> void
{
    if (_YogaRoot != nullptr)
    {
        YGNodeFreeRecursive(_YogaRoot);
        _YogaRoot = nullptr;
    }
    _YogaChildren.Reset();

    _YogaRoot = YGNodeNewWithConfig(_YogaConfig);

    const auto ParentDirection = _IrNode != nullptr
        ? ck_webumg_flexpanel::ToFlexDirection(_IrNode->Layout.Direction)
        : YGFlexDirectionColumn;
    const auto ParentAlign = _IrNode != nullptr
        ? ck_webumg_flexpanel::ToAlign(_IrNode->Layout.Align, YGAlignStretch)
        : YGAlignStretch;

    if (_IrNode != nullptr)
    { ck_webumg_flexpanel::ApplyContainerStyle(_YogaRoot, *_IrNode); }

    for (auto Index = 0; Index < _Children.Num(); ++Index)
    {
        const auto& ChildIr = _Children[Index].Get_IrNode();
        auto ChildNode = YGNodeNewWithConfig(_YogaConfig);

        if (ChildIr != nullptr)
        {
            const auto IsTextLeaf = ChildIr->Text.IsSet() && ChildIr->Children.Num() == 0;
            const auto ParentIsFlexContainer = _IrNode != nullptr
                && (_IrNode->Layout.Display == TEXT("flex") || _IrNode->Layout.Display == TEXT("inline-flex"));
            ck_webumg_flexpanel::ApplyItemStyle(ChildNode, *ChildIr, ParentDirection, ParentAlign,
                IsTextLeaf, ParentIsFlexContainer);

            if (IsTextLeaf)
            {
                YGNodeSetContext(ChildNode, const_cast<FCkWebUmg_IrNode*>(ChildIr.Get()));
                YGNodeSetMeasureFunc(ChildNode, &ck_webumg_flexpanel::MeasureTextLeaf);
            }
        }

        YGNodeInsertChild(_YogaRoot, ChildNode, Index);
        _YogaChildren.Add(ChildNode);
    }

    _YogaTreeDirty = false;
}

auto
    SCk_WebUmgFlexPanel::
    DoRunLayout(
        float InAvailableWidth,
        float InAvailableHeight) const
    -> void
{
    if (_YogaTreeDirty)
    { DoRebuildYogaTree(); }

    YGNodeStyleSetWidth(_YogaRoot, InAvailableWidth);
    YGNodeStyleSetHeight(_YogaRoot, InAvailableHeight);
    YGNodeCalculateLayout(_YogaRoot, InAvailableWidth, InAvailableHeight, YGDirectionLTR);
}

void
    SCk_WebUmgFlexPanel::
    OnArrangeChildren(
        const FGeometry& AllottedGeometry,
        FArrangedChildren& ArrangedChildren) const
{
    if (_Children.Num() == 0)
    { return; }

    const auto LocalSize = AllottedGeometry.GetLocalSize();
    DoRunLayout(LocalSize.X, LocalSize.Y);

    // Paint/hit-test order follows z-index (stable — equal z keeps source order) while layout
    // geometry stays indexed by the original slot order. Real CSS stacking contexts are §8.6
    // scope; sibling z-order covers the corpus and most mockups.
    auto PaintOrder = TArray<int32>{};
    PaintOrder.Reserve(_Children.Num());
    for (auto Index = 0; Index < _Children.Num(); ++Index)
    { PaintOrder.Add(Index); }
    PaintOrder.StableSort([&](int32 InA, int32 InB)
    {
        const auto& IrA = _Children[InA].Get_IrNode();
        const auto& IrB = _Children[InB].Get_IrNode();
        return (IrA != nullptr ? IrA->Layout.ZIndex : 0) < (IrB != nullptr ? IrB->Layout.ZIndex : 0);
    });

    for (const auto Index : PaintOrder)
    {
        const auto& Slot = _Children[Index];
        if (Slot.GetWidget()->GetVisibility() == EVisibility::Collapsed)
        { continue; }

        const auto Node = _YogaChildren[Index];
        const auto Position = FVector2D(YGNodeLayoutGetLeft(Node), YGNodeLayoutGetTop(Node));
        const auto Size = FVector2D(YGNodeLayoutGetWidth(Node), YGNodeLayoutGetHeight(Node));

        ArrangedChildren.AddWidget(
            AllottedGeometry.MakeChild(Slot.GetWidget(), Position, Size));
    }
}

FVector2D
    SCk_WebUmgFlexPanel::
    ComputeDesiredSize(
        float) const
{
    // The panel's own IR box is the desired size at the reference viewport; the harness drives
    // the root at the recorded viewport, so this stays exact. Unconstrained-Yoga desired size is
    // revisited when non-reference layouts arrive (post-v1 responsive work).
    if (_IrNode != nullptr)
    { return FVector2D(_IrNode->Get_LayoutBox().Border.W, _IrNode->Get_LayoutBox().Border.H); }
    return FVector2D::ZeroVector;
}

FChildren*
    SCk_WebUmgFlexPanel::
    GetChildren()
{
    return &_Children;
}

// --------------------------------------------------------------------------------------------------------------------
