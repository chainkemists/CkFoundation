#include "CkPmg/CkPmg_FontGlyphCache.h"

#if CK_PMG_WITH_FREETYPE
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include "ConstrainedDelaunay2.h"
#endif

namespace ck::pmg
{
    auto FontGlyph_SelfTest_FreeTypeInitDone() -> bool
    {
#if CK_PMG_WITH_FREETYPE
        FT_Library Library = nullptr;
        if (FT_Init_FreeType(&Library) != 0) { return false; }
        FT_Done_FreeType(Library);
        return true;
#else
        return false;
#endif
    }

#if CK_PMG_WITH_FREETYPE
    namespace
    {
        // Bezier flatten step counts — the tunable smoothness/vertex-count knob.
        constexpr int32 ck_pmg_ConicSteps = 8;
        constexpr int32 ck_pmg_CubicSteps = 16;

        struct FDecomposeCtx
        {
            TArray<TArray<FVector2D>>* Contours = nullptr;
            FVector2D Pen = FVector2D::ZeroVector;
        };

        FVector2D FtToVec(const FT_Vector* V) { return FVector2D(static_cast<float>(V->x), static_cast<float>(V->y)); }

        int Decomp_MoveTo(const FT_Vector* To, void* User)
        {
            auto* C = static_cast<FDecomposeCtx*>(User);
            C->Contours->AddDefaulted();
            C->Pen = FtToVec(To);
            C->Contours->Last().Add(C->Pen);
            return 0;
        }
        int Decomp_LineTo(const FT_Vector* To, void* User)
        {
            auto* C = static_cast<FDecomposeCtx*>(User);
            C->Pen = FtToVec(To);
            if (C->Contours->Num() > 0) { C->Contours->Last().Add(C->Pen); }
            return 0;
        }
        int Decomp_ConicTo(const FT_Vector* Control, const FT_Vector* To, void* User)
        {
            auto* C = static_cast<FDecomposeCtx*>(User);
            const FVector2D P0 = C->Pen, P1 = FtToVec(Control), P2 = FtToVec(To);
            for (int32 i = 1; i <= ck_pmg_ConicSteps; ++i)
            {
                const float t = static_cast<float>(i) / ck_pmg_ConicSteps;
                const float u = 1.0f - t;
                C->Contours->Last().Add(u*u*P0 + 2.0f*u*t*P1 + t*t*P2);
            }
            C->Pen = P2;
            return 0;
        }
        int Decomp_CubicTo(const FT_Vector* C1, const FT_Vector* C2, const FT_Vector* To, void* User)
        {
            auto* C = static_cast<FDecomposeCtx*>(User);
            const FVector2D P0 = C->Pen, P1 = FtToVec(C1), P2 = FtToVec(C2), P3 = FtToVec(To);
            for (int32 i = 1; i <= ck_pmg_CubicSteps; ++i)
            {
                const float t = static_cast<float>(i) / ck_pmg_CubicSteps;
                const float u = 1.0f - t;
                C->Contours->Last().Add(u*u*u*P0 + 3.0f*u*u*t*P1 + 3.0f*u*t*t*P2 + t*t*t*P3);
            }
            C->Pen = P3;
            return 0;
        }

