// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FCk_Handle SelfEntity(AActor InActor)
    {
        return UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(InActor);
    }

    FCk_Handle SelfEntity(UCk_EntityScript_UE InEntityScript)
    {
        return InEntityScript.DoGet_ScriptEntity();
    }

    FCk_Handle Ctx(FCk_Handle InHandle)
    {
        return UCk_Utils_ContextOwner_UE::Get_ContextOwner(InHandle);
    }

    bool Ensure(bool InExpression, FString InMessage)
    {
        ECk_ValidInvalid Out = ECk_ValidInvalid::Valid;
        UCk_Utils_Ensure_UE::EnsureMsgf(InExpression, FText::FromString(InMessage), Out);

        return Out == ECk_ValidInvalid::Valid;
    }

    FName ToText(FCk_Handle InHandle)
    {
        return UCk_Utils_Handle_UE::Get_DebugName(InHandle);
    }
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