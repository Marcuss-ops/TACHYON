# TachyonRenderer2D raster sources
set(TachyonRenderer2DRasterSources
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/cpu_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/draw_primitives.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/draw_list_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/draw_list_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/mask_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/mesh_deform_apply.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/mesh_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/fill_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/mask_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/path_flattener.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/path_trimmer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/path/stroke_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/perspective_rasterizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/raster/sdf_rasterizer.cpp
)
