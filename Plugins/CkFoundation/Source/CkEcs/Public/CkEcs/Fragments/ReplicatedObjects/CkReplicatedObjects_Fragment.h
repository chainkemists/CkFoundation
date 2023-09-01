#pragma once

#include "CkReplicatedObjects_Fragment_Params.h"

#include "CkMacros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FTag_Replicated {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API FCk_Fragment_ReplicatedObjects_Params
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_ReplicatedObjects_Params);

    public:
        FCk_Fragment_ReplicatedObjects_Params() = default;
        FCk_Fragment_ReplicatedObjects_Params(FCk_ReplicatedObjects InReplicatedObjects);

    private:
        FCk_ReplicatedObjects _ReplicatedObjects;

    public:
        CK_PROPERTY(_ReplicatedObjects);
    };
}
