#include "CkRenderProxyManager_Subsystem.h"

#include "CkCore/Actor/CkActor_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RenderProxyManager_Subsystem::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    // Spawn the manager actor
    _ManagerActor = Cast<ACk_RenderProxyManager_Actor>(
        UCk_Utils_Actor_UE::Request_SpawnActor(
            FCk_Utils_Actor_SpawnActor_Params{GetWorld(), ACk_RenderProxyManager_Actor::StaticClass()}
                .Set_Label(TEXT("RenderProxyManager"))
                .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel),
            [](AActor* InActor)
            {
                // Manager actor initialization if needed
            }
        )
    );
}

auto
    UCk_RenderProxyManager_Subsystem::
    Deinitialize()
    -> void
{
    if (ck::IsValid(_ManagerActor))
    {
        _ManagerActor->Destroy();
        _ManagerActor = nullptr;
    }

    Super::Deinitialize();
}

auto
    UCk_RenderProxyManager_Subsystem::
    Get_ManagerActor() const
    -> ACk_RenderProxyManager_Actor*
{
    return _ManagerActor;
}

auto
    UCk_RenderProxyManager_Subsystem::
    Get_ManagerComponent() const
    -> UCk_Component_RenderProxyManager*
{
    if (ck::Is_NOT_Valid(_ManagerActor))
    { return nullptr; }

    return _ManagerActor->Get_ProxyManagerComponent();
}

// --------------------------------------------------------------------------------------------------------------------