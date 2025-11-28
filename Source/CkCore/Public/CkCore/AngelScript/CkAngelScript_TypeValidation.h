#pragma once

#include <Containers/UnrealString.h>

#if WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------
// Type validation for AngelScript bindings
//
// AngelScript cannot handle certain C++ type patterns. This utility detects them
// BEFORE attempting registration, avoiding the asINVALID_DECLARATION errors.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscript
{

// Patterns that AngelScript cannot parse/handle
inline auto
ContainsUnsupportedTypePattern(
    const FString& InTypeString) -> bool
{
    // TWeakObjectPtr with FWeakObjectPtr second parameter
    // e.g., "TWeakObjectPtr<AActor, FWeakObjectPtr>"
    if (InTypeString.Contains(TEXT("FWeakObjectPtr")))
    {
        return true;
    }

    // std::function - callbacks not supported
    // e.g., "std::function<void __cdecl(FCk_Handle)>"
    if (InTypeString.Contains(TEXT("std::function<")))
    {
        return true;
    }

    // std::variant - not supported
    // e.g., "std::variant<FCk_Request_Tween_Pause, ...>"
    if (InTypeString.Contains(TEXT("std::variant<")))
    {
        return true;
    }

    // TFunction - Unreal's function wrapper not supported
    // e.g., "TFunction<void __cdecl(FCk_Handle &)>"
    if (InTypeString.Contains(TEXT("TFunction<")))
    {
        return true;
    }

    // TSharedPtr with explicit mode parameter
    // e.g., "TSharedPtr<FStreamableHandle, 1>"
    // Note: TSharedPtr<T> without mode might work, but TSharedPtr<T, 1> does not
    if (InTypeString.Contains(TEXT("TSharedPtr<")) && InTypeString.Contains(TEXT(", 1>")))
    {
        return true;
    }

    // TDelegate with policy
    // e.g., "TDelegate<void __cdecl(void), FDefaultDelegateUserPolicy>"
    if (InTypeString.Contains(TEXT("TDelegate<")) && InTypeString.Contains(TEXT("DelegateUserPolicy")))
    {
        return true;
    }

    // entt namespace types
    // e.g., "entt::entity"
    if (InTypeString.Contains(TEXT("entt::")))
    {
        return true;
    }

    // UE::Math namespace types
    // e.g., "UE::Math::TVector4<double>"
    if (InTypeString.Contains(TEXT("UE::Math::")))
    {
        return true;
    }

    // Specific types not exposed to AngelScript
    if (InTypeString.Contains(TEXT("FPrimitiveDrawInterface")))
    {
        return true;
    }

    if (InTypeString.Contains(TEXT("FGameplayDebuggerCanvasContext")))
    {
        return true;
    }

    if (InTypeString.Contains(TEXT("AGameplayDebuggerCategoryReplicator")))
    {
        return true;
    }

    if (InTypeString.Contains(TEXT("FRunInfo")))
    {
        return true;
    }

    if (InTypeString.Contains(TEXT("FStreamableHandle")))
    {
        return true;
    }

    // const inside template parameters
    // e.g., "TObjectPtr<UObject const>" or "TWeakObjectPtr<UObject const, ...>"
    // This pattern: "<SomeType const>" or "<SomeType const,"
    // Check for " const>" or " const," patterns inside angle brackets
    {
        int32 AngleBracketDepth = 0;
        bool InsideTemplate = false;
        
        for (int32 i = 0; i < InTypeString.Len(); ++i)
        {
            TCHAR Char = InTypeString[i];
            
            if (Char == TEXT('<'))
            {
                AngleBracketDepth++;
                InsideTemplate = true;
            }
            else if (Char == TEXT('>'))
            {
                AngleBracketDepth--;
                if (AngleBracketDepth == 0)
                {
                    InsideTemplate = false;
                }
            }
            
            // Check for " const>" or " const," while inside template
            if (InsideTemplate && AngleBracketDepth > 0)
            {
                if (i + 6 < InTypeString.Len())
                {
                    auto Substring = InTypeString.Mid(i, 7);
                    if (Substring == TEXT(" const>") || Substring == TEXT(" const,"))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// Check if a type string appears to be valid for AngelScript
// This is a positive check - returns true if it looks okay
inline auto
IsTypeValidForAngelScript(
    const FString& InTypeString) -> bool
{
    return NOT ContainsUnsupportedTypePattern(InTypeString);
}

} // namespace ck::angelscript

#endif // WITH_ANGELSCRIPT_CK
