#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/scene/builder.h"
#include <string>
#include <vector>

using namespace tachyon;
using namespace tachyon::scene;

void add_light_leak_composition(SceneBuilder &scene,
                                const std::string &transition_id) {
  std::string name = transition_id;
  size_t last_dot = name.find_last_of('.');
  if (last_dot != std::string::npos)
    name = name.substr(last_dot + 1);

  scene.composition(name, [id = transition_id, name](CompositionBuilder &comp) {
    comp.size(1280, 720).fps(30).duration(3.0);

    comp.layer("scene_a", [](LayerBuilder &l) {
      l.solid("fill").color({30, 30, 30, 255}).in(0.0).out(2.0);
    });

    comp.layer("scene_b", [id](LayerBuilder &l) {
      l.solid("fill").color({220, 220, 220, 255}).in(1.0).out(3.0);
      l.enter().id(id).duration(1.0);
    });

    // Title text
    comp.layer("title", [name](LayerBuilder &l) {
      l.text(name)
          .font("SFPro", 36)
          .color({255, 255, 255, 255})
          .done()
          .position(50, 50)
          .in(0.0)
          .out(3.0);
    });
  });
}

extern "C" __declspec(dllexport) SceneSpec build_scene() {
  SceneBuilder scene;

  std::vector<std::string> light_leaks = {
      "tachyon.transition.lightleak.organic_blobs",
      "tachyon.transition.lightleak.lava_flow",
      "tachyon.transition.lightleak.liquid_fission",
      "tachyon.transition.lightleak.cosmic_swirl",
      "tachyon.transition.lightleak.cinematic_amber",
      "tachyon.transition.lightleak.procedural_remotion"};

  for (const auto &leak : light_leaks) {
    add_light_leak_composition(scene, leak);
  }

  return scene.build();
}
