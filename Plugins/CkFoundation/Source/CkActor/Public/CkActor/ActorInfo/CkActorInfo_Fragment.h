#pragma once

#include "CkActor/ActorInfo/CkActorInfo_Fragment_Params.h"

#include "CkMacros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKACTOR_API FCk_Fragment_ActorInfo_Params
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_ActorInfo_Params);

    public:
        using ParamsType = FCk_Fragment_ActorInfo_ParamsData;

    public:
        FCk_Fragment_ActorInfo_Params() = default;
        explicit FCk_Fragment_ActorInfo_Params(
            ParamsType InParams);

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKACTOR_API FCk_Fragment_ActorInfo_Current
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_ActorInfo_Current);

    public:
        friend class UCk_Utils_ActorInfo_UE;

    public:
        FCk_Fragment_ActorInfo_Current() = default;
        explicit FCk_Fragment_ActorInfo_Current(
            AActor* InEntityActor);

    private:
        TWeakObjectPtr<AActor> _EntityActor;

    public:
        CK_PROPERTY_GET(_EntityActor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKACTOR_API FCk_Fragment_OwningActor
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_OwningActor);

    public:
        friend class UCk_Utils_ActorInfo_UE;

    public:
        FCk_Fragment_OwningActor() = default;
        explicit FCk_Fragment_OwningActor(
            AActor* InOwningActor, UCk_EcsBootstrapper_Base_UE* _Bootstrapper);

    private:
        TWeakObjectPtr<AActor> _OwningActor;
        TWeakObjectPtr<UCk_EcsBootstrapper_Base_UE> _Bootstrapper;

    public:
        CK_PROPERTY_GET(_OwningActor);
        CK_PROPERTY_GET(_Bootstrapper);
    };
}

// --------------------------------------------------------------------------------------------------------------------
