#include "CkLog_Utils.h"

#include "CkLog/CkLog_Module.h"
#include "CkLog/CkLog_Category.h"

#include <functional>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::log
{
    auto
        Get_LogMap()
        -> InvokeLogMapType&
    {
        static InvokeLogMapType LogMap;
        return LogMap;
    }

    auto
        Get_DisplayMap()
        -> InvokeLogMapType&
    {
        static InvokeLogMapType DisplayMap;
        return DisplayMap;
    }

    auto
        Get_WarningMap()
        -> InvokeLogMapType&
    {
        static InvokeLogMapType WarningMap;
        return WarningMap;
    }

    auto
        Get_ErrorMap()
        -> InvokeLogMapType&
    {
        static InvokeLogMapType ErrorMap;
        return ErrorMap;
    }

    auto
        Get_FatalMap()
        -> InvokeLogMapType&
    {
        static InvokeLogMapType FatalMap;
        return FatalMap;
    }

    auto
        Get_VerboseMap()
        -> InvokeLogMapType&
    {
        static InvokeLogMapType VerboseMap;
        return VerboseMap;
    }

    auto
        Get_VeryVerboseMap()
        -> InvokeLogMapType&
    {
        static InvokeLogMapType VeryVerboseMap;
        return VeryVerboseMap;
    }

    auto
        Get_LogMap_IsActive()
        -> IsLogActiveMapType&
    {
        static IsLogActiveMapType LogMap;
        return LogMap;
    }

    auto
        Get_DisplayMap_IsActive()
        -> IsLogActiveMapType&
    {
        static IsLogActiveMapType DisplayMap;
        return DisplayMap;
    }

    auto
        Get_WarningMap_IsActive()
        -> IsLogActiveMapType&
    {
        static IsLogActiveMapType WaningMap;
        return WaningMap;
    }

    auto
        Get_ErrorMap_IsActive()
        -> IsLogActiveMapType&
    {
        static IsLogActiveMapType ErrorMap;
        return ErrorMap;
    }

    auto
        Get_FatalMap_IsActive()
        -> IsLogActiveMapType&
    {
        static IsLogActiveMapType FatalMap;
        return FatalMap;
    }

    auto
        Get_VerboseMap_IsActive()
        -> IsLogActiveMapType&
    {
        static IsLogActiveMapType VerboseMap;
        return VerboseMap;
    }

    auto
        Get_VeryVerboseMap_IsActive()
        -> IsLogActiveMapType&
    {
        static IsLogActiveMapType VeryVerboseMap;
        return VeryVerboseMap;
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
        const auto& Trace = Get_StackTrace_Blueprint();

        return Trace.Num() > 0 ? Trace.Last() : TOptional<FString>{};
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Log_UE::
    Log_Fatal(
        FText InMsg,
       FCk_LogCategory InLogCategory)
    -> void
{
    Log_Fatal_If(true, InMsg, InLogCategory);
}

auto
    UCk_Utils_Log_UE::
    Log_Error(
        FText InMsg,
       FCk_LogCategory InLogCategory)
    -> void
{
    Log_Error_If(true, InMsg, InLogCategory);
}

auto
    UCk_Utils_Log_UE::
    Log_Warning(
        FText InMsg,
       FCk_LogCategory InLogCategory)
    -> void
{
    Log_Warning_If(true, InMsg, InLogCategory);
}

auto
    UCk_Utils_Log_UE::
    Log_Display(
        FText InMsg,
       FCk_LogCategory InLogCategory)
    -> void
{
    Log_Display_If(true, InMsg, InLogCategory);
}

auto
    UCk_Utils_Log_UE::
    Log(
        FText InMsg,
       FCk_LogCategory InLogCategory)
    -> void
{
    Log_If(true, InMsg, InLogCategory);
}

auto
    UCk_Utils_Log_UE::
    Log_Verbose(
        FText InMsg,
       FCk_LogCategory InLogCategory)
    -> void
{
    Log_Verbose_If(true, InMsg, InLogCategory);
}

auto
    UCk_Utils_Log_UE::
    Log_VeryVerbose(
        FText InMsg,
       FCk_LogCategory InLogCategory)
    -> void
{
    Log_VeryVerbose_If(true, InMsg, InLogCategory);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Log_UE::
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
    UCk_Utils_Log_UE::
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
    UCk_Utils_Log_UE::
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
    UCk_Utils_Log_UE::
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
    UCk_Utils_Log_UE::
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
    UCk_Utils_Log_UE::
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
    UCk_Utils_Log_UE::
    Get_Fatal_IsActive(
        FCk_LogCategory InLogCategory)
    -> bool
{
    return DoGet_IsLogActive(ck::log::Get_FatalMap_IsActive(), InLogCategory.Get_Name());
}

auto
    UCk_Utils_Log_UE::
    Get_Error_IsActive(
        FCk_LogCategory InLogCategory)
    -> bool
{
    return DoGet_IsLogActive(ck::log::Get_ErrorMap_IsActive(), InLogCategory.Get_Name());
}

auto
    UCk_Utils_Log_UE::
    Get_Warning_IsActive(
        FCk_LogCategory InLogCategory)
    -> bool
{
    return DoGet_IsLogActive(ck::log::Get_WarningMap_IsActive(), InLogCategory.Get_Name());
}

auto
    UCk_Utils_Log_UE::
    Get_Display_IsActive(
        FCk_LogCategory InLogCategory)
    -> bool
{
    return DoGet_IsLogActive(ck::log::Get_DisplayMap_IsActive(), InLogCategory.Get_Name());
}

auto
    UCk_Utils_Log_UE::
    Get_Log_IsActive(
        FCk_LogCategory InLogCategory)
    -> bool
{
    return DoGet_IsLogActive(ck::log::Get_LogMap_IsActive(), InLogCategory.Get_Name());
}

auto
    UCk_Utils_Log_UE::
    Get_Verbose_IsActive(
        FCk_LogCategory InLogCategory)
    -> bool
{
    return DoGet_IsLogActive(ck::log::Get_VerboseMap_IsActive(), InLogCategory.Get_Name());
}

auto
    UCk_Utils_Log_UE::
    Get_VeryVerbose_IsActive(
        FCk_LogCategory InLogCategory)
    -> bool
{
    return DoGet_IsLogActive(ck::log::Get_VeryVerboseMap_IsActive(), InLogCategory.Get_Name());
}

auto
    UCk_Utils_Log_UE::
    Get_IsLogActive_ForVerbosity(
        const FCk_LogCategory& InLogCategory,
        ECk_LogVerbosity InVerbosity)
    -> bool
{
    switch (InVerbosity)
    {
        case ECk_LogVerbosity::Log:
        {
            return Get_Log_IsActive(InLogCategory);
        }
        case ECk_LogVerbosity::Display:
        {
            return Get_Display_IsActive(InLogCategory);
        }
        case ECk_LogVerbosity::Verbose:
        {
            return Get_Verbose_IsActive(InLogCategory);
        }
        case ECk_LogVerbosity::VeryVerbose:
        {
            return Get_VeryVerbose_IsActive(InLogCategory);
        }
        case ECk_LogVerbosity::Warning:
        {
            return Get_Warning_IsActive(InLogCategory);
        }
        case ECk_LogVerbosity::Error:
        {
            return Get_Error_IsActive(InLogCategory);
        }
        case ECk_LogVerbosity::Fatal:
        {
            return Get_Fatal_IsActive(InLogCategory);
        }
        default:
        {
            return false;
        }
    }
}

auto
    UCk_Utils_Log_UE::
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
    UCk_Utils_Log_UE::
    DoInvokeLog(
        const TMap<FName, TFunction<void(const FString&)>>& InMap,
        FName InLogger,
        FText InMsg)
    -> void
{
    if (InMap.Contains(InLogger) == false)
    {
        UE_LOG(CkLogger, Error, TEXT("Could not find the Logger [%s]. Are you sure you have defined it? See CkLog.h for an example."), *InLogger.ToString());
        return;
    }

    // logging the context can be expensive and can optionally be turned off
#if CK_LOG_NO_CONTEXT
    const auto& Formatted = InMsg.ToString();
#else
    const auto& BpContext = ck::log::Get_BlueprintContext();
    const auto& Formatted = FString::Printf(TEXT("%s\n== CONTEXT:[%s] =="), *InMsg.ToString(), *BpContext.Get(TEXT("")));
#endif

    InMap[InLogger](Formatted);
}

auto
    UCk_Utils_Log_UE::
    DoGet_IsLogActive(
        const TMap<FName, TFunction<bool()>>& InMap,
        FName InLogger)
    -> bool
{
    if (InMap.Contains(InLogger) == false)
    {
        UE_LOG(CkLogger, Error, TEXT("Could not find the Logger [%s]. Are you sure you have defined it? See CkLog.h for an example."), *InLogger.ToString());
        return {};
    }

    return InMap[InLogger]();
}

// --------------------------------------------------------------------------------------------------------------------
