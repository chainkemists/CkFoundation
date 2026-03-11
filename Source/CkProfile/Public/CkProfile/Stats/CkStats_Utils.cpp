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

// Engine globals maintained each frame
ENGINE_API extern float  GAverageFPS;
extern ENGINE_API uint64 GFrameCounter;

namespace
{
    constexpr double InvGB = 1.0 / (1024.0 * 1024.0 * 1024.0);
}

// ====================================================================================================================
// Performance
// ====================================================================================================================

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

// ====================================================================================================================
// Network
// ====================================================================================================================

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

// ====================================================================================================================
// Memory
// ====================================================================================================================

auto
    UCk_Utils_Stats_UE::
    Get_RAM_UsedGB()
    -> float
{
    const auto& Stats = FPlatformMemory::GetStats();
    return static_cast<float>(Stats.UsedPhysical * InvGB);
}

auto
    UCk_Utils_Stats_UE::
    Get_RAM_AvailableGB()
    -> float
{
    const auto& Stats = FPlatformMemory::GetStats();
    return static_cast<float>(Stats.AvailablePhysical * InvGB);
}

auto
    UCk_Utils_Stats_UE::
    Get_RAM_TotalGB()
    -> float
{
    const auto& Stats = FPlatformMemory::GetStats();
    return static_cast<float>(Stats.TotalPhysical * InvGB);
}

auto
    UCk_Utils_Stats_UE::
    Get_VRAM_UsedGB()
    -> float
{
#if STATS
    // Cache the result to avoid querying every frame; refresh once per second
    static float  CachedVRAM      = 0.f;
    static double LastUpdateTime   = 0.0;

    const double Now = FPlatformTime::Seconds();

    if (Now - LastUpdateTime >= 1.0)
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

        CachedVRAM = static_cast<float>(TotalBytes * InvGB);
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

// ====================================================================================================================
// System Info
// ====================================================================================================================

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

// ====================================================================================================================
// UE Stat System
// ====================================================================================================================

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
