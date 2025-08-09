#pragma once

#include "cleantype/details/cleantype_clean.hpp"

namespace ck
{
#if WITH_ANGELSCRIPT_CK
    template <typename T>
    auto Get_RuntimeTypeToString_AngelScript() -> FString
    {
        if constexpr (std::is_same_v<T, float>)
        {
            return TEXT("float32");
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            return TEXT("float64");
        }
        else if constexpr (std::is_same_v<T, uint8>)
        {
            return TEXT("uint8");
        }
        else if constexpr (std::is_same_v<T, uint32>)
        {
            return TEXT("uint32");
        }
        else if constexpr (std::is_same_v<T, uint64>)
        {
            return TEXT("uint64");
        }
        else if constexpr (std::is_same_v<T, int8>)
        {
            return TEXT("int8");
        }
        else if constexpr (std::is_same_v<T, int32>)
        {
            return TEXT("int32");
        }
        else if constexpr (std::is_same_v<T, int64>)
        {
            return TEXT("int64");
        }
        else if constexpr (std::is_enum_v<T>)
        {
            const auto& CleanName = cleantype::clean<T>();
            const auto CleanNameStr = FString{static_cast<int32>(CleanName.length()), CleanName.data()};
            return CleanNameStr.Replace(TEXT("enum "), TEXT(""), ESearchCase::CaseSensitive);
        }
        else if constexpr (std::is_pointer_v<T>)
        {
            const auto& CleanName = cleantype::clean<T>();
            const auto CleanNameStr = FString{static_cast<int32>(CleanName.length()), CleanName.data()};
            return CleanNameStr.Replace(TEXT("*"), TEXT(""), ESearchCase::IgnoreCase);
        }

        const auto& CleanName = cleantype::clean<T>();
        auto CleanNameStr = FString{static_cast<int32>(CleanName.length()), CleanName.data()};

        // Helper lambda to perform word-boundary replacement using string operations
        auto ReplaceWholeWord = [](FString& InOutString, const FString& SearchFor, const FString& ReplaceWith) -> void
        {
            int32 StartPos = 0;

            while (true)
            {
                const int32 FoundPos = InOutString.Find(SearchFor, ESearchCase::CaseSensitive, ESearchDir::FromStart, StartPos);
                if (FoundPos == INDEX_NONE)
                {
                    break;
                }

                // Check word boundaries
                const bool IsWordStart = (FoundPos == 0) ||
                                  (NOT FChar::IsAlnum(InOutString[FoundPos - 1]) && InOutString[FoundPos - 1] != TEXT('_'));

                const int32 EndPos = FoundPos + SearchFor.Len();
                const bool IsWordEnd = (EndPos >= InOutString.Len()) ||
                                (NOT FChar::IsAlnum(InOutString[EndPos]) && InOutString[EndPos] != TEXT('_'));

                if (IsWordStart && IsWordEnd)
                {
                    // Perform the replacement
                    FString Before = InOutString.Left(FoundPos);
                    FString After = InOutString.Mid(EndPos);
                    InOutString = Before + ReplaceWith + After;

                    StartPos = FoundPos + ReplaceWith.Len();
                }
                else
                {
                    StartPos = FoundPos + 1;
                }
            }
        };

        // Apply all the type replacements with word boundaries
        ReplaceWholeWord(CleanNameStr, TEXT("float"), TEXT("float32"));
        ReplaceWholeWord(CleanNameStr, TEXT("double"), TEXT("float64"));
        ReplaceWholeWord(CleanNameStr, TEXT("unsigned char"), TEXT("uint8"));
        ReplaceWholeWord(CleanNameStr, TEXT("unsigned int"), TEXT("uint32"));
        ReplaceWholeWord(CleanNameStr, TEXT("unsigned long long"), TEXT("uint64"));
        ReplaceWholeWord(CleanNameStr, TEXT("signed char"), TEXT("int8"));
        ReplaceWholeWord(CleanNameStr, TEXT("signed int"), TEXT("int32"));
        ReplaceWholeWord(CleanNameStr, TEXT("signed long long"), TEXT("int64"));

        ReplaceWholeWord(CleanNameStr, TEXT("__int8"), TEXT("int8"));
        ReplaceWholeWord(CleanNameStr, TEXT("__int16"), TEXT("int16"));
        ReplaceWholeWord(CleanNameStr, TEXT("__int32"), TEXT("int32"));
        ReplaceWholeWord(CleanNameStr, TEXT("__int64"), TEXT("int64"));
        ReplaceWholeWord(CleanNameStr, TEXT("unsigned __int8"), TEXT("uint8"));
        ReplaceWholeWord(CleanNameStr, TEXT("unsigned __int16"), TEXT("uint16"));
        ReplaceWholeWord(CleanNameStr, TEXT("unsigned __int32"), TEXT("uint32"));
        ReplaceWholeWord(CleanNameStr, TEXT("unsigned __int64"), TEXT("uint64"));

        ReplaceWholeWord(CleanNameStr, TEXT("UE::Math::TVector<float64>"), TEXT("FVector"));
        ReplaceWholeWord(CleanNameStr, TEXT("UE::Math::TVector2<float64>"), TEXT("FVector2D"));
        ReplaceWholeWord(CleanNameStr, TEXT("UE::Math::TRotator<float64>"), TEXT("FRotator"));
        ReplaceWholeWord(CleanNameStr, TEXT("UE::Math::TTransform<float64>"), TEXT("FTransform"));
        ReplaceWholeWord(CleanNameStr, TEXT("UE::Math::TIntPoint<int>"), TEXT("FIntPoint"));
        ReplaceWholeWord(CleanNameStr, TEXT("UE::Math::TIntVector3<int>"), TEXT("FIntVector"));

        // Handle enum removal
        CleanNameStr.ReplaceInline(TEXT("enum "), TEXT(""), ESearchCase::CaseSensitive);

        // Handle pointer removal
        CleanNameStr.ReplaceInline(TEXT("*"), TEXT(""), ESearchCase::IgnoreCase);

        // Helper lambda to simplify UE container types
        auto SimplifyContainerTypes = [](FString& InOutString) -> void
        {
            // Helper to parse template arguments
            auto ParseTemplateArgs = [](const TCHAR*& Stream) -> TArray<FString>
            {
                TArray<FString> Args;

                // Skip to opening bracket
                while (*Stream && *Stream != TEXT('<'))
                {
                    Stream++;
                }

                if (*Stream == TEXT('<'))
                {
                    Stream++; // Skip '<'

                    int32 BracketDepth = 0;
                    FString CurrentArg;

                    while (*Stream)
                    {
                        if (const TCHAR CurrentChar = *Stream;
                            CurrentChar == TEXT('<'))
                        {
                            BracketDepth++;
                            CurrentArg += CurrentChar;
                        }
                        else if (CurrentChar == TEXT('>'))
                        {
                            if (BracketDepth == 0)
                            {
                                // End of template args
                                if (NOT CurrentArg.IsEmpty())
                                {
                                    Args.Add(CurrentArg.TrimStartAndEnd());
                                }
                                Stream++; // Skip '>'
                                break;
                            }

                            BracketDepth--;
                            CurrentArg += CurrentChar;
                        }
                        else if (CurrentChar == TEXT(',') && BracketDepth == 0)
                        {
                            // Argument separator at top level
                            if (NOT CurrentArg.IsEmpty())
                            {
                                Args.Add(CurrentArg.TrimStartAndEnd());
                            }
                            CurrentArg.Empty();
                        }
                        else if (NOT FChar::IsWhitespace(CurrentChar) || BracketDepth > 0)
                        {
                            CurrentArg += CurrentChar;
                        }

                        Stream++;
                    }
                }

                return Args;
            };

            int32 StartPos = 0;

            while (true)
            {
                const int32 TArrayPos = InOutString.Find(TEXT("TArray"), ESearchCase::CaseSensitive, ESearchDir::FromStart, StartPos);
                const int32 TSetPos = InOutString.Find(TEXT("TSet"), ESearchCase::CaseSensitive, ESearchDir::FromStart, StartPos);
                const int32 TMapPos = InOutString.Find(TEXT("TMap"), ESearchCase::CaseSensitive, ESearchDir::FromStart, StartPos);
                const int32 TSharedPtrPos = InOutString.Find(TEXT("TSharedPtr"), ESearchCase::CaseSensitive, ESearchDir::FromStart, StartPos);
                const int32 TWeakPtrPos = InOutString.Find(TEXT("TWeakPtr"), ESearchCase::CaseSensitive, ESearchDir::FromStart, StartPos);

                // Find the earliest occurrence
                int32 NextPos = INDEX_NONE;
                FString ContainerType;

                if (TArrayPos != INDEX_NONE && (NextPos == INDEX_NONE || TArrayPos < NextPos))
                {
                    NextPos = TArrayPos;
                    ContainerType = TEXT("TArray");
                }
                if (TSetPos != INDEX_NONE && (NextPos == INDEX_NONE || TSetPos < NextPos))
                {
                    NextPos = TSetPos;
                    ContainerType = TEXT("TSet");
                }
                if (TMapPos != INDEX_NONE && (NextPos == INDEX_NONE || TMapPos < NextPos))
                {
                    NextPos = TMapPos;
                    ContainerType = TEXT("TMap");
                }
                if (TSharedPtrPos != INDEX_NONE && (NextPos == INDEX_NONE || TSharedPtrPos < NextPos))
                {
                    NextPos = TSharedPtrPos;
                    ContainerType = TEXT("TSharedPtr");
                }
                if (TWeakPtrPos != INDEX_NONE && (NextPos == INDEX_NONE || TWeakPtrPos < NextPos))
                {
                    NextPos = TWeakPtrPos;
                    ContainerType = TEXT("TWeakPtr");
                }

                if (NextPos == INDEX_NONE)
                {
                    break; // No more containers found
                }

                // Make sure it's a word boundary (not part of another identifier)
                bool IsWordBoundary = true;
                if (NextPos > 0)
                {
                    if (TCHAR PrevChar = InOutString[NextPos - 1];
                        FChar::IsAlnum(PrevChar) || PrevChar == TEXT('_'))
                    {
                        IsWordBoundary = false;
                    }
                }

                const int32 EndOfContainerName = NextPos + ContainerType.Len();
                if (EndOfContainerName < InOutString.Len())
                {
                    if (TCHAR NextChar = InOutString[EndOfContainerName];
                        FChar::IsAlnum(NextChar) || NextChar == TEXT('_'))
                    {
                        IsWordBoundary = false;
                    }
                }

                if (NOT IsWordBoundary)
                {
                    StartPos = NextPos + 1;
                    continue;
                }

                // Parse the template arguments starting from after the container name
                const TCHAR* Stream = *InOutString + EndOfContainerName;
                TArray<FString> Args = ParseTemplateArgs(Stream);

                FString Replacement;
                if (ContainerType == TEXT("TArray") && Args.Num() > 0)
                {
                    Replacement = TEXT("TArray<") + Args[0] + TEXT(">");
                }
                else if (ContainerType == TEXT("TSet") && Args.Num() > 0)
                {
                    Replacement = TEXT("TSet<") + Args[0] + TEXT(">");
                }
                else if (ContainerType == TEXT("TMap") && Args.Num() >= 2)
                {
                    Replacement = TEXT("TMap<") + Args[0] + TEXT(", ") + Args[1] + TEXT(">");
                }
                else if (ContainerType == TEXT("TSharedPtr") && Args.Num() > 0)
                {
                    // TSharedPtr<Type, DeleterPolicy> -> TSharedPtr<Type>
                    Replacement = TEXT("TSharedPtr<") + Args[0] + TEXT(">");
                }
                else if (ContainerType == TEXT("TWeakPtr") && Args.Num() > 0)
                {
                    // TWeakPtr<Type, Mode> -> TWeakPtr<Type>
                    Replacement = TEXT("TWeakPtr<") + Args[0] + TEXT(">");
                }
                else
                {
                    // If we couldn't parse properly, skip this occurrence
                    StartPos = NextPos + 1;
                    continue;
                }

                // Calculate the end position of the original template
                const int32 OriginalEndPos = Stream - *InOutString;

                // Replace the original container declaration with the simplified one
                FString Before = InOutString.Left(NextPos);
                FString After = InOutString.Mid(OriginalEndPos);
                InOutString = Before + Replacement + After;

                // Update start position for next search
                StartPos = NextPos + Replacement.Len();
            }
        };

        // Simplify container types to remove allocators and implementation details
        SimplifyContainerTypes(CleanNameStr);

        return CleanNameStr;
    }
#else
    template <typename T>
    auto Get_RuntimeTypeToString_AngelScript() -> FString
    {
        return ck::Get_RuntimeTypeToString<T>();
    }
#endif
}
