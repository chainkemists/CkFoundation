#include "CkStateMachine_Debug_Fragment.h"

#include "CkStateMachine/State/EntityScripts/CkSmState_EntityScript.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
#if !UE_BUILD_SHIPPING
    auto
        FCk_SmBreakpoint_TransitionKey::
        operator==(
            const FCk_SmBreakpoint_TransitionKey& InOther) const
        -> bool
    {
        return SourceStateClass == InOther.SourceStateClass
            && TargetStateClass == InOther.TargetStateClass;
    }

    auto
        GetTypeHash(
            const FCk_SmBreakpoint_TransitionKey& InKey)
        -> uint32
    {
        return HashCombine(
            ::GetTypeHash(InKey.SourceStateClass.Get()),
            ::GetTypeHash(InKey.TargetStateClass.Get()));
    }
#endif
}
