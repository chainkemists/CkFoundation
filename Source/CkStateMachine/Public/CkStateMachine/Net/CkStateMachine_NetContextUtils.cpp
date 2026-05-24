#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "Engine/World.h"

namespace ck::statemachine
{
    auto
        ComputeNetContext(
            const FCk_Handle_StateMachine& InSm)
        -> ECk_Sm_NetContext
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InSm);

        if (ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}) && World->IsNetMode(NM_Standalone))
        { return ECk_Sm_NetContext::Standalone; }

        if (UCk_Utils_Net_UE::Get_HasAuthority(InSm))
        { return ECk_Sm_NetContext::Server; }

        const auto LocallyControlled = UCk_Utils_Net_UE::Get_IsEntityLocallyControlled_ByPlayer(InSm);

        if (LocallyControlled == ECk_Utils_Net_IsLocallyControlled_Result::IsLocallyControlled)
        { return ECk_Sm_NetContext::OwningClient; }

        return ECk_Sm_NetContext::NonOwningClient;
    }

    auto
        MirrorRunStatus(
            FCk_Handle& InEntity,
            ECk_SmRunStatus InNewStatus)
        -> void
    {
        if (NOT InEntity.Has<ck::FFragment_Sm_Current>())
        { return; }

        auto& Current = InEntity.Get<ck::FFragment_Sm_Current>();
        const auto OldStatus = Current.Get_RunStatus();
        if (OldStatus == InNewStatus)
        { return; }

        Current.Set_RunStatus(InNewStatus);

        auto SmHandle = UCk_Utils_StateMachine_UE::CastChecked(InEntity);

        switch (InNewStatus)
        {
            case ECk_SmRunStatus::Running:
            {
                InEntity.AddOrGet<ck::FTag_Sm_Running>();
                InEntity.Try_Remove<ck::FTag_Sm_Paused>();

                if (OldStatus == ECk_SmRunStatus::Stopped)
                {
                    ck::UUtils_Signal_OnSmStarted::Broadcast(SmHandle,
                        ck::MakePayload(SmHandle, FCk_Sm_Payload_OnStarted{}));
                }
                break;
            }
            case ECk_SmRunStatus::Paused:
            {
                InEntity.AddOrGet<ck::FTag_Sm_Paused>();
                break;
            }
            case ECk_SmRunStatus::Stopped:
            {
                InEntity.Try_Remove<ck::FTag_Sm_Running>();
                InEntity.Try_Remove<ck::FTag_Sm_Paused>();

                ck::UUtils_Signal_OnSmStopped::Broadcast(SmHandle,
                    ck::MakePayload(SmHandle, FCk_Sm_Payload_OnStopped{}));
                break;
            }
        }

        ck::sm::VeryVerbose(TEXT("Mirrored RunStatus on [{}]: [{}] -> [{}]"),
            InEntity, OldStatus, InNewStatus);
    }
}
