#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/Engine.h>
#include <Misc/CommandLine.h>
#include <Misc/CoreDelegates.h>
#include <Misc/FileHelper.h>
#include <Misc/Parse.h>

// --------------------------------------------------------------------------------------------------------------------
// -CkDeferredCmdsFile=<path> — queue console commands from a file at boot.
//
// FCommandLine stores the process command line in a fixed 16,383-char buffer (CommandLine.h
// MaxCommandLineSize), so a long `-ExecCmds="Automation RunTests <list>"` kills the editor before
// it runs anything. This hook is the bypass: a launcher (UnrealToolbox --test) writes the command
// line(s) to a file and passes only the short flag. Each non-empty line queues into
// GEngine->DeferredCommands — the same sink -ExecCmds feeds (ParseExecCommands::
// QueueDeferredCommands) — and is consumed by the engine tick's exec pump. Lines here queue AFTER
// any -ExecCmds commands (UEngine::Init runs before OnFEngineLoopInitComplete), so don't mix the
// two for order-sensitive command sets.
// --------------------------------------------------------------------------------------------------------------------

namespace ck_deferred_cmds_file
{
    auto
        QueueDeferredCmdsFromFile()
        -> void
    {
        auto FilePath = FString{};
        if (NOT FParse::Value(FCommandLine::Get(), TEXT("CkDeferredCmdsFile="), FilePath))
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(GEngine, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("-CkDeferredCmdsFile was passed but GEngine is not available at OnFEngineLoopInitComplete"))
        { return; }

        auto Lines = TArray<FString>{};
        CK_ENSURE_IF_NOT(FFileHelper::LoadFileToStringArray(Lines, *FilePath),
            TEXT("-CkDeferredCmdsFile was passed but the file [{}] could not be read"), FilePath)
        { return; }

        auto QueuedCount = int32{0};
        for (const auto& Line : Lines)
        {
            const auto& Trimmed = Line.TrimStartAndEnd();
            if (Trimmed.IsEmpty())
            { continue; }

            GEngine->DeferredCommands.Add(Trimmed);
            ++QueuedCount;
        }

        ck::core::Display(TEXT("[DeferredCmdsFile] Queued {} deferred console command(s) from [{}]"),
            QueuedCount, FilePath);
    }
}

namespace
{
    struct FDeferredCmdsFileRegistrar
    {
        FDeferredCmdsFileRegistrar()
        {
            FCoreDelegates::OnFEngineLoopInitComplete.AddStatic(
                &ck_deferred_cmds_file::QueueDeferredCmdsFromFile);
        }
    };

    static FDeferredCmdsFileRegistrar GDeferredCmdsFileRegistrar;
}
