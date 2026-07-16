#include "CkAngelscriptGenerator_RegenOwnership.h"

#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"

#include <HAL/FileManager.h>
#include <HAL/PlatformFileManager.h>
#include <HAL/PlatformProcess.h>
#include <Misc/CommandLine.h>
#include <Misc/Paths.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_angelscript_generator_regen_ownership
{
    // Session-static ownership state (mirrors the dispatcher's sCyclesRun style).
    // The held handle IS the lock — closing it (or the process dying) releases ownership.
    TUniquePtr<IFileHandle> sLockHandle;
    bool sWarnedSecondaryOnce = false;
    bool sWasSecondary        = false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptGenerator_RegenOwnership::
    Try_AcquireOrGet_IsOwner(
        FStringView InGateSite)
    -> bool
{
    CK_ENSURE_IF_NOT(IsInGameThread(),
        TEXT("RegenOwnership gate [{}] called off the game thread"), FString{InGateSite})
    { return false; }

    if (ck_angelscript_generator_regen_ownership::sLockHandle.IsValid())
    { return true; }

    const auto LockFilePath = Get_LockFilePath();
    ck_angelscript_generator_regen_ownership::sLockHandle = Try_OpenExclusiveWriteHandle(LockFilePath);

    if (ck_angelscript_generator_regen_ownership::sLockHandle.IsValid())
    {
        // Advisory breadcrumb for humans/tools inspecting the lock — never parsed, never
        // subject to the generators' determinism rules (this is not generator output).
        const auto Breadcrumb = FString::Format(TEXT("pid={0}\ncmdline={1}\n"),
            {FPlatformProcess::GetCurrentProcessId(), FCommandLine::Get()});
        const auto Utf8 = FTCHARToUTF8{*Breadcrumb};
        ck_angelscript_generator_regen_ownership::sLockHandle->Write(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
        ck_angelscript_generator_regen_ownership::sLockHandle->Flush();

        if (ck_angelscript_generator_regen_ownership::sWasSecondary)
        {
            ck_angelscript_generator_regen_ownership::sWasSecondary = false;
            ck::angelscriptgenerator::Log(
                TEXT("[RegenOwnership] Ownership ACQUIRED at gate [{}] — the prior owner exited. ")
                TEXT("Generator writes + self-heal file mutations re-enabled in this instance."),
                FString{InGateSite});
        }

        return true;
    }

    ck_angelscript_generator_regen_ownership::sWasSecondary = true;

    if (NOT ck_angelscript_generator_regen_ownership::sWarnedSecondaryOnce)
    {
        ck_angelscript_generator_regen_ownership::sWarnedSecondaryOnce = true;
        ck::angelscriptgenerator::Warning(
            TEXT("[RegenOwnership] Another editor/commandlet instance of this project owns ")
            TEXT("Script/Generated regeneration (lock: [{}]). This instance runs as SECONDARY: ")
            TEXT("generator writes, stub-recovery cleanup, and AS self-heal file mutations are ")
            TEXT("disabled here. Reads and AS compiles are unaffected. Ownership is re-checked at ")
            TEXT("every regen site and transfers automatically when the owner exits. ")
            TEXT("(First refused gate: [{}])"),
            LockFilePath, FString{InGateSite});
    }
    else
    {
        ck::angelscriptgenerator::VeryVerbose(
            TEXT("[RegenOwnership] Gate [{}] skipped — secondary instance."), FString{InGateSite});
    }

    return false;
}

auto
    FCkAngelscriptGenerator_RegenOwnership::
    Release()
    -> void
{
    if (ck_angelscript_generator_regen_ownership::sLockHandle.IsValid())
    {
        ck_angelscript_generator_regen_ownership::sLockHandle.Reset();
        ck::angelscriptgenerator::Log(TEXT("[RegenOwnership] Ownership released."));
    }
}

auto
    FCkAngelscriptGenerator_RegenOwnership::
    Get_StatusString()
    -> FString
{
    if (ck_angelscript_generator_regen_ownership::sLockHandle.IsValid())
    { return TEXT("Owner"); }

    return ck_angelscript_generator_regen_ownership::sWasSecondary
        ? TEXT("Secondary (lock held by another process)")
        : TEXT("Undetermined (no acquisition attempted yet)");
}

auto
    FCkAngelscriptGenerator_RegenOwnership::
    Get_LockFilePath()
    -> FString
{
    return FPaths::ProjectSavedDir() / TEXT("CkAngelscriptGenerator_RegenOwner.lock");
}

auto
    FCkAngelscriptGenerator_RegenOwnership::
    Try_OpenExclusiveWriteHandle(
        const FString& InLockFilePath)
    -> TUniquePtr<IFileHandle>
{
    // Defensive: Saved/ exists by module-load time in practice, but commandlet boot orders vary.
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(InLockFilePath), /*Tree=*/true);

    // bAllowRead=true → GENERIC_WRITE + FILE_SHARE_READ on Windows: readers (humans, tools) are
    // fine, but a second writer — even in the same process, which is what makes the unit test a
    // valid proxy — fails with a sharing violation and returns null.
    constexpr auto Append    = false;
    constexpr auto AllowRead = true;
    return TUniquePtr<IFileHandle>{
        FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*InLockFilePath, Append, AllowRead)};
}

// --------------------------------------------------------------------------------------------------------------------
