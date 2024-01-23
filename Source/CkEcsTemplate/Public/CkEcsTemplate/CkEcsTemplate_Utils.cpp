#include "CkEcsTemplate_Utils.h"

#include "CkEcsTemplate/CkEcsTemplate_Fragment.h"
#include "CkEcsTemplate/CkEcsTemplate_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EcsTemplate_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_EcsTemplate_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> void
{
    InHandle.Add<ck::FFragment_EcsTemplate_Params>(InParams);
    InHandle.Add<ck::FFragment_EcsTemplate_Current>();

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::ecs_template::VeryVerbose
        (
            TEXT("Skipping creation of EcsTemplate Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return;
    }

    TryAddReplicatedFragment<UCk_Fragment_EcsTemplate_Rep>(InHandle);
}

auto
    UCk_Utils_EcsTemplate_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_EcsTemplate_Current>();
}

auto
    UCk_Utils_EcsTemplate_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have EcsTemplate"), InHandle)
    { return false; }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
