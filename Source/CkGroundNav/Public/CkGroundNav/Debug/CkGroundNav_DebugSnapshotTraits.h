#pragma once

#include "CkCore/Time/CkTime.h"

#include <CoreMinimal.h>

#include <GameplayTagContainer.h>

#include <type_traits>

// --------------------------------------------------------------------------------------------------------------------
// The value-only fence every GroundNav debug capture is judged against.
//
// Apart from the captures themselves because it is not about any one of them: a snapshot of a field, a
// per-agent diagnostics fragment and whatever a later view captures all owe the same guarantee, and a
// fence that lived beside one of them would make the others include that one to reach it. Nothing here
// names a capture type, so nothing that includes this depends on any of them.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * What a member of a captured value is allowed to BE.
     *
     * The value-only contract is a comment until something checks it: every member is a copy today
     * because every member was written as one, and one TObjectPtr added in good faith turns a drawable
     * value into a dangling read that surfaces only after the world it came from is gone.
     *
     * True for the types a capture is actually made of - numbers, enums, the engine's own value
     * structs, and arrays of those - and false for everything else BY DEFAULT, which is what makes
     * this a fence rather than a blacklist: a raw pointer, a TObjectPtr, a TWeakObjectPtr, a
     * TSharedPtr, an FCk_Handle and any type nobody has judged yet all fail the same way.
     *
     * A capture that carries a type of its own specialises this beside that type, where the reason it
     * is a value can be read next to what it is made of.
     */
    template <typename T>
    struct TIsDebugSnapshotValue
    {
        static constexpr bool Value = std::is_arithmetic_v<T> || std::is_enum_v<T>;
    };

    template <>
    struct TIsDebugSnapshotValue<FVector>
    {
        static constexpr bool Value = true;
    };

    template <>
    struct TIsDebugSnapshotValue<FVector2D>
    {
        static constexpr bool Value = true;
    };

    template <>
    struct TIsDebugSnapshotValue<FBox>
    {
        static constexpr bool Value = true;
    };

    template <>
    struct TIsDebugSnapshotValue<FName>
    {
        static constexpr bool Value = true;
    };

    template <>
    struct TIsDebugSnapshotValue<FString>
    {
        static constexpr bool Value = true;
    };

    /** A tag is one FName and a world date is one double, so both are values by the same reading of
     *  the word this fence already uses for FName and for the arithmetic types. */
    template <>
    struct TIsDebugSnapshotValue<FGameplayTag>
    {
        static constexpr bool Value = true;
    };

    template <>
    struct TIsDebugSnapshotValue<FCk_Time>
    {
        static constexpr bool Value = true;
    };

    template <typename T, typename T_Allocator>
    struct TIsDebugSnapshotValue<TArray<T, T_Allocator>>
    {
        static constexpr bool Value = TIsDebugSnapshotValue<T>::Value;
    };

    /** Every member listed must pass. The language cannot enumerate a struct's members, so a type is
     *  judged by NAMING them - which is why the list lives beside the type it describes, and why a
     *  member added later belongs on it. */
    template <typename... T_Members>
    inline constexpr bool kDebugSnapshotMembersAreValues = (TIsDebugSnapshotValue<T_Members>::Value && ...);
}

// --------------------------------------------------------------------------------------------------------------------
