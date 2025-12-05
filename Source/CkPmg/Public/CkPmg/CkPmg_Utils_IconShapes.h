#pragma once

class UProceduralMeshComponent;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pmg
{
    // Circular badge-style icon shapes

    auto
        GenerateDebugShape_Warning(
            UProceduralMeshComponent* InMeshComponent,
            float InSize)
        -> void;

    auto
        GenerateDebugShape_Prohibition(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InSegments)
        -> void;

    auto
        GenerateDebugShape_NoEntry(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InSegments)
        -> void;

    auto
        GenerateDebugShape_InfoCircle(
            UProceduralMeshComponent* InMeshComponent,
            float InRadius,
            int32 InSegments)
        -> void;
}

// --------------------------------------------------------------------------------------------------------------------
