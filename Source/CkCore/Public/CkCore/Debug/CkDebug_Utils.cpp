#include "CkDebug_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#if !CK_DISABLE_STACK_TRACE
#include "Windows/WindowsPlatformStackWalk.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_DebugName_Verbosity);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Debug_UE::
    Get_DebugName(
        const UObject*          InObject,
        ECk_DebugName_Verbosity InNameVerbosity)
    -> FName
{
    return FName{Get_DebugName_AsString(InObject, InNameVerbosity)};
}

auto
    UCk_Utils_Debug_UE::
    Get_DebugName_AsString(
        const UObject*          InObject,
        ECk_DebugName_Verbosity InNameVerbosity)
    -> FString
{
    static const FString InvalidName = TEXT("INVALID UObject");

    if (ck::Is_NOT_Valid(InObject))
    { return InvalidName; }

    switch (InNameVerbosity)
    {
        case ECk_DebugName_Verbosity::Default:
        switch(UCk_Utils_Core_UserSettings_UE::Get_DefaultDebugNameVerbosity())
        {
            case ECk_Core_DefaultDebugNameVerbosityPolicy::Compact:
            {
                return Get_DebugName_AsString(InObject, ECk_DebugName_Verbosity::ShortName);
            }
            case ECk_Core_DefaultDebugNameVerbosityPolicy::Verbose:
            {
                return Get_DebugName_AsString(InObject, ECk_DebugName_Verbosity::FullName);
            }
        }
    case ECk_DebugName_Verbosity::FullName:
        return InObject->GetPathName(); ;
    case ECk_DebugName_Verbosity::ShortName:
        return InObject->GetName();
    default:
        CK_INVALID_ENUM(InNameVerbosity);
        return InvalidName;
    }
}

auto
    UCk_Utils_Debug_UE::Get_DebugName_AsText(
        const UObject*          InObject,
        ECk_DebugName_Verbosity InNameVerbosity)
    -> FText
{
    return FText::FromString(Get_DebugName_AsString(InObject, InNameVerbosity));
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_BlueprintContext()
    -> TOptional<FString>
{
    const auto& trace = Get_StackTrace_Blueprint(ck::type_traits::AsArray{});

    return trace.Num() > 0 ? trace.Last() : TOptional<FString>{};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace(
        int32 InSkipFrames,
        ECk_StackTraceVerbosityPolicy InVerbosity)
    -> FString
{
    constexpr auto StackTraceSize = std::numeric_limits<int16>::max();

    ANSICHAR StackTrace[StackTraceSize];
    StackTrace[0] = 0;

#if !CK_DISABLE_STACK_TRACE
    FPlatformStackWalk::StackWalkAndDump(StackTrace, StackTraceSize, InSkipFrames);
    StackTrace[StackTraceSize - 1] = 0;
#endif

    switch(InVerbosity)
    {
        case ECk_StackTraceVerbosityPolicy::Compact:
        {
            const auto StackTraceStr = FString{StackTrace};
            auto Lines = TArray<FString>{};

            StackTraceStr.ParseIntoArrayLines(Lines, false);

            auto ToRet = FString{};
            for (const auto& Line : Lines)
            {
                auto TrimmedLine = Line.TrimStartAndEnd();

                auto StartIndex = 0;
                if (TrimmedLine.FindChar('!', StartIndex))
                {
                    TrimmedLine.RightChopInline(StartIndex + 1, true);
                }

                auto LastPathSeparator = 0;
                if (TrimmedLine.FindLastChar('\\', LastPathSeparator))
                {
                    auto FilePathSquareBracket = 0;
                    if (TrimmedLine.FindChar('[', FilePathSquareBracket))
                    {
                        TrimmedLine.RemoveAt(FilePathSquareBracket + 1, LastPathSeparator - FilePathSquareBracket, true);
                    }
                }

                TrimmedLine.Shrink();

                ToRet += TrimmedLine + "\n";
            }

            return ToRet;
        }
        case ECk_StackTraceVerbosityPolicy::Verbose:
            break;
    }

    return FString{StackTrace};
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_Blueprint_AsArray()
    -> TArray<FString>
{
    return Get_StackTrace_Blueprint(ck::type_traits::AsArray{});
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_Blueprint_AsString()
    -> FString
{
    return Get_StackTrace_Blueprint(ck::type_traits::AsString{});
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_Blueprint(
        ck::type_traits::AsArray)
    -> TArray<FString>
{
    _LastStackTraceContextObject = nullptr;

    auto StackTrace = TArray<FString>{};

#if !CK_DISABLE_STACK_TRACE
    const auto* BlueprintExceptionTracker = FBlueprintContextTracker::TryGet();

    if (ck::Is_NOT_Valid(BlueprintExceptionTracker, ck::IsValid_Policy_NullptrOnly{}))
    { return StackTrace; }

    const auto& RawStack = BlueprintExceptionTracker->GetCurrentScriptStack();
    for (int32 FrameIdx = RawStack.Num() - 1; FrameIdx >= 0; --FrameIdx)
    {
        FStringBuilderBase StringBuilder;
        RawStack[FrameIdx]->GetStackDescription(StringBuilder);
        StackTrace.Emplace(StringBuilder.ToString());
    }

    if (NOT RawStack.IsEmpty())
    { _LastStackTraceContextObject = RawStack.Last()->Object; }
#endif

    return StackTrace;
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_Blueprint(
        ck::type_traits::AsString)
    -> FString
{
    _LastStackTraceContextObject = nullptr;

    auto StackTrace = FString{};

#if !CK_DISABLE_STACK_TRACE
    const auto* BlueprintExceptionTracker = FBlueprintContextTracker::TryGet();
    if (ck::Is_NOT_Valid(BlueprintExceptionTracker, ck::IsValid_Policy_NullptrOnly{}))
    { return StackTrace; }

    const TArrayView<const FFrame* const>& RawStack = BlueprintExceptionTracker->GetCurrentScriptStack();
    for (int32 FrameIdx = RawStack.Num() - 1; FrameIdx >= 0; --FrameIdx)
    {
        const auto& StackDescription = RawStack[FrameIdx];
        StackTrace += ck::Format_UE
        (
            TEXT("{}:{}\n"),
            StackDescription->Node,
            StackDescription->MostRecentProperty
        );
    }

    if (NOT RawStack.IsEmpty())
    { _LastStackTraceContextObject = RawStack.Last()->Object; }
#endif

    return StackTrace;
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Try_BreakInScript(
        const UObject* InContext)
    -> void
{
#if !CK_DISABLE_STACK_TRACE
    const UObject* Context = [&]() -> const UObject*
    {
        if (ck::Is_NOT_Valid(InContext))
        { return _LastStackTraceContextObject; }

        return InContext;
    }();

    if (ck::Is_NOT_Valid(Context))
    { return; }

    const auto* BlueprintExceptionTracker = FBlueprintContextTracker::TryGet();
    if (ck::Is_NOT_Valid(BlueprintExceptionTracker, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    const TArrayView<FFrame* const>& ScriptStack = BlueprintExceptionTracker->GetCurrentScriptStackWritable();

    if (ScriptStack.IsEmpty())
    { return; }

    const FBlueprintExceptionInfo ExceptionInfo(EBlueprintExceptionType::Breakpoint);
    FBlueprintCoreDelegates::ThrowScriptException(Context, *ScriptStack.Last(), ExceptionInfo);
#endif
}


// --------------------------------------------------------------------------------------------------------------------