        void TessellateContours(const TArray<TArray<FVector2D>>& InContours,
                                TArray<FVector2D>& OutVerts, TArray<FIntVector>& OutTris)
        {
            using namespace UE::Geometry;
            FConstrainedDelaunay2d Delaunay;
            Delaunay.bOrientedEdges = true;
            Delaunay.FillRule = FConstrainedDelaunay2d::EFillRule::NonZero; // TrueType winding

            // Inputs are at FONT-UNIT scale here, so a sub-pixel tolerance is safe.
            constexpr double DedupeTolSq = 0.01 * 0.01;

            for (const TArray<FVector2D>& Contour : InContours)
            {
                // Drop consecutive coincident points and a closing duplicate — degenerate
                // zero-length edges make the constrained triangulation fail outright.
                TArray<FVector2D> Clean;
                Clean.Reserve(Contour.Num());
                for (const FVector2D& P : Contour)
                {
                    if (Clean.Num() == 0 || FVector2D::DistSquared(P, Clean.Last()) > DedupeTolSq)
                    { Clean.Add(P); }
                }
                if (Clean.Num() >= 2 && FVector2D::DistSquared(Clean[0], Clean.Last()) <= DedupeTolSq)
                { Clean.Pop(); }

                const int32 N = Clean.Num();
                if (N < 3) { continue; }

                const int32 Start = Delaunay.Vertices.Num();
                for (const FVector2D& P : Clean) { Delaunay.Vertices.Add(FVector2d(P.X, P.Y)); }
                for (int32 a = N - 1, b = 0; b < N; a = b++)
                { Delaunay.Edges.Add(FIndex2i(Start + a, Start + b)); }
            }
            if (Delaunay.Vertices.Num() < 3 || !Delaunay.Triangulate()) { return; }

            OutVerts.Reserve(Delaunay.Vertices.Num());
            for (const FVector2d& V : Delaunay.Vertices) { OutVerts.Add(FVector2D(static_cast<float>(V.X), static_cast<float>(V.Y))); }
            OutTris.Reserve(Delaunay.Triangles.Num());
            for (const FIndex3i& T : Delaunay.Triangles) { OutTris.Add(FIntVector(T.A, T.B, T.C)); }
        }
    }
#endif // CK_PMG_WITH_FREETYPE

    struct FFontGlyphCache::FFaceEntry
    {
        TArray<uint8>                          Bytes;       // retained — FT_New_Memory_Face does not copy
        float                                  UnitsPerEm = 1.0f;
        TMap<uint32, TUniquePtr<FCachedGlyph>> Glyphs;      // unique_ptr -> stable glyph addresses across map growth
#if CK_PMG_WITH_FREETYPE
        FT_Face                                Face = nullptr;
#endif
    };

    FFontGlyphCache& FFontGlyphCache::Get()
    {
        static FFontGlyphCache Instance;
        return Instance;
    }

    // Out-of-line ctor/dtor: FFaceEntry (above) is complete here, so the _Faces
    // TUniquePtr cleanup never instantiates against the header's forward decl.
    FFontGlyphCache::FFontGlyphCache() = default;

    FFontGlyphCache::~FFontGlyphCache()
    {
        Shutdown();
    }

    int32 FFontGlyphCache::EnsureFace(const TArray<uint8>& InFontBytes)
    {
#if CK_PMG_WITH_FREETYPE
        if (InFontBytes.Num() == 0) { return INDEX_NONE; }

        const uint32 Hash = FCrc::MemCrc32(InFontBytes.GetData(), InFontBytes.Num());
        if (const int32* Found = _FaceKeyByHash.Find(Hash))
        {
            // Guard the (astronomically rare) CRC32 collision: only reuse on exact bytes.
            if (_Faces.IsValidIndex(*Found) && _Faces[*Found]->Bytes == InFontBytes) { return *Found; }
        }

        if (_FtLibrary == nullptr)
        {
            FT_Library Lib = nullptr;
            if (FT_Init_FreeType(&Lib) != 0) { return INDEX_NONE; }
            _FtLibrary = Lib;
        }

        auto Entry = MakeUnique<FFaceEntry>();
        Entry->Bytes = InFontBytes; // retain
        if (FT_New_Memory_Face(static_cast<FT_Library>(_FtLibrary), Entry->Bytes.GetData(), Entry->Bytes.Num(), 0, &Entry->Face) != 0)
        { return INDEX_NONE; }
        Entry->UnitsPerEm = Entry->Face->units_per_EM != 0 ? static_cast<float>(Entry->Face->units_per_EM) : 1.0f;

        const int32 Key = _Faces.Add(MoveTemp(Entry));
        _FaceKeyByHash.Add(Hash, Key); // TMap::Add overwrites on collision; byte-compare above keeps lookups correct
        return Key;
#else
        return INDEX_NONE;
#endif
    }

