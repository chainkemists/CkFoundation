#include "CkUI_ProcessorInjector.h"

#include "CkUI/WorldSpaceWidget/CkWorldSpaceWidget_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_ProcessorInjector_Update_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_WorldSpaceWidget_UpdateLocation>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_ProcessorInjector_Teardown_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_WorldSpaceWidget_Teardown>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
