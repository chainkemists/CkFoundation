#include "CkAntNavigation_Utils.h"

#include "CkAntBridge/AntNavigation/CkAntNavigation_Fragment.h"
#include "CkAntBridge/CkAntBridge_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AntNavigation_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_AntNavigation_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> void
{
    InHandle.Add<ck::FFragment_AntNavigation_Params>(InParams);
    InHandle.Add<ck::FFragment_AntNavigation_Current>();

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::ant_bridge::VeryVerbose
        (
            TEXT("Skipping creation of AntNavigation Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return;
    }

    TryAddReplicatedFragment<UCk_Fragment_AntNavigation_Rep>(InHandle);
}

auto
    UCk_Utils_AntNavigation_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_AntNavigation_Current>();
}

auto
    UCk_Utils_AntNavigation_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have AntNavigation"), InHandle)
    { return false; }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