    const FCachedGlyph& FFontGlyphCache::GetOrBuildGlyph(int32 InFaceKey, uint32 InCodepoint)
    {
#if CK_PMG_WITH_FREETYPE
        if (!_Faces.IsValidIndex(InFaceKey)) { return _EmptyGlyph; }
        FFaceEntry& Entry = *_Faces[InFaceKey];
        if (const TUniquePtr<FCachedGlyph>* Cached = Entry.Glyphs.Find(InCodepoint)) { return **Cached; }

        auto NewGlyph = MakeUnique<FCachedGlyph>();
        FCachedGlyph& Glyph = *NewGlyph;
        const FT_UInt GlyphIndex = FT_Get_Char_Index(Entry.Face, InCodepoint); // 0 -> .notdef
        if (FT_Load_Glyph(Entry.Face, GlyphIndex, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP) == 0)
        {
            const float InvEm = 1.0f / Entry.UnitsPerEm;
            Glyph.AdvanceEm = static_cast<float>(Entry.Face->glyph->advance.x) * InvEm;

            // Decompose the outline in FONT UNITS (large, well-conditioned coords).
            FT_Outline& Outline = Entry.Face->glyph->outline;
            FDecomposeCtx Ctx; Ctx.Contours = &Glyph.Contours;
            FT_Outline_Funcs Funcs{};
            Funcs.move_to = &Decomp_MoveTo; Funcs.line_to = &Decomp_LineTo;
            Funcs.conic_to = &Decomp_ConicTo; Funcs.cubic_to = &Decomp_CubicTo;
            FT_Outline_Decompose(&Outline, &Funcs, &Ctx);

            // Tessellate at font-unit scale (EM-normalized 0..1 input collapses under the
            // Delaunay tolerance and yields zero triangles).
            if (Glyph.Contours.Num() > 0)
            {
                TessellateContours(Glyph.Contours, Glyph.TessVerts, Glyph.TessTris);
            }

            // Normalize contours AND tessellated verts to EM units (size-independent cache).
            for (TArray<FVector2D>& C : Glyph.Contours) { for (FVector2D& P : C) { P *= InvEm; } }
            for (FVector2D& V : Glyph.TessVerts) { V *= InvEm; }

            Glyph.bHasGeometry = Glyph.TessTris.Num() > 0;
        }
        return *Entry.Glyphs.Add(InCodepoint, MoveTemp(NewGlyph));
#else
        return _EmptyGlyph;
#endif
    }

    float FFontGlyphCache::Get_LineHeightEm(int32 InFaceKey) const
    {
#if CK_PMG_WITH_FREETYPE
        if (!_Faces.IsValidIndex(InFaceKey)) { return 1.0f; }
        const FFaceEntry& Entry = *_Faces[InFaceKey];
        const float Em = Entry.UnitsPerEm;
        return Entry.Face->height != 0 ? static_cast<float>(Entry.Face->height) / Em : 1.2f;
#else
        return 1.2f;
#endif
    }

    void FFontGlyphCache::Shutdown()
    {
#if CK_PMG_WITH_FREETYPE
        for (TUniquePtr<FFaceEntry>& E : _Faces)
        { if (E && E->Face) { FT_Done_Face(E->Face); E->Face = nullptr; } }
        _Faces.Empty();
        _FaceKeyByHash.Empty();
        if (_FtLibrary != nullptr)
        {
            FT_Done_FreeType(static_cast<FT_Library>(_FtLibrary));
            _FtLibrary = nullptr;
        }
#else
        _Faces.Empty();
        _FaceKeyByHash.Empty();
#endif
    }
}
