#include "CkPixelArtRender/CkPixelArtRender_Log.h"
#include "CkPixelArtRender/CkPixelArtRender_State.h"

#include "Camera/PlayerCameraManager.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "RHI.h"

// --------------------------------------------------------------------------------------------------------------------

// `ck.PixelArt.Debug.ToggleLoop <Cycles>` — self-verifying zero-residue check.
//
// The renderer's disable path is a lease: a frame that ends without the renderer asking to keep running puts
// r.ScreenPercentage back. Proving that leaves nothing behind needs FRAMES between the toggles, which no batch of
// console commands can express — they all drain in a single pump. Hence a ticker: one toggle per frame, then a
// verdict comparing r.ScreenPercentage against the value captured before the first enable.
namespace ck_pixel_art_debug
{
    struct FToggleLoopState
    {
        float BaselineScreenPercentage = 0.0f;
        int32 RemainingToggles = 0;
        int32 SettleTicks = 0;
        bool IsRunning = false;
        FTSTicker::FDelegateHandle TickerHandle;
    };

    FToggleLoopState GToggleLoop = {};

    auto
        Get_CVar(
            const TCHAR* InName)
        -> IConsoleVariable*
    {
        return IConsoleManager::Get().FindConsoleVariable(InName);
    }

    auto
        DoFinish_ToggleLoop()
        -> void
    {
        auto* EnabledCVar = Get_CVar(TEXT("ck.PixelArt.Enabled"));
        auto* ScreenPercentageCVar = Get_CVar(TEXT("r.ScreenPercentage"));

        // Back to "the configuration decides" so the loop leaves no override of its own behind either.
        if (EnabledCVar != nullptr)
        { EnabledCVar->Set(-1, ECVF_SetByConsole); }

        const auto Final = ScreenPercentageCVar != nullptr ? ScreenPercentageCVar->GetFloat() : 0.0f;
        const auto IsClean = FMath::IsNearlyEqual(Final, GToggleLoop.BaselineScreenPercentage, 1e-3f);

        if (IsClean)
        {
            UE_LOG(LogCkPixelArt, Display,
                TEXT("ToggleLoop PASS: r.ScreenPercentage returned to its pre-enable value %.4f — no residue."),
                GToggleLoop.BaselineScreenPercentage);
        }
        else
        {
            UE_LOG(LogCkPixelArt, Error,
                TEXT("ToggleLoop FAIL: r.ScreenPercentage is %.4f but was %.4f before the first enable — the ")
                TEXT("renderer left residue behind."),
                Final, GToggleLoop.BaselineScreenPercentage);
        }

        FTSTicker::GetCoreTicker().RemoveTicker(GToggleLoop.TickerHandle);
        GToggleLoop = {};
    }

    auto
        DoTick_ToggleLoop(
            float InDeltaSeconds)
        -> bool
    {
        if (GToggleLoop.RemainingToggles > 0)
        {
            auto* EnabledCVar = Get_CVar(TEXT("ck.PixelArt.Enabled"));

            // Counting DOWN, so even means enable and odd means disable — that ordering is what makes the last
            // step of the loop a disable. Ending on an enable would leave the renderer legitimately driving and
            // the verdict below would read its value as residue.
            const auto Enable = GToggleLoop.RemainingToggles % 2 == 0;

            if (EnabledCVar != nullptr)
            { EnabledCVar->Set(Enable ? 1 : 0, ECVF_SetByConsole); }

            --GToggleLoop.RemainingToggles;

            return true;
        }

        // The final disable only releases on the NEXT frame's end, so let a couple of frames pass before judging.
        if (GToggleLoop.SettleTicks > 0)
        {
            --GToggleLoop.SettleTicks;
            return true;
        }

        DoFinish_ToggleLoop();

        return false;
    }

    auto
        DoStart_ToggleLoop(
            const TArray<FString>& InArgs)
        -> void
    {
        if (GToggleLoop.IsRunning)
        {
            UE_LOG(LogCkPixelArt, Warning, TEXT("ToggleLoop is already running."));
            return;
        }

        const auto Cycles = InArgs.Num() > 0 ? FCString::Atoi(*InArgs[0]) : 100;

        if (Cycles <= 0)
        {
            UE_LOG(LogCkPixelArt, Warning, TEXT("ToggleLoop needs a positive cycle count (got [%d])."), Cycles);
            return;
        }

        auto* ScreenPercentageCVar = Get_CVar(TEXT("r.ScreenPercentage"));

        if (ScreenPercentageCVar == nullptr)
        {
            UE_LOG(LogCkPixelArt, Error, TEXT("r.ScreenPercentage does not exist — nothing to check residue against."));
            return;
        }

        GToggleLoop.BaselineScreenPercentage = ScreenPercentageCVar->GetFloat();
        GToggleLoop.RemainingToggles = Cycles * 2;
        GToggleLoop.SettleTicks = 2;
        GToggleLoop.IsRunning = true;
        GToggleLoop.TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateStatic(&DoTick_ToggleLoop));

