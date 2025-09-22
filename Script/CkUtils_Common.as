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

    FCk_Handle OwnerEntity(UCk_EntityScript_UE InEntityScript)
    {
        return utils_entity_lifetime::Get_LifetimeOwner(InEntityScript.DoGet_ScriptEntity());
    }

    FCk_Handle OwnerEntity(FCk_Handle InHandle)
    {
        return utils_entity_lifetime::Get_LifetimeOwner(InHandle);
    }

    FCk_Handle Ctx(FCk_Handle InHandle)
    {
        return InHandle.Get_ContextOwner();
    }

    FCk_Handle Ctx(UCk_EntityScript_UE InEntityScript)
    {
        return ck::Ctx(InEntityScript.DoGet_ScriptEntity());
    }

    const FCk_Handle TransientEntity()
    {
        return Subsystem::GetWorldSubsystem(UCk_EcsWorld_Subsystem_UE).Get_TransientEntity();
    }

    bool Ensure(bool InExpression, FString InMessage)
    {
        ECk_ValidInvalid Out = ECk_ValidInvalid::Valid;
        UCk_Utils_Ensure_UE::EnsureMsgf(InExpression, FText::FromString(InMessage), Out);

        return Out == ECk_ValidInvalid::Valid;
    }

    FText Text(FString InString)
    {
        return FText::FromString(InString);
    }

    FText Text(FName InName)
    {
        return FText::FromName(InName);
    }
}

mixin void Destroy(FCk_Handle& InHandle)
{
    InHandle.Request_DestroyEntity(ECk_EntityLifetime_DestructionBehavior::ForceDestroy);
}

mixin void DestroyEntityScript(UCk_EntityScript_UE InEntityScript)
{
    utils_entity_lifetime::Request_DestroyEntity(InEntityScript.Get_AssociatedEntity());
}

mixin FInstancedStruct Instanced(UScriptStruct InStruct)
{
    auto Result = FInstancedStruct();
    Result.InitializeAs(InStruct);
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

namespace GameplayTags
{
    FGameplayTag ResolveGameplayTag(FName InTagName, FString InComment = "")
    {
        return utils_gameplay_tag::ResolveGameplayTag(InTagName, InComment);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FVector ToVector(FVector2D InVector2D)
    {
        return FVector(InVector2D.X, InVector2D.Y, 0.0f);
    }

    FVector2D ToVector(FVector InVector)
    {
        return FVector2D(InVector.X, InVector.Y);
    }
}

// --------------------------------------------------------------------------------------------------------------------