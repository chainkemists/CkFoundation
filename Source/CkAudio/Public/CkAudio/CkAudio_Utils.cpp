#include "CkAudio_Utils.h"

#include "CkAudio/CkAudio_Fragment.h"
#include "CkAudio/CkAudio_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Audio_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Audio_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_Audio
{
    InHandle.Add<ck::FFragment_Audio_Params>(InParams);
    InHandle.Add<ck::FFragment_Audio_Current>();

    InHandle.Add<ck::FTag_Audio_RequiresSetup>();

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::audio::VeryVerbose
        (
            TEXT("Skipping creation of Audio Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return Cast(InHandle);
    }

    UCk_Utils_Net_UE::TryAddReplicatedFragment<UCk_Fragment_Audio_Rep>(InHandle);
    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Audio_UE, FCk_Handle_Audio,
    ck::FFragment_Audio_Params, ck::FFragment_Audio_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Audio_UE::
    Request_ExampleRequest(
        FCk_Handle_Audio& InAudio,
        const FCk_Request_Audio_ExampleRequest& InRequest)
    -> FCk_Handle_Audio
{
    InAudio.AddOrGet<ck::FFragment_Audio_Requests>()._Requests.Emplace(InRequest);
    return InAudio;
}

// --------------------------------------------------------------------------------------------------------------------