        UE_LOG(LogCkPixelArt, Display,
            TEXT("ToggleLoop starting: %d enable/disable cycles, one toggle per frame. ")
            TEXT("r.ScreenPercentage before the first enable is %.4f."),
            Cycles, GToggleLoop.BaselineScreenPercentage);
    }

    FAutoConsoleCommand CCmd_ToggleLoop(
        TEXT("ck.PixelArt.Debug.ToggleLoop"),
        TEXT("Enable/disable the pixel-art renderer N times (default 100), one toggle per frame, then report ")
        TEXT("whether r.ScreenPercentage came back to its pre-enable value. Run it with the renderer OFF."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&DoStart_ToggleLoop));
}

// --------------------------------------------------------------------------------------------------------------------

// `Ck_PixelArt_DebugPan <TexelsPerFrame>` — the A/B rig for the whole snap technique.
//
// Judging pixel creep needs motion slow enough that a texel takes several frames to cross, which no human can
// produce on a gamepad and no scripted camera in the test level provides. Drifting the view target diagonally by
// a fraction of a texel per frame makes the three states legible in seconds:
//
//   ck.PixelArt.Snap 0                 -> edges crawl
//   ck.PixelArt.Debug.SnapOnly 1       -> whole-texel stepping
//   both off (the default)             -> smooth and grid-aligned
//
// Diagonal on purpose: an axis-aligned pan exercises one component of the remainder and would let a swapped or
// half-applied compensation pass unnoticed.
namespace ck_pixel_art_debug_pan
{
    struct FPanState
    {
        TWeakObjectPtr<UWorld> World;
        float TexelsPerFrame = 0.2f;
        bool IsRunning = false;
        FTSTicker::FDelegateHandle TickerHandle;
    };

    FPanState GPan = {};

    auto
        DoStop_Pan()
        -> void
    {
        if (!GPan.IsRunning)
        { return; }

        FTSTicker::GetCoreTicker().RemoveTicker(GPan.TickerHandle);
        GPan = {};

        UE_LOG(LogCkPixelArt, Display, TEXT("DebugPan stopped."));
    }

    auto
        DoTick_Pan(
            float InDeltaSeconds)
        -> bool
    {
        auto* World = GPan.World.Get();

        if (World == nullptr)
        {
            DoStop_Pan();
            return false;
        }

        auto* PlayerController = World->GetFirstPlayerController();

        if (PlayerController == nullptr || PlayerController->PlayerCameraManager == nullptr)
        {
            UE_LOG(LogCkPixelArt, Warning, TEXT("DebugPan: no player controller with a camera manager. Stopping."));
            DoStop_Pan();
            return false;
        }

        auto* ViewTarget = PlayerController->GetViewTarget();

        if (ViewTarget == nullptr)
        {
            UE_LOG(LogCkPixelArt, Warning, TEXT("DebugPan: the player controller has no view target. Stopping."));
            DoStop_Pan();
            return false;
        }

        // A texel is only defined while the renderer is actually running an orthographic view. Without a report
        // there is no texel to pan by, so say so instead of inventing a world-space speed that would make the
        // capture meaningless.
        const auto Report = FCk_PixelArtRender_StateRegistry::TryGet_FrameReport(World);
        const auto TexelWorldSize = Report.IsSet() ? Report->TexelWorldSize : 0.0;

        if (TexelWorldSize <= 0.0)
        {
            UE_LOG(LogCkPixelArt, Warning,
                TEXT("DebugPan: the renderer published no texel size this frame (is it enabled, with an ")
                TEXT("orthographic camera?). Stopping rather than panning by an arbitrary distance."));
            DoStop_Pan();
            return false;
        }

        const auto CameraRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
        const auto CameraMatrix = FRotationMatrix{CameraRotation};

        const auto Direction =
            (CameraMatrix.GetScaledAxis(EAxis::Y) + CameraMatrix.GetScaledAxis(EAxis::Z)).GetSafeNormal();

        ViewTarget->SetActorLocation(
            ViewTarget->GetActorLocation() + Direction * (GPan.TexelsPerFrame * TexelWorldSize));

        return true;
    }

