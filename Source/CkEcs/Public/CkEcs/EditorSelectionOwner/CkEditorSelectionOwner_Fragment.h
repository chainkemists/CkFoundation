#pragma once

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
namespace ck
{
    // Stamped on an editor-preview root and inherited by every lifetime-descendant at creation, so consumers
    // read it off their own entity — no chain walk. The weak ptr doubles as the per-owner cache identity:
    // it compares/hashes by object index + serial, so lookups survive the owner actor's destruction.
    struct CKECS_API FFragment_EditorSelectionOwner
    {
    public:
        CK_GENERATED_BODY(FFragment_EditorSelectionOwner);

    public:
        FFragment_EditorSelectionOwner() = default;
        explicit FFragment_EditorSelectionOwner(
            const TWeakObjectPtr<AActor>& InOwnerActor);

    private:
        TWeakObjectPtr<AActor> _OwnerActor;

    public:
        CK_PROPERTY_GET(_OwnerActor);
    };
}
#endif

// --------------------------------------------------------------------------------------------------------------------
