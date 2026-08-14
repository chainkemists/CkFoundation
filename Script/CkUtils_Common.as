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
        const auto EcsWorldSubsystem = Subsystem::GetWorldSubsystem(UCk_EcsWorld_Subsystem_UE);
        if (ck::IsValid(EcsWorldSubsystem))
        {
            return EcsWorldSubsystem.Get_TransientEntity();
        }
        else
        {
            return Subsystem::GetWorldSubsystem(UCk_EditorEcsWorld_Subsystem_UE).Get_TransientEntity();

        }
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

    // Cheap diagnostic for premature assets::load::* calls (before engine-safe). Unlike EnsureIfNot,
    // this does NOT capture stack traces (each full ensure walks C++/BP/AS stacks ~15ms) — it records
    // a count + first message via UCk_Utils_IO_UE, which UCk_DeferredAssetInit_UE surfaces as one
    // aggregated line. Called from the generated assets::load::* accessors. Returns true when reported.
    //
    // Also notes the deferred load for the surgical heal (UNGATED — must run in cook too, before the
    // commandlet gate below), so UCk_DeferredAssetInit_UE re-runs only the CDOs that actually deferred.
    bool EnsureIfNot_PrematureAssetLoad(bool InExpression, FString InMessage)
    {
        UCk_DeferredAssetInit_UE::Note_DeferredAssetLoad_FromActiveContext();

        if (InExpression)
        { return false; }

        UCk_Utils_IO_UE::Report_PrematureAssetLoad(InMessage);
        return true;
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

namespace GameplayTag
{
    TArray<FGameplayTag> MakeGameplayTagArrayFromTag(FGameplayTag InTag)
    {
        TArray<FGameplayTag> Result;
        Result.Add(InTag);
        return Result;
    }

    FGameplayTagContainer MakeContainerFromTag(FGameplayTag InTag)
    {
        FGameplayTagContainer Result;
        Result.AddTag(InTag);
        return Result;
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