#pragma once

#include "./custom_shape.h"
#include <algorithm>

inline CustomShape::CustomShape(const std::set<AttributeType>& attribs) {
    std::vector<glm::vec3> data;
    data.reserve(attribs.size()*36);
    void* offset{nullptr};

    Bind(vao_);
    Bind(buffer_);
    for (int i = 0; i < kAttribTypeNum; ++i) {
        AttributeType type = static_cast<AttributeType>(i);
        if (attribs.find(type) != attribs.end()) {
            createAttrib(&data, type);
            gl::VertexAttrib(i).pointer(
                    3, gl::DataType::kFloat, false, 0, offset).enable();
            offset = (void*)(data.size() * sizeof(glm::vec3));
        }
    }
    buffer_.data(data);
    Unbind(buffer_);
    Unbind(vao_);
}

inline void CustomShape::render() {
    Bind(vao_);
    gl::DrawArrays(gl::PrimType::kTriangles, 0, 36);
    Unbind(vao_);
}

inline void CustomShape::createAttrib(std::vector<glm::vec3>* data,
        AttributeType type) {
    switch (type) {
        case kPosition: return createPositions(data);
        case kNormal:   return createNormals(data);
        case kTexCoord: return createTexCoords(data);
    }
}

inline void CustomShape::createPositions(std::vector<glm::vec3>* data) {
    /*       (E)-----(A)
             /|      /|
             / |     / |
             (F)-----(B) |
             | (H)---|-(D)
             | /     | /
             |/      |/
             (G)-----(C)        */

    // Note: Positive Z is towards you!

    glm::vec3 A = {+0.0f, +0.5f, -0.0f};
    glm::vec3 B = {+0.5f, +0.5f, +0.5f};
    glm::vec3 C = {+0.5f, -0.5f, +0.5f};
    glm::vec3 D = {+0.5f, -0.5f, -0.5f};
    glm::vec3 E = {-0.5f, +0.5f, -0.5f};
    glm::vec3 F = {-0.5f, +0.5f, +0.5f};
    glm::vec3 G = {-0.5f, -0.5f, +0.5f};
    glm::vec3 H = {-0.5f, -0.5f, -0.5f};

    const glm::vec3 pos[] = {
        A, D, B,    C, B, D,
        A, B, E,    F, E, B,
        B, C, F,    G, F, C,
        F, G, E,    H, E, G,
        H, G, D,    C, D, G,
        E, H, A,    D, A, H
    };

    data->insert(data->end(), std::begin(pos), std::end(pos));
}

inline void CustomShape::createNormals(std::vector<glm::vec3>* data) {
    const glm::vec3 n[6] = {
        {+1,  0,  0},
        { 0, +1,  0},
        { 0,  0, +1},
        {-1,  0,  0},
        { 0, -1,  0},
        { 0,  0, -1}
    };

    for (int face = 0; face < 6; ++face) {
        for (int vertex = 0; vertex < 6; ++vertex) {
            data->push_back(n[face]);
        }
    }
};

inline void CustomShape::createTexCoords(std::vector<glm::vec3>* data) {
    const float n[6][2] = {
        {+1, +1},
        {+1,  0},
        { 0, +1},
        { 0,  0},
        { 0, +1},
        {+1,  0}
    };

    for (int face = 0; face < 6; ++face) {
        for (int vertex = 0; vertex < 6; ++vertex) {
            data->emplace_back(n[vertex][0], n[vertex][1], face);
        }
    }
}

Curve::Curve ()
{
    // Create positions vertex attribute pointer.
    UpdatePositions();
}

void Curve::UpdatePositions () 
{
    // Set indices data.
    Bind(m_vao);
    Bind(m_buffer);
    gl::VertexAttrib(0).pointer(3, gl::DataType::kFloat, false, 0, nullptr).enable();
    m_buffer.data(m_positions);
    Unbind(m_buffer);
    Unbind(m_vao);
}

void Curve::Render () 
{
    Bind(m_vao);
    gl::DrawArrays(gl::PrimType::kLineStrip, 0, m_positions.size());
    Unbind(m_vao);
}

Mesh::Mesh () 
{
    // Create positions vertex attribute pointer.
    UpdateVao();
}

void Mesh::UpdateVao () 
{
    // Set indices data.
    Bind(m_vao);
    Bind(m_buffer);
    Bind(m_ind_buffer);
    gl::VertexAttrib(0).pointer(3, gl::DataType::kFloat, false, 0, nullptr).enable();
    gl::VertexAttrib(1).pointer(3, gl::DataType::kFloat, false, 0, (void*)(m_positions.size() / 2 * sizeof(glm::vec3))).enable();
    m_buffer.data(m_positions);
    m_ind_buffer.data(m_indices);
    Unbind(m_ind_buffer);
    Unbind(m_buffer);
    Unbind(m_vao);
}

void Mesh::Render () 
{
    Bind(m_vao);
    Bind(m_ind_buffer);
    gl::DrawElements(gl::PrimType::kTriangles, m_indices.size(), gl::IndexType::kUnsignedInt);
    Unbind(m_ind_buffer);
    Unbind(m_vao);
}
