#include "CkString_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_String_UE::
    Get_Skel_Prefix()
    -> FName
{
    static FName SkelPrefix = FName(TEXT("SKEL_"));
    return SkelPrefix;
}

auto
    UCk_Utils_String_UE::
    Get_Reinst_Prefix()
    -> FName
{
    static FName ReinstPrefix = FName(TEXT("REINST_"));
    return ReinstPrefix;
}

auto
    UCk_Utils_String_UE::
    Get_Deadclass_Prefix()
    -> FName
{
    static FName DeadclassPrefix = FName(TEXT("DEADCLASS_"));
    return DeadclassPrefix;
}

auto
    UCk_Utils_String_UE::
    Get_CompiledFromBlueprint_Suffix()
    -> FName
{
    static FName CompiledFromBlueprintSuffix = FName(TEXT("_C"));
    return CompiledFromBlueprintSuffix;
}

auto
    UCk_Utils_String_UE::
    Get_InvalidName()
    -> FName
{
    return NAME_None;
}

// --------------------------------------------------------------------------------------------------------------------