    auto
        DoStart_Pan(
            const TArray<FString>& InArgs,
            UWorld* InWorld)
        -> void
    {
        const auto TexelsPerFrame = InArgs.Num() > 0 ? FCString::Atof(*InArgs[0]) : 0.2f;

        if (GPan.IsRunning)
        {
            DoStop_Pan();

            // Re-issuing with a speed restarts at that speed; re-issuing bare is the off switch, so the command
            // is its own toggle and there is nothing extra to remember mid-capture.
            if (InArgs.Num() == 0)
            { return; }
        }

        if (InWorld == nullptr)
        {
            UE_LOG(LogCkPixelArt, Warning, TEXT("DebugPan: no world. Run this from a running game."));
            return;
        }

        if (FMath::IsNearlyZero(TexelsPerFrame))
        { return; }

        GPan.World = InWorld;
        GPan.TexelsPerFrame = TexelsPerFrame;
        GPan.IsRunning = true;
        GPan.TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&DoTick_Pan));

        UE_LOG(LogCkPixelArt, Display,
            TEXT("DebugPan drifting the view target diagonally at %.3f texels/frame. Run the command again with ")
            TEXT("no argument to stop."),
            TexelsPerFrame);
    }

    FAutoConsoleCommandWithWorldAndArgs CCmd_DebugPan(
        TEXT("Ck_PixelArt_DebugPan"),
        TEXT("Drift the view target diagonally by <TexelsPerFrame> (default 0.2) so pixel creep, whole-texel ")
        TEXT("stepping and smooth compensated motion can be told apart by eye. Run again with no argument to stop."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DoStart_Pan));
}

// --------------------------------------------------------------------------------------------------------------------

// `ck.PixelArt.Debug.PerfSweep [SecondsPerStage]` - the renderer's own perf measurement.
//
// The claim worth checking is that the renderer is CHEAPER than native, because the scene rasterizes into
// a small fraction of the pixels and the upscale is one fullscreen pass. A claim like that is worthless
// without numbers from the machine and scene it is claimed on, and reading `stat gpu` off a screenshot is
// not a number - so this walks the three configurations itself and prints GPU milliseconds for each.
//
// Each stage discards its first frames: switching resolution reallocates render targets and recompiles
// nothing useful into the first few frames, and averaging those in would flatter or punish whichever stage
// happened to go first.
namespace ck_pixel_art_perf
{
    struct FStage
    {
        const TCHAR* Name;
        int32 Enabled;
        int32 InternalHeight;
    };

    const FStage GStages[] = {
        {TEXT("OFF (native)"),   0, -1},
        {TEXT("ON @ 360"),       1, 360},
        {TEXT("ON @ 180"),       1, 180}};

    constexpr int32 StageCount = UE_ARRAY_COUNT(GStages);

    // Long enough that a stray hitch cannot dominate a stage, short enough that the whole sweep is one
    // sitting. Both are in frames rather than seconds so the settle is independent of frame rate.
    constexpr int32 SettleFrames = 30;

    struct FSweepState
    {
        int32 StageIndex = 0;
        int32 FramesRemaining = 0;
        int32 SettleRemaining = 0;
        int32 SampleCount = 0;
        double AccumulatedGpuMs = 0.0;
        double StageGpuMs[StageCount] = {};
        int32 FramesPerStage = 0;
        bool IsRunning = false;
        FTSTicker::FDelegateHandle TickerHandle;
    };

    FSweepState GSweep = {};

