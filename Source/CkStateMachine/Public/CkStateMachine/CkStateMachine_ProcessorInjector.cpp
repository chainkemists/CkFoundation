#include "CkStateMachine_ProcessorInjector.h"

#include "CkStateMachine/CkStateMachine_Debug_Processor.h"
#include "CkStateMachine/CkStateMachine_Processor.h"

#if CK_BUILD_SM_GRAPH_WALK
#include "CkStateMachine/CkStateMachine_Debug_GraphWalk_Processor.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

auto UCk_StateMachine_ProcessorInjector_Requests::DoInjectProcessors(EcsWorldType& InWorld) -> void
{
    InWorld.Add<ck::FProcessor_Sm_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Sm_HandleRequests>(InWorld.Get_Registry());

#if CK_BUILD_SM_GRAPH_WALK
    InWorld.Add<ck::FProcessor_Sm_Debug_GraphWalk>(InWorld.Get_Registry());
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_StateMachine_ProcessorInjector_Update::DoInjectProcessors(EcsWorldType& InWorld) -> void
{
    InWorld.Add<ck::FProcessor_SmCondition_ResetEveryFrame>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_SmCondition_Polled>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Sm_EvalTransitions>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_SmTask_Tick>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Sm_Debug>(InWorld.Get_Registry());

#if CK_BUILD_SM_GRAPH_WALK
    InWorld.Add<ck::FProcessor_Sm_Debug_GraphWalk_Iterate>(InWorld.Get_Registry());
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_StateMachine_ProcessorInjector_Teardown::DoInjectProcessors(EcsWorldType& InWorld) -> void
{
    InWorld.Add<ck::FProcessor_Sm_EndPlay>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
