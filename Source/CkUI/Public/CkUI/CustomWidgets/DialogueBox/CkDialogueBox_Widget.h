// Inspired by https://github.com/redxdev/UnrealRichTextDialogueBox

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkUI/UserWidget/CkUserWidget.h"

#include <CoreMinimal.h>
#include <Components/RichTextBlock.h>
#include <Framework/Text/RichTextLayoutMarshaller.h>
#include <Framework/Text/SlateTextLayout.h>

#include "CkDialogueBox_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_DialogueBox_UserWidget_UE;

// --------------------------------------------------------------------------------------------------------------------

/**
 * A text block that exposes internal text layout for typewriter effect calculations
 */
UCLASS()
class CKUI_API UCk_DialogueTextBlock : public URichTextBlock
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DialogueTextBlock);

public:
    auto
    Get_TextLayout() const -> TSharedPtr<FSlateTextLayout>;


    auto
    Get_TextMarshaller() const -> TSharedPtr<FRichTextLayoutMarshaller>;


protected:
    auto
    RebuildWidget()
        -> TSharedRef<SWidget> override;

private:
    TSharedPtr<FSlateTextLayout> _TextLayout;
    TSharedPtr<FRichTextLayoutMarshaller> _TextMarshaller;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_DialogueTextSegment
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_DialogueTextSegment);

private:
    UPROPERTY()
    FString _Text;

    FRunInfo _RunInfo;

    UPROPERTY()
    float _LineHeight = 0.0f;

public:
    CK_PROPERTY(_Text);
    CK_PROPERTY(_RunInfo);
    CK_PROPERTY(_LineHeight);
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_DialogueSpeed : uint8
{
    Slow,
    Normal,
    Fast,
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_DialogueSpeedSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_DialogueSpeedSettings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Time _BaseLetterTime = UCk_Utils_Time_UE::Make_FromMilliseconds(25);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, ClampMin = 1.0))
    float _PunctuationMultiplier = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, ClampMin = 1.0))
    float _CommaMultiplier = 2.0f;

public:
    CK_PROPERTY(_BaseLetterTime);
    CK_PROPERTY(_PunctuationMultiplier);
    CK_PROPERTY(_CommaMultiplier);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_DialogueSpeedSettings, _BaseLetterTime);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCk_DialogueEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCk_DialogueLetterEvent, int32, LetterIndex);

// --------------------------------------------------------------------------------------------------------------------

/**
 * A dialogue box widget with typewriter effect supporting rich text and inline widgets/images
 */
UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKUI_API UCk_DialogueBox_UserWidget_UE : public UCk_UserWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DialogueBox_UserWidget_UE);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Dialogue")
    void
    PlayLine(
        FText InLine,
        ECk_DialogueSpeed InSpeedPreset = ECk_DialogueSpeed::Normal);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Dialogue")
    void
    PlayLineWithCustomSpeed(
        FText InLine,
        const FCk_DialogueSpeedSettings& InSpeedSettings);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Dialogue")
    void
    SkipToLineEnd();

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Dialogue")
    void
    RecalculateLayout();

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Dialogue")
    void
    PausePlaying();

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Dialogue")
    void
    ResumePlaying();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|Dialogue")
    bool
    Get_HasFinishedPlayingLine() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|Dialogue")
    float
    Get_TypewriterProgress() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|Dialogue")
    FText
    Get_CurrentLine() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|Dialogue")
    bool
    Get_IsPlaying() const;

protected:
#if WITH_EDITOR
    auto GetPaletteCategory() -> const FText override;
#endif

protected:
    // Required binding to the text block that displays dialogue
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UCk_DialogueTextBlock> LineText;

    // Speed presets
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Speed")
    FCk_DialogueSpeedSettings _SlowSpeed = FCk_DialogueSpeedSettings{UCk_Utils_Time_UE::Make_FromMilliseconds(50)};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Speed")
    FCk_DialogueSpeedSettings _NormalSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Speed")
    FCk_DialogueSpeedSettings _FastSpeed = FCk_DialogueSpeedSettings{UCk_Utils_Time_UE::Make_FromMilliseconds(10)};

    // Customizable hold time at end of line
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Settings")
    FCk_Time _EndHoldTime = UCk_Utils_Time_UE::Make_FromMilliseconds(150);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Settings")
    bool _AutoSkipEmptyLines = true;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Dialogue Events")
    FCk_DialogueEvent OnDialogueStarted;

    UPROPERTY(BlueprintAssignable, Category = "Dialogue Events")
    FCk_DialogueEvent OnDialogueCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Dialogue Events")
    FCk_DialogueLetterEvent OnLetterPlayed;

    UPROPERTY(BlueprintAssignable, Category = "Dialogue Events")
    FCk_DialogueEvent OnLineFinishedPlaying;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Ck|UI|Dialogue")
    void
    OnPlayLetter(
        int32 InLetterIndex);

    UFUNCTION(BlueprintImplementableEvent, Category = "Ck|UI|Dialogue")
    void
    OnLineCompleted();

private:
    auto
    PlayNextLetter() -> void;

    auto
    CalculateWrappedString() -> void;

    auto
    BuildSegmentString() -> FString;

    auto
    CalculateNextLetterDelay() const -> float;

    auto
    Get_CurrentCharacter() const -> TOptional<TCHAR>;

    auto
    ResetPlaybackState() -> void;

    auto
    ApplySpeedSettings(
        ECk_DialogueSpeed InSpeedPreset) -> void;

private:
    UPROPERTY()
    FText _CurrentLine;

    UPROPERTY()
    TArray<FCk_DialogueTextSegment> _Segments;

    // Cached data for performance
    FString _CachedSegmentText;
    FString _CurrentDisplayText;
    int32 _CachedLetterIndex = 0;

    // Playback state
    int32 _CurrentSegmentIndex = 0;
    int32 _CurrentLetterIndex = 0;
    int32 _MaxLetterIndex = 0;

    // Current speed settings
    FCk_DialogueSpeedSettings _ActiveSpeedSettings;

    // Flags
    bool _HasFinishedPlaying = true;
    bool _IsPaused = false;

    // Timer
    FTimerHandle _LetterTimer;

    // Line height tracking for preventing jumps
    TMap<int32, float> _LineMaxHeights;
};

// --------------------------------------------------------------------------------------------------------------------