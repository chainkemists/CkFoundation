#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"

#include "CkEcs/Net/CkNet_Utils.h"

namespace ck::statemachine
{
    auto
        ComputeNetContext(
            const FCk_Handle_StateMachine& InSm)
        -> ECk_Sm_NetContext
    {
        const auto NetMode = UCk_Utils_Net_UE::Get_EntityNetMode(InSm);

        // Unknown means the entity has no net fragment — no active net session.
        if (NetMode == ECk_Net_NetModeType::Unknown)
        { return ECk_Sm_NetContext::Standalone; }

        const auto bHasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InSm);

        if (bHasAuthority)
        { return ECk_Sm_NetContext::Server; }

        const auto LocallyControlled = UCk_Utils_Net_UE::Get_IsEntityLocallyControlled_ByPlayer(InSm);

        if (LocallyControlled == ECk_Utils_Net_IsLocallyControlled_Result::IsLocallyControlled)
        { return ECk_Sm_NetContext::OwningClient; }

        return ECk_Sm_NetContext::NonOwningClient;
    }
}
