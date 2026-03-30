#include "CkCVar_Utils.h"

#include "CkCVar/CkCVar_Log.h"

#include <Async/Async.h>
#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    struct FCallbackEntry
    {
        FName CVarName;
        FDelegateHandle Handle;
    };

    FCriticalSection CallbackRegistryLock;
    TMap<int32, FCallbackEntry> CallbackRegistry;
    std::atomic<int32> NextCallbackID{1};

    auto GenerateCallbackID() -> int32
    {
        return NextCallbackID.fetch_add(1, std::memory_order_relaxed);
    }

    template <typename TDefaultValue>
    auto FindOrRegisterCVar(FName InName, const TDefaultValue& InDefaultValue, const FString& InHelp) -> IConsoleVariable*
    {
        auto* CVar = IConsoleManager::Get().FindConsoleVariable(*InName.ToString());
        if (CVar != nullptr)
        {
            return CVar;
        }

        return IConsoleManager::Get().RegisterConsoleVariable(
            *InName.ToString(),
            InDefaultValue,
            *InHelp);
    }

    template <typename TValue, typename TDelegate>
    auto BindCallbackInternal(
        FName InName,
        const TDelegate& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy) -> FCk_CVarCallbackHandle
    {
        auto* CVar = IConsoleManager::Get().FindConsoleVariable(*InName.ToString());
        if (CVar == nullptr)
        {
            ck::cvar::Warning(TEXT("Cannot bind callback for CVar [%s] - not registered"), *InName.ToString());
            return FCk_CVarCallbackHandle{};
        }

        if (NOT InCallback.IsBound())
        {
            return FCk_CVarCallbackHandle{};
        }

        const auto ID = GenerateCallbackID();

        auto CallbackCopy = InCallback;
        auto Handle = CVar->OnChangedDelegate().AddLambda(
            [CallbackCopy](IConsoleVariable* InConsoleVariable)
            {
                auto BoundCallback = CallbackCopy;
                AsyncTask(ENamedThreads::GameThread, [BoundCallback, InConsoleVariable]()
                {
                    if (NOT BoundCallback.IsBound())
                    {
                        return;
                    }

                    if constexpr (std::is_same_v<TValue, bool>)
                    {
                        BoundCallback.Execute(InConsoleVariable->GetBool());
                    }
                    else if constexpr (std::is_same_v<TValue, int32>)
                    {
                        BoundCallback.Execute(InConsoleVariable->GetInt());
                    }
                    else if constexpr (std::is_same_v<TValue, float>)
                    {
                        BoundCallback.Execute(InConsoleVariable->GetFloat());
                    }
                    else if constexpr (std::is_same_v<TValue, FString>)
                    {
                        BoundCallback.Execute(InConsoleVariable->GetString());
                    }
                });
            });

        {
            FScopeLock Lock(&CallbackRegistryLock);
            CallbackRegistry.Add(ID, FCallbackEntry{InName, Handle});
        }

        if (InPolicy == ECk_CVar_InitialCallbackPolicy::FireImmediately)
        {
            if constexpr (std::is_same_v<TValue, bool>)
            {
                InCallback.Execute(CVar->GetBool());
            }
            else if constexpr (std::is_same_v<TValue, int32>)
            {
                InCallback.Execute(CVar->GetInt());
            }
            else if constexpr (std::is_same_v<TValue, float>)
            {
                InCallback.Execute(CVar->GetFloat());
            }
            else if constexpr (std::is_same_v<TValue, FString>)
            {
                InCallback.Execute(CVar->GetString());
            }
        }

        return FCk_CVarCallbackHandle{ID};
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Registration
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Register_Int32(
        FName InName,
        int32 InDefaultValue,
        const FString& InHelp,
        const FCk_Delegate_CVar_OnChanged_Int32& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy)
    -> FCk_CVarCallbackHandle
{
    FindOrRegisterCVar(InName, InDefaultValue, InHelp);
    return BindCallbackInternal<int32>(InName, InCallback, InPolicy);
}

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Register_Float(
        FName InName,
        float InDefaultValue,
        const FString& InHelp,
        const FCk_Delegate_CVar_OnChanged_Float& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy)
    -> FCk_CVarCallbackHandle
{
    FindOrRegisterCVar(InName, InDefaultValue, InHelp);
    return BindCallbackInternal<float>(InName, InCallback, InPolicy);
}

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Register_Bool(
        FName InName,
        bool InDefaultValue,
        const FString& InHelp,
        const FCk_Delegate_CVar_OnChanged_Bool& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy)
    -> FCk_CVarCallbackHandle
{
    constexpr auto BoolToInt = [](bool InValue) -> int32 { return InValue ? 1 : 0; };
    FindOrRegisterCVar(InName, BoolToInt(InDefaultValue), InHelp);
    return BindCallbackInternal<bool>(InName, InCallback, InPolicy);
}

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Register_String(
        FName InName,
        const FString& InDefaultValue,
        const FString& InHelp,
        const FCk_Delegate_CVar_OnChanged_String& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy)
    -> FCk_CVarCallbackHandle
{
    FindOrRegisterCVar(InName, *InDefaultValue, InHelp);
    return BindCallbackInternal<FString>(InName, InCallback, InPolicy);
}

// --------------------------------------------------------------------------------------------------------------------
// Binding
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Bind_Int32(
        const FCk_CVarRef& InRef,
        const FCk_Delegate_CVar_OnChanged_Int32& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy)
    -> FCk_CVarCallbackHandle
{
    return BindCallbackInternal<int32>(InRef.Get_Name(), InCallback, InPolicy);
}

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Bind_Float(
        const FCk_CVarRef& InRef,
        const FCk_Delegate_CVar_OnChanged_Float& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy)
    -> FCk_CVarCallbackHandle
{
    return BindCallbackInternal<float>(InRef.Get_Name(), InCallback, InPolicy);
}

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Bind_Bool(
        const FCk_CVarRef& InRef,
        const FCk_Delegate_CVar_OnChanged_Bool& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy)
    -> FCk_CVarCallbackHandle
{
    return BindCallbackInternal<bool>(InRef.Get_Name(), InCallback, InPolicy);
}

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Bind_String(
        const FCk_CVarRef& InRef,
        const FCk_Delegate_CVar_OnChanged_String& InCallback,
        ECk_CVar_InitialCallbackPolicy InPolicy)
    -> FCk_CVarCallbackHandle
{
    return BindCallbackInternal<FString>(InRef.Get_Name(), InCallback, InPolicy);
}

// --------------------------------------------------------------------------------------------------------------------
// Unbinding
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Unbind(
        FCk_CVarCallbackHandle InHandle)
    -> void
{
    if (NOT InHandle.IsValid())
    {
        return;
    }

    auto Entry = FCallbackEntry{};
    {
        FScopeLock Lock(&CallbackRegistryLock);

        if (NOT CallbackRegistry.Contains(InHandle.Get_ID()))
        {
            ck::cvar::Warning(TEXT("Cannot unbind callback with ID [%d] - not found in registry"), InHandle.Get_ID());
            return;
        }

        Entry = CallbackRegistry.FindAndRemoveChecked(InHandle.Get_ID());
    }

    auto* CVar = IConsoleManager::Get().FindConsoleVariable(*Entry.CVarName.ToString());
    if (CVar != nullptr)
    {
        CVar->OnChangedDelegate().Remove(Entry.Handle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Get
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Get_Int32(
        const FCk_CVarRef& InRef)
    -> int32
{
    auto* CVar = IConsoleManager::Get().FindConsoleVariable(*InRef.Get_Name().ToString());
    if (CVar == nullptr)
    {
        return 0;
    }
    return CVar->GetInt();
}

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Get_Float(
        const FCk_CVarRef& InRef)
    -> float
{
    auto* CVar = IConsoleManager::Get().FindConsoleVariable(*InRef.Get_Name().ToString());
    if (CVar == nullptr)
    {
        return 0.0f;
    }
    return CVar->GetFloat();
}

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Get_Bool(
        const FCk_CVarRef& InRef)
    -> bool
{
    auto* CVar = IConsoleManager::Get().FindConsoleVariable(*InRef.Get_Name().ToString());
    if (CVar == nullptr)
    {
        return false;
    }
    return CVar->GetBool();
}

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Get_String(
        const FCk_CVarRef& InRef)
    -> FString
{
    auto* CVar = IConsoleManager::Get().FindConsoleVariable(*InRef.Get_Name().ToString());
    if (CVar == nullptr)
    {
        return FString{};
    }
    return CVar->GetString();
}

// --------------------------------------------------------------------------------------------------------------------
// Set
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Set_Int32(
        const FCk_CVarRef& InRef,
        int32 InValue)
    -> void
{
    auto* CVar = IConsoleManager::Get().FindConsoleVariable(*InRef.Get_Name().ToString());
    if (CVar == nullptr)
    {
        return;
    }
    CVar->SetWithCurrentPriority(InValue);
    IConsoleManager::Get().CallAllConsoleVariableSinks();
}

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Set_Float(
        const FCk_CVarRef& InRef,
        float InValue)
    -> void
{
    auto* CVar = IConsoleManager::Get().FindConsoleVariable(*InRef.Get_Name().ToString());
    if (CVar == nullptr)
    {
        return;
    }
    CVar->SetWithCurrentPriority(InValue);
    IConsoleManager::Get().CallAllConsoleVariableSinks();
}

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Set_Bool(
        const FCk_CVarRef& InRef,
        bool InValue)
    -> void
{
    auto* CVar = IConsoleManager::Get().FindConsoleVariable(*InRef.Get_Name().ToString());
    if (CVar == nullptr)
    {
        return;
    }
    CVar->SetWithCurrentPriority(InValue ? 1 : 0);
    IConsoleManager::Get().CallAllConsoleVariableSinks();
}

auto
    UCk_Utils_CVar_UE::
    INTERNAL_Set_String(
        const FCk_CVarRef& InRef,
        const FString& InValue)
    -> void
{
    auto* CVar = IConsoleManager::Get().FindConsoleVariable(*InRef.Get_Name().ToString());
    if (CVar == nullptr)
    {
        return;
    }
    CVar->SetWithCurrentPriority(*InValue);
    IConsoleManager::Get().CallAllConsoleVariableSinks();
}

// --------------------------------------------------------------------------------------------------------------------
// Public Query
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CVar_UE::
    IsRegistered(
        const FCk_CVarRef& InRef)
    -> bool
{
    return IConsoleManager::Get().FindConsoleVariable(*InRef.Get_Name().ToString()) != nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
