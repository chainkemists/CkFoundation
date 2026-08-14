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
// -CkDeferredCmdsFile=<path> bypasses FCommandLine's fixed 16,383-char buffer, which a long
// -ExecCmds="Automation RunTests <list>" overruns and kills the editor with. Each line queues into
// GEngine->DeferredCommands AFTER any -ExecCmds, so don't mix the two for order-sensitive sets.
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

        CK_ENSURE_IF_NOT(ck::IsValid(GEngine),
            TEXT("-CkDeferredCmdsFile was passed but GEngine is not available at OnFEngineLoopInitComplete"))
        { return; }

        // The load must live OUTSIDE the ensure condition — CK_DISABLE_ENSURE_CHECKS compiles the
        // whole condition out, which would silently queue zero commands.
        auto Lines = TArray<FString>{};
        const auto FileLoaded = FFileHelper::LoadFileToStringArray(Lines, *FilePath);
        CK_ENSURE_IF_NOT(FileLoaded,
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

namespace ck_deferred_cmds_file_internal
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
