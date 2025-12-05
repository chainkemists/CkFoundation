#pragma once

class UProceduralMeshComponent;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pmg
{
    // Standalone symbol shapes

    auto
        GenerateDebugShape_MagnifyingGlass(
            UProceduralMeshComponent* InMeshComponent,
            float InSize)
        -> void;

    auto
        GenerateDebugShape_QuestionMark(
            UProceduralMeshComponent* InMeshComponent,
            float InSize)
        -> void;

    auto
        GenerateDebugShape_ExclamationMark(
            UProceduralMeshComponent* InMeshComponent,
            float InSize)
        -> void;

    auto
        GenerateDebugShape_Flag(
            UProceduralMeshComponent* InMeshComponent,
            float InSize)
        -> void;

    auto
        GenerateDebugShape_Pin(
            UProceduralMeshComponent* InMeshComponent,
            float InSize)
        -> void;
}

// --------------------------------------------------------------------------------------------------------------------
