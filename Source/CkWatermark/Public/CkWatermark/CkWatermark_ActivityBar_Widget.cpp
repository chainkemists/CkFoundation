#include "CkWatermark_ActivityBar_Widget.h"

#include <Widgets/SBoxPanel.h>
#include <Widgets/Layout/SBox.h>
#include <Widgets/Text/STextBlock.h>
#include <Widgets/Images/SImage.h>
#include <Styling/CoreStyle.h>

extern ENGINE_API uint64 GFrameCounter;

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkWatermarkActivityBar::
    Construct(const FArguments& InArgs)
    -> void
{
    _ActivityGetter       = InArgs._ActivityGetter;
    _Font                 = InArgs._Font;
    _BracketOpen          = InArgs._BracketOpen;
    _BracketClose         = InArgs._BracketClose;
    _BracketInnerPadding  = InArgs._BracketInnerPadding;
    _ChipSpacing          = InArgs._ChipSpacing;
    _HeldAccentHeight     = InArgs._HeldAccentHeight;
    _ActiveColor          = InArgs._ActiveColor;
    _HeldAccentColor      = InArgs._HeldAccentColor;
    _InactiveColor        = InArgs._InactiveColor;
    _ShadowOffset         = InArgs._ShadowOffset;
    _ShadowColorAndOpacity = InArgs._ShadowColorAndOpacity;

    _HBox = SNew(SHorizontalBox);

    ChildSlot
    [
        _HBox.ToSharedRef()
    ];
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkWatermarkActivityBar::
    Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime)
    -> void
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

    if (!_ActivityGetter)
    { return; }

    const auto [Version, StatesPtr] = _ActivityGetter();

    if (!StatesPtr || Version == _CachedVersion)
    { return; }

    _CachedVersion = Version;
    DoUpdateChips(*StatesPtr);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkWatermarkActivityBar::
    DoEnsurePoolSize(int32 InRequired)
    -> void
{
    const FSlateFontInfo ChipFont = _Font.Get();

    FSlateFontInfo CounterFont = ChipFont;
    CounterFont.Size = FMath::Max(CounterFont.Size - 3, 6);

    TAttribute<FSlateColor> AccentSlateColor = TAttribute<FSlateColor>::CreateLambda(
        [AccentColor = _HeldAccentColor]() -> FSlateColor
        {
            return FSlateColor(AccentColor.Get());
        });

    while (_ChipPool.Num() < InRequired)
    {
        TSharedPtr<SVerticalBox> Root;
        TSharedPtr<STextBlock>   LabelBlock;
        TSharedPtr<STextBlock>   CounterBlock;
        TSharedPtr<SBox>         AccentBox;

        _HBox->AddSlot()
            .AutoWidth()
            .Padding(_ChipSpacing, 0.f)
            [
                SAssignNew(Root, SVerticalBox)
                .Visibility(EVisibility::Collapsed)

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        SAssignNew(LabelBlock, STextBlock)
                        .Font(ChipFont)
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Bottom)
                    .Padding(1.f, 0.f, 0.f, 0.f)
                    [
                        SAssignNew(CounterBlock, STextBlock)
                        .Font(CounterFont)
                        .Visibility(EVisibility::Collapsed)
                    ]
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SAssignNew(AccentBox, SBox)
                    .HeightOverride(_HeldAccentHeight)
                    .Visibility(EVisibility::Collapsed)
                    [
                        SNew(SImage)
                        .Image(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
                        .ColorAndOpacity(AccentSlateColor)
                    ]
                ]
            ];

        FChipSlot Slot;
        Slot.Root         = Root;
        Slot.LabelBlock   = LabelBlock;
        Slot.CounterBlock = CounterBlock;
        Slot.AccentBar    = AccentBox;
        _ChipPool.Add(MoveTemp(Slot));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkWatermarkActivityBar::
    DoUpdateChips(const TArray<FCkWatermarkActivityState>& InStates)
    -> void
{
    const int32 Count = InStates.Num();
    DoEnsurePoolSize(Count);

    const FSlateFontInfo ChipFont = _Font.Get();

    FSlateFontInfo CounterFont = ChipFont;
    CounterFont.Size = FMath::Max(CounterFont.Size - 3, 6);

    TAttribute<FSlateColor>  ActiveColor   = _ActiveColor;
    TAttribute<FLinearColor> AccentColor   = _HeldAccentColor;
    TAttribute<FSlateColor>  InactiveColor = _InactiveColor;

    TMap<FName, int32> IdTotalCount;
    for (int32 i = 0; i < Count; ++i)
    {
        IdTotalCount.FindOrAdd(InStates[i].Id) += 1;
    }

    for (int32 i = 0; i < _ChipPool.Num(); ++i)
    {
        FChipSlot& Slot = _ChipPool[i];

        if (i >= Count)
        {
            Slot.Root->SetVisibility(EVisibility::Collapsed);
            continue;
        }

        const FCkWatermarkActivityState& State = InStates[i];
        const bool   IsActive          = State.IsActive;
        const uint64 ActivatedFrame   = State.ActivatedFrame;
        const uint64 DeactivatedFrame = State.DeactivatedFrame;

        const int32 TotalCount = IdTotalCount[State.Id];
        const FString ChipText = (TotalCount > 1)
            ? FString::Printf(TEXT("%s %s:%d %s"),
                *_BracketOpen.ToString(),
                *State.Label.ToString(),
                State.SequenceNumber,
                *_BracketClose.ToString())
            : FString::Printf(TEXT("%s %s %s"),
                *_BracketOpen.ToString(),
                *State.Label.ToString(),
                *_BracketClose.ToString());

        TAttribute<FSlateColor> ChipColor = TAttribute<FSlateColor>::CreateLambda(
            [IsActive, ActivatedFrame, DeactivatedFrame, ActiveColor, AccentColor, InactiveColor]() -> FSlateColor
            {
                if (!IsActive)
                {
                    const FSlateColor BaseColor = InactiveColor.Get();
                    FLinearColor Faded = BaseColor.GetSpecifiedColor();

                    const uint64 Age = (GFrameCounter > DeactivatedFrame)
                        ? (GFrameCounter - DeactivatedFrame)
                        : 0;

                    constexpr auto FullyFadedAfterFrames = 600.0f;
                    constexpr auto MinFadedAlpha         = 0.15f;
                    const float FadeFactor = FMath::Clamp(1.0f - static_cast<float>(Age) / FullyFadedAfterFrames, MinFadedAlpha, 1.0f);
                    Faded.A *= FadeFactor;

                    return FSlateColor(Faded);
                }

                return (GFrameCounter > ActivatedFrame)
                    ? FSlateColor(AccentColor.Get())
                    : ActiveColor.Get();
            });

        TAttribute<FText> FrameCountText = TAttribute<FText>::CreateLambda(
            [IsActive, ActivatedFrame, DeactivatedFrame]() -> FText
            {
                uint64 Held = 0;
                if (IsActive)
                {
                    if (GFrameCounter <= ActivatedFrame)
                    { return FText::GetEmpty(); }
                    Held = GFrameCounter - ActivatedFrame;
                }
                else
                {
                    if (DeactivatedFrame <= ActivatedFrame)
                    { return FText::GetEmpty(); }
                    Held = DeactivatedFrame - ActivatedFrame;
                }
                return FText::FromString(FString::Printf(TEXT("%lluf"), Held));
            });

        TAttribute<EVisibility> CounterVis = TAttribute<EVisibility>::CreateLambda(
            [IsActive, ActivatedFrame, DeactivatedFrame]() -> EVisibility
            {
                if (IsActive)
                {
                    return (GFrameCounter > ActivatedFrame)
                        ? EVisibility::SelfHitTestInvisible
                        : EVisibility::Collapsed;
                }
                return (DeactivatedFrame > ActivatedFrame)
                    ? EVisibility::SelfHitTestInvisible
                    : EVisibility::Collapsed;
            });

        TAttribute<EVisibility> AccentVis = TAttribute<EVisibility>::CreateLambda(
            [IsActive, ActivatedFrame]() -> EVisibility
            {
                return (IsActive && GFrameCounter > ActivatedFrame)
                    ? EVisibility::SelfHitTestInvisible
                    : EVisibility::Collapsed;
            });

        Slot.LabelBlock->SetText(FText::FromString(ChipText));
        Slot.LabelBlock->SetFont(ChipFont);
        Slot.LabelBlock->SetColorAndOpacity(ChipColor);
        Slot.LabelBlock->SetShadowOffset(_ShadowOffset);
        Slot.LabelBlock->SetShadowColorAndOpacity(_ShadowColorAndOpacity);

        Slot.CounterBlock->SetText(FrameCountText);
        Slot.CounterBlock->SetFont(CounterFont);
        Slot.CounterBlock->SetColorAndOpacity(ChipColor);
        Slot.CounterBlock->SetVisibility(CounterVis);
        Slot.CounterBlock->SetShadowOffset(_ShadowOffset);
        Slot.CounterBlock->SetShadowColorAndOpacity(_ShadowColorAndOpacity);

        Slot.AccentBar->SetVisibility(AccentVis);

        Slot.Root->SetVisibility(EVisibility::SelfHitTestInvisible);
    }
}

// --------------------------------------------------------------------------------------------------------------------
