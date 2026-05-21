#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"

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
}