    auto
        DoSet_CVar(
            const TCHAR* InName,
            int32 InValue)
        -> void
    {
        if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(InName))
        { CVar->Set(InValue, ECVF_SetByConsole); }
    }

    auto
        DoBegin_Stage(
            int32 InIndex)
        -> void
    {
        const auto& Stage = GStages[InIndex];

        DoSet_CVar(TEXT("ck.PixelArt.InternalHeight"), Stage.InternalHeight);
        DoSet_CVar(TEXT("ck.PixelArt.Enabled"), Stage.Enabled);

        GSweep.StageIndex = InIndex;
        GSweep.FramesRemaining = GSweep.FramesPerStage;
        GSweep.SettleRemaining = SettleFrames;
        GSweep.SampleCount = 0;
        GSweep.AccumulatedGpuMs = 0.0;

        UE_LOG(LogCkPixelArt, Display, TEXT("PerfSweep stage [%s] starting"), Stage.Name);
    }

    auto
        DoFinish_Sweep()
        -> void
    {
        // Back to configuration-driven, so the sweep leaves no override of its own behind.
        DoSet_CVar(TEXT("ck.PixelArt.Enabled"), -1);
        DoSet_CVar(TEXT("ck.PixelArt.InternalHeight"), -1);

        UE_LOG(LogCkPixelArt, Display, TEXT("PerfSweep results (GPU milliseconds per frame, mean):"));

        for (auto Index = 0; Index < StageCount; ++Index)
        {
            UE_LOG(LogCkPixelArt, Display, TEXT("  %-14s %.3f ms"), GStages[Index].Name, GSweep.StageGpuMs[Index]);
        }

        const auto Native = GSweep.StageGpuMs[0];

        if (Native > 0.0)
        {
            UE_LOG(LogCkPixelArt, Display,
                TEXT("  ON@360 is %.1f%% of native; ON@180 is %.1f%% of native"),
                100.0 * GSweep.StageGpuMs[1] / Native,
                100.0 * GSweep.StageGpuMs[2] / Native);
        }

        FTSTicker::GetCoreTicker().RemoveTicker(GSweep.TickerHandle);
        GSweep = {};
    }

    auto
        DoTick_Sweep(
            float InDeltaSeconds)
        -> bool
    {
        // RHIGetGPUFrameCycles rather than the deprecated GGPUFrameTime global. Zero means the platform
        // reported nothing this frame - counting it would silently drag every mean towards zero, which is
        // the direction that would make the renderer look good.
        const auto GpuCycles = RHIGetGPUFrameCycles();

        if (GSweep.SettleRemaining > 0)
        {
            --GSweep.SettleRemaining;
            return true;
        }

        if (GpuCycles > 0)
        {
            GSweep.AccumulatedGpuMs += FPlatformTime::ToMilliseconds(GpuCycles);
            ++GSweep.SampleCount;
        }

        --GSweep.FramesRemaining;

        if (GSweep.FramesRemaining > 0)
        { return true; }

        GSweep.StageGpuMs[GSweep.StageIndex] =
            GSweep.SampleCount > 0 ? GSweep.AccumulatedGpuMs / GSweep.SampleCount : 0.0;

        if (GSweep.StageIndex + 1 < StageCount)
        {
            DoBegin_Stage(GSweep.StageIndex + 1);
            return true;
        }

        DoFinish_Sweep();

        return false;
    }

    auto
        DoStart_Sweep(
            const TArray<FString>& InArgs)
        -> void
    {
        if (GSweep.IsRunning)
        {
            UE_LOG(LogCkPixelArt, Warning, TEXT("PerfSweep is already running."));
            return;
        }

        const auto Seconds = InArgs.Num() > 0 ? FCString::Atof(*InArgs[0]) : 5.0f;

        if (Seconds <= 0.0f)
        {
            UE_LOG(LogCkPixelArt, Warning, TEXT("PerfSweep needs a positive duration (got [%f])."), Seconds);
            return;
        }

        // Frames, not seconds: the ticker is the clock, and a stage measured in frames samples the same
        // number of times whatever the frame rate turns out to be.
        GSweep.FramesPerStage = FMath::Max(30, FMath::RoundToInt32(Seconds * 60.0f));
        GSweep.IsRunning = true;
        GSweep.TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&DoTick_Sweep));

        UE_LOG(LogCkPixelArt, Display,
            TEXT("PerfSweep starting: %d stages, %d frames each (%d discarded per stage while it settles)."),
            StageCount, GSweep.FramesPerStage, SettleFrames);

        DoBegin_Stage(0);
    }

    FAutoConsoleCommand CCmd_PerfSweep(
        TEXT("ck.PixelArt.Debug.PerfSweep"),
        TEXT("Measure mean GPU frame time with the renderer off, at 360 internal texels, and at 180 - ")
        TEXT("<SecondsPerStage> each (default 5) - then print the three numbers. Run it with a steady ")
        TEXT("camera; it restores the CVars it moved."),
        FConsoleCommandWithArgsDelegate::CreateStatic(&DoStart_Sweep));
}
