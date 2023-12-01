#include "CkLog_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Log_Settings_UE::
    Get_DefaultLoggerName() const
    -> FName
{
    return _DefaultLoggerName;
}

auto
    UCk_Utils_Log_Settings_UE::
    Get_DefaultLoggerName()
    -> FName
{
    return GetDefault<UCk_Log_Settings_UE>()->Get_DefaultLoggerName();
}

// --------------------------------------------------------------------------------------------------------------------
