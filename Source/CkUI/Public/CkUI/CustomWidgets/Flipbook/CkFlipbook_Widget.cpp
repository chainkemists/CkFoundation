#include "CkFlipbook_Widget.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Time/CkTime_Utils.h"
#include "CkUICore/Types/CkUI_Types.h"

#include <Engine/World.h>
#include <TimerManager.h>

#include <Engine/Texture2D.h>
#include <PaperFlipbook.h>
#include <PaperSprite.h>

#include <Widgets/SWidget.h>
#include <Widgets/Images/SImage.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_FlipbookWidget_UE::
    SynchronizeProperties()
    -> void
{
    Super::SynchronizeProperties();

    InitBrushes();
    ApplyAppearance();

    _FrameIndex = _StartKeyFrame;
    SetBrushAtFrame(_StartKeyFrame);

    // The designer preview world does not tick FTimerManager, hence the separate Slate active timer.
    if (NOT IsDesignTime())
    {
        if (_Autoplay)
        { PlayFlipbookAnimation(_StartKeyFrame); }
    }
    else
    {
        DisarmDesignPreview();
        if (_Autoplay && _PreviewInDesigner)
        { ArmDesignPreview(); }
    }
}

auto
    UCk_FlipbookWidget_UE::
    ReleaseSlateResources(
        bool bReleaseChildren)
    -> void
{
    DisarmDesignPreview();

    if (const auto World = GetWorld();
        ck::IsValid(World))
    {
        World->GetTimerManager().ClearTimer(_TickTimer);
    }

    Super::ReleaseSlateResources(bReleaseChildren);
}

#if WITH_EDITOR
auto
    UCk_FlipbookWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_FlipbookWidget_UE::
    InitBrushes()
    -> void
{
    _Brushes.Reset();
    _FramesTotal = 0;

    switch (_SourceMode)
    {
        case ECk_FlipbookSource::PaperFlipbook: { InitBrushesFromFlipbook();    break; }
        case ECk_FlipbookSource::SpriteSheet:   { InitBrushesFromSpriteSheet(); break; }
    }

    _FramesTotal = _Brushes.Num();
}

auto
    UCk_FlipbookWidget_UE::
    InitBrushesFromFlipbook()
    -> void
{
    if (ck::Is_NOT_Valid(_SourceFlipbook))
    { return; }

    const auto NumFrames = _SourceFlipbook->GetNumKeyFrames();

    // Hand-rolled because UPaperSpriteBlueprintLibrary::MakeBrushFromSprite is not DLL-exported.
    for (auto Frame = 0; Frame < NumFrames; ++Frame)
    {
        const auto Sprite = _SourceFlipbook->GetSpriteAtFrame(Frame);
        if (ck::Is_NOT_Valid(Sprite))
        { continue; }

        auto FrameBrush = FSlateBrush{};
        FrameBrush.SetResourceObject(Sprite);
        FrameBrush.ImageSize = _ImageSize;
        FrameBrush.TintColor = _Tint;
        _Brushes.Add(FrameBrush);
    }
}

auto
    UCk_FlipbookWidget_UE::
    InitBrushesFromSpriteSheet()
    -> void
{
    const auto Texture = _SpriteSheet.Get_Texture();
    if (ck::Is_NOT_Valid(Texture))
    { return; }

    const auto Rows    = FMath::Max(1, _SpriteSheet.Get_Rows());
    const auto Columns = FMath::Max(1, _SpriteSheet.Get_Columns());

    const auto CellU = 1.0f / static_cast<float>(Columns);
    const auto CellV = 1.0f / static_cast<float>(Rows);

    for (auto Row = 0; Row < Rows; ++Row)
    {
        for (auto Col = 0; Col < Columns; ++Col)
        {
            const auto MinU = static_cast<float>(Col) * CellU;
            const auto MinV = static_cast<float>(Row) * CellV;

            auto FrameBrush = FSlateBrush{};
            FrameBrush.SetResourceObject(Texture);
            FrameBrush.ImageSize = _ImageSize;
            FrameBrush.TintColor = _Tint;
            FrameBrush.SetUVRegion(FBox2f{FVector2f{MinU, MinV}, FVector2f{MinU + CellU, MinV + CellV}});
            _Brushes.Add(FrameBrush);
        }
    }
}

auto
    UCk_FlipbookWidget_UE::
    ApplyAppearance()
    -> void
{
    SetDesiredSizeOverride(_ImageSize);
    SetRenderScale(_RenderScale);
}

auto
    UCk_FlipbookWidget_UE::
    SetBrushAtFrame(
        int32 InFrame)
    -> void
{
    if (NOT _Brushes.IsValidIndex(InFrame))
    { return; }

    SetBrush(_Brushes[InFrame]);
}

