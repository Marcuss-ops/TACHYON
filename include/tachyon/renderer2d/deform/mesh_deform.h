#pragma once

#include "tachyon/core/math/algebra/vector2.h"
#include <vector>
#include <cstdint>
#include <optional>

#include "tachyon/render/render_intent.h"

namespace tachyon::renderer2d {

/**
 * @brief A control pin for mesh deformation (AE Puppet tool style).
 * 
 * Pins are placed on the mesh and can be animated to deform the mesh.
 * Each pin influences nearby vertices based on a falloff radius.
 */
struct MeshPin {
    std::size_t id;              ///< Unique identifier for the pin.
    math::Vector2 position;       ///< Current position of the pin (in layer space).
    math::Vector2 rest_position;  ///< Original position when pin was placed.
    float falloff_radius{100.0f}; ///< Radius of influence for this pin.
    float falloff_strength{1.0f};///< Strength of the pin's influence (0-1).
    
    MeshPin() = default;
    MeshPin(std::size_t id_, math::Vector2 pos) 
        : id(id_), position(pos), rest_position(pos) {}
};

/**
 * @brief A triangular mesh for 2D deformation.
 * 
 * The mesh is defined by vertices and triangles.
 * Vertices can be deformed by moving control pins.
 */
struct DeformMesh {
    struct Vertex {
        math::Vector2 position;      ///< Current position.
        math::Vector2 rest_position;  ///< Original position (for computing deltas).
        math::Vector2 uv;            ///< UV coordinates for texture sampling.
        std::vector<float> pin_weights; ///< Weights for each pin (cached for performance).
    };
    
    struct Triangle {
        std::uint32_t indices[3]; ///< Indices of the three vertices.
    };
    
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    std::vector<MeshPin> pins;
    
    /**
     * @brief Clear the mesh.
     */
    void clear() {
        vertices.clear();
        triangles.clear();
        pins.clear();
    }
    
    /**
     * @brief Get the number of vertices.
     */
    std::size_t vertex_count() const { return vertices.size(); }
    
    /**
     * @brief Get the number of triangles.
     */
    std::size_t triangle_count() const { return triangles.size(); }
    
    /**
     * @brief Get the number of pins.
     */
    std::size_t pin_count() const { return pins.size(); }
};

/**
 * @brief Generates a regular grid mesh over a rectangular region.
 * 
 * @param width Width of the region in pixels.
 * @param height Height of the region in pixels.
 * @param subdivisions_x Number of subdivisions along X (columns - 1).
 * @param subdivisions_y Number of subdivisions along Y (rows - 1).
 * @return A DeformMesh with triangles and UV coordinates.
 */
DeformMesh generate_grid_mesh(float width, float height, 
                              std::uint32_t subdivisions_x = 10, 
                              std::uint32_t subdivisions_y = 10);

/**
 * @brief Adds a pin to the mesh at the specified position.
 * 
 * Computes and caches weights for how much this pin influences each vertex.
 * 
 * @param mesh The mesh to add the pin to.
 * @param position Position in layer space to place the pin.
 * @return The ID of the newly created pin.
 */
std::size_t add_pin(DeformMesh& mesh, math::Vector2 position);

/**
 * @brief Updates pin weights for all vertices based on current pin positions.
 * 
 * Call this after adding pins or changing falloff parameters.
 * 
 * @param mesh The mesh to update.
 */
void update_pin_weights(DeformMesh& mesh);

/**
 * @brief Deforms the mesh based on pin movements.
 * 
 * For each vertex, computes the weighted average of pin displacements
 * based on the cached pin weights.
 * 
 * @param mesh The mesh to deform (vertex positions are updated in place).
 */
void deform_mesh(DeformMesh& mesh);

/**
 * @brief Resets mesh vertices to their rest positions.
 * 
 * @param mesh The mesh to reset.
 */
void reset_mesh_to_rest(DeformMesh& mesh);

/**
 * @brief Sets a pin's position and optionally updates mesh deformation.
 * 
 * @param mesh The mesh containing the pin.
 * @param pin_id The ID of the pin to move.
 * @param new_position New position for the pin.
 * @param auto_deform If true, automatically deforms the mesh after moving.
 * @return true if pin was found and moved.
 */
bool move_pin(DeformMesh& mesh, std::size_t pin_id, math::Vector2 new_position, bool auto_deform = true);

} // namespace tachyon::renderer2d
