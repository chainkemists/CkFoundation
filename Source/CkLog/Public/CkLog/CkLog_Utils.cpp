#include "CkLog_Utils.h"

#include "CkLog_Settings.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <functional>

namespace ck::log
{
    // --------------------------------------------------------------------------------------------------------------------

    auto
    Get_LogMap()
    -> LoggingMapType&
    {
        static LoggingMapType _LogMap;
        return _LogMap;
    }

    auto
    Get_DisplayMap()
    -> LoggingMapType&
    {
        static LoggingMapType _DisplayMap;
        return _DisplayMap;
    }

    auto
    Get_WarningMap()
    -> LoggingMapType&
    {
        static LoggingMapType _WarningMap;
        return _WarningMap;
    }

    auto
    Get_ErrorMap()
    -> LoggingMapType&
    {
        static LoggingMapType _ErrorMap;
        return _ErrorMap;
    }

    auto
    Get_FatalMap()
    -> LoggingMapType&
    {
        static LoggingMapType _FatalMap;
        return _FatalMap;

    }

    auto
    Get_VerboseMap()
    -> LoggingMapType&
    {
        static LoggingMapType _VerboseMap;
        return _VerboseMap;
    }

    auto
    Get_VeryVerboseMap()
    -> LoggingMapType&
    {
        static LoggingMapType _VeryVerboseMap;
        return _VeryVerboseMap;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
Get_LoggerOrDefault(FName InLogger)
-> FName
{
    if (ck::IsValid(InLogger))
    {
        return InLogger;
    }

    return *UCk_Utils_Log_Settings_UE::Get_DefaultLoggerName();
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Log_Utils_UE::
Log_Fatal(FText InMsg, FName InLogger)
-> void
{
    Log_Fatal_If(true, InMsg, InLogger);
}

auto UCk_Log_Utils_UE::
Log_Error(FText InMsg, FName InLogger)
-> void
{
    Log_Error_If(true, InMsg, InLogger);
}

auto UCk_Log_Utils_UE::
Log_Warning(FText InMsg, FName InLogger)
-> void
{
    Log_Warning_If(true, InMsg, InLogger);
}

auto UCk_Log_Utils_UE::
Log_Display(FText InMsg, FName InLogger)
-> void
{
    Log_Display_If(true, InMsg, InLogger);
}

auto UCk_Log_Utils_UE::
Log(FText InMsg, FName InLogger)
-> void
{
    Log_If(true, InMsg, InLogger);
}

auto UCk_Log_Utils_UE::
Log_Verbose(FText InMsg, FName InLogger)
-> void
{
    Log_Verbose_If(true, InMsg, InLogger);
}

auto UCk_Log_Utils_UE::
Log_VeryVerbose(FText InMsg, FName InLogger)
-> void
{
    Log_VeryVerbose_If(true, InMsg, InLogger);
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Log_Utils_UE::
Log_Fatal_If(bool InExpression, FText InMsg, FName InLogger)
-> ECk_LogResults
{
    if (InExpression)
    {
        DoInvokeLog(ck::log::Get_FatalMap(), InLogger, InMsg);
        return ECk_LogResults::Logged;
    }

    return ECk_LogResults::NotLogged;
}

auto UCk_Log_Utils_UE::
Log_Error_If(bool InExpression, FText InMsg, FName InLogger)
-> ECk_LogResults
{
    if (InExpression)
    {
        DoInvokeLog(ck::log::Get_ErrorMap(), InLogger, InMsg);
        return ECk_LogResults::Logged;
    }

    return ECk_LogResults::NotLogged;
}

auto UCk_Log_Utils_UE::
Log_Warning_If(bool  InExpression, FText InMsg, FName InLogger)
-> ECk_LogResults
{
    if (InExpression)
    {
        DoInvokeLog(ck::log::Get_WarningMap(), InLogger, InMsg);
        return ECk_LogResults::Logged;
    }

    return ECk_LogResults::NotLogged;
}

auto UCk_Log_Utils_UE::
Log_Display_If(bool InExpression, FText InMsg, FName InLogger)
-> ECk_LogResults
{
    if (InExpression)
    {
        DoInvokeLog(ck::log::Get_DisplayMap(), InLogger, InMsg);
        return ECk_LogResults::Logged;
    }

    return ECk_LogResults::NotLogged;
}

auto UCk_Log_Utils_UE::
Log_Verbose_If(bool  InExpression, FText InMsg, FName InLogger)
-> ECk_LogResults
{
    if (InExpression)
    {
        DoInvokeLog(ck::log::Get_VerboseMap(), InLogger, InMsg);
        return ECk_LogResults::Logged;
    }

    return ECk_LogResults::NotLogged;
}

auto UCk_Log_Utils_UE::
Log_VeryVerbose_If(bool  InExpression, FText InMsg, FName InLogger)
-> ECk_LogResults
{
    if (InExpression)
    {
        DoInvokeLog(ck::log::Get_VeryVerboseMap(), InLogger, InMsg);
        return ECk_LogResults::Logged;
    }

    return ECk_LogResults::NotLogged;
}

auto UCk_Log_Utils_UE::
Log_If(bool  InExpression, FText InMsg, FName InLogger)
-> ECk_LogResults
{
    if (InExpression)
    {
        DoInvokeLog(ck::log::Get_LogMap(), InLogger, InMsg);
        return ECk_LogResults::Logged;
    }

    return ECk_LogResults::NotLogged;
}

auto UCk_Log_Utils_UE::
DoInvokeLog(TMap<FName, TFunction<void(const FString&)>>& InMap, FName InLogger, FText InMsg)
-> void
{
    const auto& loggerToUse = Get_LoggerOrDefault(InLogger);

    CK_ENSURE_IF_NOT(InMap.Contains(loggerToUse),
        TEXT("Could not find the Logger [{}]. Are you sure you have defined it? See CkLog.h for an example."),
        loggerToUse)
    { return; }

    // logging the context can be expensive and can optionally be turned off
#if CK_LOG_NO_CONTEXT
    const auto& formatted = ck::Format_UE(TEXT("{}"), InMsg);
#else
    const auto& bpContext = UCk_Utils_Debug_StackTrace_UE::Get_BlueprintContext();
    const auto& formatted = ck::Format_UE(TEXT("{}\n== CONTEXT:[{}] =="), InMsg, bpContext);
#endif

    InMap[loggerToUse](formatted);
}

// --------------------------------------------------------------------------------------------------------------------
