#pragma once

#include "CkSlateLayout/CkUiCollection.h"
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

DECLARE_DELEGATE_RetVal(bool, FCkUiRepeatRefresh);

class SCkFlexBox;

/** Retained keyed collection presenter. Its owner supplies transactional item publication. */
class CKSLATELAYOUT_API SCkUiRepeat final : public SCompoundWidget
{
public:
    struct FItem
    {
        FString Key;
        TSharedPtr<const FCkUiRecord> Record;
        TSharedPtr<SWidget> Widget;
    };

    SLATE_BEGIN_ARGS(SCkUiRepeat) {}
        SLATE_ARGUMENT(TSharedPtr<FCkUiCollection>, Collection)
        SLATE_EVENT(FCkUiRepeatRefresh, OnRefresh)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SCkUiRepeat() override;
    virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

    /** Requests a transactional topology refresh when the collection identity sequence changed. */
    auto TryRefresh() -> bool;
    void SetItems(TArray<FItem> InItems, float InGap, const FMargin& InPadding,
        EOrientation InDirection = Orient_Vertical, ECkFlexWrap InWrap = ECkFlexWrap::NoWrap);
    void SetLastFailure(FString InFailure);
    auto GetItemWidget(const FString& InKey) const -> TSharedPtr<SWidget>;
    auto GetItemCount() const -> int32;
    const FString& GetLastFailure() const;

private:
    void MarkDirty();

    TSharedPtr<FCkUiCollection> _Collection;
    FDelegateHandle _CollectionChangedHandle;
    FCkUiRepeatRefresh _OnRefresh;
    TSharedPtr<SCkFlexBox> _Box;
    TMap<FString, TSharedPtr<SWidget>> _Items;
    FString _LastFailure;
    bool _Dirty = true;
    bool _Refreshing = false;
};
