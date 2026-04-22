#include "tachyon/importer/scene_importer.h"
#include <stdexcept>

#ifdef TACHYON_USD
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/gf/matrix4d.h>
PXR_NAMESPACE_USING_DIRECTIVE
#endif

namespace tachyon::importer {

#ifdef TACHYON_USD

class UsdImporter : public SceneImporter {
public:
    bool load(const std::string& path) override {
        try {
            m_stage = UsdStage::Open(path);
            if (!m_stage) { m_error = "Failed to open USD stage: " + path; return false; }
            auto range = m_stage->GetPlaybackRange();
            m_start_time = range.GetMin() / m_stage->GetFramesPerSecond();
            m_end_time   = range.GetMax() / m_stage->GetFramesPerSecond();
            m_is_animated = (m_end_time > m_start_time);
            return true;
        } catch (const std::exception& e) { m_error = e.what(); return false; }
    }

    core::SceneSpec build_scene_spec() const override {
        return build_scene_spec_at(m_start_time);
    }

    core::SceneSpec build_scene_spec_at(double t) const override {
        core::SceneSpec spec;
        if (!m_stage) return spec;

        UsdTimeCode time(t * m_stage->GetFramesPerSecond());

        for (const auto& prim : m_stage->Traverse()) {
            if (!prim.IsActive()) continue;
            if (prim.IsA<UsdGeomCamera>()) { extract_camera(prim, time, spec); continue; }
            if (prim.IsA<UsdGeomMesh>())   { extract_mesh(prim, time, spec);   continue; }
            if (prim.IsA<UsdGeomXformable>()) { extract_xform(prim, time, spec); }
        }
        return spec;
    }

    double start_time_seconds() const override { return m_start_time; }
    double end_time_seconds()   const override { return m_end_time; }
    bool   is_animated()        const override { return m_is_animated; }
    std::string last_error()    const override { return m_error; }

private:
    UsdStageRefPtr m_stage;
    double m_start_time{0}, m_end_time{0};
    bool   m_is_animated{false};
    mutable std::string m_error;

    void extract_camera(const UsdPrim& prim, UsdTimeCode t, core::SceneSpec& spec) const {
        UsdGeomCamera cam(prim);
        GfCamera gcam = cam.GetCamera(t);
        core::CameraSpec cs;
        cs.id   = prim.GetPath().GetString();
        cs.name = prim.GetName();
        cs.focal_length_mm   = gcam.GetFocalLength();
        cs.aperture_width_mm = gcam.GetHorizontalAperture();
        GfMatrix4d xform;
        bool reset;
        cam.GetLocalTransformation(&xform, &reset, t);
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                cs.transform.m[r][c] = (float)xform[r][c];
        spec.cameras.push_back(std::move(cs));
    }

    void extract_mesh(const UsdPrim& prim, UsdTimeCode t, core::SceneSpec& spec) const {
        UsdGeomMesh mesh(prim);
        core::MeshSpec ms;
        ms.id   = prim.GetPath().GetString();
        ms.name = prim.GetName();
        VtArray<GfVec3f> pts;
        mesh.GetPointsAttr().Get(&pts, t);
        ms.vertices.reserve(pts.size());
        for (const auto& p : pts) ms.vertices.push_back({p[0], p[1], p[2]});
        VtArray<int> indices;
        mesh.GetFaceVertexIndicesAttr().Get(&indices, t);
        ms.indices.assign(indices.begin(), indices.end());
        GfMatrix4d xform;
        bool reset;
        UsdGeomXformable(prim).GetLocalTransformation(&xform, &reset, t);
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                ms.transform.m[r][c] = (float)xform[r][c];
        spec.meshes.push_back(std::move(ms));
    }

    void extract_xform(const UsdPrim& prim, UsdTimeCode t, core::SceneSpec& spec) const {
        core::NullLayerSpec ls;
        ls.id   = prim.GetPath().GetString();
        ls.name = prim.GetName();
        GfMatrix4d xform;
        bool reset;
        UsdGeomXformable(prim).GetLocalTransformation(&xform, &reset, t);
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                ls.transform.m[r][c] = (float)xform[r][c];
        spec.null_layers.push_back(std::move(ls));
    }
};

#endif

std::unique_ptr<SceneImporter> create_usd_importer() {
#ifdef TACHYON_USD
    return std::make_unique<UsdImporter>();
#else
    return nullptr;
#endif
}

} // namespace tachyon::importer
