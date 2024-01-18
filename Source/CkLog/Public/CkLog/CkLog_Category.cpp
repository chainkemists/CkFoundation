#include "CkLog_Category.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_LogCategory::
    FCk_LogCategory(
        FName InName)
    : _Name(InName)
{
}

auto
    FCk_LogCategory::
    IsValid() const
    -> bool
{
    return _Name != NAME_None;
}

auto
    FCk_LogCategory::
    Get_Name() const
    -> FName
{
    return _Name;
}

// --------------------------------------------------------------------------------------------------------------------
