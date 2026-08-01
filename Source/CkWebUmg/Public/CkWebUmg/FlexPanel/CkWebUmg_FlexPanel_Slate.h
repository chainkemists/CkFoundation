#pragma once

#include "CkWebUmg/Ir/CkWebUmg_Ir.h"

#include "Widgets/SPanel.h"
#include "Layout/Children.h"

#include <yoga/Yoga.h>

// ====================================================================================================================
// Yoga-backed flex container (campaign DECISION 1, option C). One panel = one CSS flex container;
// nested containers are nested panels, each running its own local Yoga pass — CSS flex containers
// lay out their children independently given their own box, so local decomposition is exact.
//
// Slate reconciliation (PriorArt §3): the real, width-constrained layout runs in OnArrangeChildren
// where the allotted geometry is known; ComputeDesiredSize runs an unconstrained Yoga pass. Text
// leaves register Yoga measure callbacks that answer via Slate's font measurement.
// ====================================================================================================================

class CKWEBUMG_API SCk_WebUmgFlexPanel : public SPanel
{
public:
    class FSlot : public TSlotBase<FSlot>
    {
    public:
        SLATE_SLOT_BEGIN_ARGS(FSlot, TSlotBase<FSlot>)
        SLATE_SLOT_END_ARGS()

        using TSlotBase<FSlot>::TSlotBase;

        auto Get_IrNode() const -> const TSharedPtr<const FCkWebUmg_IrNode>& { return _IrNode; }
        auto Set_IrNode(TSharedPtr<const FCkWebUmg_IrNode> InNode) -> void { _IrNode = MoveTemp(InNode); }

    private:
        TSharedPtr<const FCkWebUmg_IrNode> _IrNode;
    };

    SLATE_BEGIN_ARGS(SCk_WebUmgFlexPanel)
        {}
        SLATE_ARGUMENT(TSharedPtr<const FCkWebUmg_IrNode>, IrNode)
    SLATE_END_ARGS()

public:
    SCk_WebUmgFlexPanel();
    virtual ~SCk_WebUmgFlexPanel() override;

    auto Construct(const FArguments& InArgs) -> void;

    auto AddIrChild(
        TSharedPtr<const FCkWebUmg_IrNode> InChildIr,
        const TSharedRef<SWidget>& InWidget) -> void;

public:
    virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
    virtual FVector2D ComputeDesiredSize(float InLayoutScaleMultiplier) const override;
    virtual FChildren* GetChildren() override;

private:
    auto DoRebuildYogaTree() const -> void;
    auto DoRunLayout(float InAvailableWidth, float InAvailableHeight) const -> void;

private:
    TPanelChildren<FSlot> _Children;
    TSharedPtr<const FCkWebUmg_IrNode> _IrNode;

    YGConfigRef _YogaConfig = nullptr;
    mutable YGNodeRef _YogaRoot = nullptr;
    mutable TArray<YGNodeRef> _YogaChildren;
    mutable bool _YogaTreeDirty = true;
};

// --------------------------------------------------------------------------------------------------------------------
