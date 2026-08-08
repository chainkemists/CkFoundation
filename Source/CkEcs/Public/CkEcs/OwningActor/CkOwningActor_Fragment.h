#pragma once

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_OwningActor_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKECS_API FFragment_OwningActor
    {
    public:
        CK_GENERATED_BODY(FFragment_OwningActor);

    public:
        friend class UCk_Utils_OwningActor_UE;

    public:
        FFragment_OwningActor() = default;
        explicit FFragment_OwningActor(
            AActor* InEntityOwningActor);

    private:
        TWeakObjectPtr<AActor> _EntityOwningActor;

    public:
        CK_PROPERTY_GET(_EntityOwningActor);
    };
}

// --------------------------------------------------------------------------------------------------------------------
