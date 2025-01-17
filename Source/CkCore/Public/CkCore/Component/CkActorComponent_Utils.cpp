#include "CkActorComponent_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorComponent_UE::
    Get_AllowTickOnDedicatedServer(
        const UActorComponent* InActorComponent)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActorComponent), TEXT("Invalid Component supplied to Get_AllowTickOnDedicatedServer"))
    { return {}; }

    return InActorComponent->PrimaryComponentTick.bAllowTickOnDedicatedServer;
}

auto
    UCk_Utils_ActorComponent_UE::
    Set_AllowTickOnDedicatedServer(
        UActorComponent* InActorComponent,
        bool InServerTickEnabled)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActorComponent), TEXT("Invalid Component supplied to Set_AllowTickOnDedicatedServer"))
    { return; }

    InActorComponent->PrimaryComponentTick.bAllowTickOnDedicatedServer = InServerTickEnabled;
    InActorComponent->PrimaryComponentTick.UnRegisterTickFunction();
    InActorComponent->PrimaryComponentTick.RegisterTickFunction(InActorComponent->GetComponentLevel());
}

// --------------------------------------------------------------------------------------------------------------------
