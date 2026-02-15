#include "CkWatermarkStat_NetworkType_Widget.h"

#include <HAL/PlatformMisc.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_NetworkType_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("Network"));
}

auto
    UCkWatermarkStat_NetworkType_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    switch (FPlatformMisc::GetNetworkConnectionType())
    {
        case ENetworkConnectionType::None:        return FText::FromString(TEXT("None"));
        case ENetworkConnectionType::AirplaneMode: return FText::FromString(TEXT("Airplane"));
        case ENetworkConnectionType::Cell:         return FText::FromString(TEXT("Cell"));
        case ENetworkConnectionType::WiFi:         return FText::FromString(TEXT("WiFi"));
        case ENetworkConnectionType::WiMAX:        return FText::FromString(TEXT("WiMAX"));
        case ENetworkConnectionType::Bluetooth:    return FText::FromString(TEXT("Bluetooth"));
        case ENetworkConnectionType::Ethernet:     return FText::FromString(TEXT("Ethernet"));
        default:                                   return FText::FromString(TEXT("Unknown"));
    }
}

// --------------------------------------------------------------------------------------------------------------------
