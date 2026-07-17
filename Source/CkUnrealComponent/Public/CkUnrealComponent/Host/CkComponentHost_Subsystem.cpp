#include "CkComponentHost_Subsystem.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ComponentHost_Subsystem_UE::
    Get(
        UWorld* InWorld)
    -> UCk_ComponentHost_Subsystem_UE*
{
    if (ck::Is_NOT_Valid(InWorld))
    { return nullptr; }

    return InWorld->GetSubsystem<UCk_ComponentHost_Subsystem_UE>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ComponentHost_Subsystem_UE::
    Get_HostActor()
    -> AActor*
{
    if (ck::IsValid(_HostActor))
    { return _HostActor; }

    auto* World = GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return nullptr; }

    auto SpawnParams = FActorSpawnParameters{};
    SpawnParams.Name = TEXT("Ck_UnrealComponent_Host");
    SpawnParams.ObjectFlags = RF_Transient;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    _HostActor = World->SpawnActor<AActor>(AActor::StaticClass(), SpawnParams);

    return _HostActor;
}

// --------------------------------------------------------------------------------------------------------------------
