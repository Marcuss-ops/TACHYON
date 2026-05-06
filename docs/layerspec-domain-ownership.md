# LayerSpec Domain Ownership

`LayerSpec` remains a compatibility contract, but new domain-specific fields must not be added casually. The rule is:

- core fields stay in `LayerSpec`
- domain-specific data moves toward a normalized payload or dedicated nested spec
- legacy fields remain parse-time only and should not be read by new runtime code

The audit script treats the fields below as the current non-core ownership map. Any new field that is not a core field and is not listed here must be added to this document before it is accepted.

## Text

| Field | Owner | Destination |
| --- | --- | --- |
| `text_content` | Text | `TextLayerSpec.text` |
| `font_id` | Text | `TextLayerSpec.font` |
| `font_size` | Text | `TextLayerSpec.font_size` |
| `alignment` | Text | `TextLayerSpec.layout` |
| `font_axes` | Text | `TextLayerSpec.font_axes` |
| `fill_color` | Text | `TextLayerSpec.fill` |
| `stroke_color` | Text | `TextLayerSpec.stroke` |
| `stroke_width` | Text | `TextLayerSpec.stroke` |
| `stroke_width_property` | Text | `TextLayerSpec.stroke` |
| `metallic` | Text / 3D material | `MeshLayerSpec.material` |
| `roughness` | Text / 3D material | `MeshLayerSpec.material` |
| `ior` | Text / 3D material | `MeshLayerSpec.material` |
| `transmission` | Text / 3D material | `MeshLayerSpec.material` |
| `emission_strength` | Text / 3D material | `MeshLayerSpec.material` |
| `emission_color` | Text / 3D material | `MeshLayerSpec.material` |

## Subtitles

| Field | Owner | Destination |
| --- | --- | --- |
| `subtitle_path` | Subtitle | `SubtitleLayerSpec.source` |
| `subtitle_outline_color` | Subtitle | `SubtitleLayerSpec.style` |
| `subtitle_outline_width` | Subtitle | `SubtitleLayerSpec.style` |
| `word_timestamp_path` | Subtitle / highlight | `SubtitleLayerSpec.word_timings` |

## Shape

| Field | Owner | Destination |
| --- | --- | --- |
| `shape_path` | Shape | `ShapeLayerSpec.path` |
| `shape_spec` | Shape | `ShapeLayerSpec.shape` |
| `line_cap` | Shape | `ShapeLayerSpec.stroke` |
| `line_join` | Shape | `ShapeLayerSpec.stroke` |
| `miter_limit` | Shape | `ShapeLayerSpec.stroke` |

## Camera

| Field | Owner | Destination |
| --- | --- | --- |
| `camera_type` | Camera | `CameraLayerSpec.mode` |
| `camera_zoom` | Camera | `CameraLayerSpec.lens` |
| `camera_poi` | Camera | `CameraLayerSpec.target` |
| `camera_aperture` | Camera | `CameraLayerSpec.lens` |
| `camera_focus_distance` | Camera | `CameraLayerSpec.lens` |
| `has_parallax` | Camera 2D | `Camera2DLayerSpec.parallax` |
| `parallax_factor` | Camera 2D | `Camera2DLayerSpec.parallax` |
| `camera2d_id` | Camera 2D | `Camera2DLayerSpec.binding` |
| `camera_shake_seed` | Camera shake | `CameraShakeSpec.seed` |
| `camera_shake_amplitude_pos` | Camera shake | `CameraShakeSpec.motion` |
| `camera_shake_amplitude_rot` | Camera shake | `CameraShakeSpec.motion` |
| `camera_shake_frequency` | Camera shake | `CameraShakeSpec.motion` |
| `camera_shake_roughness` | Camera shake | `CameraShakeSpec.motion` |

## Light

| Field | Owner | Destination |
| --- | --- | --- |
| `light_type` | Light | `LightLayerSpec.kind` |
| `light_color` | Light | `LightLayerSpec.emission` |
| `light_intensity` | Light | `LightLayerSpec.emission` |
| `attenuation_near` | Light | `LightLayerSpec.falloff` |
| `attenuation_far` | Light | `LightLayerSpec.falloff` |
| `cone_angle` | Light | `LightLayerSpec.spot` |
| `cone_feather` | Light | `LightLayerSpec.spot` |
| `falloff_type` | Light | `LightLayerSpec.falloff` |
| `casts_shadows` | Light | `LightLayerSpec.shadow` |
| `shadow_darkness` | Light | `LightLayerSpec.shadow` |
| `shadow_radius` | Light | `LightLayerSpec.shadow` |

