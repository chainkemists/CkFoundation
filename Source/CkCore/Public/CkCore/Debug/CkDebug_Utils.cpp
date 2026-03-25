#include "CkDebug_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Settings/CkCore_Settings.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Blueprint/BlueprintExceptionInfo.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptManager.h>
#include <as_context.h>
#include <angelscript.h>
#endif

#if !CK_DISABLE_STACK_TRACE
#include "Windows/WindowsPlatformStackWalk.h"
#endif

// --------------------------------------------------------------------------------------------------------------------
// Global symbol cache for address resolution
// Thread-safe, persists across debug session, cleared on hot-reload
// Background thread continuously resolves unresolved addresses
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    struct FSymbolCacheEntry
    {
        TUniquePtr<FString> Symbol;
        std::atomic<bool> IsResolved;

        FSymbolCacheEntry()
            : Symbol(MakeUnique<FString>(TEXT("<resolving...>")))
            , IsResolved(false)
        {
        }

        // Copy constructor for TMap compatibility
        FSymbolCacheEntry(const FSymbolCacheEntry& Other)
            : Symbol(MakeUnique<FString>(*Other.Symbol))
            , IsResolved(Other.IsResolved.load())
        {
        }

        // Move constructor
        FSymbolCacheEntry(FSymbolCacheEntry&& Other) noexcept
            : Symbol(MoveTemp(Other.Symbol))
            , IsResolved(Other.IsResolved.load())
        {
        }

        // Assignment operators
        auto operator=(const FSymbolCacheEntry& Other) -> FSymbolCacheEntry&
        {
            if (this != &Other)
            {
                Symbol = MakeUnique<FString>(*Other.Symbol);
                IsResolved.store(Other.IsResolved.load());
            }
            return *this;
        }

        auto operator=(FSymbolCacheEntry&& Other) noexcept -> FSymbolCacheEntry&
        {
            if (this != &Other)
            {
                Symbol = MoveTemp(Other.Symbol);
                IsResolved.store(Other.IsResolved.load());
            }
            return *this;
        }
    };

    TMap<uint64, FSymbolCacheEntry> GSymbolCache;
    FRWLock GSymbolCacheLock;

    std::atomic<bool> GBackgroundThreadRunning{false};
    FRunnableThread* GBackgroundResolverThread = nullptr;

    // Background resolver thread
    class FCallstackResolverThread : public FRunnable
    {
    public:
        auto Run() -> uint32 override
        {
            while (GBackgroundThreadRunning.load())
            {
                ResolveUnresolvedAddresses();
                FPlatformProcess::Sleep(0.01f); // Sleep 100ms between passes
            }
            return 0;
        }

    private:
        auto ResolveUnresolvedAddresses() -> void
        {
            // Collect unresolved addresses
            auto UnresolvedAddresses = TArray<uint64>{};
            {
                FReadScopeLock ReadLock(GSymbolCacheLock);
                for (const auto& Pair : GSymbolCache)
                {
                    if (NOT Pair.Value.IsResolved.load())
                    {
                        UnresolvedAddresses.Add(Pair.Key);
                    }
                }
            }

            // Resolve each unresolved address
            for (const auto Address : UnresolvedAddresses)
            {
                constexpr auto MaxSymbolLength = 1024;
                ANSICHAR SymbolInfo[MaxSymbolLength];
                SymbolInfo[0] = 0;

                FPlatformStackWalk::ProgramCounterToHumanReadableString(
                    0,
                    Address,
                    SymbolInfo,
                    MaxSymbolLength);

                auto Resolved = FString{};
                if (SymbolInfo[0] != 0)
                {
                    Resolved = FString{SymbolInfo};

                    // Compact format
                    auto ExclamationIndex = 0;
                    if (Resolved.FindChar('!', ExclamationIndex))
                    {
                        Resolved.RightChopInline(ExclamationIndex + 1);
                    }

                    auto BracketIndex = 0;
                    if (Resolved.FindChar('[', BracketIndex))
                    {
                        auto LastSlashIndex = 0;
                        if (Resolved.FindLastChar('\\', LastSlashIndex) && LastSlashIndex > BracketIndex)
                        {
                            const auto CharsToRemove = LastSlashIndex - BracketIndex;
                            Resolved.RemoveAt(BracketIndex + 1, CharsToRemove);
                        }
                    }

                    Resolved = Resolved.TrimStartAndEnd();
                }
                else
                {
                    Resolved = FString::Printf(TEXT("0x%016llx"), Address);
                }

                // Update cache with resolved symbol
                {
                    FWriteScopeLock WriteLock(GSymbolCacheLock);
                    if (auto* Entry = GSymbolCache.Find(Address))
                    {
                        *Entry->Symbol = MoveTemp(Resolved);
                        Entry->IsResolved.store(true);
                    }
                }
            }
        }
    };

    // Start background resolver thread
    auto StartBackgroundResolver() -> void
    {
        if (GBackgroundThreadRunning.load())
        {
            return;
        }

        GBackgroundThreadRunning.store(true);
        GBackgroundResolverThread = FRunnableThread::Create(
            new FCallstackResolverThread(),
            TEXT("CallstackResolver"),
            0,
            TPri_BelowNormal);
    }

    // Stop background resolver thread (called on module shutdown)
    auto StopBackgroundResolver() -> void
    {
        if (NOT GBackgroundThreadRunning.load())
        {
            return;
        }

        GBackgroundThreadRunning.store(false);
        if (GBackgroundResolverThread != nullptr)
        {
            GBackgroundResolverThread->WaitForCompletion();
            delete GBackgroundResolverThread;
            GBackgroundResolverThread = nullptr;
        }
    }
}

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
#if CK_BUILD_TEST_OR_SHIPPING
    return {};
