#pragma once

#include <Containers/UnrealString.h>

#if WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------
// AngelScript cannot parse certain C++ type patterns; detecting them BEFORE registration avoids
// asINVALID_DECLARATION errors.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscript
{

inline auto
ContainsUnsupportedTypePattern(
    const FString& InTypeString) -> bool
{
    if (InTypeString.Contains(TEXT("FWeakObjectPtr")))
    {
        return true;
    }

    if (InTypeString.Contains(TEXT("std::function<")))
    {
        return true;
    }

    if (InTypeString.Contains(TEXT("std::variant<")))
    {
        return true;
    }

    if (InTypeString.Contains(TEXT("TFunction<")))
    {
        return true;
    }

    // Bare TSharedPtr<T> parses; only the explicit-mode form TSharedPtr<T, 1> does not.
    if (InTypeString.Contains(TEXT("TSharedPtr<")) && InTypeString.Contains(TEXT(", 1>")))
    {
        return true;
    }

    // Always carries a private second template parameter the AngelScript parser cannot tokenize.
    if (InTypeString.Contains(TEXT("TStrongObjectPtr<")))
    {
        return true;
    }

    // FCk_UI_Extension/FCk_UI_ExtensionPoint are not registered AngelScript types, so any
    // TSharedPtr exposing them fails the inner-identifier check on registration.
    if (InTypeString.Contains(TEXT("TSharedPtr<FCk_UI_Extension")))
    {
        return true;
    }

    if (InTypeString.Contains(TEXT("TDelegate<")) && InTypeString.Contains(TEXT("DelegateUserPolicy")))
    {
        return true;
    }

    if (InTypeString.Contains(TEXT("entt::")))
    {
        return true;
    }

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

    if (InTypeString.Contains(TEXT("FMemberReference")))
    {
        return true;
    }

    if (InTypeString.Contains(TEXT("FStreamableHandle")))
    {
        return true;
    }

    // AngelScript expects "const UMyClass", never the trailing-const spelling "UMyClass const".
    if (InTypeString.EndsWith(TEXT(" const")) || InTypeString.EndsWith(TEXT("*const")))
    {
        return true;
    }

    // Same trailing-const spelling, but nested inside template parameters: "<UObject const>" / "<UObject const,".
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

inline auto
IsTypeValidForAngelScript(
    const FString& InTypeString) -> bool
{
    return NOT ContainsUnsupportedTypePattern(InTypeString);
}

} // namespace ck::angelscript

#endif // WITH_ANGELSCRIPT_CK
