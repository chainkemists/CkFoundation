#include "CkQueue/Navigation/CkQueue_NavigationRevisionSubsystem.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkQueue/CkQueue_Log.h"

#include "NavigationData.h"
#include "NavigationSystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Queue_NavigationRevisionSubsystem_UE::
    PostInitialize()
    -> void
{
    Super::PostInitialize();

    TryEnsureBound();
}

auto
    UCk_Queue_NavigationRevisionSubsystem_UE::
    TryEnsureBound()
    -> bool
{
    if (_IsBound)
    { return true; }

    auto NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    if (ck::Is_NOT_Valid(NavigationSystem))
    {
        ck::queue::Verbose(TEXT("Queue navigation revision observer has no NavigationSystem in world [{}]"), GetWorld());
        return false;
    }

    NavigationSystem->OnNavigationGenerationFinishedDelegate.AddUniqueDynamic(
        this,
        &UCk_Queue_NavigationRevisionSubsystem_UE::OnNavigationGenerationFinished);
    _IsBound = true;
    _Revision = _Revision == MAX_int32 ? 1 : _Revision + 1;
    return true;
}

auto
    UCk_Queue_NavigationRevisionSubsystem_UE::
    Deinitialize()
    -> void
{
    auto NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    if (_IsBound && ck::IsValid(NavigationSystem))
    {
        NavigationSystem->OnNavigationGenerationFinishedDelegate.RemoveDynamic(
            this,
            &UCk_Queue_NavigationRevisionSubsystem_UE::OnNavigationGenerationFinished);
    }
    _IsBound = false;

    Super::Deinitialize();
}

void
    UCk_Queue_NavigationRevisionSubsystem_UE::
    OnNavigationGenerationFinished(
        ANavigationData* InNavigationData)
{
    const auto NavigationDataIsValid = ck::IsValid(InNavigationData);
    CK_ENSURE_IF_NOT(NavigationDataIsValid,
        TEXT("Queue navigation generation observer received invalid NavigationData"))
    { return; }

    _Revision = _Revision == MAX_int32 ? 1 : _Revision + 1;
}

// --------------------------------------------------------------------------------------------------------------------
