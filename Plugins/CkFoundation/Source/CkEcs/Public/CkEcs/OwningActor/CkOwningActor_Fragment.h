#pragma once

#include "CkMacros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKECS_API FCk_Fragment_OwningActor_Current
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_OwningActor_Current);

    public:
        friend class UCk_Utils_OwningActor_UE;

    public:
        FCk_Fragment_OwningActor_Current() = default;
        explicit FCk_Fragment_OwningActor_Current(
            AActor* InEntityOwningActor);

    private:
        TWeakObjectPtr<AActor> _EntityOwningActor;

    public:
        CK_PROPERTY_GET(_EntityOwningActor);
    };
}

// --------------------------------------------------------------------------------------------------------------------
