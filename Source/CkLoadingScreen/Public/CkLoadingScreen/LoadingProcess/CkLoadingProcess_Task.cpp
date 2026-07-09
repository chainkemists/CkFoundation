#include "CkLoadingProcess_Task.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkLoadingScreen/CkLoadingScreen_Log.h"
#include "CkLoadingScreen/Subsystem/CkLoadingScreen_Subsystem.h"

#include <Engine/Engine.h>
#include <Engine/GameInstance.h>
#include <Engine/World.h>
#include <UObject/ScriptInterface.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_LoadingProcess_Task_UE::
    Create(
        UObject* InWorldContextObject,
        const FString& InShowLoadingScreenReason,
        float InTimeoutSeconds)
    -> UCk_LoadingProcess_Task_UE*
{
    const auto World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    const auto GameInstance = ck::IsValid(World) ? World->GetGameInstance() : nullptr;
    const auto LoadingScreenSubsystem = ck::IsValid(GameInstance)
        ? GameInstance->GetSubsystem<UCk_LoadingScreen_Subsystem_UE>()
        : nullptr;

    if (ck::Is_NOT_Valid(LoadingScreenSubsystem))
    {
        // Legitimate absence — the subsystem does not exist on dedicated servers.
        ck::loading_screen::Verbose(
            TEXT("No LoadingScreen subsystem available for [{}] — not creating a loading process task for [{}]"),
            InWorldContextObject, InShowLoadingScreenReason);
        return {};
    }

    const auto NewLoadingTask = NewObject<UCk_LoadingProcess_Task_UE>(LoadingScreenSubsystem);
    NewLoadingTask->Request_SetReason(InShowLoadingScreenReason);
    NewLoadingTask->_TimeoutSeconds = InTimeoutSeconds;
    NewLoadingTask->_TimeCreated = FPlatformTime::Seconds();

    LoadingScreenSubsystem->Register_LoadingProcessor(NewLoadingTask);

    return NewLoadingTask;
}

auto
    UCk_LoadingProcess_Task_UE::
    Request_Unregister()
    -> void
{
    const auto LoadingScreenSubsystem = Cast<UCk_LoadingScreen_Subsystem_UE>(GetOuter());

    CK_ENSURE_IF_NOT(ck::IsValid(LoadingScreenSubsystem),
        TEXT("LoadingProcess Task [{}] is NOT outered to the LoadingScreen subsystem. Was it created via Create()?"), this)
    { return; }

    LoadingScreenSubsystem->Unregister_LoadingProcessor(this);
}

auto
    UCk_LoadingProcess_Task_UE::
    Request_SetReason(
        const FString& InReason)
    -> void
{
    _Reason = InReason;
}

auto
    UCk_LoadingProcess_Task_UE::
    Get_ShouldShowLoadingScreen(
        FString& OutReason) const
    -> bool
{
    if (_HasTimedOut)
    { return false; }

    if (_TimeoutSeconds > 0.0f)
    {
        const auto ElapsedSeconds = FPlatformTime::Seconds() - _TimeCreated;
        if (ElapsedSeconds > static_cast<double>(_TimeoutSeconds))
        {
            _HasTimedOut = true;

            CK_TRIGGER_ENSURE(TEXT("LoadingProcess Task [{}] exceeded its [{}]s watchdog timeout — "
                "releasing the loading screen hold (fail-open). The owning system never called Request_Unregister."),
                _Reason, _TimeoutSeconds);

            return false;
        }
    }

    OutReason = _Reason;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
