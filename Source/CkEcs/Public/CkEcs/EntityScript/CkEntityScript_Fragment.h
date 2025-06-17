#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_EntityScript_ContinueConstruction);
    CK_DEFINE_ECS_TAG(FTag_EntityScript_FinishConstruction);
    CK_DEFINE_ECS_TAG(FTag_EntityScript_BeginPlay);
    CK_DEFINE_ECS_TAG(FTag_EntityScript_HasBegunPlay);
    CK_DEFINE_ECS_TAG(FTag_EntityScript_HasEndedPlay);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_EntityScript_RequestSpawnEntity = FCk_Request_EntityScript_SpawnEntity;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API FFragment_EntityScript_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityScript_Current);

    public:
        friend class UCk_Utils_EntityScript_UE;

    public:
        FFragment_EntityScript_Current() = default;

        explicit
        FFragment_EntityScript_Current(
            UCk_EntityScript_UE* InScript);

    private:
        TStrongObjectPtr<UCk_EntityScript_UE> _Script;

    public:
        CK_PROPERTY_GET(_Script);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API FRequest_EntityScript_Replicate
    {
    public:
        CK_GENERATED_BODY(FRequest_EntityScript_Replicate);

    public:
        FRequest_EntityScript_Replicate() = default;
        FRequest_EntityScript_Replicate(
            const FCk_Handle& InOwner,
            const FInstancedStruct& InSpawnParams,
            UCk_EntityScript_UE* InScript);

    private:
        FCk_Handle _Owner;
        FInstancedStruct _SpawnParams;
        TWeakObjectPtr<UCk_EntityScript_UE> _Script;

    public:
        CK_PROPERTY_GET(_Owner);
        CK_PROPERTY_GET(_SpawnParams);
        CK_PROPERTY_GET(_Script);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKECS_API,
        OnConstructed,
        FCk_Delegate_EntityScript_Constructed_MC,
        FCk_Handle_EntityScript);
}

// --------------------------------------------------------------------------------------------------------------------
