#include "tachyon/importer/scene_importer.h"
#include <stdexcept>

#ifdef TACHYON_ALEMBIC
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
using namespace Alembic::AbcGeom;
#endif

namespace tachyon::importer {

#ifdef TACHYON_ALEMBIC

class AlembicImporter : public SceneImporterBase {
public:
    bool load(const std::string& path) override {
        try {
            m_archive = IArchive(Alembic::AbcCoreOgawa::ReadArchive(), path);
            if (!m_archive.valid()) { m_error = "Invalid Alembic archive: " + path; return false; }
            auto ts = m_archive.getTimeSampling(0);
            m_start_time = ts->getSampleTime(0);
            size_t nsamp = m_archive.getMaxNumSamplesForTimeSamplingIndex(0);
            m_end_time   = (nsamp > 0) ? ts->getSampleTime(nsamp - 1) : 0.0;
            m_is_animated = nsamp > 1;
            return true;
        } catch (const std::exception& e) { m_error = e.what(); return false; }
    }

    core::SceneSpec build_scene_spec() const override {
        return build_scene_spec_at(m_start_time);
    }

    core::SceneSpec build_scene_spec_at(double t) const override {
        core::SceneSpec spec;
        if (!m_archive.valid()) return spec;
        traverse_object(m_archive.getTop(), t, spec);
        return spec;
    }

private:
    IArchive m_archive;

    void traverse_object(IObject obj, double t, core::SceneSpec& spec) const {
        for (size_t i = 0; i < obj.getNumChildren(); ++i) {
            IObject child = obj.getChild(i);
            if (IPolyMesh::matches(child.getMetaData())) {
                extract_polymesh(IPolyMesh(child, kWrapExisting), t, spec);
            }
            traverse_object(child, t, spec);
        }
    }

    void extract_polymesh(const IPolyMesh& mesh, double t, core::SceneSpec& spec) const {
        core::MeshSpec ms;
        ms.id   = mesh.getFullName();
        ms.name = mesh.getName();
        ISampleSelector sel(t);
        IPolyMeshSchema::Sample samp;
        mesh.getSchema().get(samp, sel);
        auto pts = samp.getPositions();
        if (pts) {
            ms.vertices.reserve(pts->size());
            for (const auto& p : *pts) ms.vertices.push_back({p[0], p[1], p[2]});
        }
        auto idx = samp.getFaceIndices();
        if (idx) ms.indices.assign(idx->get(), idx->get() + idx->size());
        spec.meshes.push_back(std::move(ms));
    }
};

#endif

std::unique_ptr<SceneImporter> create_alembic_importer() {
#ifdef TACHYON_ALEMBIC
    return std::make_unique<AlembicImporter>();
#else
    return nullptr;
#endif
}

} // namespace tachyon::importer
