#pragma once

#include "CkEcs/Entity/CkEntity.h"
#include "CkMacros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKECSBASICS_API FCk_Fragment_EntityHolder
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_EntityHolder);

    public:
        template <typename>
        friend class TUtils_EntityHolder;

    public:
        using EntityType = FCk_Entity;

    public:
        FCk_Fragment_EntityHolder() = default;
        explicit FCk_Fragment_EntityHolder(EntityType InEntity);

    private:
        EntityType _Entity;

    public:
        CK_PROPERTY_GET(_Entity);

    private:
        CK_PROPERTY_GET_NON_CONST(_Entity);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FCk_Fragment_ParentEntity : public FCk_Fragment_EntityHolder
    {
        using FCk_Fragment_EntityHolder::FCk_Fragment_EntityHolder;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FCk_Fragment_TargetEntity : public FCk_Fragment_EntityHolder
    {
        using FCk_Fragment_EntityHolder::FCk_Fragment_EntityHolder;
    };
}

// --------------------------------------------------------------------------------------------------------------------
