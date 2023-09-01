#include "CkLog_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

const FString _DefaultEngineIni = FPaths::ProjectConfigDir() / TEXT("DefaultEngine.ini");

// --------------------------------------------------------------------------------------------------------------------

UCk_Log_Settings_UE::
    UCk_Log_Settings_UE()
{
    if (GConfig->GetString(TEXT("Ck_LoggerSettings"), TEXT("DefaultLoggerName"), _DefaultLoggerName, GEngineIni) == false)
    {
        _DefaultLoggerName = TEXT("CkLogger");
    }
}

#if WITH_EDITOR
auto
    UCk_Log_Settings_UE::
    PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    SaveToIni();
}

auto
    UCk_Log_Settings_UE::
    GetCategoryName() const
    -> FName
{
    return NAME_Game;
}

auto
    UCk_Log_Settings_UE::
    SaveToIni()
    -> void
{
    GConfig->SetString(TEXT("Ck_LoggerSettings"), TEXT("DefaultLoggerName"), *_DefaultLoggerName, _DefaultEngineIni);

    GConfig->Flush(false, _DefaultEngineIni);
    SaveConfig(CPF_Config, *_DefaultEngineIni);
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Log_Settings_UE::
    Get_DefaultLoggerName()
    -> FString
{
    FString defaultLoggerName = TEXT("CkLogger");

    GConfig->GetString(TEXT("Ck_LoggerSettings"), TEXT("DefaultLoggerName"), defaultLoggerName, GEngineIni);

    return defaultLoggerName;
}

// --------------------------------------------------------------------------------------------------------------------
