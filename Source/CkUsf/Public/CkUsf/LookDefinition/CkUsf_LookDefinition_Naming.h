#pragma once

#include "CoreMinimal.h"

namespace ck::usf
{
    // Generated masters live in CkFoundation plugin content under GeneratedLooks/.
    inline auto Get_GeneratedMasterPackageRoot() -> FString
    {
        return TEXT("/CkFoundation/CkUsf/GeneratedLooks");
    }

    // InPackageRootOverride is a TEST seam. A real-RHI automation run is spread over concurrent editors, and
    // two of them saving the same generated .uasset kills both (SavePackage ERROR_ALREADY_EXISTS), so the
    // generation tests write to a lane-unique root instead. An empty override IS the shipped root, so every
    // editor-facing path resolves exactly what it always did.
    inline auto Get_GeneratedMasterPackagePath(const FName InLookName, const FString& InPackageRootOverride = {}) -> FString
    {
        const auto Root = InPackageRootOverride.IsEmpty() ? Get_GeneratedMasterPackageRoot() : InPackageRootOverride;
        return FString::Printf(TEXT("%s/M_CkUsf_Look_%s"), *Root, *InLookName.ToString());
    }

    inline auto Get_GeneratedMasterObjectPath(const FName InLookName, const FString& InPackageRootOverride = {}) -> FString
    {
        const auto Pkg = Get_GeneratedMasterPackagePath(InLookName, InPackageRootOverride);
        return FString::Printf(TEXT("%s.M_CkUsf_Look_%s"), *Pkg, *InLookName.ToString());
    }
}
