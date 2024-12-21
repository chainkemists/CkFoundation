#include "CkProjectile_ProcessorInjector.h"

#include "CkProjectile/CkProjectile_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Projectile_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Projectile_Update>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Projectile_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
