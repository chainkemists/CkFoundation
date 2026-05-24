#include "CkStateMachine_Replication.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/Net/CkStateMachine_RepData.h"
#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------
//
// Replication-handler registration for CkStateMachine's two payload shapes.
//
// Both RepData types are container-style replicated fragments (no per-entity UObject driver),
// attached on authority by FProcessor_Sm_Setup and replicated via FCk_RepData_Container. The
// handlers below are STUBS that log only — the actual replay-and-commit logic lives in Phase 7's
// ApplyReplicatedHistory processor. Registering the handlers now (Phase 5.1) means the rep
// driver doesn't silently drop changes on the client side once Phase 5.2 starts attaching the
// payload.
//
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    struct FCk_StateMachineRepHandlerRegistrar
    {
        FCk_StateMachineRepHandlerRegistrar()
        {
            FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
                []() -> UScriptStruct* { return FCk_RepData_StateMachine_WithHistory::StaticStruct(); },
                {
                    .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                    {
                        const auto& Payload = New.Get<FCk_RepData_StateMachine_WithHistory>();
                        ck::sm::Verbose(TEXT("[STUB] WithHistory OnChange for [{}] (history size [{}], status [{}])"),
                            Entity, Payload.Get_History().Num(), Payload.Get_RunStatus());
                        // Phase 7 wires the replay path here.
                    },
                    .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                    {
                        const auto& Payload = Data.Get<FCk_RepData_StateMachine_WithHistory>();
                        ck::sm::Verbose(TEXT("[STUB] WithHistory OnAdd for [{}] (history size [{}], status [{}])"),
                            Entity, Payload.Get_History().Num(), Payload.Get_RunStatus());
                        // Phase 8 wires the OnAdd-vs-Setup race resolution here.
                    }
                });

            FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
                []() -> UScriptStruct* { return FCk_RepData_StateMachine_NoHistory::StaticStruct(); },
                {
                    .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                    {
                        const auto& Payload = New.Get<FCk_RepData_StateMachine_NoHistory>();
                        ck::sm::Verbose(TEXT("[STUB] NoHistory OnChange for [{}] (state [{}] seq [{}] status [{}])"),
                            Entity, Payload.Get_CurrentStateClass(), Payload.Get_Seq(), Payload.Get_RunStatus());
                        // Phase 7 wires snap-forward logic here.
                    },
                    .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                    {
                        const auto& Payload = Data.Get<FCk_RepData_StateMachine_NoHistory>();
                        ck::sm::Verbose(TEXT("[STUB] NoHistory OnAdd for [{}] (state [{}] seq [{}] status [{}])"),
                            Entity, Payload.Get_CurrentStateClass(), Payload.Get_Seq(), Payload.Get_RunStatus());
                    }
                });
        }
    };

    // Unity-build safety: prefix the static instance with the module so a stray anonymous-namespace
    // global of the same name in another .cpp can't collide (per project memory feedback).
    static FCk_StateMachineRepHandlerRegistrar GCkStateMachineRepHandlerRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
