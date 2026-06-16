#include "CkIskmRenderer/Notify/CkIskmNotify_AnimInstance_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkIskmRenderer/Notify/CkIskmNotify_AnimInstance.h"

auto
    UCk_Utils_IskmNotify_UE::
    Get_OwningProxyHandle(
        const UCk_IskmNotify_AnimInstance* InAnimInstance)
    -> FCk_Handle_IskmProxy
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAnimInstance, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Invalid AnimInstance passed to [Ck][IskmNotify] Get Owning Proxy Handle"))
    { return {}; }

    return InAnimInstance->_OwningHandle;
}

auto
    UCk_Utils_IskmNotify_UE::
    Get_OwningEntity(
        const UCk_IskmNotify_AnimInstance* InAnimInstance)
    -> FCk_Handle
{
    // FCk_Handle_IskmProxy derives from FCk_Handle (via FCk_Handle_TypeSafe), so the
    // proxy handle slices to the base entity handle the proxy is composed on.
    return Get_OwningProxyHandle(InAnimInstance);
}