#else
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
#endif
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
    Get_StackTrace_AsArray(
        int32 InSkipFrames,
        ECk_StackTraceVerbosity_Policy InVerbosity)
    -> TArray<FString>
{
    return Get_StackTrace(ck::type_traits::AsArray{}, InSkipFrames, InVerbosity);
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace(
        ck::type_traits::AsArray,
        int32 InSkipFrames,
        ECk_StackTraceVerbosity_Policy InVerbosity)
    -> TArray<FString>
{
    const auto StackTraceString = Get_StackTrace(InSkipFrames, InVerbosity);

    auto Lines = TArray<FString>{};
    StackTraceString.ParseIntoArrayLines(Lines, false);

    return Lines;
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_AddressesOnly(
        const int32 InMaxFrames,
        const int32 InSkipFrames)
    -> TArray<uint64>
{
    auto Addresses = TArray<uint64>{};

#if !CK_DISABLE_STACK_TRACE
    // Allocate buffer for stack frames on the stack (fast)
    auto* BackTraceBuffer = static_cast<uint64*>(FMemory_Alloca(InMaxFrames * sizeof(uint64)));

    // Capture stack addresses (fast - no symbol resolution)
    // Skip frames within CaptureStackBackTrace to avoid extra copying
    const auto CapturedFrames = FPlatformStackWalk::CaptureStackBackTrace(
        BackTraceBuffer,
        InMaxFrames,
        nullptr);

    // Skip requested frames (utility function + macro frame)
    const auto FramesToSkip = FMath::Min(InSkipFrames, static_cast<int32>(CapturedFrames));
    const auto FramesToKeep = static_cast<int32>(CapturedFrames) - FramesToSkip;

    if (FramesToKeep > 0)
    {
        Addresses.Reserve(FramesToKeep);
        for (auto I = FramesToSkip; I < static_cast<int32>(CapturedFrames); ++I)
        {
            Addresses.Add(BackTraceBuffer[I]);
        }
    }
#endif

    return Addresses;
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_ResolveAddresses(
        const TArray<uint64>& InAddresses)
    -> TArray<FString>
{
    auto ResolvedFrames = TArray<FString>{};

#if !CK_DISABLE_STACK_TRACE
    ResolvedFrames.Reserve(InAddresses.Num());

    for (const auto Address : InAddresses)
    {
        constexpr auto MaxSymbolLength = 1024;
        ANSICHAR SymbolInfo[MaxSymbolLength];
        SymbolInfo[0] = 0;

        // Resolve symbol info for this address
        // ProgramCounterToHumanReadableString formats as: "FunctionName [File.cpp:123]"
        FPlatformStackWalk::ProgramCounterToHumanReadableString(
            0,
            Address,
            SymbolInfo,
            MaxSymbolLength);

        if (SymbolInfo[0] != 0)
        {
            // Successfully resolved - convert to FString
            auto Frame = FString{SymbolInfo};

            // Compact format - remove module name before '!'
            auto ExclamationIndex = 0;
            if (Frame.FindChar('!', ExclamationIndex))
            {
                Frame.RightChopInline(ExclamationIndex + 1);
            }

            // Remove full path before filename in brackets
            auto BracketIndex = 0;
            if (Frame.FindChar('[', BracketIndex))
            {
                auto LastSlashIndex = 0;
                if (Frame.FindLastChar('\\', LastSlashIndex) && LastSlashIndex > BracketIndex)
                {
                    const auto CharsToRemove = LastSlashIndex - BracketIndex;
                    Frame.RemoveAt(BracketIndex + 1, CharsToRemove);
                }
            }

            ResolvedFrames.Add(Frame.TrimStartAndEnd());
        }
        else
        {
            // Couldn't resolve - just show address
            ResolvedFrames.Add(FString::Printf(TEXT("0x%016llx"), Address));
        }
    }
#endif

    return ResolvedFrames;
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_ResolveAddress(const uint64 InAddress)
    -> const FString*
{
#if !CK_DISABLE_STACK_TRACE
    // Ensure background resolver is running
    StartBackgroundResolver();

    // Try read-only access first (fast path - already in cache)
    {
        FReadScopeLock ReadLock(GSymbolCacheLock);
        if (const auto* Cached = GSymbolCache.Find(InAddress))
        {
            return Cached->Symbol.Get();
        }
    }

    // Not cached - add placeholder entry (background thread will resolve it)
    {
        FWriteScopeLock WriteLock(GSymbolCacheLock);

        // Double-check - another thread might have cached it while we waited
        if (const auto* Cached = GSymbolCache.Find(InAddress))
        {
            return Cached->Symbol.Get();
        }

        // Add unresolved entry - background thread will resolve it
        auto& NewEntry = GSymbolCache.Add(InAddress, FSymbolCacheEntry{});
        return NewEntry.Symbol.Get();
    }
#else
    return nullptr;
#endif
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

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_Angelscript_AsArray()
    -> TArray<FString>
{
    return Get_StackTrace_Angelscript(ck::type_traits::AsArray{});
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_Angelscript_AsString()
    -> FString
{
    return Get_StackTrace_Angelscript(ck::type_traits::AsString{});
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_Angelscript(
        ck::type_traits::AsArray)
    -> TArray<FString>
{
    return Get_StackTrace_Angelscript(ck::type_traits::AsArray{}, UCk_Utils_Core_UserSettings_UE::Get_MaxNumberOfAngelscriptStackFrames());
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_Angelscript(
        ck::type_traits::AsArray,
        int32 InMaxFrames)
    -> TArray<FString>
{
    auto StackTrace = TArray<FString>{};

#if !CK_DISABLE_STACK_TRACE && WITH_ANGELSCRIPT_CK
    if (NOT FAngelscriptManager::IsInitialized())
    { return StackTrace; }

    auto FullStackTrace = FAngelscriptManager::GetAngelscriptCallstack();

    const auto FramesToCapture = FMath::Min(InMaxFrames, FullStackTrace.Num());

    for (auto I = 0; I < FramesToCapture; ++I)
    {
        StackTrace.Add(FullStackTrace[I]);
    }
#endif

    return StackTrace;
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_Angelscript(
        ck::type_traits::AsString)
    -> FString
{
    return Get_StackTrace_Angelscript(ck::type_traits::AsString{}, UCk_Utils_Core_UserSettings_UE::Get_MaxNumberOfAngelscriptStackFrames());
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Get_StackTrace_Angelscript(
        ck::type_traits::AsString,
        int32 InMaxFrames)
    -> FString
{
    auto StackTrace = FString{};

#if !CK_DISABLE_STACK_TRACE && WITH_ANGELSCRIPT_CK
    if (NOT FAngelscriptManager::IsInitialized())
    { return StackTrace; }

    if (const auto Context = FAngelscriptManager::GetCurrentScriptContext();
        ck::Is_NOT_Valid(Context, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    auto FullStackTrace = FAngelscriptManager::GetAngelscriptCallstack();

    const auto FramesToCapture = FMath::Min(InMaxFrames, FullStackTrace.Num());

    for (auto I = 0; I < FramesToCapture; ++I)
    {
        StackTrace += FString::Printf(TEXT("[%d] %s\n"), I, *FullStackTrace[I]);
    }
#endif

    return StackTrace;
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Request_BreakInAngelscript(
        const FText& InDescription)
    -> void
{
#if !CK_DISABLE_STACK_TRACE && WITH_ANGELSCRIPT_CK
    if (NOT FAngelscriptManager::IsInitialized())
    { return; }

    // Check if we're in Angelscript execution context
    auto* CurrentContext = FAngelscriptManager::GetCurrentScriptContext();
    if (ck::Is_NOT_Valid(CurrentContext, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    // Try to break into angelscript debugger
    auto DescriptionString = InDescription.ToString();
    FAngelscriptManager::TryBreakpointAngelscriptDebugging(*DescriptionString);
#endif
}

auto
    UCk_Utils_Debug_StackTrace_UE::
    Try_BreakInAngelscript(
        const UObject* InContext,
        const FText& InDescription)
    -> void
{
#if !CK_DISABLE_STACK_TRACE && WITH_ANGELSCRIPT_CK
    // Check if we're in Angelscript execution context
    if (NOT FAngelscriptManager::IsInitialized())
    { return; }

    auto* CurrentContext = FAngelscriptManager::GetCurrentScriptContext();
    if (ck::Is_NOT_Valid(CurrentContext, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    // Log the callstack
    const auto StackTrace = Get_StackTrace_Angelscript(ck::type_traits::AsString{});
    if (NOT StackTrace.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Angelscript Stack Trace:\n%s"), *StackTrace);
    }

    // Check break policy
    const auto ShouldBreak = UCk_Utils_Core_UserSettings_UE::Get_EnsureBreakInAngelscriptPolicy() == ECk_EnsureBreakInAngelscript_Policy::AlwaysBreak;
    if (NOT ShouldBreak)
    { return; }

    Request_BreakInAngelscript(InDescription);
#endif
}