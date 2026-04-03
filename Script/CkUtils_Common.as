// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FCk_Handle ToEntity(const AActor InActor)
    {
        return InActor.Get_ActorEntityHandle();
    }

    FCk_Handle ToEntity(const UCk_EntityScript_UE InEntityScript)
    {
        return InEntityScript.DoGet_ScriptEntity();
    }

    AActor ToActor(const FCk_Handle InHandle, ECk_SanityCheck InSanityCheck = ECk_SanityCheck::Checked)
    {
        switch (InSanityCheck)
        {
            case ECk_SanityCheck::Checked:
                return InHandle.Get_EntityOwningActor();
            case ECk_SanityCheck::UnChecked:
                return InHandle.TryGet_EntityOwningActor();
        }
    }

    FCk_Handle OwnerEntity(const UCk_EntityScript_UE InEntityScript)
    {
        return utils_entity_lifetime::Get_LifetimeOwner(InEntityScript.DoGet_ScriptEntity());
    }

    FCk_Handle OwnerEntity(const FCk_Handle InHandle)
    {
        return utils_entity_lifetime::Get_LifetimeOwner(InHandle);
    }

    FCk_Handle Ctx(const FCk_Handle InHandle)
    {
        return InHandle.Get_ContextOwner();
    }

    FCk_Handle Ctx(const UCk_EntityScript_UE InEntityScript)
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

    bool EnsureIfNot(bool InExpression, FString InMessage)
    {
        return ck::Ensure(InExpression, InMessage) == false;
    }

    void TriggerEnsure(FString InMessage)
    {
        UCk_Utils_Ensure_UE::TriggerEnsure(FText::FromString(InMessage));
    }

    FText Text(FString InString)
    {
        return FText::FromString(InString);
    }

    FText Text(FName InName)
    {
        return FText::FromName(InName);
    }

    bool HasAuthority()
    {
        return utils_net::Get_HasAuthority(ck::TransientEntity());
    }

    bool CanExecuteCosmeticEvents()
    {
        return utils_net::Get_CanExecuteCosmeticEvents(ck::TransientEntity());
    }

    void MARK_PROPERTY_DIRTY(UObject Object, FName PropertyName)
    {
        UNetPushModelHelpers::MarkPropertyDirty(Object, PropertyName);
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

    const FGameplayTag EmptyTag = FGameplayTag();
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

    FString ToString(float32 InValue)
    {
        return String::Conv_DoubleToString(InValue);
    }

    FString ToString(float64 InValue)
    {
        return String::Conv_DoubleToString(InValue);
    }

    FString ToString(int32 InValue)
    {
        return String::Conv_IntToString(InValue);
    }

    FString ToString(int64 InValue)
    {
        return String::Conv_Int64ToString(InValue);
    }

    FString ToString(bool InValue)
    {
        return String::Conv_BoolToString(InValue);
    }

    FString ToString(FIntVector InValue)
    {
        return String::Conv_IntVectorToString(InValue);
    }

    FString ToString(FIntVector2 InValue)
    {
        return String::Conv_IntVector2ToString(InValue);
    }

    FString ToString(FVector InValue)
    {
        return String::Conv_VectorToString(InValue);
    }

    FString ToString(FVector2D InValue)
    {
        return String::Conv_Vector2dToString(InValue);
    }

    TArray<float32> ToFloatArray(FVector2D InValue)
    {
        auto Result = TArray<float32>();
        Result.Reserve(3);
        Result.Add(InValue.X);
        Result.Add(InValue.Y);

        return Result;
    }

    TArray<float32> ToFloatArray(FVector InValue)
    {
        auto Result = TArray<float32>();
        Result.Reserve(3);
        Result.Add(InValue.X);
        Result.Add(InValue.Y);
        Result.Add(InValue.Z);

        return Result;
    }

    TArray<float32> ToFloatArray(FVector4 InValue)
    {
        auto Result = TArray<float32>();
        Result.Reserve(3);
        Result.Add(InValue.X);
        Result.Add(InValue.Y);
        Result.Add(InValue.Z);
        Result.Add(InValue.W);

        return Result;
    }

    TArray<float32> ToFloatArray(FLinearColor InValue)
    {
        auto Result = TArray<float32>();
        Result.Reserve(3);
        Result.Add(InValue.R);
        Result.Add(InValue.G);
        Result.Add(InValue.B);
        Result.Add(InValue.A);

        return Result;
    }

    int32 INDEX_NONE()
    {
        return -1;
    }
}

// --------------------------------------------------------------------------------------------------------------------