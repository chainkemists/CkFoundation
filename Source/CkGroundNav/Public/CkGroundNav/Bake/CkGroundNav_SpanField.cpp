#include "CkGroundNav_SpanField.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        FCk_GroundNav_QuantizedNormal::
        Make(
            const FVector& InNormal)
        -> FCk_GroundNav_QuantizedNormal
    {
        const auto Safe = InNormal.GetSafeNormal();

        // GetSafeNormal returns the zero vector when it cannot normalize. A surface with no normal is
        // not walkable anywhere, and straight up would be the one answer that silently reads as a
        // perfect floor - so a degenerate normal is stored pointing sideways instead.
        if (Safe.IsNearlyZero())
        { return FCk_GroundNav_QuantizedNormal{127, 0, 0}; }

        const auto Quantize = [](double InComponent) -> int8
        {
            const auto Scaled = FMath::RoundToInt(InComponent * 127.0);
            return static_cast<int8>(FMath::Clamp(Scaled, -127, 127));
        };

        return FCk_GroundNav_QuantizedNormal{Quantize(Safe.X), Quantize(Safe.Y), Quantize(Safe.Z)};
    }

    auto
        FCk_GroundNav_QuantizedNormal::
        Get_Normal() const
        -> FVector
    {
        return FVector{
            static_cast<double>(_X) / 127.0,
            static_cast<double>(_Y) / 127.0,
            static_cast<double>(_Z) / 127.0}.GetSafeNormal();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_SpanField::
        Get_TotalSpanCount() const
        -> int32
    {
        auto Total = 0;

        for (const auto& Column : _Columns)
        { Total += Column.Num(); }

        return Total;
    }
}

// --------------------------------------------------------------------------------------------------------------------
