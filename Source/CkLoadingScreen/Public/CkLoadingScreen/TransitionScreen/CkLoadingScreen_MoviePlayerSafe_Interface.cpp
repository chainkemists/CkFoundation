#include "CkLoadingScreen_MoviePlayerSafe_Interface.h"

#include "CkCore/Validation/CkIsValid.h"

#include <UObject/Class.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    ICk_LoadingScreen_MoviePlayerSafe::
    Get_IsMoviePlayerSafe(
        const UClass* InWidgetClass)
    -> bool
{
    if (ck::Is_NOT_Valid(InWidgetClass, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    return InWidgetClass->ImplementsInterface(UCk_LoadingScreen_MoviePlayerSafe::StaticClass());
}

// --------------------------------------------------------------------------------------------------------------------
