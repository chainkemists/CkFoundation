delegate void FCk_Delegate_OnActorReplicationComplete(FCk_Handle InModule);

namespace utils_ecs
{
    // @TODO: Should probably exist in code
    void Promise_OnActorReplicationComplete(AActor InActor, FCk_Delegate_OnActorReplicationComplete InDelegate)
    {
        if (ck::EnsureIfNot(ck::IsValid(InActor), "Invalid Actor supplied to 'Promise_OnActorReplicationComplete'"))
        { return; }

        auto EntityBridge = UCk_EntityBridge_ActorComponent_UE::Get(InActor);

        if (ck::EnsureIfNot(EntityBridge != nullptr, f"[{InActor.ToString()}] is NOT ECS Ready (missing Entity Bridge AC)"))
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