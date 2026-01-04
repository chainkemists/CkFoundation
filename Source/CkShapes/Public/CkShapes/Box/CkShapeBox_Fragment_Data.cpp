#include "CkShapeBox_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
	FCk_ShapeBox_Dimensions::
	operator==(
		const ThisType& InOther) const
	-> bool
{
	return _HalfExtents.Equals(InOther._HalfExtents) &&
		FMath::IsNearlyEqual(_ConvexRadius, InOther._ConvexRadius);
}

// --------------------------------------------------------------------------------------------------------------------
