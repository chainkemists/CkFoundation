#pragma once

#include <CoreMinimal.h>

#include <GameplayTagContainer.h>

#include <type_traits>

// --------------------------------------------------------------------------------------------------------------------
// The value-only fence every PERSISTED field is judged against.
//
// Apart from the serializer for the same reason the debug capture's fence is apart from the captures:
// nothing here names a field type, so nothing that includes this depends on any of them.
//
// STRICTER THAN THE DEBUG FENCE BESIDE IT, and the difference is the whole point. A debug capture is
// read by the process that made it, where an FName and an FGameplayTag are stable numbers. A blob is
// read by a process that never saw the table those numbers index, so a persisted FName is a number
// that means something else - or nothing - the next time it is loaded. Both are REJECTED here, and the
// tags a field carries travel as indices into a table of their own NAMES written beside the field.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * What a member of a persisted value is allowed to BE.
     *
     * True for the types a blob is actually made of - numbers, enums, the engine's own value structs,
     * and arrays of those - and false for everything else BY DEFAULT, which is what makes this a fence
     * rather than a blacklist: a raw pointer, a TObjectPtr, a TWeakObjectPtr, a TSharedPtr, an
     * FCk_Handle and any type nobody has judged yet all fail the same way.
     *
     * A persisted type of the module's own specialises this beside the assertion that judges its
     * members, so a composite built out of already-judged parts passes in turn.
     */
    template <typename T>
    struct TIsPersistableValue
    {
        static constexpr bool Value = std::is_arithmetic_v<T> || std::is_enum_v<T>;
    };

    template <>
    struct TIsPersistableValue<FVector>
    {
        static constexpr bool Value = true;
    };

    template <>
    struct TIsPersistableValue<FVector2D>
    {
        static constexpr bool Value = true;
    };

    template <>
    struct TIsPersistableValue<FIntPoint>
    {
        static constexpr bool Value = true;
    };

    template <>
    struct TIsPersistableValue<FBox>
    {
        static constexpr bool Value = true;
    };

    template <>
    struct TIsPersistableValue<FTransform>
    {
        static constexpr bool Value = true;
    };

    /** FREE TEXT ONLY - a description a reader shows a human. A string that IDENTIFIES something is a
     *  name by another spelling and belongs in the blob's name table, where a load can say so when it
     *  no longer resolves. */
    template <>
    struct TIsPersistableValue<FString>
    {
        static constexpr bool Value = true;
    };

    /** Rejected, unlike in the debug fence: an FName is an index into a per-process table, and the next
     *  process to load the blob numbers its names differently. */
    template <>
    struct TIsPersistableValue<FName>
    {
        static constexpr bool Value = false;
    };

    /** Rejected for the reason the FName above is - a tag IS one FName - and written as an index into
     *  the blob's name table instead, so a load that cannot resolve it answers a status. */
    template <>
    struct TIsPersistableValue<FGameplayTag>
    {
        static constexpr bool Value = false;
    };

    /** A container of tags is a set of the same per-process numbers, and travels as an array of table
     *  indices. */
    template <>
    struct TIsPersistableValue<FGameplayTagContainer>
    {
        static constexpr bool Value = false;
    };

    /** Spelled out rather than left to the default so the reason is readable where it bites: a blob
     *  outlives the process that wrote it, and an address does not survive the trip. */
    template <typename T>
    struct TIsPersistableValue<T*>
    {
        static constexpr bool Value = false;
    };

    template <typename T, ESPMode T_Mode>
    struct TIsPersistableValue<TSharedPtr<T, T_Mode>>
    {
        static constexpr bool Value = false;
    };

    template <typename T, typename T_Allocator>
    struct TIsPersistableValue<TArray<T, T_Allocator>>
    {
        static constexpr bool Value = TIsPersistableValue<T>::Value;
    };

    /** Every member listed must pass. The language cannot enumerate a struct's members, so a type is
     *  judged by NAMING them - which is why the list lives beside the type it describes, and why a
     *  member added later belongs on it.
     *
     *  The members are DECAYED first: a reflected type keeps its members private, so the only way to
     *  name one is through its getter, and a getter hands back a const reference. */
    template <typename... T_Members>
    inline constexpr bool kPersistableMembersAreValues =
        (TIsPersistableValue<std::decay_t<T_Members>>::Value && ...);
}

// --------------------------------------------------------------------------------------------------------------------
