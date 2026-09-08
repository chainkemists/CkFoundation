#pragma once

#include "CkFlexBox.h"

#include <initializer_list>

namespace ck::slate
{
    inline auto Content(
        const TSharedRef<SWidget>& InWidget,
        const EHorizontalAlignment InHorizontalAlignment = HAlign_Fill,
        const EVerticalAlignment InVerticalAlignment = VAlign_Fill) -> FCkFlexItem
    {
        return FCkFlexItem(InWidget, 0.0f, 0.0f, InHorizontalAlignment, InVerticalAlignment);
    }

    inline auto Fill(const TSharedRef<SWidget>& InWidget) -> FCkFlexItem
    {
        return FCkFlexItem(InWidget, 1.0f, 1.0f, HAlign_Fill, VAlign_Fill);
    }

    inline auto Row(const float InGap, const std::initializer_list<FCkFlexItem> InItems, const FMargin InPadding = FMargin(0.0f)) -> TSharedRef<SCkFlexBox>
    {
        TArray<FCkFlexItem> Items;
        Items.Reserve(static_cast<int32>(InItems.size()));
        for (const auto& Item : InItems)
        {
            Items.Add(Item);
        }
        return SNew(SCkFlexBox).Direction(Orient_Horizontal).Gap(InGap).Padding(InPadding).Items(MoveTemp(Items));
    }

    inline auto Column(const float InGap, const std::initializer_list<FCkFlexItem> InItems, const FMargin InPadding = FMargin(0.0f)) -> TSharedRef<SCkFlexBox>
    {
        TArray<FCkFlexItem> Items;
        Items.Reserve(static_cast<int32>(InItems.size()));
        for (const auto& Item : InItems)
        {
            Items.Add(Item);
        }
        return SNew(SCkFlexBox).Direction(Orient_Vertical).Gap(InGap).Padding(InPadding).Items(MoveTemp(Items));
    }
}
