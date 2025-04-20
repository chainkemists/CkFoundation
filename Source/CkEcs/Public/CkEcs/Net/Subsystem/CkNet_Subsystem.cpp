#include "CkNet_Subsystem.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Net_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);
    const auto EcsWorldSubsystem = InCollection.InitializeDependency<UCk_EcsWorld_Subsystem_UE>();
    auto TransientEntity = EcsWorldSubsystem->Get_TransientEntity();

    if (GetWorld()->IsNetMode(NM_DedicatedServer) || GetWorld()->IsNetMode(NM_ListenServer) || GetWorld()->IsNetMode(NM_Standalone))
    {
        UCk_Utils_Net_UE::Add(TransientEntity,
            FCk_Net_ConnectionSettings{ECk_Replication::DoesNotReplicate, ECk_Net_NetModeType::Host, ECk_Net_EntityNetRole::Authority});
    }
    else
    {
        UCk_Utils_Net_UE::Add(TransientEntity,
            FCk_Net_ConnectionSettings{ECk_Replication::DoesNotReplicate, ECk_Net_NetModeType::Client, ECk_Net_EntityNetRole::Authority});
    }
}

// --------------------------------------------------------------------------------------------------------------------
