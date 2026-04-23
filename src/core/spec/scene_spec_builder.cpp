#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"

#include <memory>
#include <vector>

namespace tachyon {

struct SceneSpecBuilder::SceneSpecBuilderImpl {
    class CompositionBuilder {
    public:
        CompositionBuilder& SetId(std::string id) {
            id_ = std::move(id);
            return *this;
        }
        CompositionBuilder& SetName(std::string name) {
            name_ = std::move(name);
            return *this;
        }
        CompositionBuilder& SetWidth(std::int64_t w) {
            width_ = w;
            return *this;
        }
        CompositionBuilder& SetHeight(std::int64_t h) {
            height_ = h;
            return *this;
        }
        CompositionBuilder& SetDuration(double d) {
            duration_ = d;
            return *this;
        }
        CompositionBuilder& SetFps(std::optional<std::int64_t> f) {
            fps_ = f;
            return *this;
        }
        CompositionBuilder& SetBackground(std::optional<std::string> b) {
            background_ = b;
            return *this;
        }

        CompositionSpec Build() {
            CompositionSpec comp;
            comp.id = id_.has_value() ? *id_ : "default";
            comp.name = name_.has_value() ? *name_ : "Unnamed";
            comp.width = static_cast<int>(width_);
            comp.height = static_cast<int>(height_);
            comp.duration = duration_;
            if (fps_.has_value()) {
                comp.fps = *fps_;
                comp.frame_rate.numerator = *fps_;
                comp.frame_rate.denominator = 1;
            }
            comp.background = background_;
            return comp;
        }

    private:
        std::optional<std::string> id_;
        std::optional<std::string> name_;
        std::int64_t width_{1920};
        std::int64_t height_{1080};
        double duration_{10.0};
        std::optional<std::int64_t> fps_;
        std::optional<std::string> background_;
    };

    std::optional<std::string> project_id;
    std::optional<std::string> project_name;
    std::optional<std::string> project_authoring_tool;
    std::optional<std::int64_t> project_root_seed;
    std::vector<CompositionBuilder> compositions;

    SceneSpec Build() {
        SceneSpec spec;
        spec.version = "1.0";
        spec.spec_version = "1.0";
        if (project_id) spec.project.id = *project_id;
        if (project_name) spec.project.name = *project_name;
        if (project_authoring_tool) spec.project.authoring_tool = *project_authoring_tool;
        if (project_root_seed) spec.project.root_seed = *project_root_seed;
        
        for (auto& builder : compositions) {
            spec.compositions.push_back(builder.Build());
        }
        return spec;
    }
};

SceneSpecBuilder::SceneSpecBuilder() : impl_(std::make_unique<SceneSpecBuilderImpl>()) {}
SceneSpecBuilder::~SceneSpecBuilder() = default;

SceneSpecBuilder& SceneSpecBuilder::SetProjectId(std::string id) {
    impl_->project_id = std::move(id);
    return *this;
}

SceneSpecBuilder& SceneSpecBuilder::SetProjectName(std::string name) {
    impl_->project_name = std::move(name);
    return *this;
}

SceneSpecBuilder& SceneSpecBuilder::SetProjectAuthoringTool(std::string tool) {
    impl_->project_authoring_tool = std::move(tool);
    return *this;
}

SceneSpecBuilder& SceneSpecBuilder::SetProjectRootSeed(std::optional<std::int64_t> seed) {
    impl_->project_root_seed = seed;
    return *this;
}

SceneSpecBuilder& SceneSpecBuilder::AddComposition(
    std::string id, std::string name, std::int64_t width,
    std::int64_t height, double duration, std::optional<std::int64_t> fps,
    std::optional<std::string> background) {
    SceneSpecBuilderImpl::CompositionBuilder builder;
    builder.SetId(std::move(id)).SetName(std::move(name)).SetWidth(width).SetHeight(height).SetDuration(duration).SetFps(fps).SetBackground(background);
    impl_->compositions.push_back(std::move(builder));
    return *this;
}

SceneSpec SceneSpecBuilder::Build() && {
    return impl_->Build();
}

} // namespace tachyon
