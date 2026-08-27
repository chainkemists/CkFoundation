// Same-process double-OpenWrite is a valid proxy for the two-process case: Windows sharing-mode
// conflicts are enforced PER HANDLE, not per process. Every case runs against a temp path under
// ProjectIntermediateDir(), never the real Saved/ lock — a live editor running this suite must
// not lose (or grant away) its own ownership.

#include "CkAngelscriptGenerator/CkAngelscriptGenerator_RegenOwnership.h"

#include "CkCore/Macros/CkMacros.h"

#include <HAL/FileManager.h>
#include <HAL/PlatformFileManager.h>
#include <Misc/AutomationTest.h>
#include <Misc/Paths.h>

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
    auto Get_TestLockDir() -> FString
    {
        return FPaths::ProjectIntermediateDir() / TEXT("CkRegenOwnershipTest");
    }

    auto Get_TestLockPath() -> FString
    {
        return Get_TestLockDir() / TEXT("probe.lock");
    }

    auto Cleanup_TestLockDir() -> void
    {
        constexpr auto RequireExists = false;
        constexpr auto Tree          = true;
        IFileManager::Get().DeleteDirectory(*Get_TestLockDir(), RequireExists, Tree);
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_RegenOwnership_SecondOpenFails,
    "CkAngelscriptGenerator.UnitTests.RegenOwnership.SecondOpenFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_RegenOwnership_SecondOpenFails::RunTest(const FString&)
{
    Cleanup_TestLockDir();
    const auto LockPath = Get_TestLockPath();

    auto FirstHandle = FCkAngelscriptGenerator_RegenOwnership::Try_OpenExclusiveWriteHandle(LockPath);
    if (NOT TestTrue(TEXT("first open acquires the lock"), FirstHandle.IsValid()))
    { return false; }

#if PLATFORM_WINDOWS
    auto SecondHandle = FCkAngelscriptGenerator_RegenOwnership::Try_OpenExclusiveWriteHandle(LockPath);
    TestFalse(TEXT("second open on a held path fails (sharing violation)"), SecondHandle.IsValid());
#else
    AddInfo(TEXT("POSIX OpenWrite does not enforce write exclusivity - conflict assertion skipped ")
            TEXT("(the guard degrades to always-owner off-Windows by design)."));
#endif

    FirstHandle.Reset();
    Cleanup_TestLockDir();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_RegenOwnership_ReadAllowedWhileHeld,
    "CkAngelscriptGenerator.UnitTests.RegenOwnership.ReadAllowedWhileHeld",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_RegenOwnership_ReadAllowedWhileHeld::RunTest(const FString&)
{
    Cleanup_TestLockDir();
    const auto LockPath = Get_TestLockPath();

    auto Handle = FCkAngelscriptGenerator_RegenOwnership::Try_OpenExclusiveWriteHandle(LockPath);
    if (NOT TestTrue(TEXT("lock acquired"), Handle.IsValid()))
    { return false; }

    const auto Breadcrumb = FString{TEXT("pid=12345\n")};
    const auto Utf8 = FTCHARToUTF8{*Breadcrumb};
    Handle->Write(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
    Handle->Flush();

    // The held handle has GENERIC_WRITE, so Windows requires the READER to share-write too:
    // this combination is exactly what OpenWrite(bAllowRead=true) permits and bAllowRead=false
    // would refuse. It proves the share flag is live, not that any reader can open the lock.
    auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    constexpr auto AllowWriteWhileReading = true;
    auto ReadHandle = TUniquePtr<IFileHandle>{PlatformFile.OpenRead(*LockPath, AllowWriteWhileReading)};
    if (NOT TestTrue(TEXT("a cooperating reader can open the lock while the write handle is held"),
            ReadHandle.IsValid()))
    {
        Handle.Reset();
        Cleanup_TestLockDir();
        return false;
    }

    TestTrue(TEXT("the held lock file exposes its breadcrumb bytes to the reader"),
        ReadHandle->Size() > 0);

    ReadHandle.Reset();
    Handle.Reset();
    Cleanup_TestLockDir();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_RegenOwnership_ReleaseThenReacquire,
    "CkAngelscriptGenerator.UnitTests.RegenOwnership.ReleaseThenReacquire",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_RegenOwnership_ReleaseThenReacquire::RunTest(const FString&)
{
    Cleanup_TestLockDir();
    const auto LockPath = Get_TestLockPath();

    auto FirstHandle = FCkAngelscriptGenerator_RegenOwnership::Try_OpenExclusiveWriteHandle(LockPath);
    if (NOT TestTrue(TEXT("initial acquisition succeeds"), FirstHandle.IsValid()))
    { return false; }

    // Models owner exit → lazy takeover: closing the handle is exactly what process death does.
    FirstHandle.Reset();

    auto SecondHandle = FCkAngelscriptGenerator_RegenOwnership::Try_OpenExclusiveWriteHandle(LockPath);
    TestTrue(TEXT("re-acquisition succeeds after release"), SecondHandle.IsValid());

    SecondHandle.Reset();
    Cleanup_TestLockDir();
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
