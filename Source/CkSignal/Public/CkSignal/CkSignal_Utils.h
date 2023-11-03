#pragma once

#include "CkCore/Payload/CkPayload.h"

#include "CkSignal/CkSignal_Fragment_Data.h"
#include "CkSignal/CkSignal_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedSignal>
    class TUtils_Signal
    {
    public:
        using SignalType = T_DerivedSignal;

    public:
        static auto Has(FCk_Handle InHandle) -> bool;

        template <typename... T_Args>
        static auto Broadcast(FCk_Handle InHandle, TPayload<T_Args...>&& InPayload);

        template <auto T_Candidate, ECk_Signal_BindingPolicy InPayloadInFlightBehavior, ECk_Signal_PostFireBehavior InPostFireBehavior>
        static auto Bind(FCk_Handle InHandle);

        template <auto T_Candidate,  ECk_Signal_BindingPolicy InPayloadInFlightBehavior, ECk_Signal_PostFireBehavior InPostFireBehavior,
            typename T_Instance>
        static auto Bind(T_Instance&& InInstance, FCk_Handle InHandle);

        template <auto T_Candidate>
        static auto Bind(FCk_Handle InHandle, ECk_Signal_BindingPolicy InPayloadInFlightBehavior, ECk_Signal_PostFireBehavior InPostFireBehavior);

        template <auto T_Candidate, typename T_Instance>
        static auto Bind(T_Instance&& InInstance, FCk_Handle InHandle, ECk_Signal_BindingPolicy InPayloadInFlightBehavior,
            ECk_Signal_PostFireBehavior InPostFireBehavior);

        template <auto T_Candidate>
        static auto Unbind(FCk_Handle InHandle);

        template <auto T_Candidate, typename T_Instance>
        static auto Unbind(T_Instance&& InInstance, FCk_Handle InHandle);
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedSignal, typename T_DerivedSignal_Unreal>
    class TUtils_Signal_UnrealMulticast : public TUtils_Signal<T_DerivedSignal>
    {
        CK_GENERATED_BODY(TUtils_Signal_UnrealMulticast<T_DerivedSignal COMMA T_DerivedSignal_Unreal>);

    public:
        using Super = TUtils_Signal<T_DerivedSignal>;
        using Super::SignalType;

        using UnrealSignalType = T_DerivedSignal_Unreal;
        using UnrealDynamicDelegateType = typename T_DerivedSignal_Unreal::MulticastType::FDelegate;

    public:
        using Super::Bind;
        using Super::Unbind;

        template <ECk_Signal_BindingPolicy T_PayloadInFlightBehavior>
        static auto
        Bind(
            FCk_Handle InHandle,
            UnrealDynamicDelegateType InDelegate);

        static auto
        Bind(
            FCk_Handle InHandle,
            UnrealDynamicDelegateType InDelegate,
            ECk_Signal_BindingPolicy InPayloadInFlightBehavior);

        static auto
        Unbind(
            FCk_Handle InHandle,
            UnrealDynamicDelegateType InDelegate);
    };

}

// --------------------------------------------------------------------------------------------------------------------

#include "CkSignal_Utils.inl.h"

// --------------------------------------------------------------------------------------------------------------------
