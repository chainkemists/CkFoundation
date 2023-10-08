#include "CkReplicatedObject.h"

#include "CkCore/Ensure/CkEnsure.h"

#include <EngineMinimal.h>
#include <Engine/NetDriver.h>

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

#define CK_ENSURE_OUTER_IS_VALID_OR_RETURN()\
    CK_ENSURE_IF_NOT(ck::IsValid(GetOwningActor()), TEXT("Outer is [{}]. A ReplicatedObject MUST have an owner that is an Actor."),\
        GetOwningActor(), ck::Context(this))\
    {\
        return {};\
    }

UCk_ReplicatedObject_UE::
UCk_ReplicatedObject_UE()
{
    if (IsTemplate())
    { return; }

    CK_SCOPE_CALL(CK_ENSURE_OUTER_IS_VALID_OR_RETURN());
}

auto UCk_ReplicatedObject_UE::
GetOwningActor() const -> AActor*
{
    return GetTypedOuter<AActor>();
}

auto UCk_ReplicatedObject_UE::
GetWorld() const -> UWorld*
{
    if (IsTemplate())
    { return Super::GetWorld(); }

    CK_ENSURE_OUTER_IS_VALID_OR_RETURN();
    return GetOwningActor()->GetWorld();
}

auto UCk_ReplicatedObject_UE::
CallRemoteFunction(UFunction* Function,
    void* Parms,
    FOutParmRec* OutParms,
    FFrame* Stack) -> bool
{
    if (IsTemplate())
    { return Super::CallRemoteFunction(Function, Parms, OutParms, Stack); }

    CK_ENSURE_OUTER_IS_VALID_OR_RETURN();

    const auto MyOwner = GetOwningActor();
    const auto Context = GEngine->GetWorldContextFromWorld(GetWorld());

    if (ck::Is_NOT_Valid(Context, ck::IsValid_Policy_NullptrOnly{}))
    { return false;}

    for (FNamedNetDriver& Driver : Context->ActiveNetDrivers)
    {
        if (Driver.NetDriver != nullptr && Driver.NetDriver->ShouldReplicateFunction(MyOwner, Function))
        {
            Driver.NetDriver->ProcessRemoteFunction(MyOwner, Function, Parms, OutParms, Stack, this);
            return true;
        }
    }

    return false;
}

auto UCk_ReplicatedObject_UE::
GetFunctionCallspace(
    UFunction* Function,
    FFrame* Stack) -> int32
{
    if (IsTemplate())
    { Super::GetFunctionCallspace(Function, Stack); }

    CK_ENSURE_OUTER_IS_VALID_OR_RETURN();

    if ((Function->FunctionFlags & FUNC_Static))
    {
        // Try to use the same logic as function libraries for static functions,
        // will try to use the global context to check authority only/cosmetic
        return GEngine->GetGlobalFunctionCallspace(Function, this, Stack);
    }

    auto* MyOwner = GetOwningActor();
    return MyOwner->GetFunctionCallspace(Function, Stack);
}

auto UCk_ReplicatedObject_UE::
GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    const auto BpClass = Cast<UBlueprintGeneratedClass>(GetClass());

    if (ck::Is_NOT_Valid(BpClass))
    {
        const auto& Blueprint = Cast<UBlueprint>(this);
        if (ck::Is_NOT_Valid(Blueprint))
        {
            return;
        }

        return;
    }

    BpClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
}

auto UCk_ReplicatedObject_UE::
IsSupportedForNetworking() const -> bool
{
    if (IsTemplate())
    { return IsSupportedForNetworking(); }

    CK_ENSURE_OUTER_IS_VALID_OR_RETURN();

    // Our Replicated UObject is ALWAYS assumed to be replicated
    return true;
}

#undef CK_ENSURE_OUTER_IS_VALID_OR_RETURN

// --------------------------------------------------------------------------------------------------------------------
