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
    // IsEditor=true makes UCommandletHelpers spin up GEditor, without which not every UClass
    // contributing to the generated output is registered.
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

    // This is the one gate that must own or fail loudly: a silent SECONDARY skips every generator
    // write, so nothing is rewritten, `git diff` finds nothing, and CI reports a FALSE CLEAN.
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

    FCkAngelscriptEntityScriptParamsGenerator::GenerateAll();
    FCkAutoTestWrapperGenerator::GenerateAll();

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

    // Exit 0 means the check RAN, not that it was clean — the CI script's following
    // `git diff --exit-code` is the drift gate, so its logs name the files that drifted.
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
