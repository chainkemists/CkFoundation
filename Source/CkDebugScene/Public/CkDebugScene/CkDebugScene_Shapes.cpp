#include "CkDebugScene/CkDebugScene_Shapes.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_debug_scene_shapes
{
    using FMesh = TSharedPtr<FCk_DebugScene_Mesh>;

    // Cache key: a shape id plus its quantised parameters. Two calls with the same parameters must
    // return the SAME mesh -- that is what keeps a thousand rings at one UStaticMesh.
    struct FKey
    {
        int32 _Shape = 0;
        int32 _A = 0;
        int32 _B = 0;
        int32 _C = 0;

        auto operator==(const FKey& InOther) const -> bool
        {
            return _Shape == InOther._Shape && _A == InOther._A && _B == InOther._B && _C == InOther._C;
        }
    };

    auto GetTypeHash(const FKey& InKey) -> uint32
    {
        return HashCombine(HashCombine(::GetTypeHash(InKey._Shape), ::GetTypeHash(InKey._A)),
                           HashCombine(::GetTypeHash(InKey._B), ::GetTypeHash(InKey._C)));
    }

    auto Get_Cache() -> TMap<FKey, FMesh>&
    {
        static auto Cache = TMap<FKey, FMesh>{};
        return Cache;
    }

    // Ratios are cached at 1/1000 resolution: far finer than anything readable on screen, and it
    // stops a caller sweeping a float from allocating an unbounded number of static meshes.
    auto Quantise(float InRatio) -> int32
    {
        return FMath::RoundToInt(FMath::Clamp(InRatio, 0.0f, 1.0f) * 1000.0f);
    }

    // Every emitted triangle goes through here. Zero-area triangles are DROPPED, not clamped or
    // nudged: a UV-parameterised surface legitimately collapses at its poles (the last cap ring of
    // a capsule, both ends of a sphere, the inner edge of a zero-hole ring), so a quad there is
    // genuinely a triangle. Emitting the collapsed half would be rejected wholesale by
    // Create_FromTriangles, taking the entire valid mesh down with it.
    //
    // The test matches Create_FromTriangles exactly, so nothing this accepts can be refused there.
    auto Add_Tri(TArray<FCk_DebugScene_Triangle>& InOut, const FVector& InA, const FVector& InB, const FVector& InC)
        -> void
    {
        if (FVector::CrossProduct(InB - InA, InC - InA).SizeSquared() <= SMALL_NUMBER)
        { return; }

        InOut.Add(FCk_DebugScene_Triangle{InA, InB, InC});
    }

    // Both windings. 2D primitives are viewed from arbitrary angles in a free camera, and a
    // single-sided filled shape simply vanishes from below -- that reads as a bug, not as a style.
    auto Add_TriDoubleSided(TArray<FCk_DebugScene_Triangle>& InOut, const FVector& InA, const FVector& InB,
                            const FVector& InC) -> void
    {
        Add_Tri(InOut, InA, InB, InC);
        Add_Tri(InOut, InA, InC, InB);
    }

    auto Add_Quad(TArray<FCk_DebugScene_Triangle>& InOut, const FVector& InA, const FVector& InB, const FVector& InC,
                  const FVector& InD) -> void
    {
        Add_Tri(InOut, InA, InB, InC);
        Add_Tri(InOut, InA, InC, InD);
    }

    auto Ring_Point(double InAngle, double InRadius, double InZ) -> FVector
    {
        return FVector{InRadius * FMath::Cos(InAngle), InRadius * FMath::Sin(InAngle), InZ};
    }

    auto Angle(int32 InIndex, int32 InSegments) -> double
    {
        return 2.0 * UE_DOUBLE_PI * (static_cast<double>(InIndex) / static_cast<double>(InSegments));
    }

    auto GetOrBuild(const FKey& InKey, TFunctionRef<TArray<FCk_DebugScene_Triangle>()> InBuild) -> FMesh
    {
        auto& Cache = Get_Cache();
        if (auto* Found = Cache.Find(InKey))
        { return *Found; }

        auto Built = FCk_DebugScene_Mesh::Create_FromTriangles(InBuild());
        Cache.Add(InKey, Built);
        return Built;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::debug_scene::shapes
{
    namespace impl = ck_debug_scene_shapes;

    auto
        Get_Box()
        -> TSharedPtr<FCk_DebugScene_Mesh>
    {
        return impl::GetOrBuild(impl::FKey{1}, []
        {
            auto Tris = TArray<FCk_DebugScene_Triangle>{};
            Tris.Reserve(12);
            constexpr auto H = 0.5;
            const auto P000 = FVector{-H, -H, -H}; const auto P100 = FVector{ H, -H, -H};
            const auto P110 = FVector{ H,  H, -H}; const auto P010 = FVector{-H,  H, -H};
            const auto P001 = FVector{-H, -H,  H}; const auto P101 = FVector{ H, -H,  H};
            const auto P111 = FVector{ H,  H,  H}; const auto P011 = FVector{-H,  H,  H};

            impl::Add_Quad(Tris, P001, P101, P111, P011); // +Z
            impl::Add_Quad(Tris, P010, P110, P100, P000); // -Z
            impl::Add_Quad(Tris, P000, P100, P101, P001); // -Y
            impl::Add_Quad(Tris, P110, P010, P011, P111); // +Y
            impl::Add_Quad(Tris, P100, P110, P111, P101); // +X
            impl::Add_Quad(Tris, P010, P000, P001, P011); // -X
            return Tris;
        });
    }

    auto
        Get_Sphere(int32 InRings, int32 InSegments)
        -> TSharedPtr<FCk_DebugScene_Mesh>
    {
        const auto Rings = FMath::Max(InRings, 3);
        const auto Segments = FMath::Max(InSegments, 3);
        return impl::GetOrBuild(impl::FKey{2, Rings, Segments}, [Rings, Segments]
        {
            auto Tris = TArray<FCk_DebugScene_Triangle>{};
            Tris.Reserve(Rings * Segments * 2);
            const auto At = [](double InPhi, double InTheta)
            {
                return FVector{FMath::Sin(InPhi) * FMath::Cos(InTheta),
                               FMath::Sin(InPhi) * FMath::Sin(InTheta), FMath::Cos(InPhi)};
            };
            for (auto Ring = 0; Ring < Rings; ++Ring)
            {
                const auto PhiA = UE_DOUBLE_PI * (static_cast<double>(Ring) / Rings);
                const auto PhiB = UE_DOUBLE_PI * (static_cast<double>(Ring + 1) / Rings);
                for (auto Seg = 0; Seg < Segments; ++Seg)
                {
                    const auto ThetaA = impl::Angle(Seg, Segments);
                    const auto ThetaB = impl::Angle(Seg + 1, Segments);
                    impl::Add_Quad(Tris, At(PhiA, ThetaA), At(PhiA, ThetaB), At(PhiB, ThetaB), At(PhiB, ThetaA));
                }
            }
            return Tris;
        });
    }

    auto
        Get_Cylinder(int32 InSegments)
        -> TSharedPtr<FCk_DebugScene_Mesh>
    {
        const auto Segments = FMath::Max(InSegments, 3);
        return impl::GetOrBuild(impl::FKey{3, Segments}, [Segments]
        {
            auto Tris = TArray<FCk_DebugScene_Triangle>{};
            Tris.Reserve(Segments * 4);
            constexpr auto H = 0.5;
            const auto TopHub = FVector{0.0, 0.0, H};
            const auto BottomHub = FVector{0.0, 0.0, -H};
            for (auto Seg = 0; Seg < Segments; ++Seg)
            {
                const auto A = impl::Angle(Seg, Segments);
                const auto B = impl::Angle(Seg + 1, Segments);
                const auto TopA = impl::Ring_Point(A, 1.0, H);
                const auto TopB = impl::Ring_Point(B, 1.0, H);
                const auto BotA = impl::Ring_Point(A, 1.0, -H);
                const auto BotB = impl::Ring_Point(B, 1.0, -H);
                impl::Add_Quad(Tris, BotA, BotB, TopB, TopA);
                impl::Add_Tri(Tris, TopHub, TopA, TopB);
                impl::Add_Tri(Tris, BottomHub, BotB, BotA);
            }
            return Tris;
        });
    }

    auto
        Get_Cone(int32 InSegments)
        -> TSharedPtr<FCk_DebugScene_Mesh>
    {
        const auto Segments = FMath::Max(InSegments, 3);
        return impl::GetOrBuild(impl::FKey{4, Segments}, [Segments]
        {
            auto Tris = TArray<FCk_DebugScene_Triangle>{};
            Tris.Reserve(Segments * 2);
            constexpr auto H = 0.5;
            const auto Apex = FVector{0.0, 0.0, H};
            const auto BaseHub = FVector{0.0, 0.0, -H};
            for (auto Seg = 0; Seg < Segments; ++Seg)
            {
                const auto A = impl::Angle(Seg, Segments);
                const auto B = impl::Angle(Seg + 1, Segments);
                const auto BaseA = impl::Ring_Point(A, 1.0, -H);
                const auto BaseB = impl::Ring_Point(B, 1.0, -H);
                impl::Add_Tri(Tris, Apex, BaseA, BaseB);
                impl::Add_Tri(Tris, BaseHub, BaseB, BaseA);
            }
            return Tris;
        });
    }

    auto
        Get_Capsule(float InSegmentRatio, int32 InRings, int32 InSegments)
        -> TSharedPtr<FCk_DebugScene_Mesh>
    {
        const auto Rings = FMath::Max(InRings, 2);
        const auto Segments = FMath::Max(InSegments, 3);
        // Ratio is unbounded above (a long thin capsule is legitimate), so it is quantised directly
        // rather than through Quantise(), which clamps to 0..1.
        const auto RatioKey = FMath::RoundToInt(FMath::Clamp(InSegmentRatio, 0.0f, 1000.0f) * 100.0f);
        return impl::GetOrBuild(impl::FKey{5, Rings, Segments, RatioKey}, [Rings, Segments, RatioKey]
        {
            auto Tris = TArray<FCk_DebugScene_Triangle>{};
            // Half the STRAIGHT section. The caps are unit hemispheres either side of it, so the
            // mesh keeps true capsule proportions under a uniform scale.
            const auto H = (static_cast<double>(RatioKey) / 100.0) * 0.5;
            for (auto Seg = 0; Seg < Segments; ++Seg)
            {
                const auto A = impl::Angle(Seg, Segments);
                const auto B = impl::Angle(Seg + 1, Segments);
                impl::Add_Quad(Tris, impl::Ring_Point(A, 1.0, -H), impl::Ring_Point(B, 1.0, -H),
                               impl::Ring_Point(B, 1.0, H), impl::Ring_Point(A, 1.0, H));
            }
            for (auto Cap = 0; Cap < 2; ++Cap)
            {
                const auto IsTop = Cap == 0;
                const auto OffsetZ = IsTop ? H : -H;
                const auto At = [IsTop, OffsetZ](double InPhi, double InTheta)
                {
                    const auto Z = FMath::Sin(InPhi) * (IsTop ? 1.0 : -1.0);
                    const auto R = FMath::Cos(InPhi);
                    return FVector{R * FMath::Cos(InTheta), R * FMath::Sin(InTheta), Z + OffsetZ};
                };
                for (auto Ring = 0; Ring < Rings; ++Ring)
                {
                    const auto PhiA = (UE_DOUBLE_PI * 0.5) * (static_cast<double>(Ring) / Rings);
                    const auto PhiB = (UE_DOUBLE_PI * 0.5) * (static_cast<double>(Ring + 1) / Rings);
                    for (auto Seg = 0; Seg < Segments; ++Seg)
                    {
                        const auto ThetaA = impl::Angle(Seg, Segments);
                        const auto ThetaB = impl::Angle(Seg + 1, Segments);
                        if (IsTop)
                        {
                            impl::Add_Quad(Tris, At(PhiA, ThetaA), At(PhiA, ThetaB), At(PhiB, ThetaB),
                                           At(PhiB, ThetaA));
                        }
                        else
                        {
                            impl::Add_Quad(Tris, At(PhiA, ThetaA), At(PhiB, ThetaA), At(PhiB, ThetaB),
                                           At(PhiA, ThetaB));
                        }
                    }
                }
            }
            return Tris;
        });
    }

    auto
        Get_Torus(float InTubeRatio, int32 InMajorSegments, int32 InMinorSegments)
        -> TSharedPtr<FCk_DebugScene_Mesh>
    {
        const auto Major = FMath::Max(InMajorSegments, 3);
        const auto Minor = FMath::Max(InMinorSegments, 3);
        const auto TubeKey = impl::Quantise(InTubeRatio);
        return impl::GetOrBuild(impl::FKey{6, Major, Minor, TubeKey}, [Major, Minor, TubeKey]
        {
            auto Tris = TArray<FCk_DebugScene_Triangle>{};
            Tris.Reserve(Major * Minor * 2);
            const auto Tube = FMath::Clamp(static_cast<double>(TubeKey) / 1000.0, 0.001, 0.999);
            const auto Centre = 1.0 - Tube;
            const auto At = [Centre, Tube](double InMajorAngle, double InMinorAngle)
            {
                const auto R = Centre + Tube * FMath::Cos(InMinorAngle);
                return FVector{R * FMath::Cos(InMajorAngle), R * FMath::Sin(InMajorAngle),
                               Tube * FMath::Sin(InMinorAngle)};
            };
            for (auto MajorIndex = 0; MajorIndex < Major; ++MajorIndex)
            {
                const auto UA = impl::Angle(MajorIndex, Major);
                const auto UB = impl::Angle(MajorIndex + 1, Major);
                for (auto MinorIndex = 0; MinorIndex < Minor; ++MinorIndex)
                {
                    const auto VA = impl::Angle(MinorIndex, Minor);
                    const auto VB = impl::Angle(MinorIndex + 1, Minor);
                    impl::Add_Quad(Tris, At(UA, VA), At(UB, VA), At(UB, VB), At(UA, VB));
                }
            }
            return Tris;
        });
    }

    auto
        Get_Quad()
        -> TSharedPtr<FCk_DebugScene_Mesh>
    {
        return impl::GetOrBuild(impl::FKey{7}, []
        {
            auto Tris = TArray<FCk_DebugScene_Triangle>{};
            constexpr auto H = 0.5;
            impl::Add_TriDoubleSided(Tris, FVector{-H, -H, 0.0}, FVector{H, -H, 0.0}, FVector{H, H, 0.0});
            impl::Add_TriDoubleSided(Tris, FVector{-H, -H, 0.0}, FVector{H, H, 0.0}, FVector{-H, H, 0.0});
            return Tris;
        });
    }

    auto
        Get_Disc(int32 InSegments)
        -> TSharedPtr<FCk_DebugScene_Mesh>
    {
        const auto Segments = FMath::Max(InSegments, 3);
        return impl::GetOrBuild(impl::FKey{8, Segments}, [Segments]
        {
            auto Tris = TArray<FCk_DebugScene_Triangle>{};
            Tris.Reserve(Segments * 2);
            const auto Hub = FVector::ZeroVector;
            for (auto Seg = 0; Seg < Segments; ++Seg)
            {
                impl::Add_TriDoubleSided(Tris, Hub, impl::Ring_Point(impl::Angle(Seg, Segments), 1.0, 0.0),
                                         impl::Ring_Point(impl::Angle(Seg + 1, Segments), 1.0, 0.0));
            }
            return Tris;
        });
    }

    auto
        Get_Ring(float InInnerRatio, int32 InSegments)
        -> TSharedPtr<FCk_DebugScene_Mesh>
    {
        const auto Segments = FMath::Max(InSegments, 3);
        const auto InnerKey = impl::Quantise(InInnerRatio);
        return impl::GetOrBuild(impl::FKey{9, Segments, InnerKey}, [Segments, InnerKey]
        {
            auto Tris = TArray<FCk_DebugScene_Triangle>{};
            Tris.Reserve(Segments * 4);
            const auto Inner = FMath::Clamp(static_cast<double>(InnerKey) / 1000.0, 0.0, 0.999);
            for (auto Seg = 0; Seg < Segments; ++Seg)
            {
                const auto A = impl::Angle(Seg, Segments);
                const auto B = impl::Angle(Seg + 1, Segments);
                const auto OuterA = impl::Ring_Point(A, 1.0, 0.0);
                const auto OuterB = impl::Ring_Point(B, 1.0, 0.0);
                const auto InnerA = impl::Ring_Point(A, Inner, 0.0);
                const auto InnerB = impl::Ring_Point(B, Inner, 0.0);
                impl::Add_TriDoubleSided(Tris, InnerA, OuterA, OuterB);
                impl::Add_TriDoubleSided(Tris, InnerA, OuterB, InnerB);
            }
            return Tris;
        });
    }

    auto
        Get_Triangle()
        -> TSharedPtr<FCk_DebugScene_Mesh>
    {
        return impl::GetOrBuild(impl::FKey{10}, []
        {
            auto Tris = TArray<FCk_DebugScene_Triangle>{};
            impl::Add_TriDoubleSided(Tris, impl::Ring_Point(impl::Angle(0, 3), 1.0, 0.0),
                                     impl::Ring_Point(impl::Angle(1, 3), 1.0, 0.0),
                                     impl::Ring_Point(impl::Angle(2, 3), 1.0, 0.0));
            return Tris;
        });
    }

    auto
        Get_Cross(float InThicknessRatio)
        -> TSharedPtr<FCk_DebugScene_Mesh>
    {
        const auto ThicknessKey = impl::Quantise(InThicknessRatio);
        return impl::GetOrBuild(impl::FKey{11, ThicknessKey}, [ThicknessKey]
        {
            auto Tris = TArray<FCk_DebugScene_Triangle>{};
            const auto T = FMath::Clamp(static_cast<double>(ThicknessKey) / 1000.0, 0.01, 1.0) * 0.5;
            constexpr auto H = 0.5;
            // Two overlapping bars; the overlap is harmless for an unlit debug fill.
            impl::Add_TriDoubleSided(Tris, FVector{-H, -T, 0.0}, FVector{H, -T, 0.0}, FVector{H, T, 0.0});
            impl::Add_TriDoubleSided(Tris, FVector{-H, -T, 0.0}, FVector{H, T, 0.0}, FVector{-H, T, 0.0});
            impl::Add_TriDoubleSided(Tris, FVector{-T, -H, 0.0}, FVector{T, -H, 0.0}, FVector{T, H, 0.0});
            impl::Add_TriDoubleSided(Tris, FVector{-T, -H, 0.0}, FVector{T, H, 0.0}, FVector{-T, H, 0.0});
            return Tris;
        });
    }

    auto
        Get_Arrow(float InShaftRatio, float InHeadRatio)
        -> TSharedPtr<FCk_DebugScene_Mesh>
    {
        const auto ShaftKey = impl::Quantise(InShaftRatio);
        const auto HeadKey = impl::Quantise(InHeadRatio);
        return impl::GetOrBuild(impl::FKey{12, ShaftKey, HeadKey}, [ShaftKey, HeadKey]
        {
            auto Tris = TArray<FCk_DebugScene_Triangle>{};
            const auto ShaftHalf = FMath::Clamp(static_cast<double>(ShaftKey) / 1000.0, 0.01, 1.0) * 0.5;
            const auto HeadLength = FMath::Clamp(static_cast<double>(HeadKey) / 1000.0, 0.01, 0.99);
            constexpr auto H = 0.5;
            const auto ShaftEnd = H - HeadLength;
            impl::Add_TriDoubleSided(Tris, FVector{-H, -ShaftHalf, 0.0}, FVector{ShaftEnd, -ShaftHalf, 0.0},
                                     FVector{ShaftEnd, ShaftHalf, 0.0});
            impl::Add_TriDoubleSided(Tris, FVector{-H, -ShaftHalf, 0.0}, FVector{ShaftEnd, ShaftHalf, 0.0},
                                     FVector{-H, ShaftHalf, 0.0});
            impl::Add_TriDoubleSided(Tris, FVector{ShaftEnd, -ShaftHalf * 2.5, 0.0}, FVector{H, 0.0, 0.0},
                                     FVector{ShaftEnd, ShaftHalf * 2.5, 0.0});
            return Tris;
        });
    }

    auto
        Clear_Cache()
        -> void
    {
        ck_debug_scene_shapes::Get_Cache().Empty();
    }
}
