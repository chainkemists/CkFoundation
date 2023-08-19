#include "CkEntityLifetime_Processor.h"

#include "CkEcs/CkEcsLog.h"

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    FCk_Processor_EntityLifetime_EntityJustCreated::FCk_Processor_EntityLifetime_EntityJustCreated(
        const FRegistryType& InRegistry)
            : _Registry(InRegistry)
    {
    }

    auto FCk_Processor_EntityLifetime_EntityJustCreated::Tick(FTimeType) -> void
    {
        _Registry.Clear<FCk_Tag_EntityJustCreated>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto FCk_Processor_EntityLifetime_TriggerDestroyEntity::Tick(FTimeType InDeltaT) -> void
    {
        Super::Tick(InDeltaT);
        _Registry.Clear<FCk_Tag_TriggerDestroyEntity>();
    }

    auto FCk_Processor_EntityLifetime_TriggerDestroyEntity::ForEachEntity(FTimeType InDeltaT,
                                                                          FHandleType InHandle) const -> void
    {
        ecs::VeryVerbose(TEXT("Entity [{}] set to 'Pending Destroy'"), InHandle);
        InHandle.Add<FCk_Tag_PendingDestroyEntity>(InHandle);

        // TODO: Invoke Signal when Signals are ready
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto FCk_Processor_EntityLifetime_PendingDestroyEntity::ForEachEntity(FTimeType InDeltaT,
        FHandleType InHandle) const -> void
    {
        ecs::VeryVerbose(TEXT("Destroying Entity [{}]"), InHandle);

        InHandle.Remove<FCk_Tag_PendingDestroyEntity>();
    }

    // --------------------------------------------------------------------------------------------------------------------
}