auto
    UCk_FlipbookWidget_UE::
    Get_FrameInterval() const
    -> FCk_Time
{
    const auto PlayRateScale = static_cast<double>(FMath::Max(0.01f, _PlayRate));

    if (_SourceMode == ECk_FlipbookSource::SpriteSheet)
    {
        const auto NumFrames     = FMath::Max(1, _FramesTotal);
        const auto TotalSeconds  = _SpriteSheet.Get_Duration().Get_Seconds();
        const auto SecondsPerCell = (TotalSeconds > 0.0 ? TotalSeconds : 1.0) / static_cast<double>(NumFrames);
        return UCk_Utils_Time_UE::Make_FromSeconds(SecondsPerCell / PlayRateScale);
    }

    if (ck::Is_NOT_Valid(_SourceFlipbook))
    { return UCk_Utils_Time_UE::Make_FromSeconds(0.1); }

    const auto Fps = _SourceFlipbook->GetFramesPerSecond();
    const auto SecondsPerFrame = Fps > 0.0f ? 1.0 / static_cast<double>(Fps) : 0.1;
    return UCk_Utils_Time_UE::Make_FromSeconds(SecondsPerFrame / PlayRateScale);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_FlipbookWidget_UE::
    PlayFlipbookAnimation(
        int32 InStartFrame)
    -> void
{
    if (_Brushes.IsEmpty())
    { InitBrushes(); }

    CK_ENSURE_IF_NOT(NOT _Brushes.IsEmpty(),
        TEXT("Flipbook has no frames to play — is SourceFlipbook set? {}"), ck::Context(this))
    { return; }

    const auto World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return; }

    _FrameIndex = _Brushes.IsValidIndex(InStartFrame) ? InStartFrame : 0;
    SetBrushAtFrame(_FrameIndex);

    constexpr auto Loop = true;
    World->GetTimerManager().SetTimer(
        _TickTimer, this, &UCk_FlipbookWidget_UE::HandleFlipbookTick, Get_FrameInterval().Get_Seconds(), Loop);

    _IsPlaying = true;
}

auto
    UCk_FlipbookWidget_UE::
    PauseFlipbookAnimation()
    -> void
{
    if (const auto World = GetWorld();
        ck::IsValid(World))
    {
        World->GetTimerManager().PauseTimer(_TickTimer);
    }
    _IsPlaying = false;
}

auto
    UCk_FlipbookWidget_UE::
    StopFlipbookAnimation()
    -> void
{
    if (const auto World = GetWorld();
        ck::IsValid(World))
    {
        World->GetTimerManager().ClearTimer(_TickTimer);
    }

    _IsPlaying = false;
    _FrameIndex = _StartKeyFrame;
    SetBrushAtFrame(_StartKeyFrame);
}

auto
    UCk_FlipbookWidget_UE::
    RestartWithUpdatedParameters()
    -> void
{
    StopFlipbookAnimation();
    InitBrushes();
    ApplyAppearance();
    SetBrushAtFrame(_StartKeyFrame);

    if (_Autoplay)
    { PlayFlipbookAnimation(_StartKeyFrame); }
}

auto
    UCk_FlipbookWidget_UE::
    Request_SetSourceFlipbook(
        UPaperFlipbook* InSourceFlipbook)
    -> void
{
    _SourceMode     = ECk_FlipbookSource::PaperFlipbook;
    _SourceFlipbook = InSourceFlipbook;
    RestartWithUpdatedParameters();
}

auto
    UCk_FlipbookWidget_UE::
    Request_SetSpriteSheet(
        const FCk_FlipbookSpriteSheet& InSpriteSheet)
    -> void
{
    _SourceMode  = ECk_FlipbookSource::SpriteSheet;
    _SpriteSheet = InSpriteSheet;
    RestartWithUpdatedParameters();
}

auto
    UCk_FlipbookWidget_UE::
    Request_SetLooping(
        bool InLoop)
    -> void
{
    _Loop = InLoop;
}

auto
    UCk_FlipbookWidget_UE::
    Request_SetAutoplay(
        bool InAutoplay)
    -> void
{
    _Autoplay = InAutoplay;
}

auto
    UCk_FlipbookWidget_UE::
    HandleFlipbookTick()
    -> void
{
    SetBrushAtFrame(_FrameIndex);

    const auto IsLastFrame = _FrameIndex + 1 >= _FramesTotal;
    if (NOT IsLastFrame)
    {
        ++_FrameIndex;
        return;
    }

    if (_Loop)
    {
        _FrameIndex = 0;
        OnLoop.Broadcast();
        return;
    }

    if (const auto World = GetWorld();
        ck::IsValid(World))
    {
        World->GetTimerManager().ClearTimer(_TickTimer);
    }
    _IsPlaying = false;
    OnFinished.Broadcast();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_FlipbookWidget_UE::
    ArmDesignPreview()
    -> void
{
    if (NOT MyImage.IsValid() || _Brushes.IsEmpty())
    { return; }

    _FrameIndex = _StartKeyFrame;
    SetBrushAtFrame(_FrameIndex);

    _DesignPreviewTimer = MyImage->RegisterActiveTimer(
        static_cast<float>(Get_FrameInterval().Get_Seconds()),
        FWidgetActiveTimerDelegate::CreateUObject(this, &UCk_FlipbookWidget_UE::HandleDesignPreviewTick));
}

auto
    UCk_FlipbookWidget_UE::
    DisarmDesignPreview()
    -> void
{
    if (const auto Handle = _DesignPreviewTimer.Pin();
        Handle.IsValid() && MyImage.IsValid())
    {
        MyImage->UnRegisterActiveTimer(Handle.ToSharedRef());
    }
    _DesignPreviewTimer.Reset();
}

auto
    UCk_FlipbookWidget_UE::
    HandleDesignPreviewTick(
        double InCurrentTime,
        float  InDeltaTime)
    -> EActiveTimerReturnType
{
    SetBrushAtFrame(_FrameIndex);

    const auto IsLastFrame = _FrameIndex + 1 >= _FramesTotal;
    if (NOT IsLastFrame)
    {
        ++_FrameIndex;
        return EActiveTimerReturnType::Continue;
    }

    if (_Loop)
    {
        _FrameIndex = 0;
        return EActiveTimerReturnType::Continue;
    }

    return EActiveTimerReturnType::Stop;
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_FlipbookWidget_UE::Get_IsPlaying() const -> bool { return _IsPlaying; }
auto UCk_FlipbookWidget_UE::Get_CurrentFrame() const -> int32 { return _FrameIndex; }
auto UCk_FlipbookWidget_UE::Get_TotalFrames() const -> int32 { return _FramesTotal; }

// --------------------------------------------------------------------------------------------------------------------
