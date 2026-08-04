#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkResourceLoader/CkResourceLoader_Fragment_Data.h"

#include "CkVoiceChat/VoiceChannel/CkVoiceChannel_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VoiceChannel_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_VoiceChannel_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_VoiceChannel_NeedsIdx);
    CK_DEFINE_ECS_TAG(FTag_VoiceChannel_PendingAssetLoad);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_VoiceChannel_Params = FCk_Fragment_VoiceChannel_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVOICECHAT_API FFragment_VoiceChannel_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceChannel_Current);

    public:
        friend class FProcessor_VoiceChannel_Setup;
        friend class FProcessor_VoiceChannel_AssignIdx;
        friend class FProcessor_VoiceChannel_HandleRequests;
        friend class UCk_Utils_VoiceChannel_UE;

    private:
        // Unassigned until the authority-side allocator runs; real indices replicate to clients
        // via the control plane. u8 caps the session at 255 channels (wire header budget) - the
        // allocator ensures on exhaustion, indices are never reused.
        uint8 _ChannelIdx = VoiceChannel_UnassignedIdx;

        TMap<FCk_Handle, FCk_VoiceChat_MemberFlags> _Members;
        TSet<FCk_Handle> _ServerMuted;

        // The batch's streamable handle is the GC root for the resolved assets below; it releases
        // via the fragment's destruction on entity teardown (all machines - EndPlay is
        // authority-only, playback resolution is not).
        FCk_ResourceLoader_RootedAssetBatch _LoadedAudioAssets;
        TWeakObjectPtr<USoundAttenuation> _ResolvedAttenuation;
        TWeakObjectPtr<USoundEffectSourcePresetChain> _ResolvedSourceEffectChain;

    public:
        CK_PROPERTY_GET(_ChannelIdx);
        CK_PROPERTY_GET(_Members);
        CK_PROPERTY_GET(_ServerMuted);
        CK_PROPERTY_GET(_ResolvedAttenuation);
        CK_PROPERTY_GET(_ResolvedSourceEffectChain);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVOICECHAT_API FFragment_VoiceChannel_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceChannel_Requests);

    public:
        friend class FProcessor_VoiceChannel_HandleRequests;
        friend class FProcessor_VoiceChannel_CancelPendingRequests;
        friend class UCk_Utils_VoiceChannel_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_VoiceChannel_Join,
            FCk_Request_VoiceChannel_Leave,
            FCk_Request_VoiceChannel_SetMemberFlags,
            FCk_Request_VoiceChannel_ServerMute,
            FCk_Request_VoiceChannel_ServerUnmute>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // World-scoped (transient entity), authority-side channel index allocator. Session-append-only:
    // a destroyed channel's index is retired, never reused - a stale wire ChannelIdx must resolve
    // to nothing rather than to a different channel.
    struct CKVOICECHAT_API FCk_VoiceChat_ChannelRegistryEntry
    {
    public:
        CK_GENERATED_BODY(FCk_VoiceChat_ChannelRegistryEntry);

    private:
        FGameplayTag _ChannelName;
        FCk_Handle_VoiceChannel _Channel;
        uint8 _ChannelIdx = VoiceChannel_UnassignedIdx;

    public:
        CK_PROPERTY_GET(_ChannelName);
        CK_PROPERTY_GET(_Channel);
        CK_PROPERTY_GET(_ChannelIdx);

    public:
        CK_DEFINE_CONSTRUCTORS(FCk_VoiceChat_ChannelRegistryEntry, _ChannelName, _Channel, _ChannelIdx);
    };

    struct CKVOICECHAT_API FFragment_VoiceChat_ChannelRegistry
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceChat_ChannelRegistry);

    public:
        friend class FProcessor_VoiceChannel_AssignIdx;
        friend class UCk_Utils_VoiceChannel_UE;

    private:
        TArray<FCk_VoiceChat_ChannelRegistryEntry> _Entries;
        int32 _NextIdx = 0;

    public:
        CK_PROPERTY_GET(_Entries);
        CK_PROPERTY_GET(_NextIdx);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // TRANSIENT: channels are runtime net state; nothing voice-related is ever saved.
    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfVoiceChannels, FCk_Handle_VoiceChannel);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVOICECHAT_API, OnVoiceChannel_MemberJoined, FCk_Delegate_VoiceChannel_Membership, FCk_Handle_VoiceChannel, FCk_Handle_VoiceTalker);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVOICECHAT_API, OnVoiceChannel_MemberLeft, FCk_Delegate_VoiceChannel_Membership, FCk_Handle_VoiceChannel, FCk_Handle_VoiceTalker);
}

// --------------------------------------------------------------------------------------------------------------------
