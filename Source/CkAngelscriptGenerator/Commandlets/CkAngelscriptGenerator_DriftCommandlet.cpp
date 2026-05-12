#include "CkAngelscriptGenerator_DriftCommandlet.h"

#include "CkAngelscriptGenerator/AutoTests/CkAutoTestWrapperGenerator.h"
#include "CkAngelscriptGenerator/CkAngelscriptEntityScriptParamsGenerator.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
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

    // Always exit 0. The CI script's subsequent `git diff --exit-code`
    // is the gate that fails on actual drift — that way CI logs name
    // the specific files that drifted instead of a generic "commandlet
    // failed."
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
