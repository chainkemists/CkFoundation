#include "CkEnsure_Tracker.h"

#include "CkCore/Ensure/CkEnsure_Log.h"

#include <Misc/Crc.h>
#include <Misc/ScopeLock.h>

// --------------------------------------------------------------------------------------------------------------------
namespace ck::ensure
{
    auto
        FCk_EnsureSignature::
        operator==(
            const FCk_EnsureSignature& InOther) const
        -> bool
    {
        return File == InOther.File
            && Line == InOther.Line
            && Expression == InOther.Expression
            && ScriptSite == InOther.ScriptSite;
    }

    auto
        FCk_EnsureSignature::
        ToDisplayString() const
        -> FString
    {
        const auto& NativeSite = FString::Printf(TEXT("%s:%d [%s]"), *File.ToString(), Line, *Expression);
        if (ScriptSite.IsEmpty())
        { return NativeSite; }

        return FString::Printf(TEXT("%s via %s"), *NativeSite, *ScriptSite);
    }

    auto
        GetTypeHash(
            const FCk_EnsureSignature& InSignature)
        -> uint32
    {
        auto Hash = FCrc::StrCrc32(*InSignature.File.ToString());
        Hash = HashCombineFast(Hash, static_cast<uint32>(InSignature.Line));
        Hash = HashCombineFast(Hash, FCrc::StrCrc32(*InSignature.Expression));
        return HashCombineFast(Hash, FCrc::StrCrc32(*InSignature.ScriptSite));
    }

    auto
        FCk_EnsureOccurrenceTracker::
        Record(
            const FCk_EnsureSignature& InSignature)
        -> FCk_EnsureRecordResult
    {
        const auto Lock = FScopeLock{&_Mutex};

        auto& SignatureCount = _OccurrenceCounts.FindOrAdd(InSignature);
        const auto IsFirstOccurrence = SignatureCount == 0;
        ++SignatureCount;
        ++_TotalCount;

        return FCk_EnsureRecordResult
        {
            IsFirstOccurrence,
            SignatureCount,
            _TotalCount,
            _OccurrenceCounts.Num(),
        };
    }

    auto
        FCk_EnsureOccurrenceTracker::
        GetTotalCount() const
        -> uint64
    {
        const auto Lock = FScopeLock{&_Mutex};
        return _TotalCount;
    }

    auto
        FCk_EnsureOccurrenceTracker::
        GetUniqueCount() const
        -> int32
    {
        const auto Lock = FScopeLock{&_Mutex};
        return _OccurrenceCounts.Num();
    }

    auto
        FCk_EnsureOccurrenceTracker::
        GetOccurrenceCount(
            const FCk_EnsureSignature& InSignature) const
        -> uint64
    {
        const auto Lock = FScopeLock{&_Mutex};
        const auto* const Count = _OccurrenceCounts.Find(InSignature);
        return Count != nullptr ? *Count : 0;
    }

    auto
        FCk_EnsureOccurrenceTracker::
        GetSnapshot() const
        -> TArray<FCk_EnsureOccurrence>
    {
        const auto Lock = FScopeLock{&_Mutex};

        auto Snapshot = TArray<FCk_EnsureOccurrence>{};
        Snapshot.Reserve(_OccurrenceCounts.Num());
        for (const auto& [Signature, Count] : _OccurrenceCounts)
        {
            Snapshot.Add(FCk_EnsureOccurrence{Signature, Count});
        }
        return Snapshot;
    }

    auto
        FCk_EnsureOccurrenceTracker::
        ConsumeSnapshotAndReset()
        -> TArray<FCk_EnsureOccurrence>
    {
        const auto Lock = FScopeLock{&_Mutex};

        auto Snapshot = TArray<FCk_EnsureOccurrence>{};
        Snapshot.Reserve(_OccurrenceCounts.Num());
        for (const auto& [Signature, Count] : _OccurrenceCounts)
        {
            Snapshot.Add(FCk_EnsureOccurrence{Signature, Count});
        }

        _OccurrenceCounts.Reset();
        _TotalCount = 0;
        return Snapshot;
    }

    auto
        FCk_EnsureOccurrenceTracker::
        Reset()
        -> void
    {
        const auto Lock = FScopeLock{&_Mutex};
        _OccurrenceCounts.Reset();
        _TotalCount = 0;
    }

    auto
        Get_EnsureOccurrenceTracker()
        -> FCk_EnsureOccurrenceTracker&
    {
        static auto Tracker = FCk_EnsureOccurrenceTracker{};
        return Tracker;
    }

    auto
        Request_LogEnsureOccurrenceSummaryAndReset()
        -> void
    {
        auto Snapshot = Get_EnsureOccurrenceTracker().ConsumeSnapshotAndReset();
        Snapshot.Sort([](const FCk_EnsureOccurrence& InA, const FCk_EnsureOccurrence& InB)
        {
            return InA.Signature.ToDisplayString() < InB.Signature.ToDisplayString();
        });

        for (const auto& Occurrence : Snapshot)
        {
            if (Occurrence.Count <= 1)
            { continue; }

            Warning(
                TEXT("Ensure aggregate: [{}] occurrences ([{}] repeats suppressed) at [{}]"),
                Occurrence.Count,
                Occurrence.Count - 1,
                Occurrence.Signature.ToDisplayString());
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
