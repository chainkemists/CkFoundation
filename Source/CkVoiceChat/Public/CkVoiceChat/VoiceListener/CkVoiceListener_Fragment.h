#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkVoiceChat/VoiceListener/CkVoiceListener_Fragment_Data.h"

#include <UObject/WeakObjectPtr.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VoiceListener_UE;
class APlayerState;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The local mute set changed and has not been reported upstream yet; the sync processor
    // clears it once the control relay accepted the full set (or applied it directly on a host).
    CK_DEFINE_ECS_TAG(FTag_VoiceListener_MutesDirty);

    // --------------------------------------------------------------------------------------------------------------------

    // Local-machine ears state: per-talker client mute and receive volume. The mute set is also
    // reported upstream as a routing exclusion, so muted audio is never sent at all.
    struct CKVOICECHAT_API FFragment_VoiceListener_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceListener_Current);

    public:
        friend class FProcessor_VoiceListener_HandleRequests;
        friend class FProcessor_VoiceListener_SyncMutes;
        friend class UCk_Utils_VoiceListener_UE;

    private:
        TSet<FCk_Handle> _MutedTalkers;
        TMap<FCk_Handle, float> _TalkerVolumes;

    public:
        CK_PROPERTY_GET(_MutedTalkers);
        CK_PROPERTY_GET(_TalkerVolumes);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVOICECHAT_API FFragment_VoiceListener_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceListener_Requests);

    public:
        friend class FProcessor_VoiceListener_HandleRequests;
        friend class FProcessor_VoiceListener_CancelPendingRequests;
        friend class UCk_Utils_VoiceListener_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_VoiceListener_MuteTalker,
            FCk_Request_VoiceListener_UnmuteTalker,
            FCk_Request_VoiceListener_SetTalkerVolume>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // World-scoped (transient entity): THIS machine's ears. Registered by Add (last added wins -
    // one local listener per world is the contract); the receive path uses it for the local
    // defense-in-depth mute gate.
    struct CKVOICECHAT_API FFragment_VoiceChat_LocalListener
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceChat_LocalListener);

    private:
        FCk_Handle_VoiceListener _Listener;

    public:
        CK_PROPERTY(_Listener);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // One full mute-set report from a player's machine, enqueued by the control relay RPC and
    // applied by the AuthorityOnly control processor. Full-set replace (not deltas): idempotent
    // and ordering-proof over a reliable channel.
    struct CKVOICECHAT_API FCk_VoiceChat_MuteSetUpdate
    {
    public:
        CK_GENERATED_BODY(FCk_VoiceChat_MuteSetUpdate);

    private:
        TWeakObjectPtr<APlayerState> _Player;
        TArray<FCk_Handle> _MutedTalkers;

    public:
        CK_PROPERTY_GET(_Player);
        CK_PROPERTY_GET(_MutedTalkers);

    public:
        CK_DEFINE_CONSTRUCTORS(FCk_VoiceChat_MuteSetUpdate, _Player, _MutedTalkers);
    };

    struct CKVOICECHAT_API FFragment_VoiceChat_ControlInbox
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceChat_ControlInbox);

    private:
        TArray<FCk_VoiceChat_MuteSetUpdate> _Updates;

    public:
        CK_PROPERTY(_Updates);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // World-scoped (transient entity), authority-side: which talkers each player has muted. The
    // routing processor consults it per recipient - the privacy property lives HERE, not in
    // client playback.
    struct CKVOICECHAT_API FFragment_VoiceChat_ListenerMuteMatrix
    {
    public:
        CK_GENERATED_BODY(FFragment_VoiceChat_ListenerMuteMatrix);

    private:
        TMap<TWeakObjectPtr<APlayerState>, TSet<FCk_Handle>> _MutedByPlayer;

    public:
        CK_PROPERTY(_MutedByPlayer);
    };
}

// --------------------------------------------------------------------------------------------------------------------
