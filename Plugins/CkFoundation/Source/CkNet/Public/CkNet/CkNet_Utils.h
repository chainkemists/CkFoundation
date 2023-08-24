#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkMacros/CkMacros.h"

#include "CkNet_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKNET_API UCk_Utils_Net_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Net_UE);

public:
    friend class UCk_EcsConstructionScript_ActorComponent;

public:
    UFUNCTION(BlueprintPure)
    static bool Get_IsEntityNetMode_DedicatedServer(FCk_Handle InHandle);

    UFUNCTION(BlueprintPure)
    static bool Get_IsEntityNetMode_Client(FCk_Handle InHandle);

private:
    static auto Request_MarkEntityAs_DedicatedServer(FCk_Handle InHandle) -> void;

public:
    template <typename T_ReplicatedFragment, typename T_UnaryUpdateFunc>
    static auto UpdateReplicatedFragment(FCk_Handle InHandle, T_UnaryUpdateFunc InUpdateFunc) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <typename T_ReplicatedFragment, typename T_UnaryUpdateFunc>
auto
    UCk_Utils_Net_UE::
    UpdateReplicatedFragment(
        FCk_Handle InHandle,
        T_UnaryUpdateFunc InUpdateFunc)
    -> void
{
    static_assert(std::is_base_of_v<class UCk_Ecs_ReplicatedObject, T_ReplicatedFragment>, "Replicated Fragment MUST derive from UCk_Ecs_ReplicatedObject");

    if (Get_IsEntityNetMode_Client(InHandle))
    { return; }

    InHandle.Try_Transform<TObjectPtr<T_ReplicatedFragment>>([InUpdateFunc](TObjectPtr<T_ReplicatedFragment>& InRepComp)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InRepComp), TEXT("Invalid Replicated Fragment TObjectPtr"))
        { return; }

        InUpdateFunc(InRepComp.Get());
    });
}

// --------------------------------------------------------------------------------------------------------------------
