#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"
#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_EntityScript_BeginPlay);
    CK_DEFINE_ECS_TAG(FTag_EntityScript_HasBegunPlay);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_EntityScript_RequestSpawnEntity = FCk_Request_EntityScript_SpawnEntity;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API FFragment_EntityScript_RequestConstruct
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityScript_RequestConstruct);

    private:
        FCk_EntityScript_PostConstruction_Func _PostConstruction_Func;

    public:
        CK_PROPERTY_GET(_PostConstruction_Func);

        CK_DEFINE_CONSTRUCTORS(FFragment_EntityScript_RequestConstruct, _PostConstruction_Func);
    };

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
}

// --------------------------------------------------------------------------------------------------------------------
