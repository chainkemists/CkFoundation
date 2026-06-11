#include "CkBallistic_Fragment_Data.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Ballistic_TrajectoryParams::
    Get_DragK() const
    -> double
{
    const auto TerminalSpeed = _TerminalVelocity.Size();

    // RMPP parity: a (near) zero-length terminal velocity is not a valid Carpentier trajectory.
    // Fall back to a very draggy projectile rather than dividing by zero
    constexpr auto FallbackDragK = 9.0;

    CK_ENSURE_IF_NOT(TerminalSpeed > UE_DOUBLE_SMALL_NUMBER,
        TEXT("TerminalVelocity [{}] must not be zero-length. Falling back to DragK [{}]"),
        _TerminalVelocity, FallbackDragK)
    { return FallbackDragK; }

    return FMath::Abs(0.5 * _Gravity.Size() / TerminalSpeed);
}

auto
    FCk_Ballistic_TrajectoryParams::
    Get_TerminalVelocityWithWind() const
    -> FVector
{
    return _TerminalVelocity + _Wind;
}

// --------------------------------------------------------------------------------------------------------------------
