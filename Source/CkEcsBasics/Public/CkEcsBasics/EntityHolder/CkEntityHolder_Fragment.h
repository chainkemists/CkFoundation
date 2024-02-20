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

    private:
        EntityType _Entity;

    public:
        CK_PROPERTY_GET(_Entity);

    private:
        CK_PROPERTY_GET_NON_CONST(_Entity);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_EntityHolder, _Entity);
    };
}

#define CK_DEFINE_ENTITY_HOLDER(_NameOfEntityHolder_)      \
struct _NameOfEntityHolder_ : public FFragment_EntityHolder\
{                                                          \
    using FFragment_EntityHolder::FFragment_EntityHolder;  \
}

// --------------------------------------------------------------------------------------------------------------------
