#include "CkMontagePlayer_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MontagePlayer_State::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_Montage() == InOther.Get_Montage() &&
        Get_SectionName() == InOther.Get_SectionName() &&
        Get_StartPosition() == InOther.Get_StartPosition() &&
        FMath::IsNearlyEqual(Get_PlayRate(), InOther.Get_PlayRate()) &&
        Get_BlendInTime() == InOther.Get_BlendInTime() &&
        Get_BlendOutTime() == InOther.Get_BlendOutTime() &&
        Get_ServerStartTime() == InOther.Get_ServerStartTime() &&
        Get_PlayInstanceId() == InOther.Get_PlayInstanceId() &&
        Get_Kind() == InOther.Get_Kind();
}

// --------------------------------------------------------------------------------------------------------------------
