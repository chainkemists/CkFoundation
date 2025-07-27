// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FCk_Handle SelfEntity(AActor InActor)
    {
        return InActor.Get_ActorEntityHandle();
    }

    FCk_Handle SelfEntity(UCk_EntityScript_UE InEntityScript)
    {
        return InEntityScript.DoGet_ScriptEntity();
    }

    FCk_Handle Ctx(FCk_Handle InHandle)
    {
        return InHandle.Get_ContextOwner();
    }

    bool Ensure(bool InExpression, FString InMessage)
    {
        ECk_ValidInvalid Out = ECk_ValidInvalid::Valid;
        UCk_Utils_Ensure_UE::EnsureMsgf(InExpression, FText::FromString(InMessage), Out);

        return Out == ECk_ValidInvalid::Valid;
    }

    FName ToText(FCk_Handle InHandle)
    {
        return InHandle.Get_DebugName();
    }
}

mixin void Destroy(FCk_Handle& InHandle)
{
    InHandle.Request_DestroyEntity(ECk_EntityLifetime_DestructionBehavior::ForceDestroy);
}

mixin FGameplayTagContainer
MakeGameplayTagContainer(FGameplayTag InTag)
{
    auto Container = FGameplayTagContainer();
    Container.AddTag(InTag);
    return Container;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FVector ToVector(FVector2D InVector2D)
    {
        return FVector(InVector2D.X, InVector2D.Y, 0.0f);
    }
}

// --------------------------------------------------------------------------------------------------------------------