#pragma once

#include "CkEcs/Concepts/CkConcepts.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <concepts::ValidHandleType T_HandleType = FCk_Handle>
    struct TFragment_EntityHolder
    {
    public:
        CK_GENERATED_BODY(TFragment_EntityHolder<T_HandleType>);

    public:
        template <typename>
        friend class TUtils_EntityHolder;

    public:
        using EntityType = T_HandleType;

    private:
        EntityType _Entity;

    public:
        CK_PROPERTY_GET(_Entity);

    private:
        CK_PROPERTY_GET_NON_CONST(_Entity);

    public:
        CK_DEFINE_CONSTRUCTORS(TFragment_EntityHolder, _Entity);
    };

}

#define CK_DEFINE_ENTITY_HOLDER(_NameOfEntityHolder_, _HandleType_)             \
struct _NameOfEntityHolder_ : public ck::TFragment_EntityHolder<_HandleType_>   \
{                                                                               \
    using TFragment_EntityHolder::TFragment_EntityHolder;                       \
}

// --------------------------------------------------------------------------------------------------------------------
