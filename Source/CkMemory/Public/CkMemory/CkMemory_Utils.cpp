#include "CkMemory_Utils.h"

#include "Windows/WindowsPlatformMemory.h"
#include <Kismet/GameplayStatics.h>
#include <Stats/StatsData.h>

#include <numeric>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Memory_UE::
    Get_MemoryCountSnapshot(
        const UObject* InContext)
    -> FCk_Utils_Memory_MemoryCountSnapshot_Result
{
#if STATS
    static constexpr float InvKB = 1.0f / 1024.0f;
    static constexpr float InvMB = InvKB * InvKB;
    static constexpr float InvGB = InvKB * InvMB;

    const auto& Stats = FPlatformMemory::GetStats();

    TArray<FStatMessage> OutStatMessages;
    GetPermanentStats(OutStatMessages);

    const FName NameStatgroupRHI(FStatGroup_STATGROUP_RHI::GetGroupName());

    const int64 TotalRHIMemory = std::accumulate(OutStatMessages.begin(), OutStatMessages.end(), 0LL,
    [&](int64 InAccumulator, const FStatMessage& InStatMessage) {
        const auto& MetaInfo = InStatMessage.NameAndInfo;

        if (const auto& LastGroup = MetaInfo.GetGroupName(); LastGroup == NameStatgroupRHI && MetaInfo.GetFlag(EStatMetaFlags::IsMemory)) {
            return InAccumulator + InStatMessage.GetValue_int64();
        }

        return InAccumulator;
    }
    );

    const auto MemoryCountSnapshot = FCk_Utils_Memory_MemoryCountSnapshot_Result{}
                                        .Set_RHIMemoryUsed          (TotalRHIMemory          * InvGB)
                                        .Set_PhysicalMemoryUsed     (Stats.UsedPhysical      * InvGB)
                                        .Set_PhysicalMemoryAvailable(Stats.AvailablePhysical * InvGB)
                                        .Set_PhysicalMemoryTotal    (Stats.TotalPhysical     * InvGB)
                                        .Set_VirtualMemoryUsed      (Stats.UsedVirtual       * InvGB)
                                        .Set_VirtualMemoryAvailable (Stats.AvailableVirtual  * InvGB)
                                        .Set_VirtualMemoryTotal     (Stats.TotalVirtual      * InvGB);

    return MemoryCountSnapshot;
#else
    return {};
#endif
}

// --------------------------------------------------------------------------------------------------------------------
