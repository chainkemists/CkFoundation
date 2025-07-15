#include "CkSceneNode_ProcessorInjector.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SceneNode_ProcessorInjector_HandleRequests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_SceneNode_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SceneNode_ProcessorInjector_Update_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_SceneNode_UpdateLocal>(InWorld.Get_Registry());
    InWorld.Add<ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer0>>(InWorld.Get_Registry());
    InWorld.Add<ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer1>>(InWorld.Get_Registry());
    InWorld.Add<ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer2>>(InWorld.Get_Registry());
    InWorld.Add<ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer3>>(InWorld.Get_Registry());
    InWorld.Add<ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer4>>(InWorld.Get_Registry());
    InWorld.Add<ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer5>>(InWorld.Get_Registry());
    InWorld.Add<ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer6>>(InWorld.Get_Registry());
    InWorld.Add<ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer7>>(InWorld.Get_Registry());
    InWorld.Add<ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer8>>(InWorld.Get_Registry());
    InWorld.Add<ck::TProcessor_SceneNode_Update<ck::FTag_SceneNode_Layer9>>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
