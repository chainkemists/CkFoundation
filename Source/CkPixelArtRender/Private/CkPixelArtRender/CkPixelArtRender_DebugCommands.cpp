#include "CkPixelArtRender/CkPixelArtRender_Log.h"

#include "Containers/Ticker.h"
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
