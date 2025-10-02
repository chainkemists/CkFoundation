delegate void FCk_Delegate_OnActorReplicationComplete(FCk_Handle InModule);

namespace utils_ecs
{
    // @TODO: Should probably exist in code
    void Promise_OnActorReplicationComplete(AActor InActor, FCk_Delegate_OnActorReplicationComplete InDelegate)
    {
        if (ck::Ensure(ck::IsValid(InActor), "Invalid Actor supplied to 'Promise_OnActorReplicationComplete'") == false)
        { return; }

        auto EntityBridge = UCk_EntityBridge_ActorComponent_UE::Get(InActor);

        if (ck::Ensure(ck::IsValid(InActor), f"[{InActor.ToString()}] is NOT ECS Ready") == false)
        { return; }

        if (EntityBridge.Get_IsReplicationComplete())
        {
            InDelegate.ExecuteIfBound(InActor.Get_ActorEntityHandle());
        }
        else
        {
            EntityBridge._OnReplicationComplete_MC.AddUFunction(InDelegate.GetUObject(), InDelegate.GetFunctionName());
        }
    }
}