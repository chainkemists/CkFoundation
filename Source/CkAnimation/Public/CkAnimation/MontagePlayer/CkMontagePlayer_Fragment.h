#pragma once

#include "CkMontagePlayer_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include <Serialization/Archive.h>
#include <UObject/SoftObjectPath.h>

#include "CkMontagePlayer_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_MontagePlayer_UE;

namespace ck { class FSnapshotContext; }

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_MontagePlayer_MayRequireReplication);
    CK_DEFINE_ECS_TAG(FTag_MontagePlayer_HasActiveMontage);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANIMATION_API FFragment_MontagePlayer_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_MontagePlayer_Params);

    public:
        friend class ::UCk_Utils_MontagePlayer_UE; // Request_RebindSkeletalMeshComponent replaces _Params post-restore

    public:
        using ParamsType = FCk_Fragment_MontagePlayer_ParamsData;

    public:
        using IsSnapshotable = void;

        // Tier-C with an EMPTY payload: the only param is a runtime SkeletalMeshComponent weak ptr
        // that cannot round-trip a save. The fragment itself must still be snapshotted — Has()/Cast
        // are keyed on it — so restored MontagePlayers come back composed but with the mesh UNSET:
        // server-side playback is inert until something re-binds it (FProcessor_MontagePlayer_
        // HandleRequests ensures on a missing mesh). Client-side playback is unaffected (clients
        // compose via their own Construct).
        auto SerializeSnapshot(FArchive& /*InAr*/, ck::FSnapshotContext& /*InCtx*/) -> void {}

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_MontagePlayer_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANIMATION_API FFragment_MontagePlayer_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_MontagePlayer_Current);

    public:
        friend class FProcessor_MontagePlayer_HandleRequests;
        friend class FProcessor_MontagePlayer_MonitorAnimInstance;
        friend class FProcessor_MontagePlayer_Replicate;
        friend class ::UCk_Utils_MontagePlayer_UE;

    public:
        using IsSnapshotable = void;

        // Tier-C: _State only — the weak AnimInstance/montage pointers and the catch-up countdown
        // are live playback wiring, re-established on each world by the replication dispatch. The
        // montage persists by SOFT PATH (skip-if-missing on load); note the loaded TObjectPtr is
        // not GC-visible from entt storage, same as live gameplay state. _ServerStartTime is
        // restored verbatim — across a reload the new world's clock restarts, so client catch-up
        // sees a non-positive elapsed and plays from StartPosition (no fast-forward). True
        // "resume at saved playback position" is a pending design call with the Lead.
        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& /*InCtx*/) -> void
        {
            auto MontagePath = FString{};
            auto SectionName = FName{};
            auto StartPositionSeconds = 0.0;
            auto PlayRate = 1.0f;
            auto BlendInSeconds = 0.0;
            auto BlendOutSeconds = 0.0;
            auto ServerStartSeconds = 0.0;
            auto PlayInstanceId = int32{0};
            auto KindByte = uint8{0};

            if (InAr.IsSaving())
            {
                MontagePath = FSoftObjectPath{_State.Get_Montage()}.ToString();
                SectionName = _State.Get_SectionName();
                StartPositionSeconds = _State.Get_StartPosition().Get_Seconds();
                PlayRate = _State.Get_PlayRate();
                BlendInSeconds = _State.Get_BlendInTime().Get_Seconds();
                BlendOutSeconds = _State.Get_BlendOutTime().Get_Seconds();
                ServerStartSeconds = _State.Get_ServerStartTime().Get_Seconds();
                PlayInstanceId = _State.Get_PlayInstanceId();
                KindByte = static_cast<uint8>(_State.Get_Kind());
            }

            InAr << MontagePath;
            InAr << SectionName;
            InAr << StartPositionSeconds;
            InAr << PlayRate;
            InAr << BlendInSeconds;
            InAr << BlendOutSeconds;
            InAr << ServerStartSeconds;
            InAr << PlayInstanceId;
            InAr << KindByte;

            if (InAr.IsLoading())
            {
                auto Montage = static_cast<UAnimMontage*>(nullptr);
                if (NOT MontagePath.IsEmpty())
                { Montage = Cast<UAnimMontage>(FSoftObjectPath{MontagePath}.TryLoad()); }

                _State = FCk_MontagePlayer_State{Montage};
                _State
                    .Set_SectionName(SectionName)
                    .Set_StartPosition(FCk_Time{StartPositionSeconds})
                    .Set_PlayRate(PlayRate)
                    .Set_BlendInTime(FCk_Time{BlendInSeconds})
                    .Set_BlendOutTime(FCk_Time{BlendOutSeconds})
                    .Set_ServerStartTime(FCk_Time{ServerStartSeconds})
                    .Set_PlayInstanceId(PlayInstanceId)
                    .Set_Kind(static_cast<ECk_MontagePlayer_StateKind>(KindByte));
            }
        }

    private:
        FCk_MontagePlayer_State _State;

        TWeakObjectPtr<UAnimMontage> _ActiveMontage;
        TWeakObjectPtr<UAnimInstance> _LastSeenAnimInstance;

        // Catch-up countdown — when > 0 the AnimInstance is running at adjusted PlayRate for
        // the next N real-seconds, then restored to 1.0.
        FCk_Time _CatchUpRemaining;

    public:
        CK_PROPERTY_GET(_State);
        CK_PROPERTY_GET(_ActiveMontage);
        CK_PROPERTY_GET(_LastSeenAnimInstance);
        CK_PROPERTY_GET(_CatchUpRemaining);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_MontagePlayer_Current, _State);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANIMATION_API FFragment_MontagePlayer_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_MontagePlayer_Requests);

    public:
        friend class FProcessor_MontagePlayer_HandleRequests;
        friend class ::UCk_Utils_MontagePlayer_UE;

    public:
        using PlayType = FCk_Request_MontagePlayer_Play;
        using StopType = FCk_Request_MontagePlayer_Stop;
        using PauseType = FCk_Request_MontagePlayer_Pause;
        using ResumeType = FCk_Request_MontagePlayer_Resume;
        using JumpType = FCk_Request_MontagePlayer_JumpToSection;

        using RequestType = std::variant<PlayType, StopType, PauseType, ResumeType, JumpType>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKANIMATION_API,
        MontagePlayer_OnStarted,
        FCk_Delegate_MontagePlayer_OnStarted,
        FCk_Handle_MontagePlayer,
        FCk_MontagePlayer_State);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKANIMATION_API,
        MontagePlayer_OnFinished,
        FCk_Delegate_MontagePlayer_OnFinished,
        FCk_Handle_MontagePlayer,
        FCk_MontagePlayer_State,
        ECk_MontagePlayer_FinishReason);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_MontagePlayer_Requests);
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKANIMATION_API FCk_RepData_MontagePlayer
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_MontagePlayer);

    UPROPERTY()
    FCk_MontagePlayer_State Value;
};

namespace ck
{
    using FFragment_ContainerRef_MontagePlayer = TFragment_ContainerEntryRef<FCk_RepData_MontagePlayer>;
}

// --------------------------------------------------------------------------------------------------------------------
