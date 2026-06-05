#pragma once

#include "CoreMinimal.h"

namespace ck::usf
{
    // Generated masters live in CkFoundation plugin content under GeneratedLooks/.
    inline auto Get_GeneratedMasterPackagePath(const FName InLookName) -> FString
    {
        return FString::Printf(TEXT("/CkFoundation/CkUsf/GeneratedLooks/M_CkUsf_Look_%s"),
            *InLookName.ToString());
    }

    inline auto Get_GeneratedMasterObjectPath(const FName InLookName) -> FString
    {
        const auto Pkg = Get_GeneratedMasterPackagePath(InLookName);
        return FString::Printf(TEXT("%s.M_CkUsf_Look_%s"), *Pkg, *InLookName.ToString());
    }
}
