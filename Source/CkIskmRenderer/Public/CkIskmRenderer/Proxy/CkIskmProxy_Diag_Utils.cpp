#include "CkIskmRenderer/Proxy/CkIskmProxy_Diag_Utils.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment.h"

auto
    UCk_Utils_IskmProxy_Diag_UE::
    Get_AnimInstance(
        const FCk_Handle_IskmProxy& InHandle)
    -> UAnimInstance*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Diag::Get_AnimInstance: invalid IskmProxy handle"))
    { return nullptr; }

    auto* SKMC = InHandle.Get<ck::FFragment_IskmProxy_Current>().Get_BaseSKMC().Get();
    CK_ENSURE_IF_NOT(ck::IsValid(SKMC),
        TEXT("Diag::Get_AnimInstance: BaseSKMC missing for proxy [{}] — Setup did not complete."),
        InHandle)
    { return nullptr; }

    // Returning null here is legitimate: the SKMC may be in single-node mode
    // with no AnimInstance class set (PlayAnimation's transient state between
    // SetAnimInstanceClass(nullptr) and PlayAnimation completing). Tests treat
    // a stable non-null pointer across re-issues as the success signal.
    return SKMC->GetAnimInstance();
}
