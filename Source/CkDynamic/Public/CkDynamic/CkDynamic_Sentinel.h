#pragma once

#include "CkCore/Macros/CkMacros.h"   // NOT

#include <InstancedStruct.h>

// --------------------------------------------------------------------------------------------------------------------

class UScriptStruct;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::dynamic
{
    // Per-type, default-initialized FInstancedStruct handed back on script error paths so AngelScript receives a
    // validly-typed-but-empty struct rather than garbage. Shared by UCk_Utils_DynamicFragment_UE::Get_Fragment and
    // the script-query batch's Get (a stashed-batch / removed-fragment / wrong-type access returns this).
    //
    // inline => the static sentinel map is a single instance shared across every translation unit that includes this,
    // so two callers requesting the sentinel for the same struct get the same object.
    //
    // NOTE: UCk_Utils_DynamicFragment_UE::Get_Fragment still carries its own anonymous-namespace copy of this
    // helper; consolidate both onto this header when the hot Get_Fragment path can be touched safely.
    inline auto
        Get_InvalidSentinel_FragmentData(
            const UScriptStruct* InStructType)
        -> FInstancedStruct&
    {
        static TMap<const UScriptStruct*, FInstancedStruct> Sentinels;
        auto& Instance = Sentinels.FindOrAdd(InStructType);
        if (NOT Instance.IsValid() || Instance.GetScriptStruct() != InStructType)
        {
            Instance.InitializeAs(InStructType);
        }
        return Instance;
    }
}
