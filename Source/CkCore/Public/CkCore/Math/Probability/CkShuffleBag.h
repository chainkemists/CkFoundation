#pragma once

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"

#include <Containers/Array.h>
#include <Math/RandomStream.h>
#include <Math/UnrealMathUtility.h>
#include <Misc/Optional.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    /**
     * Sampling WITHOUT replacement over a fixed listing of outcomes (the "shuffle bag" pattern,
     * same family as Tetris' 7-bag randomizer). Fill the bag with outcomes at their intended
     * ratio (e.g. 2 crits + 6 misses for a 25% rate), Draw removes a random one, and an emptied
     * bag automatically refills and reshuffles. Compared to independent rolls: every full cycle
     * of Get_NumTotal() draws yields each outcome EXACTLY as often as listed, and the worst-case
     * gap between two occurrences of an outcome is bounded instead of unbounded.
     *
     * Deliberately predictable in the small: the tail of a nearly-empty bag can be inferred by
     * counting draws — that is the streak protection working. Do not use it where outcomes must
     * stay unpredictable under observation (e.g. adversarial/competitive procs); plain RNG or a
     * pseudo-random-distribution scheme fits there.
     *
     * Seeded construction draws from an owned FRandomStream (deterministic per seed); unseeded
     * construction uses FMath's global RNG.
     */
    template <typename T>
    class TShuffleBag
    {
        CK_GENERATED_BODY(TShuffleBag<T>);

    public:
        using ValueType = T;

    public:
        TShuffleBag() = default;
        explicit TShuffleBag(TArray<T> InContents);
        TShuffleBag(TArray<T> InContents, int32 InSeed);

    public:
        auto Draw() -> T;
        auto Reset() -> void;

    public:
        auto Get_IsEmpty() const -> bool;
        auto Get_NumTotal() const -> int32;
        auto Get_NumRemainingInCycle() const -> int32;

    private:
        auto DoInitialize(TArray<T>&& InContents) -> void;
        auto DoRefillAndShuffle() -> void;
        auto DoRandRange(int32 InMinInclusive, int32 InMaxInclusive) -> int32;

    private:
        TArray<T> _FullListing;
        TArray<T> _Current;
        TOptional<FRandomStream> _Stream;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    TShuffleBag<T>::TShuffleBag(TArray<T> InContents)
    {
        DoInitialize(MoveTemp(InContents));
    }

    template <typename T>
    TShuffleBag<T>::TShuffleBag(TArray<T> InContents, int32 InSeed)
        : _Stream(FRandomStream{InSeed})
    {
        DoInitialize(MoveTemp(InContents));
    }

    template <typename T>
    auto TShuffleBag<T>::Draw() -> T
    {
        CK_ENSURE_IF_NOT(NOT Get_IsEmpty(), TEXT("Draw called on an empty TShuffleBag"))
        { return {}; }

        if (_Current.IsEmpty())
        { DoRefillAndShuffle(); }

        return _Current.Pop(EAllowShrinking::No);
    }

    template <typename T>
    auto TShuffleBag<T>::Reset() -> void
    {
        DoRefillAndShuffle();
    }

    template <typename T>
    auto TShuffleBag<T>::Get_IsEmpty() const -> bool
    {
        return _FullListing.IsEmpty();
    }

    template <typename T>
    auto TShuffleBag<T>::Get_NumTotal() const -> int32
    {
        return _FullListing.Num();
    }

    template <typename T>
    auto TShuffleBag<T>::Get_NumRemainingInCycle() const -> int32
    {
        return _Current.Num();
    }

    template <typename T>
    auto TShuffleBag<T>::DoInitialize(TArray<T>&& InContents) -> void
    {
        CK_ENSURE_IF_NOT(NOT InContents.IsEmpty(), TEXT("TShuffleBag constructed with empty Contents"))
        { return; }

        _FullListing = MoveTemp(InContents);
        DoRefillAndShuffle();
    }

    template <typename T>
    auto TShuffleBag<T>::DoRefillAndShuffle() -> void
    {
        _Current = _FullListing;

        for (auto Index = _Current.Num() - 1; Index > 0; --Index)
        {
            const auto SwapIndex = DoRandRange(0, Index);

            if (SwapIndex != Index)
            { _Current.Swap(Index, SwapIndex); }
        }
    }

    template <typename T>
    auto TShuffleBag<T>::DoRandRange(int32 InMinInclusive, int32 InMaxInclusive) -> int32
    {
        if (_Stream.IsSet())
        { return _Stream->RandRange(InMinInclusive, InMaxInclusive); }

        return FMath::RandRange(InMinInclusive, InMaxInclusive);
    }
}

// --------------------------------------------------------------------------------------------------------------------
