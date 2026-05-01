#include <gtest/gtest.h>
#include "tachyon/text/fonts/management/font_resolver.h"
#include "tachyon/text/fonts/core/font_face.h"

namespace tachyon::text {

TEST(FontResolverTest, RegularFaceResolves) {
    FontResolver resolver;
    FontFace regular_face;
    FontFace bold_face;
    FontFace italic_face;
    FontFace bold_italic_face;

    FontFaceInfo regular_info;
    regular_info.family = "TestFont";
    regular_info.weight = 400;
    regular_info.style = FontStyle::Normal;
    regular_face.set_info(regular_info);

    FontFaceInfo bold_info;
    bold_info.family = "TestFont";
    bold_info.weight = 700;
    bold_info.style = FontStyle::Normal;
    bold_face.set_info(bold_info);

    FontFaceInfo italic_info;
    italic_info.family = "TestFont";
    italic_info.weight = 400;
    italic_info.style = FontStyle::Italic;
    italic_face.set_info(italic_info);

    FontFaceInfo bold_italic_info;
    bold_italic_info.family = "TestFont";
    bold_italic_info.weight = 700;
    bold_italic_info.style = FontStyle::Italic;
    bold_italic_face.set_info(bold_italic_info);

    resolver.register_face(regular_face, "TestFont", 400, FontStyle::Normal);
    resolver.register_face(bold_face, "TestFont", 700, FontStyle::Normal);
    resolver.register_face(italic_face, "TestFont", 400, FontStyle::Italic);
    resolver.register_face(bold_italic_face, "TestFont", 700, FontStyle::Italic);

    FontRequest req;
    req.family = "TestFont";
    req.weight = 400;
    req.style = FontStyle::Normal;
    ResolvedFont result = resolver.resolve(req);
    EXPECT_NE(result.face, nullptr);
    EXPECT_EQ(result.face->info().weight, 400u);
    EXPECT_EQ(result.face->info().style, FontStyle::Normal);
    EXPECT_FALSE(result.is_fallback);
}

TEST(FontResolverTest, BoldFaceResolves) {
    FontResolver resolver;
    FontFace bold_face;
    FontFaceInfo bold_info;
    bold_info.family = "TestFont";
    bold_info.weight = 700;
    bold_info.style = FontStyle::Normal;
    bold_face.set_info(bold_info);
    resolver.register_face(bold_face, "TestFont", 700, FontStyle::Normal);

    FontRequest req;
    req.family = "TestFont";
    req.weight = 700;
    req.style = FontStyle::Normal;
    ResolvedFont result = resolver.resolve(req);
    ASSERT_NE(result.face, nullptr);
    EXPECT_EQ(result.face->info().weight, 700u);
}

TEST(FontResolverTest, ItalicFaceResolves) {
    FontResolver resolver;
    FontFace italic_face;
    FontFaceInfo italic_info;
    italic_info.family = "TestFont";
    italic_info.weight = 400;
    italic_info.style = FontStyle::Italic;
    italic_face.set_info(italic_info);
    resolver.register_face(italic_face, "TestFont", 400, FontStyle::Italic);

    FontRequest req;
    req.family = "TestFont";
    req.weight = 400;
    req.style = FontStyle::Italic;
    ResolvedFont result = resolver.resolve(req);
    ASSERT_NE(result.face, nullptr);
    EXPECT_EQ(result.face->info().style, FontStyle::Italic);
}

TEST(FontResolverTest, WeightFallback) {
    FontResolver resolver;
    FontFace regular_face;
    FontFaceInfo regular_info;
    regular_info.family = "FallbackTest";
    regular_info.weight = 400;
    regular_face.set_info(regular_info);
    resolver.register_face(regular_face, "FallbackTest", 400, FontStyle::Normal);

    FontRequest req;
    req.family = "FallbackTest";
    req.weight = 500;
    req.style = FontStyle::Normal;
    ResolvedFont result = resolver.resolve(req);
    ASSERT_NE(result.face, nullptr);
}

TEST(FontResolverTest, StyleFallback) {
    FontResolver resolver;
    FontFace regular_face;
    FontFaceInfo regular_info;
    regular_info.family = "StyleFallback";
    regular_info.weight = 400;
    regular_info.style = FontStyle::Normal;
    regular_face.set_info(regular_info);
    resolver.register_face(regular_face, "StyleFallback", 400, FontStyle::Normal);

    FontRequest req;
    req.family = "StyleFallback";
    req.weight = 400;
    req.style = FontStyle::Italic;
    ResolvedFont result = resolver.resolve(req);
    ASSERT_NE(result.face, nullptr);
    EXPECT_EQ(result.face->info().style, FontStyle::Normal);
}

TEST(FontResolverTest, StretchHandling) {
    FontResolver resolver;
    FontFace normal_stretch_face;
    FontFace condensed_face;

    FontFaceInfo normal_info;
    normal_info.family = "StretchTest";
    normal_info.weight = 400;
    normal_info.stretch = FontStretch::Normal;
    normal_stretch_face.set_info(normal_info);

    FontFaceInfo condensed_info;
    condensed_info.family = "StretchTest";
    condensed_info.weight = 400;
    condensed_info.stretch = FontStretch::Condensed;
    condensed_face.set_info(condensed_info);

    resolver.register_face(normal_stretch_face, "StretchTest", 400, FontStyle::Normal, FontStretch::Normal);
    resolver.register_face(condensed_face, "StretchTest", 400, FontStyle::Normal, FontStretch::Condensed);

    FontRequest req;
    req.family = "StretchTest";
    req.weight = 400;
    req.style = FontStyle::Normal;
    req.stretch = FontStretch::Expanded;
    ResolvedFont result = resolver.resolve(req);
    ASSERT_NE(result.face, nullptr);
    EXPECT_EQ(result.face->info().stretch, FontStretch::Normal);
}

TEST(FontResolverTest, GetFacesForFamily) {
    FontResolver resolver;
    FontFace face1, face2, face3;

    face1.set_info({"Family1", "", 400, FontStyle::Normal});
    face2.set_info({"Family1", "", 700, FontStyle::Normal});
    face3.set_info({"Family2", "", 400, FontStyle::Normal});

    resolver.register_face(face1, "Family1", 400, FontStyle::Normal);
    resolver.register_face(face2, "Family1", 700, FontStyle::Normal);
    resolver.register_face(face3, "Family2", 400, FontStyle::Normal);

    auto family1_faces = resolver.get_faces_for_family("Family1");
    EXPECT_EQ(family1_faces.size(), 2u);

    auto family3_faces = resolver.get_faces_for_family("Family3");
    EXPECT_EQ(family3_faces.size(), 0u);
}

TEST(FontResolverTest, Clear) {
    FontResolver resolver;
    FontFace face;
    face.set_info({"ClearTest", "", 400});
    resolver.register_face(face, "ClearTest", 400, FontStyle::Normal);

    FontRequest req;
    req.family = "ClearTest";
    EXPECT_NE(resolver.resolve(req).face, nullptr);

    resolver.clear();
    EXPECT_EQ(resolver.resolve(req).face, nullptr);
}

} // namespace tachyon::text
