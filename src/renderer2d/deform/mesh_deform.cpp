#include "tachyon/renderer2d/deform/mesh_deform.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace tachyon::renderer2d {

static float inverse_distance_weight(float distance, float falloff_radius) {
    if (distance >= falloff_radius || falloff_radius <= 0.0f) {
        return 0.0f;
    }
    if (distance <= 0.0f) {
        return 1.0f;
    }
    // Smooth falloff using cosine curve: (1 + cos(pi * d/r)) / 2
    float t = std::clamp(distance / falloff_radius, 0.0f, 1.0f);
    return (1.0f + std::cos(3.14159265f * t)) * 0.5f;
}

DeformMesh generate_grid_mesh(float width, float height, 
                              std::uint32_t subdivisions_x, 
                              std::uint32_t subdivisions_y) {
    DeformMesh mesh;
    
    // Clamp subdivisions to reasonable values
    subdivisions_x = std::max(1u, std::min(subdivisions_x, 100u));
    subdivisions_y = std::max(1u, std::min(subdivisions_y, 100u));
    
    const float step_x = width / static_cast<float>(subdivisions_x);
    const float step_y = height / static_cast<float>(subdivisions_y);
    
    // Create vertices
    for (std::uint32_t y = 0; y <= subdivisions_y; ++y) {
        for (std::uint32_t x = 0; x <= subdivisions_x; ++x) {
            DeformMesh::Vertex v;
            v.position = math::Vector2(x * step_x, y * step_y);
            v.rest_position = v.position;
            v.uv = math::Vector2(
                static_cast<float>(x) / static_cast<float>(subdivisions_x),
                static_cast<float>(y) / static_cast<float>(subdivisions_y)
            );
            mesh.vertices.push_back(v);
        }
    }
    
    // Create triangles (two per quad)
    for (std::uint32_t y = 0; y < subdivisions_y; ++y) {
        for (std::uint32_t x = 0; x < subdivisions_x; ++x) {
            std::uint32_t tl = y * (subdivisions_x + 1) + x;
            std::uint32_t tr = tl + 1;
            std::uint32_t bl = tl + (subdivisions_x + 1);
            std::uint32_t br = bl + 1;
            
            // First triangle (TL, TR, BL)
            DeformMesh::Triangle t1;
            t1.indices[0] = tl;
            t1.indices[1] = tr;
            t1.indices[2] = bl;
            mesh.triangles.push_back(t1);
            
            // Second triangle (TR, BR, BL)
            DeformMesh::Triangle t2;
            t2.indices[0] = tr;
            t2.indices[1] = br;
            t2.indices[2] = bl;
            mesh.triangles.push_back(t2);
        }
    }
    
    return mesh;
}

std::size_t add_pin(DeformMesh& mesh, math::Vector2 position) {
    MeshPin pin(mesh.pins.size(), position);
    
    // Initialize pin weights for all vertices
    for (auto& vertex : mesh.vertices) {
        vertex.pin_weights.push_back(0.0f);
    }
    
    mesh.pins.push_back(pin);
    
    // Update weights for the new pin
    update_pin_weights(mesh);
    
    return pin.id;
}

void update_pin_weights(DeformMesh& mesh) {
    const std::size_t vertex_count = mesh.vertices.size();
    const std::size_t pin_count = mesh.pins.size();
    
    // Reset all weights
    for (auto& vertex : mesh.vertices) {
        vertex.pin_weights.assign(pin_count, 0.0f);
    }
    
    // Compute weights for each pin
    for (std::size_t p = 0; p < pin_count; ++p) {
        const auto& pin = mesh.pins[p];
        
        for (std::size_t v = 0; v < vertex_count; ++v) {
            const auto& vertex = mesh.vertices[v];
            float dist = (vertex.rest_position - pin.rest_position).length();
            float weight = inverse_distance_weight(dist, pin.falloff_radius) * pin.falloff_strength;
            mesh.vertices[v].pin_weights[p] = weight;
        }
    }
    
    // Normalize weights for each vertex so they sum to 1.0 (or 0 if no influence)
    for (std::size_t v = 0; v < vertex_count; ++v) {
        float total_weight = 0.0f;
        for (std::size_t p = 0; p < pin_count; ++p) {
            total_weight += mesh.vertices[v].pin_weights[p];
        }
        
        if (total_weight > 0.0f) {
            for (std::size_t p = 0; p < pin_count; ++p) {
                mesh.vertices[v].pin_weights[p] /= total_weight;
            }
        }
    }
}

void deform_mesh(DeformMesh& mesh) {
    const std::size_t pin_count = mesh.pins.size();
    if (pin_count == 0) return;
    
    for (auto& vertex : mesh.vertices) {
        math::Vector2 new_pos(0.0f, 0.0f);
        
        for (std::size_t p = 0; p < pin_count; ++p) {
            const auto& pin = mesh.pins[p];
            float weight = vertex.pin_weights[p];
            
            if (weight > 0.0f) {
                math::Vector2 delta = pin.position - pin.rest_position;
                new_pos += delta * weight;
            }
        }
        
        vertex.position = vertex.rest_position + new_pos;
    }
}

void reset_mesh_to_rest(DeformMesh& mesh) {
    for (auto& vertex : mesh.vertices) {
        vertex.position = vertex.rest_position;
    }
}

bool move_pin(DeformMesh& mesh, std::size_t pin_id, math::Vector2 new_position, bool auto_deform) {
    for (auto& pin : mesh.pins) {
        if (pin.id == pin_id) {
            pin.position = new_position;
            if (auto_deform) {
                deform_mesh(mesh);
            }
            return true;
        }
    }
    return false;
}

} // namespace tachyon::renderer2d
