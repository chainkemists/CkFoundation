#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <UObject/ObjectKey.h>

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
namespace ck
{
    // Stamped by UCk_Utils_LiveTune_UE::Link: records which (tuning asset, member property) each linked
    // feature on this entity was seeded from, so an editor-time asset edit can be traced back to the
    // entities it seeded. Editor-only change transport — nothing reads this at runtime. One entry per
    // Link call, because features that stamp their fragments directly on the target share one entity.
    struct CKECS_API FFragment_LiveTune_Stamp
    {
    public:
        CK_GENERATED_BODY(FFragment_LiveTune_Stamp);

    public:
        struct FEntry
        {
            FObjectKey _Asset;
            FName _Member;

            auto operator==(const FEntry&) const -> bool = default;
        };

    private:
        TArray<FEntry> _Entries;

    public:
        CK_PROPERTY_GET(_Entries);

    private:
        friend class UCk_LiveTune_Subsystem_UE;
    };
}
#endif

// --------------------------------------------------------------------------------------------------------------------
