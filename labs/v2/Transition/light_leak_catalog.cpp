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
    comp.size(1920, 1080).fps(30).duration(4.0);

    comp.layer("scena_a", [](LayerBuilder &l) {
      l.solid("fill").color({230, 230, 230, 255}).in(0.0).out(2.5);
    });

    // 2. Scene B (Middle): Dark Brown
    comp.layer("scena_b", [id](LayerBuilder &l) {
      l.solid("fill").color({230, 230, 230, 255}).in(1.5).out(4.0);

      l.enter().id(id).duration(1.0);
    });

    // Title text
    comp.layer("title", [name](LayerBuilder &l) {
      l.text(name)
          .font("SFPro", 48)
          .color({255, 255, 255, 255})
          .done()
          .position(100, 650)
          .in(0.0)
          .out(5.0);
    });
  });
}

extern "C" __declspec(dllexport) SceneSpec build_scene() {
  SceneBuilder scene;

  std::vector<std::string> light_leaks = {
      "tachyon.transition.light_leak",
      "tachyon.transition.light_leak_solar",
      "tachyon.transition.light_leak_nebula",
      "tachyon.transition.light_leak_sunset",
      "tachyon.transition.light_leak_ghost",
      "tachyon.transition.film_burn",
      "tachyon.transition.lightleak.soft_warm_edge",
      "tachyon.transition.lightleak.golden_sweep",
      "tachyon.transition.lightleak.creamy_white",
      "tachyon.transition.lightleak.dusty_archive",
      "tachyon.transition.lightleak.lens_flare_pass",
      "tachyon.transition.lightleak.amber_sweep",
      "tachyon.transition.lightleak.neon_pulse",
      "tachyon.transition.lightleak.prism_shatter",
      "tachyon.transition.lightleak.vintage_sepia",
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
