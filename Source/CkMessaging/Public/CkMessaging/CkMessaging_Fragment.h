#pragma once

#include "CkMessaging_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkEcs/Record/CkRecord_Fragment.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfMessengers, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKMESSAGING_API, Messaging, FCk_Delegate_Messaging_OnBroadcast_MC, FCk_Handle, FGameplayTag, FInstancedStruct);

}

// --------------------------------------------------------------------------------------------------------------------
