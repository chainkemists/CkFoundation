#include "CkPixelArtRender/CkPixelArtRender_Log.h"
#include "CkPixelArtRender/CkPixelArtRender_State.h"

#include "Camera/PlayerCameraManager.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

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