## Repeater

| Field | Owner | Destination |
| --- | --- | --- |
| `repeater_type` | Repeater | `RepeaterSpec.kind` |
| `repeater_count` | Repeater | `RepeaterSpec.count` |
| `repeater_stagger_delay` | Repeater | `RepeaterSpec.timing` |
| `repeater_offset_position_x` | Repeater | `RepeaterSpec.transform` |
| `repeater_offset_position_y` | Repeater | `RepeaterSpec.transform` |
| `repeater_offset_rotation` | Repeater | `RepeaterSpec.transform` |
| `repeater_offset_scale_x` | Repeater | `RepeaterSpec.transform` |
| `repeater_offset_scale_y` | Repeater | `RepeaterSpec.transform` |
| `repeater_start_opacity` | Repeater | `RepeaterSpec.opacity` |
| `repeater_end_opacity` | Repeater | `RepeaterSpec.opacity` |
| `repeater_grid_cols` | Repeater | `RepeaterSpec.layout` |
| `repeater_radial_radius` | Repeater | `RepeaterSpec.layout` |
| `repeater_radial_start_angle` | Repeater | `RepeaterSpec.layout` |
| `repeater_radial_end_angle` | Repeater | `RepeaterSpec.layout` |

## Effects, Masks, Tracking

| Field | Owner | Destination |
| --- | --- | --- |
| `effects` | Effects | `EffectSpec[]` |
| `animated_effects` | Effects | `AnimatedEffectSpec[]` |
| `text_animators` | Text animation | `TextAnimatorSpec[]` |
| `text_highlights` | Text animation | `TextHighlightSpec[]` |
| `mask_paths` | Masks | `MaskSpec[]` |
| `track_bindings` | Tracking | `TrackBinding[]` |
| `time_remap` | Temporal | `TimeRemapCurve` |
| `frame_blend` | Temporal | `FrameBlendMode` |
| `markers` | Metadata | `MarkerSpec[]` |

## Animation presets

| Field | Owner | Destination |
| --- | --- | --- |
| `in_preset` | Legacy animation | remove |
| `during_preset` | Legacy animation | remove |
| `out_preset` | Legacy animation | remove |
| `in_duration` | Legacy animation | remove |
| `out_duration` | Legacy animation | remove |
| `animation_in_preset` | Animation presets | `AnimationPresetSpec` |
| `animation_during_preset` | Animation presets | `AnimationPresetSpec` |
| `animation_out_preset` | Animation presets | `AnimationPresetSpec` |
| `animation_in_duration` | Animation presets | `AnimationPresetSpec` |
| `animation_out_duration` | Animation presets | `AnimationPresetSpec` |

## Transitions

| Field | Owner | Destination |
| --- | --- | --- |
| `transition_in` | Transition | `LayerTransitionSpec` |
| `transition_out` | Transition | `LayerTransitionSpec` |

## Procedural / 3D

| Field | Owner | Destination |
| --- | --- | --- |
| `procedural` | Procedural | `ProceduralLayerSpec` |
| `particle_spec` | Procedural | `ProceduralLayerSpec` |
| `extrude` | 3D text | `MeshLayerSpec.geometry` |
| `bevel` | 3D text | `MeshLayerSpec.geometry` |
| `extrusion_depth` | 3D text | `MeshLayerSpec.geometry` |
| `bevel_size` | 3D text | `MeshLayerSpec.geometry` |
| `hole_bevel_ratio` | 3D text | `MeshLayerSpec.geometry` |
| `three_d` | 3D bridge | `ThreeDSpec` |

## Canonical core fields

The following fields are intentionally treated as core and are not part of the domain audit list:

- `id`
- `name`
- `type`
- `type_string`
- `asset_id`
- `preset_id`
- `blend_mode`
- `enabled`
- `visible`
- `is_3d`
- `is_adjustment_layer`
- `motion_blur`
- `start_time`
- `in_point`
- `out_point`
- `opacity`
- `width`
- `height`
- `transform`
- `transform3d`
- `opacity_property`
- `mask_feather`
- `time_remap_property`
- `parent`
- `track_matte_layer_id`
- `track_matte_type`
- `precomp_id`
- `duration`
- `loop`
- `hold_last_frame`

If a new field is added and it is not obviously one of the canonical core fields above, it must be added to one of the tables above and the runtime owner must be chosen explicitly.
