#include "CkAngelscriptGenerator_DriftCommandlet.h"

#include "CkAngelscriptGenerator/AutoTests/CkAutoTestWrapperGenerator.h"
#include "CkAngelscriptGenerator/CkAngelscriptEntityScriptParamsGenerator.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_RegenOwnership.h"
#include "CkAngelscriptGenerator/DynamicHandles/CkDynamicHandleSubsystem.h"

#include "Editor.h"
#include "HAL/PlatformTime.h"

// --------------------------------------------------------------------------------------------------------------------

UCkAngelscriptGenerator_DriftCommandlet::UCkAngelscriptGenerator_DriftCommandlet()
{
    // Commandlets that mutate files based on UClass reflection need an
    // editor context — the editor needs to be capable of registering
    // every UClass that contributes to the generated output. Setting
    // IsEditor=true makes UCommandletHelpers spin up GEditor for us.
    IsClient       = false;
    IsServer       = false;
    IsEditor       = true;
    LogToConsole   = true;
    ShowErrorCount = true;
}

// --------------------------------------------------------------------------------------------------------------------

int32 UCkAngelscriptGenerator_DriftCommandlet::Main(const FString& /*InParams*/)
{
    const auto StartSeconds = FPlatformTime::Seconds();

    ck::angelscriptgenerator::Log(
        TEXT("[DriftCommandlet] === AS generator drift check starting ==="));

    // Single-writer gate (G11): the commandlet's whole purpose is to REGENERATE the canonical
    // files and let the post-step `git diff` judge drift. Running as a silent SECONDARY would
    // skip every generator write at G3/G4/G7 → the files never get rewritten → `git diff` finds
    // no change → a FALSE-CLEAN CI verdict. Acquiring up front (it normally runs alone in CI)
    // and failing loudly when it can't is the only safe behavior: a failure to RUN the check is
    // a different, must-be-loud failure class than the drift verdict itself (which still owns the
    // exit-0-with-git-diff contract below).
    if (NOT FCkAngelscriptGenerator_RegenOwnership::Try_AcquireOrGet_IsOwner(TEXT("DriftCommandlet.Main")))
    {
        ck::angelscriptgenerator::Error(
            TEXT("[DriftCommandlet] Another editor/commandlet instance of this project owns ")
            TEXT("Script/Generated regen (lock: [{}]). The drift check cannot regenerate the ")
            TEXT("canonical files and would report a FALSE-CLEAN result. Close the other instance ")
            TEXT("or run the drift check on a clean agent."),
            FCkAngelscriptGenerator_RegenOwnership::Get_LockFilePath());
        return 1;
    }

    // EntitySpawnParams + AutoTestActors generators — reflection-only,
    // synchronous. After this returns, both files on disk match the
    // current reflection state byte-for-byte (the generators short-
    // circuit when content is already identical, so this is a no-op
    // on a clean worktree).
    FCkAngelscriptEntityScriptParamsGenerator::GenerateAll();
    FCkAutoTestWrapperGenerator::GenerateAll();

    // DynamicHandleTypes.json — accessed via the editor subsystem
    // (GEditor is initialized because we set IsEditor=true above).
    // Falls back gracefully if the subsystem isn't available.
    if (GEditor != nullptr)
    {
        if (auto* Subsystem = GEditor->GetEditorSubsystem<UCkDynamicHandleSubsystem>();
            Subsystem != nullptr)
        {
            Subsystem->GenerateHandleTypeRegistry();
        }
        else
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[DriftCommandlet] UCkDynamicHandleSubsystem unavailable — ")
                TEXT("DynamicHandleTypes.json drift not checked."));
        }
    }
    else
    {
        ck::angelscriptgenerator::Warning(
            TEXT("[DriftCommandlet] GEditor null despite IsEditor=true — ")
            TEXT("DynamicHandleTypes.json drift not checked."));
    }

    const auto ElapsedSeconds = FPlatformTime::Seconds() - StartSeconds;

    ck::angelscriptgenerator::Log(
        TEXT("[DriftCommandlet] === Done in {:.3f}s. ")
        TEXT("Drift verdict lives in the post-commandlet `git diff` step. ==="),
        ElapsedSeconds);

    // Exit 0 on a successful RUN. The CI script's subsequent `git diff --exit-code`
    // is the gate that fails on actual drift — that way CI logs name the specific
    // files that drifted instead of a generic "commandlet failed." (The only non-zero
    // return is the G11 single-writer refusal above, which is a failure-to-run, not a
    // drift verdict.)
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
