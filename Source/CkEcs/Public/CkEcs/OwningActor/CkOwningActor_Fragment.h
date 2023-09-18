#pragma once

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKECS_API FFragment_OwningActor_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_OwningActor_Current);

    public:
        friend class UCk_Utils_OwningActor_UE;

    public:
        FFragment_OwningActor_Current() = default;
        explicit FFragment_OwningActor_Current(
            AActor* InEntityOwningActor);

    private:
        TWeakObjectPtr<AActor> _EntityOwningActor;

    public:
        CK_PROPERTY_GET(_EntityOwningActor);
    };
}

// --------------------------------------------------------------------------------------------------------------------
