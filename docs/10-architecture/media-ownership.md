# Media Ownership Rules

## Media ownership

- **AssetManager** owns asset identity and metadata only.
- **PathResolver** owns filesystem/proxy path resolution.
- **MediaManager** owns runtime loading and media caches.
- **ImageManager** is an internal image decode/cache service, not an asset catalog.

## Path ownership

- Only PathResolver may convert asset references into filesystem paths.
- ProxyManifest only maps original paths to proxy paths.
- Renderers must not resolve filesystem paths directly.

## Color ownership

- `tachyon::color` owns canonical color math.
- Renderers may optimize color operations but must not define divergent color semantics.
- Output/HDR/EXR paths must use `tachyon::color`.

## AssetManager boundaries

AssetManager must NOT:
- Decode media (no `load_image()`, `decode_video()`, `get_frame()`)
- Own runtime frame/image/video caches
- Resolve filesystem paths directly

AssetManager may:
- `resolve()` - return metadata/path info
- `metadata()` - query asset properties
- `register_asset()` - catalog new assets

## Runtime flow

```
LayerSpec.asset_id
      ↓
AssetManager / AssetResolver
      ↓
ResolvedAsset.runtime_path
      ↓
MediaManager
      ↓
ImageManager / VideoDecoderPool / Audio decoder
```
