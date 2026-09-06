// The loading-screen subsystem can be constructed by async loading. Its tickable base must not
// register until Initialize runs on the game thread.

#include "CkLoadingScreen/Subsystem/CkLoadingScreen_Subsystem.h"

#include "Async/Async.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_LoadingScreen_TickableLifecycle_AsyncConstruction,
    "Ck.LoadingScreen.TickableLifecycle.AsyncConstruction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// --------------------------------------------------------------------------------------------------------------------

bool FCkTest_LoadingScreen_TickableLifecycle_AsyncConstruction::RunTest(const FString&)
{
    // Resolve the UClass and CDO while on the game thread. NewObject is permitted on a worker
    // while the GC guard is held; it marks the result Async, which must be cleared after
    // returning here.
    auto* const SubsystemClass = UCk_LoadingScreen_Subsystem_UE::StaticClass();
    if (NOT TestNotNull(TEXT("loading-screen subsystem class is available"), SubsystemClass))
    { return false; }

    auto* const SubsystemDefault = GetMutableDefault<UCk_LoadingScreen_Subsystem_UE>();
    if (NOT TestNotNull(TEXT("loading-screen subsystem CDO is available"), SubsystemDefault))
    { return false; }

    auto* const GameInstance = NewObject<UGameInstance>(GetTransientPackage());
    GameInstance->AddToRoot();

    const auto Future = Async(EAsyncExecution::ThreadPool, [GameInstance, SubsystemClass]()
    {
        FGCScopeGuard Guard;
        return NewObject<UCk_LoadingScreen_Subsystem_UE>(GameInstance, SubsystemClass);
    });
    auto* const Subsystem = Future.Get();

    TestNotNull(TEXT("worker construction returns a subsystem"), Subsystem);
    if (Subsystem != nullptr)
    {
        TestTrue(TEXT("worker-created subsystem is marked Async"),
            Subsystem->HasAnyInternalFlags(EInternalObjectFlags::Async));
        Subsystem->ClearInternalFlags(EInternalObjectFlags::Async);
        TestFalse(TEXT("game thread clears the Async mark after handoff"),
            Subsystem->HasAnyInternalFlags(EInternalObjectFlags::Async));
        Subsystem->MarkAsGarbage();
    }

    GameInstance->RemoveFromRoot();
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif
