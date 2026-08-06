#include "Ck2dGridSystem_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_2dGridSystem_Spec::
    Get_ResolvedActiveCoordinates() const
    -> TArray<FIntPoint>
{
    auto ActiveCoords = TArray<FIntPoint>{};

    for (auto Y = 0; Y < _Dimensions.Y; ++Y)
    {
        for (auto X = 0; X < _Dimensions.X; ++X)
        {
            if (const auto Coord = FIntPoint(X, Y);
                Get_IsCoordinateActive(Coord))
            {
                ActiveCoords.Add(Coord);
            }
        }
    }

    return ActiveCoords;
}

auto
    FCk_2dGridSystem_Spec::
    Get_IsCoordinateActive(
        const FIntPoint& InCoordinate) const
    -> bool
{
    auto IsActive = _DefaultCellState == ECk_EnableDisable::Enable;

    if (_ExceptionCoordinates.Contains(InCoordinate))
    {
        IsActive = NOT IsActive; // Flip the state for exception coordinates
    }

    return IsActive;
}

// --------------------------------------------------------------------------------------------------------------------
