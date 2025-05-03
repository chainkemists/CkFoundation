#include "CkDebug_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Blueprint/BlueprintExceptionInfo.h>

#if !CK_DISABLE_STACK_TRACE
#include "Windows/WindowsPlatformStackWalk.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_DebugNameVerbosity_Policy);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Debug_UE::
    Get_DebugName(
        const UObject* InObject,
        ECk_DebugNameVerbosity_Policy InNameVerbosity)
    -> FName
{
    return FName{Get_DebugName_AsString(InObject, InNameVerbosity)};
}

auto
    UCk_Utils_Debug_UE::
    Get_DebugName_AsString(
        const UObject* InObject,
        ECk_DebugNameVerbosity_Policy InNameVerbosity)
    -> FString
{
    static const FString InvalidName = TEXT("INVALID UObject");

    if (ck::Is_NOT_Valid(InObject))
    { return InvalidName; }

    switch (InNameVerbosity)
    {
        case ECk_DebugNameVerbosity_Policy::Default:
        {
            return Get_DebugName_AsString(InObject, UCk_Utils_Core_UserSettings_UE::Get_DefaultDebugNameVerbosity());
        }
        case ECk_DebugNameVerbosity_Policy::Verbose:
        {
            return InObject->GetPathName();
        }
        case ECk_DebugNameVerbosity_Policy::Compact:
        {
            return InObject->GetName();
        }
        default:
        {
            CK_INVALID_ENUM(InNameVerbosity);
            return InvalidName;
        }
    }
}

auto
    UCk_Utils_Debug_UE::
    Get_DebugName_AsText(
        const UObject* InObject,
        ECk_DebugNameVerbosity_Policy InNameVerbosity)
    -> FText
{
    return FText::FromString(Get_DebugName_AsString(InObject, InNameVerbosity));
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_BlueprintContext()
    -> TOptional<FString>
{
    const auto& Trace = Get_StackTrace_Blueprint(ck::type_traits::AsArray{});

    return Trace.Num() > 0 ? Trace.Last() : TOptional<FString>{};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace(
        int32 InSkipFrames,
        ECk_StackTraceVerbosity_Policy InVerbosity)
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
        case ECk_StackTraceVerbosity_Policy::Compact:
        {
            const auto StackTraceStr = FString{StackTrace};
            auto Lines = TArray<FString>{};

            StackTraceStr.ParseIntoArrayLines(Lines, false);

            auto ToRet = FString{};
            for (const auto& Line : Lines)
            {
                auto TrimmedLine = Line.TrimStartAndEnd();

                if (auto StartIndex = 0; TrimmedLine.FindChar('!', StartIndex))
                {
                    TrimmedLine.RightChopInline(StartIndex + 1, EAllowShrinking::Yes);
                }

                if (auto LastPathSeparator = 0; TrimmedLine.FindLastChar('\\', LastPathSeparator))
                {
                    if (auto FilePathSquareBracket = 0; TrimmedLine.FindChar('[', FilePathSquareBracket))
                    {
                        TrimmedLine.RemoveAt(FilePathSquareBracket + 1, LastPathSeparator - FilePathSquareBracket, EAllowShrinking::Yes);
                    }
                }

                TrimmedLine.Shrink();

                ToRet += TrimmedLine + "\n";
            }

            return ToRet;
        }
        case ECk_StackTraceVerbosity_Policy::Verbose:
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
    return Get_StackTrace_Blueprint(ck::type_traits::AsArray{}, UCk_Utils_Core_UserSettings_UE::Get_MaxNumberOfBlueprintStackFrames());
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_Blueprint(
        ck::type_traits::AsArray,
        int32 InMaxFrames)
    -> TArray<FString>
{
    _LastStackTraceContextObject = nullptr;

    auto StackTrace = TArray<FString>{};

#if !CK_DISABLE_STACK_TRACE
    const auto* BlueprintExceptionTracker = FBlueprintContextTracker::TryGet();

    if (ck::Is_NOT_Valid(BlueprintExceptionTracker, ck::IsValid_Policy_NullptrOnly{}))
    { return StackTrace; }

    const auto& RawStack = BlueprintExceptionTracker->GetCurrentScriptStack();
    for (int32 FrameIdx = std::min(InMaxFrames, RawStack.Num()) - 1; FrameIdx >= 0; --FrameIdx)
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
    return Get_StackTrace_Blueprint(ck::type_traits::AsString{}, UCk_Utils_Core_UserSettings_UE::Get_MaxNumberOfBlueprintStackFrames());
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_Blueprint(
        ck::type_traits::AsString,
        const int32 InMaxFrames)
    -> FString
{
    _LastStackTraceContextObject = nullptr;

    auto StackTrace = FString{};

#if !CK_DISABLE_STACK_TRACE
    const auto* BlueprintExceptionTracker = FBlueprintContextTracker::TryGet();
    if (ck::Is_NOT_Valid(BlueprintExceptionTracker, ck::IsValid_Policy_NullptrOnly{}))
    { return StackTrace; }

    const TArrayView<const FFrame* const>& RawStack = BlueprintExceptionTracker->GetCurrentScriptStack();
    for (int32 FrameIdx = std::min(InMaxFrames, RawStack.Num()) - 1; FrameIdx >= 0; --FrameIdx)
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
        const UObject* InContext,
        const FText& InDescription)
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

    const auto ExceptionType = UCk_Utils_Core_UserSettings_UE::Get_EnsureBreakInBlueprintsPolicy() == ECk_EnsureBreakInBlueprints_Policy::AlwaysBreak
        ? EBlueprintExceptionType::Breakpoint
        : EBlueprintExceptionType::AccessViolation;

    const auto ExceptionInfo = FBlueprintExceptionInfo{ExceptionType, InDescription};
    FBlueprintCoreDelegates::ThrowScriptException(Context, *ScriptStack.Last(), ExceptionInfo);
#endif
}


// --------------------------------------------------------------------------------------------------------------------
