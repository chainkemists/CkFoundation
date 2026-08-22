#include "CkStats_Utils.h"

#include "CkCore/Engine/CkGameState.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/Engine.h>
#include <Engine/GameViewportClient.h>
#include <Engine/World.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>
#include <HAL/PlatformMemory.h>
#include <HAL/PlatformMisc.h>
#include <HAL/PlatformTime.h>
#include <Misc/App.h>
#include <Stats/StatsData.h>

#include <DynamicRHI.h>
#include <RHIGlobals.h>
#include <RenderTimer.h>

ENGINE_API extern float  GAverageFPS;
extern ENGINE_API uint64 GFrameCounter;

namespace ck_stats_utils
{
    constexpr double InvGB = 1.0 / (1024.0 * 1024.0 * 1024.0);

    // The engine's own test for whether a GPU timing exists is `RawGPUFrameTime > 0`
    // (FStatUnitData::DrawStat). Zero cycles means the RHI handed back no timestamp, so reporting it
    // as a zero-millisecond cost would read as an infinitely fast GPU.
    auto
        Get_GpuAvailability(
            uint32 InGpuCycles)
        -> ECk_Stats_MetricAvailability
    {
        if (GUsingNullRHI)
        {
            return ECk_Stats_MetricAvailability::Unavailable_NullRhi;
        }

        if (InGpuCycles == 0)
        {
            return ECk_Stats_MetricAvailability::Unavailable_NoGpuTimestamps;
        }

        return ECk_Stats_MetricAvailability::Available;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Stats_UE::
    Get_FPS()
    -> float
{
    return GAverageFPS;
}

auto
    UCk_Utils_Stats_UE::
    Get_FrameTimeMs()
    -> float
{
    return static_cast<float>(FApp::GetDeltaTime() * 1000.0);
}

auto
    UCk_Utils_Stats_UE::
    Get_FrameCount()
    -> int64
{
    return static_cast<int64>(GFrameCounter);
}

auto
    UCk_Utils_Stats_UE::
    Get_ThreadTimings()
    -> FCk_Stats_ThreadTimings
{
    // Every expression below mirrors FStatUnitData::DrawStat (Engine/Private/UnrealClient.cpp) so
    // these numbers and `stat unit`'s cannot drift apart. The frame delta is deliberately last
    // frame's rather than FApp::GetDeltaTime(): the engine uses that form there because it accounts
    // for end-of-frame idling and therefore lines up with the thread times beside it.
    const auto FrameTimeMs        = static_cast<float>((FApp::GetCurrentTime() - FApp::GetLastTime()) * 1000.0);
    const auto GameThreadTimeMs   = static_cast<float>(FPlatformTime::ToMilliseconds(GGameThreadTime));
    const auto RenderThreadTimeMs = static_cast<float>(FPlatformTime::ToMilliseconds(GRenderThreadTime));
    const auto RhiThreadTimeMs    = static_cast<float>(FPlatformTime::ToMilliseconds(GRHIThreadTime));

    // Index 0 only: a level-performance reading on a workstation. Splitting per-GPU would need a
    // whole reporting axis, so the multi-GPU case is a deliberate omission rather than an oversight.
    const auto GpuCycles = RHIGetGPUFrameCycles();
    const auto GpuTimeMs = static_cast<float>(FPlatformTime::ToMilliseconds(GpuCycles));

    return FCk_Stats_ThreadTimings
    {
        FrameTimeMs,
        GameThreadTimeMs,
        RenderThreadTimeMs,
        RhiThreadTimeMs,
        GpuTimeMs,
        ck_stats_utils::Get_GpuAvailability(GpuCycles)
    };
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Stats_UE::
    Get_PingMs(
        const UObject* InContext)
    -> float
{
    if (ck::Is_NOT_Valid(InContext))
    { return -1.f; }

    const auto* World = InContext->GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return -1.f; }

    const auto* PC = World->GetFirstPlayerController();

    if (ck::Is_NOT_Valid(PC) || NOT PC->IsLocalController())
    { return -1.f; }

    const auto* PS = PC->GetPlayerState<APlayerState>();

    if (ck::Is_NOT_Valid(PS))
    { return -1.f; }

    return PS->GetPingInMilliseconds();
}

auto
    UCk_Utils_Stats_UE::
    Get_ServerFPS(
        const UObject* InContext)
    -> float
{
    if (ck::Is_NOT_Valid(InContext))
    { return -1.f; }

    const auto* World = InContext->GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return -1.f; }

    const auto* GS = World->GetGameState<ACk_GameState_UE>();

    if (ck::Is_NOT_Valid(GS))
    { return -1.f; }

    return GS->Get_ServerFPS();
}

auto
    UCk_Utils_Stats_UE::
    Get_NetworkConnectionType()
    -> ECk_Stats_NetworkConnectionType
{
    switch (FPlatformMisc::GetNetworkConnectionType())
    {
        case ENetworkConnectionType::None:         return ECk_Stats_NetworkConnectionType::NotConnected;
        case ENetworkConnectionType::AirplaneMode: return ECk_Stats_NetworkConnectionType::AirplaneMode;
        case ENetworkConnectionType::Cell:          return ECk_Stats_NetworkConnectionType::Cell;
        case ENetworkConnectionType::WiFi:          return ECk_Stats_NetworkConnectionType::WiFi;
        case ENetworkConnectionType::WiMAX:         return ECk_Stats_NetworkConnectionType::WiMAX;
        case ENetworkConnectionType::Bluetooth:     return ECk_Stats_NetworkConnectionType::Bluetooth;
        case ENetworkConnectionType::Ethernet:      return ECk_Stats_NetworkConnectionType::Ethernet;
        default:                                    return ECk_Stats_NetworkConnectionType::Unknown;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Stats_UE::
    Get_RAM_UsedGB()
    -> float
{
    const auto& Stats = FPlatformMemory::GetStats();
    return static_cast<float>(Stats.UsedPhysical * ck_stats_utils::InvGB);
}

auto
    UCk_Utils_Stats_UE::
    Get_RAM_AvailableGB()
    -> float
{
    const auto& Stats = FPlatformMemory::GetStats();
    return static_cast<float>(Stats.AvailablePhysical * ck_stats_utils::InvGB);
}

auto
    UCk_Utils_Stats_UE::
    Get_RAM_TotalGB()
    -> float
{
    const auto& Stats = FPlatformMemory::GetStats();
    return static_cast<float>(Stats.TotalPhysical * ck_stats_utils::InvGB);
}

auto
    UCk_Utils_Stats_UE::
    Get_VRAM_UsedGB()
    -> float
{
#if STATS
    constexpr auto RefreshIntervalSeconds = 1.0;

    static float  CachedVRAM      = 0.f;
    static double LastUpdateTime   = 0.0;

    const double Now = FPlatformTime::Seconds();

    if (Now - LastUpdateTime >= RefreshIntervalSeconds)
    {
        LastUpdateTime = Now;

        auto StatMessages = TArray<FStatMessage>{};
        GetPermanentStats(StatMessages);

        const auto StatGroupRHI = FName{FStatGroup_STATGROUP_RHI::GetGroupName()};

        int64 TotalBytes = 0;
        for (const auto& Msg : StatMessages)
        {
            if (Msg.NameAndInfo.GetGroupName() == StatGroupRHI &&
                Msg.NameAndInfo.GetFlag(EStatMetaFlags::IsMemory))
            {
                TotalBytes += Msg.GetValue_int64();
            }
        }

        CachedVRAM = static_cast<float>(TotalBytes * ck_stats_utils::InvGB);
    }

    return CachedVRAM;
#else
    return 0.f;
#endif
}

auto
    UCk_Utils_Stats_UE::
    Get_MemoryPressureStatus()
    -> ECk_Stats_MemoryPressureStatus
{
    switch (FPlatformMemory::GetStats().GetMemoryPressureStatus())
    {
        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Nominal:  return ECk_Stats_MemoryPressureStatus::Nominal;
        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Warning:  return ECk_Stats_MemoryPressureStatus::Warning;
        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Critical: return ECk_Stats_MemoryPressureStatus::Critical;
        default:                                                           return ECk_Stats_MemoryPressureStatus::Unknown;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Stats_UE::
    Get_CPUBrand()
    -> FString
{
    static const FString Brand = FPlatformMisc::GetCPUBrand().TrimStartAndEnd();
    return Brand;
}

auto
    UCk_Utils_Stats_UE::
    Get_CPUCoreCount()
    -> int32
{
    return FPlatformMisc::NumberOfCores();
}

auto
    UCk_Utils_Stats_UE::
    Get_CPUThreadCount()
    -> int32
{
    return FPlatformMisc::NumberOfCoresIncludingHyperthreads();
}

auto
    UCk_Utils_Stats_UE::
    Get_GPUBrand()
    -> FString
{
    static const FString Brand = FPlatformMisc::GetPrimaryGPUBrand().TrimStartAndEnd();
    return Brand;
}

auto
    UCk_Utils_Stats_UE::
    Get_OSVersion()
    -> FString
{
    static const FString Version = FPlatformMisc::GetOSVersion();
    return Version;
}

auto
    UCk_Utils_Stats_UE::
    Get_BuildConfig()
    -> FString
{
#if UE_BUILD_SHIPPING
    return TEXT("Shipping");
#elif UE_BUILD_TEST
    return TEXT("Test");
#else
    return TEXT("Development");
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Stats_UE::
    Get_IsStatEnabled(
        const FString& InStatName)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(GEngine), TEXT("Invalid GEngine"))
    { return {}; }

    const auto& GameViewport = GEngine->GameViewport;

    CK_ENSURE_IF_NOT(ck::IsValid(GameViewport), TEXT("Invalid GameViewport"))
    { return {}; }

    return GameViewport->IsStatEnabled(InStatName);
}

// --------------------------------------------------------------------------------------------------------------------
