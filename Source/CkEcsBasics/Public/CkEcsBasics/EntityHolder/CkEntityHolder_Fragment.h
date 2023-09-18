#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKECSBASICS_API FFragment_EntityHolder
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityHolder);

    public:
        template <typename>
        friend class TUtils_EntityHolder;

    public:
        using EntityType = FCk_Handle;

    public:
        FFragment_EntityHolder() = default;
        explicit FFragment_EntityHolder(EntityType InEntity);

    private:
        EntityType _Entity;

    public:
        CK_PROPERTY_GET(_Entity);

    private:
        CK_PROPERTY_GET_NON_CONST(_Entity);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_ParentEntity : public FFragment_EntityHolder
    {
        using FFragment_EntityHolder::FFragment_EntityHolder;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_TargetEntity : public FFragment_EntityHolder
    {
        using FFragment_EntityHolder::FFragment_EntityHolder;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_OwningEntity : public FFragment_EntityHolder
    {
        using FFragment_EntityHolder::FFragment_EntityHolder;
    };
}

// --------------------------------------------------------------------------------------------------------------------
