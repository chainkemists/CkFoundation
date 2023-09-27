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
    static const FString invalidName = TEXT("INVALID UObject");

    if (ck::Is_NOT_Valid(InObject))
    { return invalidName; }

    switch (InNameVerbosity)
    {
    case ECk_DebugName_Verbosity::FullName:
        return InObject->GetPathName(); ;
    case ECk_DebugName_Verbosity::ShortName:
        return InObject->GetName();
    default:
        CK_INVALID_ENUM(InNameVerbosity);
        return invalidName;
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
        int32 InSkipFrames)
    -> FString
{
    constexpr auto stackTraceSize = std::numeric_limits<int16>::max();

    ANSICHAR stackTrace[stackTraceSize];
    stackTrace[0] = 0;

#if !CK_DISABLE_STACK_TRACE
    FPlatformStackWalk::StackWalkAndDump(stackTrace, stackTraceSize, InSkipFrames);
    stackTrace[stackTraceSize - 1] = 0;
#endif

    return FString{stackTrace};
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
    auto stackTrace = TArray<FString>{};

#if !CK_DISABLE_STACK_TRACE
    const auto* blueprintExceptionTracker = FBlueprintContextTracker::TryGet();

    if (ck::Is_NOT_Valid(blueprintExceptionTracker, ck::IsValid_Policy_NullptrOnly{}))
    { return stackTrace; }

    const auto& rawStack = blueprintExceptionTracker->GetCurrentScriptStack();
    for (int32 frameIdx = rawStack.Num() - 1; frameIdx >= 0; --frameIdx)
    {
        FStringBuilderBase stringBuilder;
        rawStack[frameIdx]->GetStackDescription(stringBuilder);
        stackTrace.Emplace(stringBuilder.ToString());
    }
#endif

    return stackTrace;
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_Blueprint(
        ck::type_traits::AsString)
    -> FString
{
    FString stackTrace;

#if !CK_DISABLE_STACK_TRACE
    const auto* blueprintExceptionTracker = FBlueprintContextTracker::TryGet();
    if (ck::Is_NOT_Valid(blueprintExceptionTracker, ck::IsValid_Policy_NullptrOnly{}))
    { return stackTrace; }

    const TArrayView<const FFrame* const>& rawStack = blueprintExceptionTracker->GetCurrentScriptStack();
    for (int32 frameIdx = rawStack.Num() - 1; frameIdx >= 0; --frameIdx)
    {
        const auto& stackDescription = rawStack[frameIdx];
        stackTrace += ck::Format_UE
        (
            TEXT("{}:{}\n"),
            stackDescription->Node,
            stackDescription->MostRecentProperty
        );
    }
#endif

    return stackTrace;
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Try_BreakInScript(
        const UObject* InContext)
    -> void
{
#if !CK_DISABLE_STACK_TRACE
    if (ck::Is_NOT_Valid(InContext))
    { return; }

    const auto* blueprintExceptionTracker = FBlueprintContextTracker::TryGet();
    if (ck::Is_NOT_Valid(blueprintExceptionTracker, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    const TArrayView<FFrame* const>& scriptStack = blueprintExceptionTracker->GetCurrentScriptStackWritable();

    if (scriptStack.IsEmpty())
    { return; }

    const FBlueprintExceptionInfo exceptionInfo(EBlueprintExceptionType::Breakpoint);
    FBlueprintCoreDelegates::ThrowScriptException(InContext, *scriptStack.Last(), exceptionInfo);
#endif
}


// --------------------------------------------------------------------------------------------------------------------
