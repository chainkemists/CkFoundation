#include "CkSnapshot/Inspection/CkSnapshot_Inspection_Sha256.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    namespace ck_snapshot_inspection_sha256
    {
        constexpr uint32 k_RoundConstants[64] =
        {
            0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
            0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
            0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
            0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
            0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
            0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
            0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
            0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
        };

        constexpr auto
            RotateRight(
                uint32 InValue,
                uint32 InBits)
            -> uint32
        {
            return (InValue >> InBits) | (InValue << (32u - InBits));
        }

        constexpr auto k_HexDigits = TEXT("0123456789abcdef");
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_Snapshot_Sha256::
        DoCompress(
            const uint8* InBlock)
        -> void
    {
        using namespace ck_snapshot_inspection_sha256;

        uint32 Schedule[64] = {};

        for (auto Index = 0; Index < 16; ++Index)
        {
            Schedule[Index] =
                  (static_cast<uint32>(InBlock[Index * 4 + 0]) << 24)
                | (static_cast<uint32>(InBlock[Index * 4 + 1]) << 16)
                | (static_cast<uint32>(InBlock[Index * 4 + 2]) << 8)
                |  static_cast<uint32>(InBlock[Index * 4 + 3]);
        }

        for (auto Index = 16; Index < 64; ++Index)
        {
            const auto Prev15 = Schedule[Index - 15];
            const auto Prev2  = Schedule[Index - 2];

            const auto Sigma0 = RotateRight(Prev15, 7)  ^ RotateRight(Prev15, 18) ^ (Prev15 >> 3);
            const auto Sigma1 = RotateRight(Prev2, 17)  ^ RotateRight(Prev2, 19)  ^ (Prev2 >> 10);

            Schedule[Index] = Schedule[Index - 16] + Sigma0 + Schedule[Index - 7] + Sigma1;
        }

        auto A = _State[0];
        auto B = _State[1];
        auto C = _State[2];
        auto D = _State[3];
        auto E = _State[4];
        auto F = _State[5];
        auto G = _State[6];
        auto H = _State[7];

        for (auto Index = 0; Index < 64; ++Index)
        {
            const auto BigSigma1 = RotateRight(E, 6) ^ RotateRight(E, 11) ^ RotateRight(E, 25);
            const auto Choose    = (E & F) ^ ((~E) & G);
            const auto Temp1     = H + BigSigma1 + Choose + k_RoundConstants[Index] + Schedule[Index];

            const auto BigSigma0 = RotateRight(A, 2) ^ RotateRight(A, 13) ^ RotateRight(A, 22);
            const auto Majority  = (A & B) ^ (A & C) ^ (B & C);
            const auto Temp2     = BigSigma0 + Majority;

            H = G;
            G = F;
            F = E;
            E = D + Temp1;
            D = C;
            C = B;
            B = A;
            A = Temp1 + Temp2;
        }

        _State[0] += A;
        _State[1] += B;
        _State[2] += C;
        _State[3] += D;
        _State[4] += E;
        _State[5] += F;
        _State[6] += G;
        _State[7] += H;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_Snapshot_Sha256::
        Update(
            const uint8* InData,
            int64 InNumBytes)
        -> void
    {
        if (InData == nullptr || InNumBytes <= 0)
        { return; }

        _TotalBits += static_cast<uint64>(InNumBytes) * 8u;

        auto Offset = static_cast<int64>(0);
        while (Offset < InNumBytes)
        {
            const auto Room  = 64 - _BlockLength;
            const auto Taken = static_cast<int32>(FMath::Min<int64>(Room, InNumBytes - Offset));

            FMemory::Memcpy(_Block + _BlockLength, InData + Offset, static_cast<SIZE_T>(Taken));
            _BlockLength += Taken;
            Offset       += Taken;

            if (_BlockLength == 64)
            {
                DoCompress(_Block);
                _BlockLength = 0;
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_Snapshot_Sha256::
        Finalize_ToHexString()
        -> FString
    {
        using namespace ck_snapshot_inspection_sha256;

        const auto MessageBits = _TotalBits;

        _Block[_BlockLength] = 0x80u;
        ++_BlockLength;

        if (_BlockLength > 56)
        {
            FMemory::Memzero(_Block + _BlockLength, static_cast<SIZE_T>(64 - _BlockLength));
            DoCompress(_Block);
            _BlockLength = 0;
        }

        FMemory::Memzero(_Block + _BlockLength, static_cast<SIZE_T>(56 - _BlockLength));

        for (auto Index = 0; Index < 8; ++Index)
        { _Block[56 + Index] = static_cast<uint8>((MessageBits >> (56 - Index * 8)) & 0xFFu); }

        DoCompress(_Block);
        _BlockLength = 0;

        auto Hex = FString{};
        Hex.Reserve(64);

        for (auto Index = 0; Index < 8; ++Index)
        {
            const auto Word = _State[Index];
            for (auto Nibble = 7; Nibble >= 0; --Nibble)
            { Hex.AppendChar(k_HexDigits[(Word >> (Nibble * 4)) & 0xFu]); }
        }

        return Hex;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_Sha256Hex(
            const TArray<uint8>& InBytes)
        -> FString
    {
        auto Hasher = FCk_Snapshot_Sha256{};
        Hasher.Update(InBytes.GetData(), InBytes.Num());
        return Hasher.Finalize_ToHexString();
    }
}

// --------------------------------------------------------------------------------------------------------------------
