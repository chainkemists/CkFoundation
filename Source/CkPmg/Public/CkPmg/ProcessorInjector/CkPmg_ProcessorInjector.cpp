#include "CkPmg_ProcessorInjector.h"

#include "CkPmg/CkPmg_Processor.h"
#include "CkPmg/CkPmg_Processor_BasicShapes.h"
#include "CkPmg/CkPmg_Processor_IconShapes.h"
#include "CkPmg/CkPmg_Processor_SymbolShapes.h"
#include "CkPmg/CkPmg_Processor_FlatShapes.h"
#include "CkPmg/CkPmg_Processor_AngularShapes.h"
#include "CkPmg/CkPmg_Processor_DirectionalShapes.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Pmg_ProcessorInjector_Setup_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_Pmg_Donut_Setup>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Pmg_Sphere_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Box_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Cone_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Cylinder_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Capsule_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Pyramid_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Hemisphere_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Torus_Setup>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Pmg_Circle_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Triangle_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Plane_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Ring_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Cross_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Star_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Checkmark_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Diamond_Setup>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Pmg_Warning_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Prohibition_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_NoEntry_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_MagnifyingGlass_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_QuestionMark_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_ExclamationMark_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Flag_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_InfoCircle_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Pin_Setup>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Pmg_Wedge_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Arc_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_WedgeCone_Setup>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Pmg_Arrow_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_Pivot_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_DashedLine_Setup>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Pmg_ProcessorInjector_HandleRequests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Pmg_Donut_HandleRequests>(InWorld.Get_Registry());
}

auto
    UCk_Pmg_ProcessorInjector_Transform_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Pmg_Donut_UpdateTransform>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Pmg_DebugShape_CheckDuration>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Pmg_DebugShape_UpdateTransform>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Pmg_ProcessorInjector_EndPlay_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Pmg_Donut_EndPlay>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Pmg_DebugShape_EndPlay>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
