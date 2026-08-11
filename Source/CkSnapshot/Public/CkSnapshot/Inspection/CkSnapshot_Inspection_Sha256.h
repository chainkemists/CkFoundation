#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "Containers/Array.h"
#include "Containers/UnrealString.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    /** FIPS 180-4 SHA-256.
     *
     *  Self-contained on purpose: FPlatformMisc::GetSHA256Signature has no Windows override in this engine, so
     *  the generic implementation it falls through to is an unconditional checkf(false) crash. */
    class CKSNAPSHOT_API FCk_Snapshot_Sha256
    {
    public:
        auto
        Update(
            const uint8* InData,
            int64 InNumBytes) -> void;

        /** Lowercase 64-character hex digest. Consumes the state — call once per instance. */
        auto
        Finalize_ToHexString() -> FString;

    private:
        auto
        DoCompress(
            const uint8* InBlock) -> void;

    private:
        uint32 _State[8] =
        {
            0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
            0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
        };

        uint8  _Block[64] = {};
        int32  _BlockLength = 0;
        uint64 _TotalBits = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** Lowercase 64-character SHA-256 hex digest of InBytes. An empty array hashes to the well-known
     *  e3b0c442...b7852b855 digest of the empty string. */
    CKSNAPSHOT_API auto
    Get_Sha256Hex(
        const TArray<uint8>& InBytes) -> FString;
}

// --------------------------------------------------------------------------------------------------------------------
