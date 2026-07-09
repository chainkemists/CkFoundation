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
        const FString& InShowLoadingScreenReason)
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
    OutReason = _Reason;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
