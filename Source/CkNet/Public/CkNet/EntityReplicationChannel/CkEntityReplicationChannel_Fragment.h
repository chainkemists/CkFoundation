#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkNet/EntityReplicationChannel/CkEntityReplicationChannel_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityReplicationChannelOwner_UE;

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_EntityReplicationChannel);
    CK_DEFINE_ECS_TAG(FTag_EntityReplicationChannel_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKNET_API FFragment_EntityReplicationChannelOwner
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityReplicationChannelOwner);

        friend class UCk_Utils_EntityReplicationChannelOwner_UE;

    private:
        int32 _NextAvailableEcsChannelIndex = 0;

    public:
        CK_PROPERTY_GET(_NextAvailableEcsChannelIndex);
    };


    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfEntityReplicationChannels, FCk_Handle_EntityReplicationChannel);
}

// --------------------------------------------------------------------------------------------------------------------
