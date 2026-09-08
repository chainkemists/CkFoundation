#include "CkSlateLayout/SCkUiRepeat.h"

#include "CkSlateLayout/CkFlexBox.h"
#include "Widgets/SNullWidget.h"

void SCkUiRepeat::Construct(const FArguments& InArgs)
{
    _Collection = InArgs._Collection;
    _OnRefresh = InArgs._OnRefresh;
    if (_Collection.IsValid())
    { _CollectionChangedHandle = _Collection->OnChanged().AddSP(SharedThis(this), &SCkUiRepeat::MarkDirty); }
    AddMetadata(MakeShared<FCkFlexMeasureMetaData>(
        [WeakRepeat = TWeakPtr<SCkUiRepeat>(SharedThis(this))](const FCkFlexMeasureArgs& InArgs) -> FVector2D
        {
            const TSharedPtr<SCkUiRepeat> Repeat = WeakRepeat.Pin();
            const TSharedPtr<SCkFlexBox> Box = Repeat.IsValid() ? Repeat->_Box : nullptr;
            const TSharedPtr<FCkFlexMeasureMetaData> Measure = Box.IsValid() ? Box->GetMetaData<FCkFlexMeasureMetaData>() : nullptr;
            return Measure.IsValid() ? Measure->Measure(InArgs) : FVector2D::ZeroVector;
        },
        [WeakRepeat = TWeakPtr<SCkUiRepeat>(SharedThis(this))](const float Width, const float Height)
        {
            const TSharedPtr<SCkUiRepeat> Repeat = WeakRepeat.Pin();
            const TSharedPtr<SCkFlexBox> Box = Repeat.IsValid() ? Repeat->_Box : nullptr;
            const TSharedPtr<FCkFlexMeasureMetaData> Measure = Box.IsValid() ? Box->GetMetaData<FCkFlexMeasureMetaData>() : nullptr;
            if (Measure.IsValid()) { Measure->NotifyArranged(Width, Height); }
        }));
    ChildSlot[SNullWidget::NullWidget];
}

SCkUiRepeat::~SCkUiRepeat()
{
    if (_Collection.IsValid() && _CollectionChangedHandle.IsValid())
    { _Collection->OnChanged().Remove(_CollectionChangedHandle); }
}

void SCkUiRepeat::MarkDirty()
{
    _Dirty = true;
}

void SCkUiRepeat::Tick(const FGeometry&, double, float)
{
    TryRefresh();
}

auto SCkUiRepeat::TryRefresh() -> bool
{
    if (!_Dirty || _Refreshing || !_OnRefresh.IsBound()) { return false; }
    TGuardValue<bool> Guard(_Refreshing, true);
    const bool Refreshed = _OnRefresh.Execute();
    if (Refreshed) { _Dirty = false; }
    return Refreshed;
}

void SCkUiRepeat::SetItems(TArray<FItem> InItems, const float InGap, const FMargin& InPadding,
    const EOrientation InDirection, const ECkFlexWrap InWrap)
{
    auto Slots = TArray<SCkFlexBox::FSlot::FSlotArguments>{};
    Slots.Reserve(InItems.Num());
    _Items.Reset();
    for (FItem& Item : InItems)
    {
        if (!Item.Widget.IsValid()) { continue; }
        _Items.Add(Item.Key, Item.Widget);
        auto Slot = SCkFlexBox::Slot();
        Slot.Grow(0.0f).Shrink(0.0f)[Item.Widget.ToSharedRef()];
        Slots.Add(MoveTemp(Slot));
    }
    auto Arguments = SCkFlexBox::FArguments{};
    Arguments._Direction = InDirection;
    Arguments._Wrap = InWrap;
    Arguments._Gap = InGap;
    Arguments._Padding = InPadding;
    Arguments._Slots = MoveTemp(Slots);
    _Box = SArgumentNew(Arguments, SCkFlexBox);
    ChildSlot[_Box.ToSharedRef()];
    _LastFailure.Reset();
    _Dirty = false;
}

void SCkUiRepeat::SetLastFailure(FString InFailure)
{
    _LastFailure = MoveTemp(InFailure);
}

auto SCkUiRepeat::GetItemWidget(const FString& InKey) const -> TSharedPtr<SWidget>
{
    return _Items.FindRef(InKey);
}

auto SCkUiRepeat::GetItemCount() const -> int32
{
    return _Items.Num();
}

const FString& SCkUiRepeat::GetLastFailure() const
{
    return _LastFailure;
}
