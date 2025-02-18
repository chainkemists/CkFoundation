#include "CkMemory_Subsystem.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Windows/WindowsPlatformMemory.h"

// ReSharper disable once CppUnusedIncludeDirective
#include <EngineMinimal.h>
#include <numeric>

// ReSharper disable once CppUnusedIncludeDirective
#include <Kismet/GameplayStatics.h>

#include <Misc/ScopeLock.h>

#if STATS
// ReSharper disable once CppUnusedIncludeDirective
#include <Stats/StatsData.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

class FMemoryStatsUpdateTask : public FRunnable
{
private:
    static constexpr float InvKB = 1.0f / 1024.0f;
    static constexpr float InvMB = InvKB * InvKB;
    static constexpr float InvGB = InvKB * InvMB;

    FThreadSafeCounter _StopTaskCounter;
    mutable FCriticalSection _DataLock;
    FCk_Utils_Memory_MemoryCountSnapshot_Result _CurrentSnapshot;

public:
    auto
        Init()
        -> bool override
    {
        return true;
    }

    auto
        Run()
        -> uint32 override
    {
        while (_StopTaskCounter.GetValue() == 0)
        {
#if STATS
            auto StatMessages = TArray<FStatMessage>{};
            GetPermanentStats(StatMessages);

            const auto StatGroupRHI = FName{FStatGroup_STATGROUP_RHI::GetGroupName()};
            const int64 TotalRHIMemory = std::accumulate(
                StatMessages.begin(),
                StatMessages.end(),
                0LL,
                [&](int64 InAccumulator, const FStatMessage& InStatMessage) {
                    const auto& MetaInfo = InStatMessage.NameAndInfo;
                    if (const auto& LastGroup = MetaInfo.GetGroupName();
                        LastGroup == StatGroupRHI &&
                        MetaInfo.GetFlag(EStatMetaFlags::IsMemory))
                    {
                        return InAccumulator + InStatMessage.GetValue_int64();
                    }
                    return InAccumulator;
                }
            );

            const auto& Stats = FPlatformMemory::GetStats();

            FCk_Utils_Memory_MemoryCountSnapshot_Result NewSnapshot;
            NewSnapshot.Set_RHIMemoryUsed          (TotalRHIMemory          * InvGB)
                      .Set_PhysicalMemoryUsed      (Stats.UsedPhysical      * InvGB)
                      .Set_PhysicalMemoryAvailable (Stats.AvailablePhysical * InvGB)
                      .Set_PhysicalMemoryTotal     (Stats.TotalPhysical     * InvGB)
                      .Set_VirtualMemoryUsed       (Stats.UsedVirtual       * InvGB)
                      .Set_VirtualMemoryAvailable  (Stats.AvailableVirtual  * InvGB)
                      .Set_VirtualMemoryTotal      (Stats.TotalVirtual      * InvGB);

            {
                FScopeLock Lock(&_DataLock);
                _CurrentSnapshot = NewSnapshot;
            }
#endif
            // Sleep for 1 second before next update
            FPlatformProcess::Sleep(1.0f);
        }
        return 0;
    }

    auto
        Stop()
            -> void override
    {
        _StopTaskCounter.Increment();
    }

    auto
        GetCurrentSnapshot() const
            -> FCk_Utils_Memory_MemoryCountSnapshot_Result
    {
        auto Lock = FScopeLock{&_DataLock};
        return _CurrentSnapshot;
    }
};

// --------------------------------------------------------------------------------------------------------------------

TUniquePtr<FMemoryStatsUpdateTask> UCk_Stats_Subsystem_UE::_MemoryUpdateTask;
FRunnableThread* UCk_Stats_Subsystem_UE::_MemoryUpdateThread = nullptr;

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Stats_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

#if STATS
    if (ck::Is_NOT_Valid(_MemoryUpdateTask))
    {
        _MemoryUpdateTask = MakeUnique<FMemoryStatsUpdateTask>();
        _MemoryUpdateThread = FRunnableThread::Create(
            _MemoryUpdateTask.Get(),
            TEXT("MemoryStatsThread"),
            0,
            TPri_BelowNormal
        );
    }
#endif STATS
}

auto
    UCk_Stats_Subsystem_UE::
    Deinitialize()
    -> void
{
    Super::Deinitialize();

#if STATS
    if (ck::IsValid(_MemoryUpdateTask))
    {
        _MemoryUpdateTask->Stop();
        _MemoryUpdateThread->WaitForCompletion();
        delete _MemoryUpdateThread;
        _MemoryUpdateThread = nullptr;
        _MemoryUpdateTask.Reset();
    }
#endif STATS
}

auto
    UCk_Stats_Subsystem_UE::
    Get_MemoryCountSnapshot(
        const UObject* InContext)
    -> FCk_Utils_Memory_MemoryCountSnapshot_Result
{
    if (_MemoryUpdateTask)
    {
        return _MemoryUpdateTask->GetCurrentSnapshot();
    }
    return FCk_Utils_Memory_MemoryCountSnapshot_Result{};
}
