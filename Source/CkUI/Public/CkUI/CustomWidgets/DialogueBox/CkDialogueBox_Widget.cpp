#include "CkDialogueBox_Widget.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Game/CkGame_Utils.h"

#include <Engine/Font.h>
#include <Internationalization/BreakIterator.h>
#include <Styling/SlateStyle.h>
#include <TimerManager.h>
#include <Widgets/Text/SRichTextBlock.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::dialogue
{
    constexpr auto ZERO_WIDTH_SPACE = static_cast<TCHAR>(0x200B);

    auto
        IsCharacterPunctuation(
            TCHAR InChar)
        -> bool
    {
        return InChar == '.' || InChar == '!' || InChar == '?';
    }

    auto
        IsCharacterComma(
            TCHAR InChar)
        -> bool
    {
        return InChar == ',' || InChar == ';' || InChar == ':';
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DialogueTextBlock::
    Get_TextLayout() const
    -> TSharedPtr<FSlateTextLayout>
{
    return _TextLayout;
}

auto
    UCk_DialogueTextBlock::
    Get_TextMarshaller() const
    -> TSharedPtr<FRichTextLayoutMarshaller>
{
    return _TextMarshaller;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DialogueTextBlock::
    RebuildWidget()
    -> TSharedRef<SWidget>
{
    // Based on URichTextBlock::RebuildWidget
    UpdateStyleData();

    TArray<TSharedRef<class ITextDecorator>> CreatedDecorators;
    CreateDecorators(CreatedDecorators);

    _TextMarshaller = FRichTextLayoutMarshaller::Create(
        CreateMarkupParser(),
        CreateMarkupWriter(),
        CreatedDecorators,
        StyleInstance.Get());

    MyRichTextBlock =
        SNew(SRichTextBlock)
        .TextStyle(bOverrideDefaultStyle ? &GetDefaultTextStyleOverride() : &DefaultTextStyle)
        .Marshaller(_TextMarshaller)
        .CreateSlateTextLayout(
            FCreateSlateTextLayout::CreateWeakLambda(this,
                [this](SWidget* InOwner, const FTextBlockStyle& InDefaultTextStyle) mutable
                {
                    _TextLayout = FSlateTextLayout::Create(InOwner, InDefaultTextStyle);
                    return StaticCastSharedPtr<FSlateTextLayout>(_TextLayout).ToSharedRef();
                }));

    return MyRichTextBlock.ToSharedRef();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DialogueBox_UserWidget_UE::
    PlayLine(
        FText InLine,
        ECk_DialogueSpeed InSpeedPreset)
    -> void
{
    ApplySpeedSettings(InSpeedPreset);
    PlayLineWithCustomSpeed(InLine, _ActiveSpeedSettings);
}

auto
    UCk_DialogueBox_UserWidget_UE::
    PlayLineWithCustomSpeed(
        FText InLine,
        const FCk_DialogueSpeedSettings& InSpeedSettings)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(GetWorld()),
        TEXT("Trying to play dialogue line without valid world context"))
    { return; }

    auto& TimerManager = GetWorld()->GetTimerManager();
    TimerManager.ClearTimer(_LetterTimer);

    _ActiveSpeedSettings = InSpeedSettings;
    _CurrentLine = InLine;
    ResetPlaybackState();

    if (_CurrentLine.IsEmpty())
    {
        if (ck::IsValid(LineText))
        {
            LineText->SetText(FText::GetEmpty());
        }

        _HasFinishedPlaying = true;
        OnLineCompleted();
        OnLineFinishedPlaying.Broadcast();
        OnDialogueCompleted.Broadcast();

        if (_AutoSkipEmptyLines)
        {
            SetVisibility(ESlateVisibility::Hidden);
        }
        return;
    }

    if (ck::IsValid(LineText))
    {
        LineText->SetText(FText::GetEmpty());
    }

    _HasFinishedPlaying = false;
    _IsPaused = false;

    OnDialogueStarted.Broadcast();

    FTimerDelegate Delegate;
    Delegate.BindUObject(this, &ThisClass::PlayNextLetter);

    const auto InitialDelay = _ActiveSpeedSettings.Get_BaseLetterTime().Get_Seconds();
    TimerManager.SetTimer(_LetterTimer, Delegate, InitialDelay, false);

    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

auto
    UCk_DialogueBox_UserWidget_UE::
    SkipToLineEnd()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(GetWorld()),
        TEXT("Cannot skip to line end without valid world context"))
    { return; }

    auto& TimerManager = GetWorld()->GetTimerManager();
    TimerManager.ClearTimer(_LetterTimer);

    _CurrentLetterIndex = _MaxLetterIndex - 1;
    _CurrentDisplayText = BuildSegmentString();

    if (ck::IsValid(LineText))
    {
        LineText->SetText(FText::FromString(_CurrentDisplayText));
    }

    _HasFinishedPlaying = true;
    _IsPaused = false;

    OnLineCompleted();
    OnLineFinishedPlaying.Broadcast();
    OnDialogueCompleted.Broadcast();
}

auto
    UCk_DialogueBox_UserWidget_UE::
    RecalculateLayout()
    -> void
{
    if (_HasFinishedPlaying || _CurrentLine.IsEmpty())
    { return; }

    // Store current progress
    const auto CurrentProgress = Get_TypewriterProgress();

    // Clear cached data
    _Segments.Empty();
    _CachedSegmentText.Empty();
    _CachedLetterIndex = 0;
    _CurrentSegmentIndex = 0;
    _LineMaxHeights.Empty();

    // Recalculate
    CalculateWrappedString();

    // Restore progress
    _CurrentLetterIndex = FMath::RoundToInt(CurrentProgress * _MaxLetterIndex);
    _CurrentDisplayText = BuildSegmentString();

    if (ck::IsValid(LineText))
    {
        LineText->SetText(FText::FromString(_CurrentDisplayText));
    }
}

auto
    UCk_DialogueBox_UserWidget_UE::
    PausePlaying()
    -> void
{
    if (_HasFinishedPlaying || _IsPaused)
    { return; }

    _IsPaused = true;

    if (ck::IsValid(GetWorld()))
    {
        GetWorld()->GetTimerManager().PauseTimer(_LetterTimer);
    }
}

auto
    UCk_DialogueBox_UserWidget_UE::
    ResumePlaying()
    -> void
{
    if (_HasFinishedPlaying || NOT _IsPaused)
    { return; }

    _IsPaused = false;

    if (ck::IsValid(GetWorld()))
    {
        GetWorld()->GetTimerManager().UnPauseTimer(_LetterTimer);
    }
}

auto
    UCk_DialogueBox_UserWidget_UE::
    Get_HasFinishedPlayingLine() const
    -> bool
{
    return _HasFinishedPlaying;
}

auto
    UCk_DialogueBox_UserWidget_UE::
    Get_TypewriterProgress() const
    -> float
{
    return _MaxLetterIndex > 0
        ? static_cast<float>(_CurrentLetterIndex) / static_cast<float>(_MaxLetterIndex)
        : 0.0f;
}

auto
    UCk_DialogueBox_UserWidget_UE::
    Get_CurrentLine() const
    -> FText
{
    return _CurrentLine;
}

auto
    UCk_DialogueBox_UserWidget_UE::
    Get_IsPlaying() const
    -> bool
{
    return NOT _HasFinishedPlaying && NOT _IsPaused;
}

auto
    UCk_DialogueBox_UserWidget_UE::
    PlayNextLetter()
    -> void
{
    if (_Segments.IsEmpty())
    {
        CalculateWrappedString();
    }

    if (_CurrentLetterIndex < _MaxLetterIndex)
    {
        _CurrentDisplayText = BuildSegmentString();

        if (ck::IsValid(LineText))
        {
            LineText->SetText(FText::FromString(_CurrentDisplayText));
        }

        OnPlayLetter(_CurrentLetterIndex);
        OnLetterPlayed.Broadcast(_CurrentLetterIndex);

        ++_CurrentLetterIndex;

        // Schedule next letter with appropriate delay
        const auto NextDelay = CalculateNextLetterDelay();

        FTimerDelegate Delegate;
        Delegate.BindUObject(this, &ThisClass::PlayNextLetter);

        GetWorld()->GetTimerManager().SetTimer(_LetterTimer, Delegate, NextDelay, false);
    }
    else
    {
        // Ensure final text is displayed
        _CurrentDisplayText = BuildSegmentString();

        if (ck::IsValid(LineText))
        {
            LineText->SetText(FText::FromString(_CurrentDisplayText));
        }

        // Hold at end before marking complete
        FTimerDelegate Delegate;
        Delegate.BindUObject(this, &ThisClass::SkipToLineEnd);

        GetWorld()->GetTimerManager().SetTimer(
            _LetterTimer,
            Delegate,
            _EndHoldTime.Get_Seconds(),
            false);
    }
}

auto
    UCk_DialogueBox_UserWidget_UE::
    CalculateWrappedString()
    -> void
{
    if (NOT ck::IsValid(LineText) || NOT LineText->Get_TextLayout().IsValid())
    {
        // Fallback for simple text
        FCk_DialogueTextSegment Segment;
        Segment.Set_Text(_CurrentLine.ToString());
        _Segments.Add(Segment);
        _MaxLetterIndex = Segment.Get_Text().Len();
        return;
    }

    const auto Layout = LineText->Get_TextLayout();
    const auto Marshaller = LineText->Get_TextMarshaller();

    const auto& TextBoxGeometry = LineText->GetCachedGeometry();
    const auto TextBoxSize = TextBoxGeometry.GetLocalSize();

    Layout->SetWrappingWidth(TextBoxSize.X);
    Marshaller->SetText(_CurrentLine.ToString(), *Layout.Get());
    Layout->UpdateIfNeeded();

    auto bHasWrittenText = false;
    auto CurrentLineIndex = 0;

    for (const auto& View : Layout->GetLineViews())
    {
        const auto& Model = Layout->GetLineModels()[View.ModelIndex];
        auto MaxHeightForLine = 0.0f;

        for (const auto& Block : View.Blocks)
        {
            const auto Run = Block->GetRun();

            FCk_DialogueTextSegment Segment;
            Run->AppendTextTo(Segment.Get_Text(), Block->GetTextRange());

            // Handle zero-width space from decorators
            if (Segment.Get_Text().Len() == 1 &&
                Segment.Get_Text()[0] == ck::dialogue::ZERO_WIDTH_SPACE)
            {
                Segment.Get_Text().Empty();
            }

            Segment.Set_RunInfo(Run->GetRunInfo());

            // Track line height
            const auto BlockHeight = Block->GetSize().Y;
            MaxHeightForLine = FMath::Max(MaxHeightForLine, BlockHeight);
            Segment.Set_LineHeight(BlockHeight);

            _Segments.Add(Segment);

            // Count letters (including decorators without text)
            _MaxLetterIndex += FMath::Max(
                Segment.Get_Text().Len(),
                Segment.Get_RunInfo().Name.IsEmpty() ? 0 : 1);

            if (NOT Segment.Get_Text().IsEmpty() ||
                NOT Segment.Get_RunInfo().Name.IsEmpty())
            {
                bHasWrittenText = true;
            }
        }

        // Store max height for this line
        if (MaxHeightForLine > 0.0f)
        {
            _LineMaxHeights.Add(CurrentLineIndex, MaxHeightForLine);
        }

        if (bHasWrittenText)
        {
            FCk_DialogueTextSegment NewlineSegment;
            NewlineSegment.Set_Text(TEXT("\n"));
            _Segments.Add(NewlineSegment);
            ++_MaxLetterIndex;
            ++CurrentLineIndex;
        }
    }

    // Reset layout
    Layout->SetWrappingWidth(0);
    LineText->SetText(LineText->GetText());
}

auto
    UCk_DialogueBox_UserWidget_UE::
    BuildSegmentString()
    -> FString
{
    TStringBuilder<512> Result;
    Result.Append(_CachedSegmentText);

    auto Idx = _CachedLetterIndex;

    while (Idx <= _CurrentLetterIndex && _CurrentSegmentIndex < _Segments.Num())
    {
        const auto& Segment = _Segments[_CurrentSegmentIndex];
        const auto& RunInfo = Segment.Get_RunInfo();

        if (NOT RunInfo.Name.IsEmpty())
        {
            Result.Appendf(TEXT("<%s"), *RunInfo.Name);

            if (NOT RunInfo.MetaData.IsEmpty())
            {
                for (const auto& [Key, Value] : RunInfo.MetaData)
                {
                    Result.Appendf(TEXT(" %s=\"%s\""), *Key, *Value);
                }
            }

            if (Segment.Get_Text().IsEmpty())
            {
                Result.Append(TEXT("/>"));
                ++Idx; // Decorator takes up one index
            }
            else
            {
                Result.Append(TEXT(">"));
            }
        }

        auto bIsSegmentComplete = true;

        if (NOT Segment.Get_Text().IsEmpty())
        {
            auto LettersLeft = _CurrentLetterIndex - Idx + 1;
            bIsSegmentComplete = LettersLeft >= Segment.Get_Text().Len();
            LettersLeft = FMath::Min(LettersLeft, Segment.Get_Text().Len());
            Idx += LettersLeft;

            Result.Append(Segment.Get_Text().Mid(0, LettersLeft));

            if (NOT RunInfo.Name.IsEmpty())
            {
                Result.Append(TEXT("</>"));
            }
        }

        if (bIsSegmentComplete)
        {
            _CachedLetterIndex = Idx;
            _CachedSegmentText = Result.ToString();
            ++_CurrentSegmentIndex;
        }
        else
        {
            break;
        }
    }

    return Result.ToString();
}

auto
    UCk_DialogueBox_UserWidget_UE::
    CalculateNextLetterDelay() const
    -> float
{
    auto BaseDelay = _ActiveSpeedSettings.Get_BaseLetterTime().Get_Seconds();

    // Check current character for punctuation
    if (const auto CurrentChar = Get_CurrentCharacter(); CurrentChar.IsSet())
    {
        if (ck::dialogue::IsCharacterPunctuation(CurrentChar.GetValue()))
        {
            BaseDelay *= _ActiveSpeedSettings.Get_PunctuationMultiplier();
        }
        else if (ck::dialogue::IsCharacterComma(CurrentChar.GetValue()))
        {
            BaseDelay *= _ActiveSpeedSettings.Get_CommaMultiplier();
        }
    }

    return BaseDelay;
}

auto
    UCk_DialogueBox_UserWidget_UE::
    Get_CurrentCharacter() const
    -> TOptional<TCHAR>
{
    if (_CurrentSegmentIndex >= _Segments.Num())
    { return {}; }

    const auto& Segment = _Segments[_CurrentSegmentIndex];
    const auto& Text = Segment.Get_Text();

    if (Text.IsEmpty())
    { return {}; }

    const auto LocalIndex = _CurrentLetterIndex - _CachedLetterIndex;

    if (LocalIndex >= 0 && LocalIndex < Text.Len())
    {
        return Text[LocalIndex];
    }

    return {};
}

auto
    UCk_DialogueBox_UserWidget_UE::
    ResetPlaybackState()
    -> void
{
    _CurrentLetterIndex = 0;
    _CachedLetterIndex = 0;
    _CurrentSegmentIndex = 0;
    _MaxLetterIndex = 0;
    _Segments.Empty();
    _CachedSegmentText.Empty();
    _CurrentDisplayText.Empty();
    _LineMaxHeights.Empty();
}

auto
    UCk_DialogueBox_UserWidget_UE::
    ApplySpeedSettings(
        ECk_DialogueSpeed InSpeedPreset)
    -> void
{
    switch (InSpeedPreset)
    {
        case ECk_DialogueSpeed::Slow:
        {
            _ActiveSpeedSettings = _SlowSpeed;
            break;
        }
        case ECk_DialogueSpeed::Fast:
        {
            _ActiveSpeedSettings = _FastSpeed;
            break;
        }
        case ECk_DialogueSpeed::Normal:
        default:
        {
            _ActiveSpeedSettings = _NormalSpeed;
            break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_DialogueBox_UserWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

// --------------------------------------------------------------------------------------------------------------------