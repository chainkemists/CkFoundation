#include "CkEcsEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EcsEnsure_UE::
    EnsureMsgf_IsValid(
        const FCk_Handle& InHandle,
        FText InMsg,
        ECk_ValidInvalid& OutHitStatus,
        const UObject* InContext)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("{}.{}"), InMsg, ck::Context(InContext))
    {
        OutHitStatus = ECk_ValidInvalid::Invalid;
        return;
    }

    OutHitStatus = ECk_ValidInvalid::Valid;
}

// --------------------------------------------------------------------------------------------------------------------

#undef CK_ENSURE_BP
