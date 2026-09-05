#include "CkGroundNav_FieldSerialize.h"

#include "CkGroundNav/Field/CkGroundNav_FieldLinks.h"

#include <Serialization/MemoryReader.h>
#include <Serialization/MemoryWriter.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace fieldserialize_private
    {
        // Every scalar goes through here, in both directions, with its size spelled out. An FArchive's
        // operator<< for an engine value type is versioned by the engine and would move under us; a
        // fixed-width byte copy is the format this file promises.
        template <typename T>
        auto
            Do_Value(
                FArchive& InAr,
                T&        InOutValue)
            -> void
        {
            static_assert(std::is_arithmetic_v<T>,
                "Only fixed-width arithmetic values are copied straight into a blob");

            InAr.Serialize(&InOutValue, static_cast<int64>(sizeof(T)));
        }

        template <typename T_Enum>
        auto
            Do_WriteEnum(
                FArchive& InAr,
                T_Enum    InValue)
            -> void
        {
            auto Byte = static_cast<uint8>(InValue);
            Do_Value(InAr, Byte);
        }

        template <typename T_Enum>
        auto
            Do_ReadEnum(
                FArchive& InAr,
                T_Enum&   OutValue)
            -> void
        {
            auto Byte = uint8{0};
            Do_Value(InAr, Byte);
            OutValue = static_cast<T_Enum>(Byte);
        }

        // ------------------------------------------------------------------------------------------------------------

        /**
         * What a read has learnt so far: where it is, what the blob's tags resolved to, and the first
         * thing that went wrong.
         *
         * The FIRST failure wins and every later step short-circuits on it, so a blob that is truncated
         * mid-way through a tag index answers Truncated rather than whatever the half-read bytes
         * happened to look like.
         */
        struct FReadState
        {
        public:
            FArchive* _Ar = nullptr;

            TArray<FGameplayTag> _Tags;

            ECk_GroundNav_LoadStatus _Failure = ECk_GroundNav_LoadStatus::Loaded;

        public:
            auto Get_Failed() const -> bool
            {
                return _Failure != ECk_GroundNav_LoadStatus::Loaded || _Ar->IsError();
            }

            auto Do_Fail(ECk_GroundNav_LoadStatus InStatus) -> void
            {
                if (_Failure == ECk_GroundNav_LoadStatus::Loaded)
                { _Failure = InStatus; }
            }

            /** The archive running out of room is reported as Truncated unless something more specific
             *  was already found, because a blob that ends early is exactly that. */
            auto Get_Status() const -> ECk_GroundNav_LoadStatus
            {
                if (_Failure != ECk_GroundNav_LoadStatus::Loaded)
                { return _Failure; }

                return _Ar->IsError() ? ECk_GroundNav_LoadStatus::Truncated : ECk_GroundNav_LoadStatus::Loaded;
            }

            auto Get_BytesLeft() const -> int64 { return _Ar->TotalSize() - _Ar->Tell(); }
        };

        // The smallest number of bytes one element of an array can occupy on disk. These bound the
        // reservation a count asks for and nothing else, so each is deliberately NO LARGER than the
        // encoding's true minimum: a floor on cost rather than a description of the format, which is
        // what keeps them from refusing a blob that is perfectly readable.
        constexpr auto kMinBytesPerStringByte = int64{1};
        constexpr auto kMinBytesPerIndex = int64{4};
        constexpr auto kMinBytesPerNestedArray = int64{4};
        constexpr auto kMinBytesPerTagName = int64{4};
        constexpr auto kMinBytesPerTagContainer = int64{4};
        constexpr auto kMinBytesPerMarkupRecord = int64{64};
        constexpr auto kMinBytesPerLinkRecord = int64{64};
        constexpr auto kMinBytesPerPlate = int64{32};
        constexpr auto kMinBytesPerPortal = int64{32};
        constexpr auto kMinBytesPerBoundarySegment = int64{64};
        constexpr auto kMinBytesPerSeamStub = int64{16};
        constexpr auto kMinBytesPerOpenBody = int64{32};
        constexpr auto kMinBytesPerPoint = int64{24};
        constexpr auto kMinBytesPerTile = int64{64};

        /**
         * A count read from a blob is untrusted arithmetic: a corrupt one is a reservation the size of
         * whatever number the bytes happened to spell. Bounded by the bytes that are actually left
         * DIVIDED BY the smallest one element of what follows can be, so the worst a bad count can ask
         * for is proportional to the blob itself rather than to the number the bytes spelled.
         */
        auto
            Do_ReadCount(
                FReadState& InOutState,
                int64       InElementBytes,
                int32&      OutCount)
            -> bool
        {
            OutCount = 0;

            if (InOutState.Get_Failed())
            { return false; }

            Do_Value(*InOutState._Ar, OutCount);

            const auto MaxCount = InOutState.Get_BytesLeft() / FMath::Max(InElementBytes, int64{1});

            if (InOutState.Get_Failed() || OutCount < 0 || OutCount > MaxCount)
            {
                OutCount = 0;
                InOutState.Do_Fail(ECk_GroundNav_LoadStatus::Truncated);
                return false;
            }

            return true;
        }

        // ---- Engine value types, member by member -------------------------------------------------------------------
        //
        // Written out rather than handed to the engine's own operator<<: FVector's serializer branches
        // on the archive's engine version for large-world coordinates, and a blob whose layout depends
        // on which engine wrote it is not a format.
        //
        // Every floating-point member is read into a local and JUDGED BEFORE the value it belongs to is
        // constructed. A NaN or an infinity is a bit pattern corrupt bytes spell as readily as any
        // other, and one that reached an FBox or an FTransform would poison every bound, footprint and
        // projection derived from it while every comparison against it quietly answered false. Corrupt
        // is the answer, and the caller's field is left alone exactly as it is for any other refusal.

        auto
            Do_WriteVector(
                FArchive&      InAr,
                const FVector& InValue)
            -> void
        {
            auto X = InValue.X;
            auto Y = InValue.Y;
            auto Z = InValue.Z;

            Do_Value(InAr, X);
            Do_Value(InAr, Y);
            Do_Value(InAr, Z);
        }

        auto
            Do_ReadVector(
                FReadState& InOutState,
                FVector&    OutValue)
            -> void
        {
            auto X = decltype(FVector::X){0};
            auto Y = decltype(FVector::Y){0};
            auto Z = decltype(FVector::Z){0};

            Do_Value(*InOutState._Ar, X);
            Do_Value(*InOutState._Ar, Y);
            Do_Value(*InOutState._Ar, Z);

            const auto IsFinite = FMath::IsFinite(X) && FMath::IsFinite(Y) && FMath::IsFinite(Z);

            if (NOT IsFinite)
            {
                InOutState.Do_Fail(ECk_GroundNav_LoadStatus::Corrupt);
                return;
            }

            OutValue = FVector{X, Y, Z};
        }

        auto
            Do_WriteVector2D(
                FArchive&        InAr,
                const FVector2D& InValue)
            -> void
        {
            auto X = InValue.X;
            auto Y = InValue.Y;

            Do_Value(InAr, X);
            Do_Value(InAr, Y);
        }

        auto
            Do_ReadVector2D(
                FReadState& InOutState,
                FVector2D&  OutValue)
            -> void
        {
            auto X = decltype(FVector2D::X){0};
            auto Y = decltype(FVector2D::Y){0};

            Do_Value(*InOutState._Ar, X);
            Do_Value(*InOutState._Ar, Y);

            const auto IsFinite = FMath::IsFinite(X) && FMath::IsFinite(Y);

            if (NOT IsFinite)
            {
                InOutState.Do_Fail(ECk_GroundNav_LoadStatus::Corrupt);
                return;
            }

            OutValue = FVector2D{X, Y};
        }

        auto
            Do_WriteIntPoint(
                FArchive&        InAr,
                const FIntPoint& InValue)
            -> void
        {
            auto X = InValue.X;
            auto Y = InValue.Y;

            Do_Value(InAr, X);
            Do_Value(InAr, Y);
        }

        auto
            Do_ReadIntPoint(
                FArchive&  InAr,
                FIntPoint& OutValue)
            -> void
        {
            Do_Value(InAr, OutValue.X);
            Do_Value(InAr, OutValue.Y);
        }

        auto
            Do_WriteBox(
                FArchive&   InAr,
                const FBox& InValue)
            -> void
        {
            Do_WriteVector(InAr, InValue.Min);
            Do_WriteVector(InAr, InValue.Max);

            auto IsValid = InValue.IsValid;
            Do_Value(InAr, IsValid);
        }

        auto
            Do_ReadBox(
                FReadState& InOutState,
                FBox&       OutValue)
            -> void
        {
            auto Min = FVector::ZeroVector;
            auto Max = FVector::ZeroVector;

            Do_ReadVector(InOutState, Min);
            Do_ReadVector(InOutState, Max);

            auto IsValid = decltype(FBox::IsValid){0};
            Do_Value(*InOutState._Ar, IsValid);

            if (InOutState.Get_Failed())
            { return; }

            OutValue.Min = Min;
            OutValue.Max = Max;
            OutValue.IsValid = IsValid;
        }

        auto
            Do_WriteTransform(
                FArchive&         InAr,
                const FTransform& InValue)
            -> void
        {
            const auto Rotation = InValue.GetRotation();

            auto QuatX = Rotation.X;
            auto QuatY = Rotation.Y;
            auto QuatZ = Rotation.Z;
            auto QuatW = Rotation.W;

            Do_Value(InAr, QuatX);
            Do_Value(InAr, QuatY);
            Do_Value(InAr, QuatZ);
            Do_Value(InAr, QuatW);

            Do_WriteVector(InAr, InValue.GetTranslation());
            Do_WriteVector(InAr, InValue.GetScale3D());
        }

        auto
            Do_ReadTransform(
                FReadState& InOutState,
                FTransform& OutValue)
            -> void
        {
            auto QuatX = decltype(FQuat::X){0};
            auto QuatY = decltype(FQuat::Y){0};
            auto QuatZ = decltype(FQuat::Z){0};
            auto QuatW = decltype(FQuat::W){0};

            Do_Value(*InOutState._Ar, QuatX);
            Do_Value(*InOutState._Ar, QuatY);
            Do_Value(*InOutState._Ar, QuatZ);
            Do_Value(*InOutState._Ar, QuatW);

            const auto RotationIsFinite = FMath::IsFinite(QuatX) && FMath::IsFinite(QuatY) &&
                                          FMath::IsFinite(QuatZ) && FMath::IsFinite(QuatW);

            if (NOT RotationIsFinite)
            {
                InOutState.Do_Fail(ECk_GroundNav_LoadStatus::Corrupt);
                return;
            }

            const auto Rotation = FQuat{QuatX, QuatY, QuatZ, QuatW};

            // A rotation is a UNIT quaternion. Bytes spelling anything else would be silently
            // renormalised on first use and would place whatever they rotate somewhere nobody authored.
            if (NOT Rotation.IsNormalized())
            {
                InOutState.Do_Fail(ECk_GroundNav_LoadStatus::Corrupt);
                return;
            }

            auto Translation = FVector::ZeroVector;
            auto Scale = FVector::OneVector;

            Do_ReadVector(InOutState, Translation);
            Do_ReadVector(InOutState, Scale);

            if (InOutState.Get_Failed())
            { return; }

            OutValue = FTransform{Rotation, Translation, Scale};
        }

        /**
         * UTF-8, with its byte count in front.
         *
         * Its own encoding rather than the engine's string archiver so the bytes a given string
         * produces are the same everywhere, and so a fixture that has to find a known string inside a
         * blob can look for exactly the bytes it wrote.
         */
        auto
            Do_WriteString(
                FArchive&      InAr,
                const FString& InValue)
            -> void
        {
            const auto Utf8 = FTCHARToUTF8{*InValue};

            auto ByteCount = static_cast<int32>(Utf8.Length());
            Do_Value(InAr, ByteCount);

            if (ByteCount <= 0)
            { return; }

            auto Bytes = TArray<uint8>{};
            Bytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()), ByteCount);

            InAr.Serialize(Bytes.GetData(), ByteCount);
        }

        auto
            Do_ReadString(
                FReadState& InOutState,
                FString&    OutValue)
            -> void
        {
            OutValue.Reset();

            auto ByteCount = 0;

            if (NOT Do_ReadCount(InOutState, kMinBytesPerStringByte, ByteCount) || ByteCount == 0)
            { return; }

            auto Bytes = TArray<uint8>{};
            Bytes.SetNumZeroed(ByteCount + 1);

            InOutState._Ar->Serialize(Bytes.GetData(), ByteCount);

            if (InOutState.Get_Failed())
            { return; }

            OutValue = FString{UTF8_TO_TCHAR(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()))};
        }

        // ---- Arrays -------------------------------------------------------------------------------------------------

        template <typename T>
        auto
            Do_WriteValueArray(
                FArchive&              InAr,
                const TArray<T>&       InValues)
            -> void
        {
            auto Count = InValues.Num();
            Do_Value(InAr, Count);

            for (auto Index = 0; Index < Count; ++Index)
            {
                auto Value = InValues[Index];
                Do_Value(InAr, Value);
            }
        }

        template <typename T>
        auto
            Do_ReadValueArray(
                FReadState& InOutState,
                TArray<T>&  OutValues)
            -> void
        {
            OutValues.Reset();

            auto Count = 0;

            constexpr auto ElementBytes = static_cast<int64>(sizeof(T));

            if (NOT Do_ReadCount(InOutState, ElementBytes, Count))
            { return; }

            OutValues.SetNumZeroed(Count);

            for (auto Index = 0; Index < Count; ++Index)
            { Do_Value(*InOutState._Ar, OutValues[Index]); }
        }

        auto
            Do_WriteIndexArrays(
                FArchive&                     InAr,
                const TArray<TArray<int32>>&  InValues)
            -> void
        {
            auto Count = InValues.Num();
            Do_Value(InAr, Count);

            for (const auto& Inner : InValues)
            { Do_WriteValueArray(InAr, Inner); }
        }

        auto
            Do_ReadIndexArrays(
                FReadState&            InOutState,
                TArray<TArray<int32>>& OutValues)
            -> void
        {
            OutValues.Reset();

            auto Count = 0;

            if (NOT Do_ReadCount(InOutState, kMinBytesPerNestedArray, Count))
            { return; }

            OutValues.SetNum(Count);

            for (auto Index = 0; Index < Count; ++Index)
            {
                Do_ReadValueArray(InOutState, OutValues[Index]);

                if (InOutState.Get_Failed())
                { return; }
            }
        }

        // ---- The name table -----------------------------------------------------------------------------------------

        /**
         * The tag names one blob carries, and the index each of them is written as.
         *
         * SORTED BY STRING, not by first sighting. Two fields whose content matches must produce the
         * same table however their tags reached the writer, and a table in encounter order would make a
         * blob depend on the order records happened to be authored in.
         */
        struct FTagTable
        {
        public:
            TArray<FString> _Names;

            TMap<FString, int32> _IndexByName;
        };

        auto
            Do_GatherTag(
                TSet<FString>&      InOutNames,
                const FGameplayTag& InTag)
            -> void
        {
            if (NOT InTag.IsValid())
            { return; }

            InOutNames.Add(InTag.GetTagName().ToString());
        }

        auto
            Do_GatherTile(
                TSet<FString>&                InOutNames,
                const FCk_GroundNav_Tile&     InTile)
            -> void
        {
            for (const auto& Policy : InTile._Plates._AreaPolicies)
            {
                auto Tags = TArray<FGameplayTag>{};
                Policy.GetGameplayTagArray(Tags);

                for (const auto& Tag : Tags)
                { Do_GatherTag(InOutNames, Tag); }
            }
        }

        auto
            Do_GatherParams(
                TSet<FString>&                   InOutNames,
                const FCk_GroundNav_FieldParams& InParams)
            -> void
        {
            for (const auto& Record : InParams._MarkupRecords)
            { Do_GatherTag(InOutNames, Record.Get_AreaTag()); }

            for (const auto& Record : InParams._Links)
            {
                Do_GatherTag(InOutNames, Record.Get_AreaTag());
                Do_GatherTag(InOutNames, Record.Get_UserTypeTag());
            }
        }

        auto
            Make_TagTable(
                const TSet<FString>& InNames)
            -> FTagTable
        {
            auto Table = FTagTable{};

            Table._Names = InNames.Array();
            Table._Names.Sort([](const FString& InLeft, const FString& InRight) -> bool
            {
                return InLeft.Compare(InRight, ESearchCase::CaseSensitive) < 0;
            });

            for (auto Index = 0; Index < Table._Names.Num(); ++Index)
            { Table._IndexByName.Emplace(Table._Names[Index], Index); }

            return Table;
        }

        /** INDEX_NONE for a tag nothing authored, which is a value and not a failure. */
        auto
            Get_TagIndex(
                const FTagTable&    InTable,
                const FGameplayTag& InTag)
            -> int32
        {
            if (NOT InTag.IsValid())
            { return INDEX_NONE; }

            const auto* Found = InTable._IndexByName.Find(InTag.GetTagName().ToString());

            return Found != nullptr ? *Found : INDEX_NONE;
        }

        auto
            Do_WriteTagIndex(
                FArchive&           InAr,
                const FTagTable&    InTable,
                const FGameplayTag& InTag)
            -> void
        {
            auto Index = Get_TagIndex(InTable, InTag);
            Do_Value(InAr, Index);
        }

        /** An index outside the table names a tag the blob does not carry, which is the same defect as
         *  a name this process cannot resolve and answers the same way. */
        auto
            Do_ReadTagIndex(
                FReadState&   InOutState,
                FGameplayTag& OutTag)
            -> void
        {
            OutTag = FGameplayTag{};

            if (InOutState.Get_Failed())
            { return; }

            auto Index = int32{INDEX_NONE};
            Do_Value(*InOutState._Ar, Index);

            if (InOutState.Get_Failed() || Index == INDEX_NONE)
            { return; }

            if (NOT InOutState._Tags.IsValidIndex(Index))
            {
                InOutState.Do_Fail(ECk_GroundNav_LoadStatus::UnknownTag);
                return;
            }

            OutTag = InOutState._Tags[Index];
        }

        auto
            Do_WriteTagContainer(
                FArchive&                    InAr,
                const FTagTable&             InTable,
                const FGameplayTagContainer& InContainer)
            -> void
        {
            auto Tags = TArray<FGameplayTag>{};
            InContainer.GetGameplayTagArray(Tags);

            auto Count = Tags.Num();
            Do_Value(InAr, Count);

            for (const auto& Tag : Tags)
            { Do_WriteTagIndex(InAr, InTable, Tag); }
        }

        auto
            Do_ReadTagContainer(
                FReadState&            InOutState,
                FGameplayTagContainer& OutContainer)
            -> void
        {
            OutContainer.Reset();

            auto Count = 0;

            if (NOT Do_ReadCount(InOutState, kMinBytesPerIndex, Count))
            { return; }

            for (auto Index = 0; Index < Count; ++Index)
            {
                auto Tag = FGameplayTag{};
                Do_ReadTagIndex(InOutState, Tag);

                if (InOutState.Get_Failed())
                { return; }

                OutContainer.AddTag(Tag);
            }
        }

        // ---- The header ---------------------------------------------------------------------------------------------

        /**
         * Everything in front of a blob's body: what it is, when it was made, the lattice it belongs
         * to, and the names its indices speak.
         *
         * The lattice is here rather than only inside the params so a single-tile blob can be judged
         * against the field it is about to be read into without decoding a byte of the tile.
         */
        struct FBlobHeader
        {
        public:
            uint32 _Magic = 0;

            int32 _Version = 0;

            ECk_GroundNav_BlobContent _Content = ECk_GroundNav_BlobContent::WholeField;

            int64 _CookSecondsUtc = 0;

            FVector2D _OriginXY = FVector2D::ZeroVector;
            FIntPoint _Divisions = FIntPoint::ZeroValue;

            float _TileSizeUu = 0.0f;
            float _CellSizeUu = 0.0f;
            float _CellHeightUu = 0.0f;
            float _MinZUu = 0.0f;
            float _MaxZUu = 0.0f;
            float _MaxClearanceUu = 0.0f;

            uint64 _Fingerprint = 0;
        };

        auto
            Do_WriteHeader(
                FArchive&                        InAr,
                const FCk_GroundNav_FieldParams& InParams,
                ECk_GroundNav_BlobContent        InContent,
                const TArray<FString>&           InTagNames)
            -> void
        {
            auto Magic = kFieldBlobMagic;
            auto Version = kFieldBlobFormatVersion;

            Do_Value(InAr, Magic);
            Do_Value(InAr, Version);
            Do_WriteEnum(InAr, InContent);

            // An absolute UTC second, which is a date and not a reading of this machine's clock rate.
            // It is also the only run of bytes two writes of one field differ in.
            auto CookSecondsUtc = FDateTime::UtcNow().ToUnixTimestamp();
            Do_Value(InAr, CookSecondsUtc);

            Do_WriteVector2D(InAr, InParams._OriginXY);
            Do_WriteIntPoint(InAr, InParams._Divisions);

            auto TileSizeUu = InParams._Config.Get_TileSizeUu();
            auto CellSizeUu = InParams._Config.Get_CellSizeUu();
            auto CellHeightUu = InParams._Config.Get_CellHeightUu();
            auto MinZUu = InParams._MinZUu;
            auto MaxZUu = InParams._MaxZUu;
            auto MaxClearanceUu = InParams._MaxClearanceUu;

            Do_Value(InAr, TileSizeUu);
            Do_Value(InAr, CellSizeUu);
            Do_Value(InAr, CellHeightUu);
            Do_Value(InAr, MinZUu);
            Do_Value(InAr, MaxZUu);
            Do_Value(InAr, MaxClearanceUu);

            // Zero until a field carries a fingerprint of its own. The slot is written now rather than
            // added later because adding it later would be a format version nobody gains anything from.
            auto Fingerprint = uint64{0};
            Do_Value(InAr, Fingerprint);

            auto TagCount = InTagNames.Num();
            Do_Value(InAr, TagCount);

            for (const auto& Name : InTagNames)
            { Do_WriteString(InAr, Name); }
        }

        /**
         * The header, the table, and the tags the table resolves to.
         *
         * The magic and the version are judged BEFORE anything else is read: a blob of another format
         * decoded past that point produces members read at the wrong offsets, which is a field that
         * looks plausible and is not the one that was written.
         */
        auto
            Do_ReadHeader(
                FReadState&  InOutState,
                FBlobHeader& OutHeader,
                TArray<FString>* OutTagNames)
            -> void
        {
            auto& Ar = *InOutState._Ar;

            Do_Value(Ar, OutHeader._Magic);

            if (Ar.IsError())
            {
                InOutState.Do_Fail(ECk_GroundNav_LoadStatus::Truncated);
                return;
            }

            if (OutHeader._Magic != kFieldBlobMagic)
            {
                InOutState.Do_Fail(ECk_GroundNav_LoadStatus::WrongMagic);
                return;
            }

            Do_Value(Ar, OutHeader._Version);

            if (Ar.IsError())
            {
                InOutState.Do_Fail(ECk_GroundNav_LoadStatus::Truncated);
                return;
            }

            if (OutHeader._Version != kFieldBlobFormatVersion)
            {
                InOutState.Do_Fail(ECk_GroundNav_LoadStatus::WrongVersion);
                return;
            }

            Do_ReadEnum(Ar, OutHeader._Content);
            Do_Value(Ar, OutHeader._CookSecondsUtc);

            Do_ReadVector2D(InOutState, OutHeader._OriginXY);
            Do_ReadIntPoint(Ar, OutHeader._Divisions);

            Do_Value(Ar, OutHeader._TileSizeUu);
            Do_Value(Ar, OutHeader._CellSizeUu);
            Do_Value(Ar, OutHeader._CellHeightUu);
            Do_Value(Ar, OutHeader._MinZUu);
            Do_Value(Ar, OutHeader._MaxZUu);
            Do_Value(Ar, OutHeader._MaxClearanceUu);
            Do_Value(Ar, OutHeader._Fingerprint);

            auto TagCount = 0;

            if (NOT Do_ReadCount(InOutState, kMinBytesPerTagName, TagCount))
            { return; }

            InOutState._Tags.Reset();
            InOutState._Tags.Reserve(TagCount);

            for (auto Index = 0; Index < TagCount; ++Index)
            {
                auto Name = FString{};
                Do_ReadString(InOutState, Name);

                if (InOutState.Get_Failed())
                { return; }

                if (OutTagNames != nullptr)
                { OutTagNames->Emplace(Name); }

                // ErrorIfNotFound off: a name this process has never heard of is answered as a load
                // status by the caller, not as an ensure inside the tag manager.
                const auto Tag = FGameplayTag::RequestGameplayTag(FName{*Name}, false);

                if (NOT Tag.IsValid())
                {
                    InOutState.Do_Fail(ECk_GroundNav_LoadStatus::UnknownTag);
                    return;
                }

                InOutState._Tags.Emplace(Tag);
            }
        }

        auto
            Get_LatticeMatches(
                const FBlobHeader&               InHeader,
                const FCk_GroundNav_FieldParams& InParams)
            -> bool
        {
            return InHeader._OriginXY == InParams._OriginXY &&
                   InHeader._Divisions == InParams._Divisions &&
                   InHeader._TileSizeUu == InParams._Config.Get_TileSizeUu() &&
                   InHeader._CellSizeUu == InParams._Config.Get_CellSizeUu() &&
                   InHeader._CellHeightUu == InParams._Config.Get_CellHeightUu() &&
                   InHeader._MinZUu == InParams._MinZUu &&
                   InHeader._MaxZUu == InParams._MaxZUu;
        }

        // ---- The authored half --------------------------------------------------------------------------------------

        /**
         * Only the dimensions the shape type SELECTS are written. The other three are the shape's own
         * unused slots, and a reader that restored them would be inventing an authored value nobody
         * can see through the type that is set.
         */
        auto
            Do_WriteAnyShape(
                FArchive&            InAr,
                const FCk_AnyShape&  InShape)
            -> void
        {
            const auto ShapeType = InShape.Get_ShapeType();
            Do_WriteEnum(InAr, ShapeType);

            switch (ShapeType)
            {
                case ECk_Shape_Type::Box:
                {
                    Do_WriteVector(InAr, InShape.Get_Box().Get_HalfExtents());

                    auto ConvexRadius = InShape.Get_Box().Get_ConvexRadius();
                    Do_Value(InAr, ConvexRadius);
                    break;
                }
                case ECk_Shape_Type::Capsule:
                {
                    auto HalfHeight = InShape.Get_Capsule().Get_HalfHeight();
                    auto Radius = InShape.Get_Capsule().Get_Radius();

                    Do_Value(InAr, HalfHeight);
                    Do_Value(InAr, Radius);
                    break;
                }
                case ECk_Shape_Type::Cylinder:
                {
                    auto HalfHeight = InShape.Get_Cylinder().Get_HalfHeight();
                    auto Radius = InShape.Get_Cylinder().Get_Radius();
                    auto ConvexRadius = InShape.Get_Cylinder().Get_ConvexRadius();

                    Do_Value(InAr, HalfHeight);
                    Do_Value(InAr, Radius);
                    Do_Value(InAr, ConvexRadius);
                    break;
                }
                case ECk_Shape_Type::Sphere:
                {
                    auto Radius = InShape.Get_Sphere().Get_Radius();
                    Do_Value(InAr, Radius);
                    break;
                }
                case ECk_Shape_Type::None:
                default:
                {
                    break;
                }
            }
        }

        auto
            Do_ReadAnyShape(
                FReadState&   InOutState,
                FCk_AnyShape& OutShape)
            -> void
        {
            OutShape = FCk_AnyShape{};

            if (InOutState.Get_Failed())
            { return; }

            auto& Ar = *InOutState._Ar;

            auto ShapeType = ECk_Shape_Type::None;
            Do_ReadEnum(Ar, ShapeType);

            switch (ShapeType)
            {
                case ECk_Shape_Type::Box:
                {
                    auto HalfExtents = FVector::OneVector;
                    auto ConvexRadius = 0.0f;

                    Do_ReadVector(InOutState, HalfExtents);
                    Do_Value(Ar, ConvexRadius);

                    auto Dimensions = FCk_ShapeBox_Dimensions{HalfExtents};
                    Dimensions.Set_ConvexRadius(ConvexRadius);

                    OutShape = FCk_AnyShape{Dimensions};
                    break;
                }
                case ECk_Shape_Type::Capsule:
                {
                    auto HalfHeight = 0.0f;
                    auto Radius = 0.0f;

                    Do_Value(Ar, HalfHeight);
                    Do_Value(Ar, Radius);

                    OutShape = FCk_AnyShape{FCk_ShapeCapsule_Dimensions{HalfHeight, Radius}};
                    break;
                }
                case ECk_Shape_Type::Cylinder:
                {
                    auto HalfHeight = 0.0f;
                    auto Radius = 0.0f;
                    auto ConvexRadius = 0.0f;

                    Do_Value(Ar, HalfHeight);
                    Do_Value(Ar, Radius);
                    Do_Value(Ar, ConvexRadius);

                    auto Dimensions = FCk_ShapeCylinder_Dimensions{HalfHeight, Radius};
                    Dimensions.Set_ConvexRadius(ConvexRadius);

                    OutShape = FCk_AnyShape{Dimensions};
                    break;
                }
                case ECk_Shape_Type::Sphere:
                {
                    auto Radius = 0.0f;
                    Do_Value(Ar, Radius);

                    OutShape = FCk_AnyShape{FCk_ShapeSphere_Dimensions{Radius}};
                    break;
                }
                case ECk_Shape_Type::None:
                default:
                {
                    break;
                }
            }
        }

        auto
            Do_WriteParams(
                FArchive&                        InAr,
                const FTagTable&                 InTable,
                const FCk_GroundNav_FieldParams& InParams)
            -> void
        {
            Do_WriteVector2D(InAr, InParams._OriginXY);
            Do_WriteIntPoint(InAr, InParams._Divisions);

            auto MinZUu = InParams._MinZUu;
            auto MaxZUu = InParams._MaxZUu;
            auto MaxClearanceUu = InParams._MaxClearanceUu;

            Do_Value(InAr, MinZUu);
            Do_Value(InAr, MaxZUu);
            Do_Value(InAr, MaxClearanceUu);

            auto CellSizeUu = InParams._Config.Get_CellSizeUu();
            auto CellHeightUu = InParams._Config.Get_CellHeightUu();
            auto TileSizeUu = InParams._Config.Get_TileSizeUu();
            auto MaxColumnsPerTile = InParams._Config.Get_MaxColumnsPerTile();

            Do_Value(InAr, CellSizeUu);
            Do_Value(InAr, CellHeightUu);
            Do_Value(InAr, TileSizeUu);
            Do_Value(InAr, MaxColumnsPerTile);

            Do_WriteAnyShape(InAr, InParams._Profile.Get_StandingExtents());

            auto MaxSlopeDegrees = InParams._Profile.Get_MaxSlopeDegrees();
            auto MaxSlopeChangeDegrees = InParams._Profile.Get_MaxSlopeChangeDegrees();
            auto StepHeightUu = InParams._Profile.Get_StepHeightUu();
            auto LedgeSensitivity = InParams._Profile.Get_LedgeSensitivity();
            auto RoughPerchToleranceUu = InParams._Profile.Get_RoughPerchToleranceUu();

            Do_Value(InAr, MaxSlopeDegrees);
            Do_Value(InAr, MaxSlopeChangeDegrees);
            Do_Value(InAr, StepHeightUu);
            Do_Value(InAr, LedgeSensitivity);
            Do_Value(InAr, RoughPerchToleranceUu);

            auto PlaneFitToleranceUu = InParams._MergeTunables.Get_PlaneFitToleranceUu();
            auto NormalConeDegrees = InParams._MergeTunables.Get_NormalConeDegrees();

            Do_Value(InAr, PlaneFitToleranceUu);
            Do_Value(InAr, NormalConeDegrees);

            auto MarkupCount = InParams._MarkupRecords.Num();
            Do_Value(InAr, MarkupCount);

            for (const auto& Record : InParams._MarkupRecords)
            {
                auto Id = Record.Get_Id();
                Do_Value(InAr, Id);

                Do_WriteAnyShape(InAr, Record.Get_Shape());
                Do_WriteTransform(InAr, Record.Get_WorldTransform());
                Do_WriteTagIndex(InAr, InTable, Record.Get_AreaTag());
                Do_WriteEnum(InAr, Record.Get_Enable());
                Do_WriteEnum(InAr, Record.Get_Kind());

                auto CostMultiplier = Record.Get_CostMultiplier();
                auto RequestedAtEpoch = Record.Get_RequestedAtEpoch();

                Do_Value(InAr, CostMultiplier);
                Do_Value(InAr, RequestedAtEpoch);
            }

            auto LinkCount = InParams._Links.Num();
            Do_Value(InAr, LinkCount);

            for (const auto& Record : InParams._Links)
            {
                auto Id = Record.Get_Id();
                Do_Value(InAr, Id);

                Do_WriteVector(InAr, Record.Get_Start());
                Do_WriteVector(InAr, Record.Get_End());
                Do_WriteEnum(InAr, Record.Get_Direction());

                auto CostMultiplierForward = Record.Get_CostMultiplierForward();
                auto CostMultiplierBackward = Record.Get_CostMultiplierBackward();
                auto ClearanceUu = Record.Get_ClearanceUu();

                Do_Value(InAr, CostMultiplierForward);
                Do_Value(InAr, CostMultiplierBackward);
                Do_Value(InAr, ClearanceUu);

                Do_WriteTagIndex(InAr, InTable, Record.Get_AreaTag());
                Do_WriteTagIndex(InAr, InTable, Record.Get_UserTypeTag());
                Do_WriteEnum(InAr, Record.Get_Enable());
                Do_WriteEnum(InAr, Record.Get_ProjectionMode());

                auto ProjectionHorizontalExtentUu = Record.Get_ProjectionHorizontalExtentUu();
                auto ProjectionVerticalExtentUu = Record.Get_ProjectionVerticalExtentUu();
                auto RequestedAtEpoch = Record.Get_RequestedAtEpoch();

                Do_Value(InAr, ProjectionHorizontalExtentUu);
                Do_Value(InAr, ProjectionVerticalExtentUu);
                Do_Value(InAr, RequestedAtEpoch);
            }
        }

        auto
            Do_ReadParams(
                FReadState&                InOutState,
                FCk_GroundNav_FieldParams& OutParams)
            -> void
        {
            if (InOutState.Get_Failed())
            { return; }

            auto& Ar = *InOutState._Ar;

            Do_ReadVector2D(InOutState, OutParams._OriginXY);
            Do_ReadIntPoint(Ar, OutParams._Divisions);

            Do_Value(Ar, OutParams._MinZUu);
            Do_Value(Ar, OutParams._MaxZUu);
            Do_Value(Ar, OutParams._MaxClearanceUu);

            auto CellSizeUu = 0.0f;
            auto CellHeightUu = 0.0f;
            auto TileSizeUu = 0.0f;
            auto MaxColumnsPerTile = 0;

            Do_Value(Ar, CellSizeUu);
            Do_Value(Ar, CellHeightUu);
            Do_Value(Ar, TileSizeUu);
            Do_Value(Ar, MaxColumnsPerTile);

            OutParams._Config = FCk_GroundNav_BakeConfig{CellSizeUu, CellHeightUu};
            OutParams._Config.Set_TileSizeUu(TileSizeUu);

            // The ceiling was written into the blob before the config could be told it, so restoring it
            // costs no format version: the bytes have been there since the first blob was written.
            OutParams._Config.Set_MaxColumnsPerTile(MaxColumnsPerTile);

            auto StandingExtents = FCk_AnyShape{};
            Do_ReadAnyShape(InOutState, StandingExtents);

            auto MaxSlopeDegrees = 0.0f;
            auto MaxSlopeChangeDegrees = 0.0f;
            auto StepHeightUu = 0.0f;
            auto LedgeSensitivity = 0.0f;
            auto RoughPerchToleranceUu = 0.0f;

            Do_Value(Ar, MaxSlopeDegrees);
            Do_Value(Ar, MaxSlopeChangeDegrees);
            Do_Value(Ar, StepHeightUu);
            Do_Value(Ar, LedgeSensitivity);
            Do_Value(Ar, RoughPerchToleranceUu);

            OutParams._Profile = FCk_GroundNav_AgentProfile{StandingExtents};
            OutParams._Profile.Set_MaxSlopeDegrees(MaxSlopeDegrees);
            OutParams._Profile.Set_MaxSlopeChangeDegrees(MaxSlopeChangeDegrees);
            OutParams._Profile.Set_StepHeightUu(StepHeightUu);
            OutParams._Profile.Set_LedgeSensitivity(LedgeSensitivity);
            OutParams._Profile.Set_RoughPerchToleranceUu(RoughPerchToleranceUu);

            auto PlaneFitToleranceUu = 0.0f;
            auto NormalConeDegrees = 0.0f;

            Do_Value(Ar, PlaneFitToleranceUu);
            Do_Value(Ar, NormalConeDegrees);

            OutParams._MergeTunables = FCk_GroundNav_MergeTunables{PlaneFitToleranceUu, NormalConeDegrees};

            auto MarkupCount = 0;

            if (NOT Do_ReadCount(InOutState, kMinBytesPerMarkupRecord, MarkupCount))
            { return; }

            OutParams._MarkupRecords.Reset();
            OutParams._MarkupRecords.Reserve(MarkupCount);

            for (auto Index = 0; Index < MarkupCount; ++Index)
            {
                auto Id = int32{INDEX_NONE};
                Do_Value(Ar, Id);

                auto Shape = FCk_AnyShape{};
                Do_ReadAnyShape(InOutState, Shape);

                auto WorldTransform = FTransform::Identity;
                Do_ReadTransform(InOutState, WorldTransform);

                auto AreaTag = FGameplayTag{};
                Do_ReadTagIndex(InOutState, AreaTag);

                auto Enable = ECk_EnableDisable::Enable;
                auto Kind = ECk_GroundNav_MarkupKind::Walkability;

                Do_ReadEnum(Ar, Enable);
                Do_ReadEnum(Ar, Kind);

                auto CostMultiplier = 1.0f;
                auto RequestedAtEpoch = int64{0};

                Do_Value(Ar, CostMultiplier);
                Do_Value(Ar, RequestedAtEpoch);

                if (InOutState.Get_Failed())
                { return; }

                auto Record = FCk_GroundNav_MarkupRecord{Id, Shape, WorldTransform, Kind};

                Record.Set_AreaTag(AreaTag);
                Record.Set_Enable(Enable);
                Record.Set_CostMultiplier(CostMultiplier);
                Record.Set_RequestedAtEpoch(RequestedAtEpoch);

                OutParams._MarkupRecords.Emplace(Record);
            }

            auto LinkCount = 0;

            if (NOT Do_ReadCount(InOutState, kMinBytesPerLinkRecord, LinkCount))
            { return; }

            OutParams._Links.Reset();
            OutParams._Links.Reserve(LinkCount);

            for (auto Index = 0; Index < LinkCount; ++Index)
            {
                auto Id = int32{INDEX_NONE};
                Do_Value(Ar, Id);

                auto Start = FVector::ZeroVector;
                auto End = FVector::ZeroVector;

                Do_ReadVector(InOutState, Start);
                Do_ReadVector(InOutState, End);

                auto Direction = ECk_GroundNav_LinkDirection::Bidirectional;
                Do_ReadEnum(Ar, Direction);

                auto CostMultiplierForward = 1.0f;
                auto CostMultiplierBackward = 1.0f;
                auto ClearanceUu = FCk_GroundNav_LinkRecord::kAdmitsAnyAgentClearanceUu;

                Do_Value(Ar, CostMultiplierForward);
                Do_Value(Ar, CostMultiplierBackward);
                Do_Value(Ar, ClearanceUu);

                auto AreaTag = FGameplayTag{};
                auto UserTypeTag = FGameplayTag{};

                Do_ReadTagIndex(InOutState, AreaTag);
                Do_ReadTagIndex(InOutState, UserTypeTag);

                auto Enable = ECk_EnableDisable::Enable;
                auto ProjectionMode = ECk_NavSurface_ProjectionMode::Closest;

                Do_ReadEnum(Ar, Enable);
                Do_ReadEnum(Ar, ProjectionMode);

                auto ProjectionHorizontalExtentUu = 0.0f;
                auto ProjectionVerticalExtentUu = 0.0f;
                auto RequestedAtEpoch = int64{0};

                Do_Value(Ar, ProjectionHorizontalExtentUu);
                Do_Value(Ar, ProjectionVerticalExtentUu);
                Do_Value(Ar, RequestedAtEpoch);

                if (InOutState.Get_Failed())
                { return; }

                auto Record = FCk_GroundNav_LinkRecord{Id, Start, End};

                Record.Set_Direction(Direction);
                Record.Set_CostMultiplierForward(CostMultiplierForward);
                Record.Set_CostMultiplierBackward(CostMultiplierBackward);
                Record.Set_ClearanceUu(ClearanceUu);
                Record.Set_AreaTag(AreaTag);
                Record.Set_UserTypeTag(UserTypeTag);
                Record.Set_Enable(Enable);
                Record.Set_ProjectionMode(ProjectionMode);
                Record.Set_ProjectionHorizontalExtentUu(ProjectionHorizontalExtentUu);
                Record.Set_ProjectionVerticalExtentUu(ProjectionVerticalExtentUu);
                Record.Set_RequestedAtEpoch(RequestedAtEpoch);

                OutParams._Links.Emplace(Record);
            }
        }

        // ---- The bake product ---------------------------------------------------------------------------------------

        auto
            Do_WriteTile(
                FArchive&                 InAr,
                const FTagTable&          InTable,
                const FCk_GroundNav_Tile& InTile)
            -> void
        {
            auto CoordX = InTile._Coord._X;
            auto CoordY = InTile._Coord._Y;
            auto EpochValue = InTile._Epoch._Value;

            Do_Value(InAr, CoordX);
            Do_Value(InAr, CoordY);
            Do_Value(InAr, EpochValue);
            Do_WriteEnum(InAr, InTile._Status);
            Do_WriteVector(InAr, InTile._Origin);

            auto CellSizeUu = InTile._CellSizeUu;
            auto MaxClearanceUu = InTile._MaxClearanceUu;
            auto SizeX = InTile._SizeX;
            auto SizeY = InTile._SizeY;
            auto LayerCount = InTile._LayerCount;

            Do_Value(InAr, CellSizeUu);
            Do_Value(InAr, MaxClearanceUu);
            Do_Value(InAr, SizeX);
            Do_Value(InAr, SizeY);
            Do_Value(InAr, LayerCount);

            Do_WriteValueArray(InAr, InTile._SurfaceZ);

            auto ClearanceSizeX = InTile._Clearance._SizeX;
            auto ClearanceSizeY = InTile._Clearance._SizeY;
            auto ClearanceLayerCount = InTile._Clearance._LayerCount;
            auto ClearanceCellSizeUu = InTile._Clearance._CellSizeUu;

            Do_Value(InAr, ClearanceSizeX);
            Do_Value(InAr, ClearanceSizeY);
            Do_Value(InAr, ClearanceLayerCount);
            Do_Value(InAr, ClearanceCellSizeUu);
            Do_WriteValueArray(InAr, InTile._Clearance._Cells);

            auto PlatesSizeX = InTile._Plates._SizeX;
            auto PlatesSizeY = InTile._Plates._SizeY;
            auto PlatesLayerCount = InTile._Plates._LayerCount;
            auto PlateCount = InTile._Plates._Plates.Num();

            Do_Value(InAr, PlatesSizeX);
            Do_Value(InAr, PlatesSizeY);
            Do_Value(InAr, PlatesLayerCount);
            Do_Value(InAr, PlateCount);

            for (const auto& Plate : InTile._Plates._Plates)
            {
                auto LayerIndex = Plate._LayerIndex;
                auto MinX = Plate._MinX;
                auto MinY = Plate._MinY;
                auto MaxX = Plate._MaxX;
                auto MaxY = Plate._MaxY;
                auto MaxPlaneResidualUu = Plate._MaxPlaneResidualUu;
                auto HeightRangeUu = Plate._HeightRangeUu;
                auto MinClearanceUu = Plate._MinClearanceUu;
                auto AreaPolicyIndex = Plate._AreaPolicyIndex;
                auto CostMultiplier = Plate._CostMultiplier;

                Do_Value(InAr, LayerIndex);
                Do_Value(InAr, MinX);
                Do_Value(InAr, MinY);
                Do_Value(InAr, MaxX);
                Do_Value(InAr, MaxY);
                Do_Value(InAr, MaxPlaneResidualUu);
                Do_Value(InAr, HeightRangeUu);
                Do_Value(InAr, MinClearanceUu);
                Do_Value(InAr, AreaPolicyIndex);
                Do_Value(InAr, CostMultiplier);
            }

            Do_WriteValueArray(InAr, InTile._Plates._CellToPlate);

            auto PolicyCount = InTile._Plates._AreaPolicies.Num();
            Do_Value(InAr, PolicyCount);

            for (const auto& Policy : InTile._Plates._AreaPolicies)
            { Do_WriteTagContainer(InAr, InTable, Policy); }

            auto PortalCount = InTile._Portals._Portals.Num();
            Do_Value(InAr, PortalCount);

            for (const auto& Portal : InTile._Portals._Portals)
            {
                auto PlateA = Portal._PlateA;
                auto PlateB = Portal._PlateB;
                auto Direction = Portal._Direction;

                Do_Value(InAr, PlateA);
                Do_Value(InAr, PlateB);
                Do_Value(InAr, Direction);
                Do_WriteIntPoint(InAr, Portal._FromMin);
                Do_WriteIntPoint(InAr, Portal._FromMax);

                auto MinEndZUu = Portal._MinEndZUu;
                auto MaxEndZUu = Portal._MaxEndZUu;
                auto TraversalClearanceUu = Portal._TraversalClearanceUu;

                Do_Value(InAr, MinEndZUu);
                Do_Value(InAr, MaxEndZUu);
                Do_Value(InAr, TraversalClearanceUu);
            }

            Do_WriteIndexArrays(InAr, InTile._Portals._PlateToPortals);

            const auto Do_WriteSegments = [&InAr](const TArray<FCk_GroundNav_BoundarySegment>& InSegments) -> void
            {
                auto SegmentCount = InSegments.Num();
                Do_Value(InAr, SegmentCount);

                for (const auto& Segment : InSegments)
                {
                    auto PlateIndex = Segment._PlateIndex;
                    auto LayerIndex = Segment._LayerIndex;
                    auto Side = Segment._Side;

                    Do_Value(InAr, PlateIndex);
                    Do_Value(InAr, LayerIndex);
                    Do_Value(InAr, Side);
                    Do_WriteIntPoint(InAr, Segment._FromCell);
                    Do_WriteIntPoint(InAr, Segment._ToCell);
                    Do_WriteVector(InAr, Segment._Start);
                    Do_WriteVector(InAr, Segment._End);
                    Do_WriteVector2D(InAr, Segment._InwardNormalXY);
                }
            };

            Do_WriteSegments(InTile._Boundary._Segments);
            Do_WriteSegments(InTile._Boundary._EdgeCandidates);

            auto BucketsX = InTile._Boundary._BucketsX;
            auto BucketsY = InTile._Boundary._BucketsY;

            Do_Value(InAr, BucketsX);
            Do_Value(InAr, BucketsY);
            Do_WriteIndexArrays(InAr, InTile._Boundary._Buckets);

            auto StubCount = InTile._SeamStubs.Num();
            Do_Value(InAr, StubCount);

            for (const auto& Stub : InTile._SeamStubs)
            {
                auto Direction = Stub._Direction;
                auto AlongIndex = Stub._AlongIndex;
                auto PlateIndex = Stub._PlateIndex;
                auto NearSurfaceZUu = Stub._NearSurfaceZUu;
                auto FarSurfaceZUu = Stub._FarSurfaceZUu;
                auto ClearanceUu = Stub._ClearanceUu;

                Do_Value(InAr, Direction);
                Do_Value(InAr, AlongIndex);
                Do_Value(InAr, PlateIndex);
                Do_Value(InAr, NearSurfaceZUu);
                Do_Value(InAr, FarSurfaceZUu);
                Do_Value(InAr, ClearanceUu);
            }

            auto SourceTriangleCount = InTile._BakeStats._SourceTriangleCount;
            auto RasterizedSpanCount = InTile._BakeStats._RasterizedSpanCount;
            auto RejectedCellCount = InTile._BakeStats._RejectedCellCount;

            Do_Value(InAr, SourceTriangleCount);
            Do_Value(InAr, RasterizedSpanCount);
            Do_Value(InAr, RejectedCellCount);
        }

        auto
            Do_ReadTile(
                FReadState&         InOutState,
                FCk_GroundNav_Tile& OutTile)
            -> void
        {
            if (InOutState.Get_Failed())
            { return; }

            auto& Ar = *InOutState._Ar;

            Do_Value(Ar, OutTile._Coord._X);
            Do_Value(Ar, OutTile._Coord._Y);
            Do_Value(Ar, OutTile._Epoch._Value);
            Do_ReadEnum(Ar, OutTile._Status);
            Do_ReadVector(InOutState, OutTile._Origin);

            Do_Value(Ar, OutTile._CellSizeUu);
            Do_Value(Ar, OutTile._MaxClearanceUu);
            Do_Value(Ar, OutTile._SizeX);
            Do_Value(Ar, OutTile._SizeY);
            Do_Value(Ar, OutTile._LayerCount);

            Do_ReadValueArray(InOutState, OutTile._SurfaceZ);

            Do_Value(Ar, OutTile._Clearance._SizeX);
            Do_Value(Ar, OutTile._Clearance._SizeY);
            Do_Value(Ar, OutTile._Clearance._LayerCount);
            Do_Value(Ar, OutTile._Clearance._CellSizeUu);
            Do_ReadValueArray(InOutState, OutTile._Clearance._Cells);

            Do_Value(Ar, OutTile._Plates._SizeX);
            Do_Value(Ar, OutTile._Plates._SizeY);
            Do_Value(Ar, OutTile._Plates._LayerCount);

            auto PlateCount = 0;

            if (NOT Do_ReadCount(InOutState, kMinBytesPerPlate, PlateCount))
            { return; }

            OutTile._Plates._Plates.SetNum(PlateCount);

            for (auto& Plate : OutTile._Plates._Plates)
            {
                Do_Value(Ar, Plate._LayerIndex);
                Do_Value(Ar, Plate._MinX);
                Do_Value(Ar, Plate._MinY);
                Do_Value(Ar, Plate._MaxX);
                Do_Value(Ar, Plate._MaxY);
                Do_Value(Ar, Plate._MaxPlaneResidualUu);
                Do_Value(Ar, Plate._HeightRangeUu);
                Do_Value(Ar, Plate._MinClearanceUu);
                Do_Value(Ar, Plate._AreaPolicyIndex);
                Do_Value(Ar, Plate._CostMultiplier);

                if (InOutState.Get_Failed())
                { return; }
            }

            Do_ReadValueArray(InOutState, OutTile._Plates._CellToPlate);

            auto PolicyCount = 0;

            if (NOT Do_ReadCount(InOutState, kMinBytesPerTagContainer, PolicyCount))
            { return; }

            OutTile._Plates._AreaPolicies.SetNum(PolicyCount);

            for (auto& Policy : OutTile._Plates._AreaPolicies)
            {
                Do_ReadTagContainer(InOutState, Policy);

                if (InOutState.Get_Failed())
                { return; }
            }

            auto PortalCount = 0;

            if (NOT Do_ReadCount(InOutState, kMinBytesPerPortal, PortalCount))
            { return; }

            OutTile._Portals._Portals.SetNum(PortalCount);

            for (auto& Portal : OutTile._Portals._Portals)
            {
                Do_Value(Ar, Portal._PlateA);
                Do_Value(Ar, Portal._PlateB);
                Do_Value(Ar, Portal._Direction);
                Do_ReadIntPoint(Ar, Portal._FromMin);
                Do_ReadIntPoint(Ar, Portal._FromMax);
                Do_Value(Ar, Portal._MinEndZUu);
                Do_Value(Ar, Portal._MaxEndZUu);
                Do_Value(Ar, Portal._TraversalClearanceUu);

                if (InOutState.Get_Failed())
                { return; }
            }

            Do_ReadIndexArrays(InOutState, OutTile._Portals._PlateToPortals);

            const auto Do_ReadSegments = [&InOutState, &Ar](TArray<FCk_GroundNav_BoundarySegment>& OutSegments) -> void
            {
                OutSegments.Reset();

                auto SegmentCount = 0;

                if (NOT Do_ReadCount(InOutState, kMinBytesPerBoundarySegment, SegmentCount))
                { return; }

                OutSegments.SetNum(SegmentCount);

                for (auto& Segment : OutSegments)
                {
                    Do_Value(Ar, Segment._PlateIndex);
                    Do_Value(Ar, Segment._LayerIndex);
                    Do_Value(Ar, Segment._Side);
                    Do_ReadIntPoint(Ar, Segment._FromCell);
                    Do_ReadIntPoint(Ar, Segment._ToCell);
                    Do_ReadVector(InOutState, Segment._Start);
                    Do_ReadVector(InOutState, Segment._End);
                    Do_ReadVector2D(InOutState, Segment._InwardNormalXY);

                    if (InOutState.Get_Failed())
                    { return; }
                }
            };

            Do_ReadSegments(OutTile._Boundary._Segments);
            Do_ReadSegments(OutTile._Boundary._EdgeCandidates);

            Do_Value(Ar, OutTile._Boundary._BucketsX);
            Do_Value(Ar, OutTile._Boundary._BucketsY);
            Do_ReadIndexArrays(InOutState, OutTile._Boundary._Buckets);

            auto StubCount = 0;

            if (NOT Do_ReadCount(InOutState, kMinBytesPerSeamStub, StubCount))
            { return; }

            OutTile._SeamStubs.SetNum(StubCount);

            for (auto& Stub : OutTile._SeamStubs)
            {
                Do_Value(Ar, Stub._Direction);
                Do_Value(Ar, Stub._AlongIndex);
                Do_Value(Ar, Stub._PlateIndex);
                Do_Value(Ar, Stub._NearSurfaceZUu);
                Do_Value(Ar, Stub._FarSurfaceZUu);
                Do_Value(Ar, Stub._ClearanceUu);

                if (InOutState.Get_Failed())
                { return; }
            }

            Do_Value(Ar, OutTile._BakeStats._SourceTriangleCount);
            Do_Value(Ar, OutTile._BakeStats._RasterizedSpanCount);
            Do_Value(Ar, OutTile._BakeStats._RejectedCellCount);
        }

        auto
            Do_WriteOpenBodies(
                FArchive&                             InAr,
                const TArray<FCk_GroundNav_OpenBody>& InBodies)
            -> void
        {
            auto Count = InBodies.Num();
            Do_Value(InAr, Count);

            for (const auto& Body : InBodies)
            {
                auto BodyValue = Body._Body._Value;
                Do_Value(InAr, BodyValue);

                Do_WriteString(InAr, Body._Description);
                Do_WriteBox(InAr, Body._Bounds);

                auto TriangleCount = Body._TriangleCount;
                auto OpenEdgeCount = Body._OpenEdgeCount;

                Do_Value(InAr, TriangleCount);
                Do_Value(InAr, OpenEdgeCount);

                auto PointCount = Body._OpenEdgePoints.Num();
                Do_Value(InAr, PointCount);

                for (const auto& Point : Body._OpenEdgePoints)
                { Do_WriteVector(InAr, Point); }
            }
        }

        auto
            Do_ReadOpenBodies(
                FReadState&                     InOutState,
                TArray<FCk_GroundNav_OpenBody>& OutBodies)
            -> void
        {
            OutBodies.Reset();

            auto Count = 0;

            if (NOT Do_ReadCount(InOutState, kMinBytesPerOpenBody, Count))
            { return; }

            auto& Ar = *InOutState._Ar;

            OutBodies.SetNum(Count);

            for (auto& Body : OutBodies)
            {
                Do_Value(Ar, Body._Body._Value);
                Do_ReadString(InOutState, Body._Description);
                Do_ReadBox(InOutState, Body._Bounds);
                Do_Value(Ar, Body._TriangleCount);
                Do_Value(Ar, Body._OpenEdgeCount);

                auto PointCount = 0;

                if (NOT Do_ReadCount(InOutState, kMinBytesPerPoint, PointCount))
                { return; }

                Body._OpenEdgePoints.SetNum(PointCount);

                for (auto& Point : Body._OpenEdgePoints)
                {
                    Do_ReadVector(InOutState, Point);

                    if (InOutState.Get_Failed())
                    { return; }
                }
            }
        }

        // ---- Composition --------------------------------------------------------------------------------------------

        /**
         * The derives a load owes, in the order a bake runs them.
         *
         * The order is the contract, not a convenience: the plate set a link's ends address is only
         * whole once the seams have composed, and a link resolved after the labels were numbered would
         * join two components the numbering had already been told were apart.
         */
        auto
            Do_Compose(
                FCk_GroundNav_Field& InOutField)
            -> void
        {
            DoDerive_SeamPortals(InOutField);
            DoResolve_Links(InOutField);
            DoLabel_Reachability(InOutField);
        }

        auto
            Do_WriteFieldBlob(
                const FCk_GroundNav_Field& InField,
                const TArray<bool>&        InTileIsKept,
                TArray<uint8>&             OutBlob)
            -> void
        {
            OutBlob.Reset();

            auto Names = TSet<FString>{};
            Do_GatherParams(Names, InField._Params);

            for (auto TileIndex = 0; TileIndex < InField._Tiles.Num(); ++TileIndex)
            {
                if (InTileIsKept.IsValidIndex(TileIndex) && InTileIsKept[TileIndex])
                { Do_GatherTile(Names, InField._Tiles[TileIndex]); }
            }

            const auto Table = Make_TagTable(Names);

            auto Writer = FMemoryWriter{OutBlob};

            Do_WriteHeader(Writer, InField._Params, ECk_GroundNav_BlobContent::WholeField, Table._Names);

            auto EpochValue = InField._Epoch._Value;
            Do_Value(Writer, EpochValue);

            Do_WriteParams(Writer, Table, InField._Params);

            auto TileCount = InField._Tiles.Num();
            Do_Value(Writer, TileCount);

            for (auto TileIndex = 0; TileIndex < TileCount; ++TileIndex)
            {
                if (InTileIsKept.IsValidIndex(TileIndex) && InTileIsKept[TileIndex])
                {
                    Do_WriteTile(Writer, Table, InField._Tiles[TileIndex]);
                    continue;
                }

                // A tile left out is PRESENT and Unbuilt, carrying the coord the lattice gives it and
                // nothing else. A missing entry and an unbuilt one answer the same query differently.
                auto Absent = FCk_GroundNav_Tile{};
                Absent._Coord = Get_TileCoord(InField._Params._Divisions, TileIndex);

                Do_WriteTile(Writer, Table, Absent);
            }

            Do_WriteOpenBodies(Writer, InField._OpenBodies);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Write_Field(
            const FCk_GroundNav_Field& InField,
            TArray<uint8>&             OutBlob)
        -> void
    {
        using namespace fieldserialize_private;

        auto TileIsKept = TArray<bool>{};
        TileIsKept.Init(true, InField._Tiles.Num());

        Do_WriteFieldBlob(InField, TileIsKept, OutBlob);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Write_Tile(
            const FCk_GroundNav_Field&     InField,
            const FCk_GroundNav_TileCoord& InCoord,
            TArray<uint8>&                 OutBlob)
        -> void
    {
        using namespace fieldserialize_private;

        OutBlob.Reset();

        auto Absent = FCk_GroundNav_Tile{};
        Absent._Coord = InCoord;

        const auto TileIndex = Get_TileIndex(InField._Params._Divisions, InCoord);
        const auto& Tile = InField._Tiles.IsValidIndex(TileIndex) ? InField._Tiles[TileIndex] : Absent;

        auto Names = TSet<FString>{};
        Do_GatherTile(Names, Tile);

        const auto Table = Make_TagTable(Names);

        auto Writer = FMemoryWriter{OutBlob};

        Do_WriteHeader(Writer, InField._Params, ECk_GroundNav_BlobContent::SingleTile, Table._Names);
        Do_WriteTile(Writer, Table, Tile);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Write_FieldSubset(
            const FCk_GroundNav_Field&             InField,
            const TArray<FCk_GroundNav_TileCoord>& InKeptTiles,
            TArray<uint8>&                         OutBlob)
        -> void
    {
        using namespace fieldserialize_private;

        auto TileIsKept = TArray<bool>{};
        TileIsKept.Init(false, InField._Tiles.Num());

        for (const auto& Coord : InKeptTiles)
        {
            const auto TileIndex = Get_TileIndex(InField._Params._Divisions, Coord);

            if (TileIsKept.IsValidIndex(TileIndex))
            { TileIsKept[TileIndex] = true; }
        }

        Do_WriteFieldBlob(InField, TileIsKept, OutBlob);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Read_Field(
            const TArray<uint8>& InBlob,
            FCk_GroundNav_Field& OutField)
        -> ECk_GroundNav_LoadStatus
    {
        using namespace fieldserialize_private;

        auto Reader = FMemoryReader{InBlob};

        auto State = FReadState{};
        State._Ar = &Reader;

        auto Header = FBlobHeader{};
        Do_ReadHeader(State, Header, nullptr);

        if (State.Get_Failed())
        { return State.Get_Status(); }

        if (Header._Content != ECk_GroundNav_BlobContent::WholeField)
        { return ECk_GroundNav_LoadStatus::WrongMagic; }

        // Decoded into a field of its own and moved into the caller's only once the whole read has
        // held. A caller that is handed back a half-filled field on a refusal has nothing to fall
        // back to.
        auto Field = FCk_GroundNav_Field{};

        Do_Value(Reader, Field._Epoch._Value);
        Do_ReadParams(State, Field._Params);

        auto TileCount = 0;

        if (Do_ReadCount(State, kMinBytesPerTile, TileCount))
        {
            Field._Tiles.SetNum(TileCount);

            for (auto& Tile : Field._Tiles)
            { Do_ReadTile(State, Tile); }

            Do_ReadOpenBodies(State, Field._OpenBodies);
        }

        const auto Status = State.Get_Status();

        if (Status != ECk_GroundNav_LoadStatus::Loaded)
        { return Status; }

        Do_Compose(Field);

        OutField = MoveTemp(Field);

        return ECk_GroundNav_LoadStatus::Loaded;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Compose_LoadedField(
            FCk_GroundNav_Field& InOutField)
        -> void
    {
        fieldserialize_private::Do_Compose(InOutField);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Read_TileInto(
            const TArray<uint8>&        InBlob,
            FCk_GroundNav_Field&        InOutField,
            ECk_GroundNav_ComposeOnLoad InCompose)
        -> ECk_GroundNav_LoadStatus
    {
        using namespace fieldserialize_private;

        auto Reader = FMemoryReader{InBlob};

        auto State = FReadState{};
        State._Ar = &Reader;

        auto Header = FBlobHeader{};
        Do_ReadHeader(State, Header, nullptr);

        if (State.Get_Failed())
        { return State.Get_Status(); }

        if (Header._Content != ECk_GroundNav_BlobContent::SingleTile)
        { return ECk_GroundNav_LoadStatus::WrongMagic; }

        if (NOT Get_LatticeMatches(Header, InOutField._Params))
        { return ECk_GroundNav_LoadStatus::LatticeMismatch; }

        auto Tile = FCk_GroundNav_Tile{};
        Do_ReadTile(State, Tile);

        const auto Status = State.Get_Status();

        if (Status != ECk_GroundNav_LoadStatus::Loaded)
        { return Status; }

        const auto TileIndex = Get_TileIndex(InOutField._Params._Divisions, Tile._Coord);

        if (NOT InOutField._Tiles.IsValidIndex(TileIndex))
        { return ECk_GroundNav_LoadStatus::LatticeMismatch; }

        InOutField._Tiles[TileIndex] = MoveTemp(Tile);

        if (InCompose == ECk_GroundNav_ComposeOnLoad::Now)
        { Do_Compose(InOutField); }

        return ECk_GroundNav_LoadStatus::Loaded;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Read_TagTable(
            const TArray<uint8>& InBlob,
            TArray<FString>&     OutTagNames)
        -> ECk_GroundNav_LoadStatus
    {
        using namespace fieldserialize_private;

        OutTagNames.Reset();

        auto Reader = FMemoryReader{InBlob};

        auto State = FReadState{};
        State._Ar = &Reader;

        auto Header = FBlobHeader{};
        Do_ReadHeader(State, Header, &OutTagNames);

        const auto Status = State.Get_Status();

        // UnknownTag is the ONE refusal a caller wants the names for: the whole table was read, and
        // which name failed to resolve is the answer being asked for. Every other refusal means the
        // table itself is not trustworthy, so nothing is handed back.
        if (Status != ECk_GroundNav_LoadStatus::Loaded && Status != ECk_GroundNav_LoadStatus::UnknownTag)
        {
            OutTagNames.Reset();
        }

        return Status;
    }
}

// --------------------------------------------------------------------------------------------------------------------
