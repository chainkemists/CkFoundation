#include "CkLog_Utils.h"

#include "CkLog/CkLog.h"
#include "CkLog/CkLog_Category.h"

#include <functional>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::log
{
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

    auto
        Get_AllRegisteredCategories()
        -> TArray<FName>
    {
        TArray<FName> OutCategoryNames;
        Get_LogMap().GetKeys(OutCategoryNames);

        return OutCategoryNames;
    }

    // NOTE: Copied over from CkDebug_Utils.h to avoid dependency on CkCore
    auto
        Get_StackTrace_Blueprint()
        -> TArray<FString>
    {
        auto StackTrace = TArray<FString>{};

    #if !CK_DISABLE_STACK_TRACE
        const auto* BlueprintExceptionTracker = FBlueprintContextTracker::TryGet();

        if (BlueprintExceptionTracker == nullptr)
        { return StackTrace; }

        const auto& RawStack = BlueprintExceptionTracker->GetCurrentScriptStack();
        for (int32 FrameIdx = RawStack.Num() - 1; FrameIdx >= 0; --FrameIdx)
        {
            FStringBuilderBase StringBuilder;
            RawStack[FrameIdx]->GetStackDescription(StringBuilder);
            StackTrace.Emplace(StringBuilder.ToString());
        }
    #endif

        return StackTrace;
    }

    // NOTE: Copied over from CkDebug_Utils.h to avoid dependency on CkCore
    auto
        Get_BlueprintContext()
        -> TOptional<FString>
    {
        const auto& trace = Get_StackTrace_Blueprint();

        return trace.Num() > 0 ? trace.Last() : TOptional<FString>{};
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Log_Utils_UE::
    Log_Fatal(
        FText InMsg,
       FCk_LogCategory InLogCategory)
    -> void
{
    Log_Fatal_If(true, InMsg, InLogCategory);
}

auto
    UCk_Log_Utils_UE::
    Log_Error(
        FText InMsg,
       FCk_LogCategory InLogCategory)
    -> void
{
    Log_Error_If(true, InMsg, InLogCategory);
}

auto
    UCk_Log_Utils_UE::
    Log_Warning(
        FText InMsg,
       FCk_LogCategory InLogCategory)
    -> void
{
    Log_Warning_If(true, InMsg, InLogCategory);
}

auto
    UCk_Log_Utils_UE::
    Log_Display(
        FText InMsg,
       FCk_LogCategory InLogCategory)
    -> void
{
    Log_Display_If(true, InMsg, InLogCategory);
}

auto
    UCk_Log_Utils_UE::
    Log(
        FText InMsg,
       FCk_LogCategory InLogCategory)
    -> void
{
    Log_If(true, InMsg, InLogCategory);
}

auto
    UCk_Log_Utils_UE::
    Log_Verbose(
        FText InMsg,
       FCk_LogCategory InLogCategory)
    -> void
{
    Log_Verbose_If(true, InMsg, InLogCategory);
}

auto
    UCk_Log_Utils_UE::
    Log_VeryVerbose(
        FText InMsg,
       FCk_LogCategory InLogCategory)
    -> void
{
    Log_VeryVerbose_If(true, InMsg, InLogCategory);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Log_Utils_UE::
    Log_Fatal_If(
        bool InExpression,
        FText InMsg,
        FCk_LogCategory InLogCategory)
    -> ECk_LogResults
{
    if (InExpression)
    {
        DoInvokeLog(ck::log::Get_FatalMap(), InLogCategory.Get_Name(), InMsg);
        return ECk_LogResults::Logged;
    }

    return ECk_LogResults::NotLogged;
}

auto
    UCk_Log_Utils_UE::
    Log_Error_If(
        bool InExpression,
        FText InMsg,
        FCk_LogCategory InLogCategory)
    -> ECk_LogResults
{
    if (InExpression)
    {
        DoInvokeLog(ck::log::Get_ErrorMap(), InLogCategory.Get_Name(), InMsg);
        return ECk_LogResults::Logged;
    }

    return ECk_LogResults::NotLogged;
}

auto
    UCk_Log_Utils_UE::
    Log_Warning_If(
        bool InExpression,
        FText InMsg,
        FCk_LogCategory InLogCategory)
    -> ECk_LogResults
{
    if (InExpression)
    {
        DoInvokeLog(ck::log::Get_WarningMap(), InLogCategory.Get_Name(), InMsg);
        return ECk_LogResults::Logged;
    }

    return ECk_LogResults::NotLogged;
}

auto
    UCk_Log_Utils_UE::
    Log_Display_If(
        bool InExpression,
        FText InMsg,
        FCk_LogCategory InLogCategory)
    -> ECk_LogResults
{
    if (InExpression)
    {
        DoInvokeLog(ck::log::Get_DisplayMap(), InLogCategory.Get_Name(), InMsg);
        return ECk_LogResults::Logged;
    }

    return ECk_LogResults::NotLogged;
}

auto
    UCk_Log_Utils_UE::
    Log_Verbose_If(
        bool InExpression,
        FText InMsg,
        FCk_LogCategory InLogCategory)
    -> ECk_LogResults
{
    if (InExpression)
    {
        DoInvokeLog(ck::log::Get_VerboseMap(), InLogCategory.Get_Name(), InMsg);
        return ECk_LogResults::Logged;
    }

    return ECk_LogResults::NotLogged;
}

auto
    UCk_Log_Utils_UE::
    Log_VeryVerbose_If(
        bool InExpression,
        FText InMsg,
        FCk_LogCategory InLogCategory)
    -> ECk_LogResults
{
    if (InExpression)
    {
        DoInvokeLog(ck::log::Get_VeryVerboseMap(), InLogCategory.Get_Name(), InMsg);
        return ECk_LogResults::Logged;
    }

    return ECk_LogResults::NotLogged;
}

auto
    UCk_Log_Utils_UE::
    Log_If(
        bool InExpression,
        FText InMsg,
        FCk_LogCategory InLogCategory)
    -> ECk_LogResults
{
    if (InExpression)
    {
        DoInvokeLog(ck::log::Get_LogMap(), InLogCategory.Get_Name(), InMsg);
        return ECk_LogResults::Logged;
    }

    return ECk_LogResults::NotLogged;
}

auto
    UCk_Log_Utils_UE::
    DoInvokeLog(
        TMap<FName, TFunction<void(const FString&)>>& InMap,
        FName InLogger,
        FText InMsg)
    -> void
{
    const auto& LoggerToUse = InLogger;

    if (InMap.Contains(LoggerToUse) == false)
    {
        UE_LOG(CkLogger, Error, TEXT("Could not find the Logger [%s]. Are you sure you have defined it? See CkLog.h for an example."), *LoggerToUse.ToString());
        return;
    }

    // logging the context can be expensive and can optionally be turned off
#if CK_LOG_NO_CONTEXT
    const auto& Formatted = InMsg.ToString();
#else
    const auto& BpContext = ck::log::Get_BlueprintContext();
    const auto& Formatted = FString::Printf(TEXT("%s\n== CONTEXT:[%s] =="), *InMsg.ToString(), *BpContext.Get(TEXT("")));
#endif

    InMap[LoggerToUse](Formatted);
}

// --------------------------------------------------------------------------------------------------------------------
