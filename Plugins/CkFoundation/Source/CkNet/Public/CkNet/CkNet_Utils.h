#pragma once

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkEcs/CkEcs_Utils.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

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
    friend class UCk_EcsConstructionScript_ActorComponent_UE;

public:
    UFUNCTION(BlueprintPure)
    static bool Get_IsEntityNetMode_DedicatedServer(FCk_Handle InHandle);

    UFUNCTION(BlueprintPure)
    static bool Get_IsEntityNetMode_Client(FCk_Handle InHandle);

private:
    static auto Request_MarkEntityAs_DedicatedServer(FCk_Handle InHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKNET_API UCk_Utils_Ecs_Net_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Net_UE);

public:
    template <typename T_ReplicatedFragment, typename T_UnaryUpdateFunc>
    static auto UpdateReplicatedFragment(FCk_Handle InHandle, T_UnaryUpdateFunc InUpdateFunc) -> void;

    template <typename T_ReplicatedFragment>
    static auto TryAddReplicatedFragment(FCk_Handle InHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <typename T_ReplicatedFragment, typename T_UnaryUpdateFunc>
auto
    UCk_Utils_Ecs_Net_UE::
    UpdateReplicatedFragment(
        FCk_Handle InHandle,
        T_UnaryUpdateFunc InUpdateFunc)
    -> void
{
    static_assert(std::is_base_of_v<class UCk_Ecs_ReplicatedObject_UE, T_ReplicatedFragment>, "Replicated Fragment MUST derive from UCk_Ecs_ReplicatedObject_UE");

    if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InHandle))
    { return; }

    InHandle.Try_Transform<TObjectPtr<T_ReplicatedFragment>>([InUpdateFunc](TObjectPtr<T_ReplicatedFragment>& InRepComp)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InRepComp), TEXT("Invalid Replicated Fragment TObjectPtr"))
        { return; }

        InUpdateFunc(InRepComp.Get());
    });
}

template <typename T_ReplicatedFragment>
auto
    UCk_Utils_Ecs_Net_UE::
    TryAddReplicatedFragment(
        FCk_Handle InHandle)
    -> void
{
    static_assert(std::is_base_of_v<class UCk_Ecs_ReplicatedObject_UE, T_ReplicatedFragment>, "Replicated Fragment MUST derive from UCk_Ecs_ReplicatedObject_UE");

    if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InHandle))
    { return; }

    const auto& BasicDetails   = UCk_Utils_OwningActor_UE::Get_EntityOwningActorBasicDetails(InHandle);
    const auto& OwningActor    = BasicDetails.Get_Actor().Get();
    const auto& OutermostActor = UCk_Utils_Actor_UE::Get_OutermostActor_RemoteAuthority(OwningActor);

    auto ReplicatedFragment_Object = Cast<T_ReplicatedFragment>(UCk_Ecs_ReplicatedObject_UE::Create
    (
        T_ReplicatedFragment::StaticClass(),
        OutermostActor,
        NAME_None,
        InHandle
    ));

    CK_ENSURE_IF_NOT(ck::IsValid(ReplicatedFragment_Object), TEXT("Failed to create Replicated Fragment Object for Entity [{}]"), InHandle)
    { return; }

    InHandle.Add<TObjectPtr<T_ReplicatedFragment>>(ReplicatedFragment_Object);

    UCk_Utils_ReplicatedObjects_UE::Request_AddReplicatedObject(InHandle, ReplicatedFragment_Object);
}

// --------------------------------------------------------------------------------------------------------------------
