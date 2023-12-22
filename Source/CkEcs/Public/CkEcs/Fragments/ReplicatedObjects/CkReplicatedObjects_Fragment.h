#pragma once

#include "CkReplicatedObjects_Fragment_Params.h"

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Replicated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API FFragment_ReplicatedObjects_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_ReplicatedObjects_Params);

    private:
        FCk_ReplicatedObjects _ReplicatedObjects;

    public:
        CK_PROPERTY(_ReplicatedObjects);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ReplicatedObjects_Params, _ReplicatedObjects);
    };
}

// --------------------------------------------------------------------------------------------------------------------