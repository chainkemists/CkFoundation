#include "CkShapeCylinder_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
	FCk_ShapeCylinder_Dimensions::
	operator==(
		const ThisType& InOther) const
	-> bool
{
	return FMath::IsNearlyEqual(_HalfHeight, InOther._HalfHeight) &&
		FMath::IsNearlyEqual(_Radius, InOther._Radius) &&
		FMath::IsNearlyEqual(_ConvexRadius, InOther._ConvexRadius);
}

// --------------------------------------------------------------------------------------------------------------------
