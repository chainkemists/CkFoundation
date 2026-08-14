#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include <CoreMinimal.h>
#include <Components/Image.h>
#include <Styling/SlateBrush.h>
#include <Styling/SlateColor.h>

#include "CkFlipbook_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UPaperFlipbook;
class UTexture2D;
class FActiveTimerHandle;
enum class EActiveTimerReturnType : uint8;

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCk_Flipbook_Event);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_FlipbookSource : uint8
{
    // Brushes built from a UPaperFlipbook's sprite keyframes; timing from the flipbook's FPS.
    PaperFlipbook,
    // Brushes built from a single texture sliced into Rows x Columns UV cells; timing from Duration.
    SpriteSheet
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_FlipbookSource);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWIDGETS_API FCk_FlipbookSpriteSheet
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_FlipbookSpriteSheet);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flipbook",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture2D> _Texture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flipbook",
              meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _Rows = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flipbook",
              meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _Columns = 1;

    // Total time to play through every cell once.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flipbook",
              meta = (AllowPrivateAccess = true))
    FCk_Time _Duration = FCk_Time{1.0};

public:
    CK_PROPERTY_GET(_Texture);
    CK_PROPERTY_GET(_Rows);
    CK_PROPERTY_GET(_Columns);
    CK_PROPERTY_GET(_Duration);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Animated image widget that plays a UPaperFlipbook frame-by-frame.
 * Configure via the properties below; usable directly with no WBP wrapper.
 */
UCLASS(meta = (DisplayName = "Ck Flipbook"))
class CKWIDGETS_API UCk_FlipbookWidget_UE : public UImage
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_FlipbookWidget_UE);

public:
    // Fired when a non-looping flipbook reaches its last frame.
    UPROPERTY(BlueprintAssignable, Category = "Ck|UI|Flipbook|Events")
    FCk_Flipbook_Event OnFinished;

    UPROPERTY(BlueprintAssignable, Category = "Ck|UI|Flipbook|Events")
    FCk_Flipbook_Event OnLoop;

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Flipbook")
    void
    PlayFlipbookAnimation(
        int32 InStartFrame = 0);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Flipbook")
    void
    PauseFlipbookAnimation();

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Flipbook")
    void
    StopFlipbookAnimation();

    // Call after changing any config property (SourceFlipbook, ImageSize, Tint, PlayRate, ...).
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Flipbook")
    void
    RestartWithUpdatedParameters();

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Flipbook")
    void
    Request_SetSourceFlipbook(
        UPaperFlipbook* InSourceFlipbook);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Flipbook")
    void
    Request_SetSpriteSheet(
        const FCk_FlipbookSpriteSheet& InSpriteSheet);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Flipbook")
    void
    Request_SetLooping(
        bool InLoop);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Flipbook")
    void
    Request_SetAutoplay(
        bool InAutoplay);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|Flipbook")
    bool
    Get_IsPlaying() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|Flipbook")
    int32
    Get_CurrentFrame() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|Flipbook")
    int32
    Get_TotalFrames() const;

public:
    auto SynchronizeProperties() -> void override;
    auto ReleaseSlateResources(bool bReleaseChildren) -> void override;

#if WITH_EDITOR
    auto GetPaletteCategory() -> const FText override;
#endif

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flipbook",
              meta = (AllowPrivateAccess = true))
    ECk_FlipbookSource _SourceMode = ECk_FlipbookSource::PaperFlipbook;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flipbook",
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_SourceMode == ECk_FlipbookSource::PaperFlipbook",
                      EditConditionHides))
    TObjectPtr<UPaperFlipbook> _SourceFlipbook;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flipbook",
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_SourceMode == ECk_FlipbookSource::SpriteSheet",
                      EditConditionHides))
    FCk_FlipbookSpriteSheet _SpriteSheet;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flipbook",
              meta = (AllowPrivateAccess = true))
    FVector2D _ImageSize = FVector2D(128.0, 128.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flipbook",
              meta = (AllowPrivateAccess = true))
    FVector2D _RenderScale = FVector2D(1.0, 1.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flipbook",
              meta = (AllowPrivateAccess = true))
    FSlateColor _Tint = FSlateColor(FLinearColor::White);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flipbook",
              meta = (AllowPrivateAccess = true))
    bool _Autoplay = true;

    // Design-time only — deliberately not BlueprintReadWrite; it has no runtime meaning.
    UPROPERTY(EditAnywhere, Category = "Flipbook",
              meta = (EditCondition = "_Autoplay"))
    bool _PreviewInDesigner = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flipbook",
              meta = (AllowPrivateAccess = true))
    bool _Loop = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flipbook",
              meta = (AllowPrivateAccess = true, ClampMin = "0.01"))
    float _PlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flipbook",
              meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int32 _StartKeyFrame = 0;

private:
    auto InitBrushes() -> void;
    auto InitBrushesFromFlipbook() -> void;
    auto InitBrushesFromSpriteSheet() -> void;
    auto ApplyAppearance() -> void;
    auto SetBrushAtFrame(int32 InFrame) -> void;
    auto Get_FrameInterval() const -> FCk_Time;

    UFUNCTION()
    void
    HandleFlipbookTick();

    // Design-time preview driver: the designer preview world does not tick FTimerManager, so this
    // runs off a Slate active timer on MyImage instead of the world timer PlayFlipbookAnimation uses.
    auto ArmDesignPreview() -> void;
    auto DisarmDesignPreview() -> void;

    EActiveTimerReturnType
    HandleDesignPreviewTick(
        double InCurrentTime,
        float  InDeltaTime);

private:
    UPROPERTY(Transient)
    TArray<FSlateBrush> _Brushes;

    FTimerHandle _TickTimer;
    TWeakPtr<FActiveTimerHandle> _DesignPreviewTimer;
    int32 _FrameIndex = 0;
    int32 _FramesTotal = 0;
    bool _IsPlaying = false;
};

// --------------------------------------------------------------------------------------------------------------------
