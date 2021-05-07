#pragma once

#include <set>
#include <vector>
#include <oglwrap/buffer.h>
#include <oglwrap/context.h>
#include <oglwrap/vertex_array.h>
#include <oglwrap/vertex_attrib.h>

class CustomShape {
public:
    enum AttributeType {kPosition, kNormal, kTexCoord};

    /// Creates the attribute datas for the cube, that are requested in the constructor argument.
    explicit CustomShape(const std::set<AttributeType>& attribs = {kPosition});

    /// Renders the cube.
    /** This call changes the currently active VAO. */
    void render();

    /// Returns the face winding of the cube created by this class.
    gl::FaceOrientation faceWinding() const { return gl::FaceOrientation::kCw; }

private:
    gl::VertexArray vao_;
    gl::ArrayBuffer buffer_;
    static const int kAttribTypeNum = 3;

    static void createAttrib(std::vector<glm::vec3>* data, AttributeType type);
    static void createPositions(std::vector<glm::vec3>* data);
    static void createNormals(std::vector<glm::vec3>* data);
    static void createTexCoords(std::vector<glm::vec3>* data);
};

/// 2 or 3 D curve
class Curve 
{
private:
    std::vector<glm::vec3> m_positions;
    gl::VertexArray m_vao;
    gl::ArrayBuffer m_buffer;

    void UpdatePositions ();

public:
    Curve (); 

    /// Render the curve.
    void Render();

    /// Extend the curve by adding a new point.
    void AddPoint(glm::vec3 const& point) 
    {
        m_positions.push_back(point);
        UpdatePositions();
    }

    /// Set positions of vertices in curve.
    void SetPositions(std::vector<glm::vec3>&& positions)
    {
        m_positions = std::move(positions);
        UpdatePositions();
    }

    std::vector<glm::vec3> GetPositions () const {return m_positions;} 
};

class Mesh 
{
private:
    std::vector<glm::vec3> m_positions;
    std::vector<unsigned> m_indices;
    gl::VertexArray m_vao;
    gl::ArrayBuffer m_buffer;
    gl::IndexBuffer m_ind_buffer;

    void UpdateVao ();

public:
    Mesh ();

    /// Render the mesh.
    void Render();

    /// Set positions.
    void Set (std::vector<glm::vec3>&& positions, std::vector<unsigned>&& indices)
    {
        m_positions = std::move(positions);
        m_indices = std::move(indices);
        UpdateVao ();
    }

    std::vector<glm::vec3> GetPositions () const {return m_positions;} 
};

#include "custom_shape-inl.h"
