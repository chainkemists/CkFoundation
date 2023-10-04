#include "CkEntity_ConstructionScript.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Entity_ConstructionScript_PDA::Construct(
        const FCk_Handle& InHandle) -> void
{
    DoConstruct(InHandle);
}

auto
    UCk_Entity_ConstructionScript_PDA::
    DoConstruct_Implementation(
        const FCk_Handle& InHandle)
    -> void
{
    CK_ENSURE_FALSE(TEXT("The function in the BASE class [{}] of [{}] should have been overridden"), ck::TypeToString<ThisType>, this);
}

// --------------------------------------------------------------------------------------------------------------------
