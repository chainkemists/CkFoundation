#pragma once

#include "CoreMinimal.h"

// One bone's transposed 3x4 matrix == 3 rows of float4 == 12 contiguous floats (PF_A32B32G32R32F upload-ready).
// Default value is the transposed 3x4 of the identity matrix. Lives in the (PostConfigInit) VF module so both the
// baker (CkIskmRenderer) and the GPU SRV upload can share it without a module cycle.
struct FCk_Iskm_BoneMatrix3x4
{
    float M[12] = { 1.f, 0.f, 0.f, 0.f,
                    0.f, 1.f, 0.f, 0.f,
                    0.f, 0.f, 1.f, 0.f };
};
