#pragma once

#include "CkCore/Build/CkBuild_Macros.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
namespace ck::ensure
{
    struct CKCORE_API FCk_EnsureSignature
    {
        FName File;
        int32 Line = 0;
        FString Expression;
        FString ScriptSite;

        auto operator==(const FCk_EnsureSignature& InOther) const -> bool;
        auto ToDisplayString() const -> FString;
    };

    auto CKCORE_API GetTypeHash(const FCk_EnsureSignature& InSignature) -> uint32;

    struct CKCORE_API FCk_EnsureOccurrence
    {
        FCk_EnsureSignature Signature;
        uint64 Count = 0;
    };

    struct CKCORE_API FCk_EnsureRecordResult
    {
        bool IsFirstOccurrence = false;
        uint64 SignatureCount = 0;
        uint64 TotalCount = 0;
        int32 UniqueCount = 0;
    };

    class CKCORE_API FCk_EnsureOccurrenceTracker
    {
    public:
        auto Record(const FCk_EnsureSignature& InSignature) -> FCk_EnsureRecordResult;

        auto GetTotalCount() const -> uint64;
        auto GetUniqueCount() const -> int32;
        auto GetOccurrenceCount(const FCk_EnsureSignature& InSignature) const -> uint64;
        auto GetSnapshot() const -> TArray<FCk_EnsureOccurrence>;

        auto ConsumeSnapshotAndReset() -> TArray<FCk_EnsureOccurrence>;
        auto Reset() -> void;

    private:
        mutable FCriticalSection _Mutex;
        TMap<FCk_EnsureSignature, uint64> _OccurrenceCounts;
        uint64 _TotalCount = 0;
    };

    CKCORE_API auto Get_EnsureOccurrenceTracker() -> FCk_EnsureOccurrenceTracker&;
    CKCORE_API auto Request_LogEnsureOccurrenceSummaryAndReset() -> void;
}

// --------------------------------------------------------------------------------------------------------------------
