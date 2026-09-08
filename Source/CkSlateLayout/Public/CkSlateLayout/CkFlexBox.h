#pragma once

#include "CoreMinimal.h"
#include "CkFlexWrap.h"
#include "CkFlexLayoutTypes.h"
#include "Layout/Children.h"
#include "Widgets/SPanel.h"

struct FCkFlexItem
{
    explicit FCkFlexItem(
        const TSharedRef<SWidget>& InWidget,
        const float InGrow = 0.0f,
        const float InShrink = 0.0f,
        const EHorizontalAlignment InHorizontalAlignment = HAlign_Fill,
        const EVerticalAlignment InVerticalAlignment = VAlign_Fill)
        : Widget(InWidget)
        , Grow(InGrow)
        , Shrink(InShrink)
        , HorizontalAlignment(InHorizontalAlignment)
        , VerticalAlignment(InVerticalAlignment)
    {
    }

    TSharedRef<SWidget> Widget;
    float Grow;
    float Shrink;
    EHorizontalAlignment HorizontalAlignment;
    EVerticalAlignment VerticalAlignment;
};

class CKSLATELAYOUT_API SCkFlexBox final : public SPanel
{
public:
    class FSlot final : public TBasicLayoutWidgetSlot<FSlot>
    {
    public:
        SLATE_SLOT_BEGIN_ARGS(FSlot, TBasicLayoutWidgetSlot<FSlot>)
            SLATE_ARGUMENT(TOptional<float>, Grow)
            SLATE_ARGUMENT(TOptional<float>, Shrink)
            SLATE_ARGUMENT(TOptional<float>, MinWidth)
            SLATE_ARGUMENT(TOptional<float>, MinHeight)
            SLATE_ARGUMENT(TOptional<float>, MaxWidth)
            SLATE_ARGUMENT(TOptional<float>, MaxHeight)
            SLATE_ARGUMENT(FCkFlexMeasureMetaData::FMeasure, Measure)
            SLATE_ARGUMENT(FCkFlexMeasureMetaData::FOnArranged, OnArranged)
        SLATE_SLOT_END_ARGS()

        FSlot();
        void Construct(const FChildren& InSlotOwner, FSlotArguments&& InArgs);

        float Get_Grow() const { return _Grow; }
        float Get_Shrink() const { return _Shrink; }
        float Get_MinWidth() const { return _MinWidth; }
        float Get_MinHeight() const { return _MinHeight; }
        float Get_MaxWidth() const { return _MaxWidth.Get(YGUndefined); }
        float Get_MaxHeight() const { return _MaxHeight.Get(YGUndefined); }
        bool Has_MaxWidth() const { return _MaxWidth.IsSet(); }
        bool Has_MaxHeight() const { return _MaxHeight.IsSet(); }
        const FCkFlexMeasureMetaData::FMeasure& Get_Measure() const { return _Measure; }
        const FCkFlexMeasureMetaData::FOnArranged& Get_OnArranged() const { return _OnArranged; }
        SCkFlexBox* Get_Panel() const { return _Panel; }
        bool IsStyleValid() const;

    private:
        float _Grow;
        float _Shrink;
        float _MinWidth;
        float _MinHeight;
        TOptional<float> _MaxWidth;
        TOptional<float> _MaxHeight;
        FCkFlexMeasureMetaData::FMeasure _Measure;
        FCkFlexMeasureMetaData::FOnArranged _OnArranged;
        SCkFlexBox* _Panel = nullptr;
    };

    SLATE_BEGIN_ARGS(SCkFlexBox)
        : _Direction(Orient_Horizontal)
        , _Wrap(ECkFlexWrap::NoWrap)
        , _Gap(0.0f)
        , _Padding(FMargin(0.0f))
        , _HAlign(HAlign_Fill)
        , _VAlign(VAlign_Fill)
    {
    }
        SLATE_SLOT_ARGUMENT(FSlot, Slots)
        SLATE_ARGUMENT(TArray<FCkFlexItem>, Items)
        SLATE_ARGUMENT(EOrientation, Direction)
        SLATE_ARGUMENT(ECkFlexWrap, Wrap)
        SLATE_ARGUMENT(float, Gap)
        SLATE_ARGUMENT(FMargin, Padding)
        SLATE_ARGUMENT(EHorizontalAlignment, HAlign)
        SLATE_ARGUMENT(EVerticalAlignment, VAlign)
    SLATE_END_ARGS()

    static FSlot::FSlotArguments Slot();
    SCkFlexBox();

    class CKSLATELAYOUT_API FScopedSlotArguments final : public FSlot::FSlotArguments
    {
    public:
        FScopedSlotArguments(TUniquePtr<FSlot> InSlot, SCkFlexBox& InOwner, int32 InIndex);
        ~FScopedSlotArguments();

        FScopedSlotArguments(const FScopedSlotArguments&) = delete;
        FScopedSlotArguments& operator=(const FScopedSlotArguments&) = delete;
        FScopedSlotArguments(FScopedSlotArguments&&) = default;
        FScopedSlotArguments& operator=(FScopedSlotArguments&&) = default;

    private:
        SCkFlexBox& _Owner;
        int32 _Index;
    };

    FScopedSlotArguments AddSlot(int32 InIndex = INDEX_NONE);
    int32 RemoveSlot(const TSharedRef<SWidget>& InWidget);
    void Construct(const FArguments& InArgs);
    virtual ~SCkFlexBox() override;

    virtual void OnArrangeChildren(const FGeometry& InAllottedGeometry, FArrangedChildren& OutArrangedChildren) const override;
    virtual FVector2D ComputeDesiredSize(float InLayoutScaleMultiplier) const override;
    virtual FChildren* GetChildren() override;
    int32 Get_LayoutPassCount() const { return _LayoutPassCount; }
    int32 Get_NodeCount() const { return _YogaChildren.Num(); }
    int32 Get_YogaNodeAllocationCount() const { return _YogaNodeAllocationCount; }

private:
    static YGSize MeasureLeaf(YGNodeConstRef InNode, float InWidth, YGMeasureMode InWidthMode, float InHeight, YGMeasureMode InHeightMode);
    FVector2D MeasureForConstraints(const FCkFlexMeasureArgs& InArgs) const;
    void CalculateLayout(float InWidth, float InHeight, float InScale) const;
    FVector2D MeasureSlot(const FSlot& InSlot, const FCkFlexMeasureArgs& InArgs) const;
    void BuildYogaTree();
    void RebuildYogaTree();
    void ApplySlotStyle(YGNodeRef InNode, const FSlot& InSlot) const;
    void SynchronizeSlotStyles() const;
    bool HasValidStyle() const;

    TPanelChildren<FSlot> _Children;
    YGConfigRef _YogaConfig = nullptr;
    YGNodeRef _YogaRoot = nullptr;
    TArray<YGNodeRef> _YogaChildren;
    EOrientation _Direction = Orient_Horizontal;
    ECkFlexWrap _Wrap = ECkFlexWrap::NoWrap;
    float _Gap = 0.0f;
    FMargin _Padding;
    EHorizontalAlignment _HorizontalAlignment = HAlign_Fill;
    EVerticalAlignment _VerticalAlignment = VAlign_Fill;
    mutable float _ActiveScale = 1.0f;
    mutable int32 _LayoutPassCount = 0;
    int32 _YogaNodeAllocationCount = 0;
    mutable bool _IsArranging = false;
};
