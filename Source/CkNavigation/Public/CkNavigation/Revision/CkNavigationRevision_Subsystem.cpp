#include "CkNavigation/Revision/CkNavigationRevision_Subsystem.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkNavigation/CkNavigation_Log.h"

#include <NavigationData.h>
#include <NavigationSystem.h>

auto UCk_NavigationRevisionSubsystem_UE::PostInitialize() -> void
{
    Super::PostInitialize();
    TryEnsureBound();
}

auto UCk_NavigationRevisionSubsystem_UE::TryEnsureBound() -> bool
{
    if (_IsBound)
    { return true; }

    auto* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    if (ck::Is_NOT_Valid(NavigationSystem))
    {
        ck::nav::Verbose(
            TEXT("Navigation revision observer has no NavigationSystem in world [{}]"),
            GetWorld());
        return false;
    }

    NavigationSystem->OnNavigationGenerationFinishedDelegate.AddUniqueDynamic(
        this,
        &UCk_NavigationRevisionSubsystem_UE::OnNavigationGenerationFinished);
    _IsBound = true;
    ++_Revision;
    return true;
}

auto UCk_NavigationRevisionSubsystem_UE::Deinitialize() -> void
{
    auto* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    if (_IsBound && ck::IsValid(NavigationSystem))
    {
        NavigationSystem->OnNavigationGenerationFinishedDelegate.RemoveDynamic(
            this,
            &UCk_NavigationRevisionSubsystem_UE::OnNavigationGenerationFinished);
    }
    _IsBound = false;
    Super::Deinitialize();
}

void UCk_NavigationRevisionSubsystem_UE::OnNavigationGenerationFinished(
    ANavigationData* InNavigationData)
{
    const auto NavigationDataIsValid = ck::IsValid(InNavigationData);
    CK_ENSURE_IF_NOT(NavigationDataIsValid,
        TEXT("Navigation generation observer received invalid NavigationData"))
    { }
    if (NOT NavigationDataIsValid)
    { return; }

    auto* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    const auto* DefaultNavigationData = ck::IsValid(NavigationSystem)
        ? NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)
        : nullptr;
    if (InNavigationData != DefaultNavigationData)
    { return; }

    ++_Revision;
}

// --------------------------------------------------------------------------------------------------------------------
