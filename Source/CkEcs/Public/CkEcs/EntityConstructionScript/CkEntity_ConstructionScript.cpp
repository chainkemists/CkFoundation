#include "CkEntity_ConstructionScript.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Handle/CkHandle.h"

// this is for debugging only
// TODO: wrap in pre-processor macro to compile this out
struct NAME_ConstructionScript
{
    FText _Name;
};

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityBridge_ConstructionScript_PDA::Construct(
        FCk_Handle InHandle) -> void
{

    InHandle.Add<NAME_ConstructionScript>(UCk_Utils_Debug_UE::Get_DebugName_AsText(this, ECk_DebugName_Verbosity::ShortName));
    DoConstruct(InHandle);
}

auto
    UCk_EntityBridge_ConstructionScript_PDA::
    DoConstruct_Implementation(
        const FCk_Handle& InHandle)
    -> void
{
    CK_TRIGGER_ENSURE(TEXT("The function DoConstruct in the BASE class [{}] of [{}] should have been overridden"),
        ck::Get_RuntimeTypeToString<ThisType>(), this);
}

// --------------------------------------------------------------------------------------------------------------------
