#include "oglwrap_example.hpp"

#include <oglwrap/oglwrap.h>
#include "custom_shape.h"
#include <glm/gtc/matrix_transform.hpp>
#include <event_bus.h>

struct LeftClickEvent : public Event
{
    glm::vec2 wpos; 
};

struct RightClickEvent : public Event
{
    glm::vec2 wpos; 
};

struct RButtonEvent : public Event {};

class CustomExample : public OglwrapExample {
    private:
        Curve curve;
        Mesh mesh;
        Curve center_line;
        bool rotate = false;
        glm::vec3 camPos = {0, 1, 0};
        glm::vec3 lookPos = {0, 0, 0};

        // A shader program
        gl::Program prog_;
        
        struct PlacePointHandler : public EventHandler
        {
            Curve& curve;

            PlacePointHandler (Curve& curve_) : curve{curve_} {}
            virtual void Handle (std::shared_ptr<Event> e) override
            {
                auto ecast = std::static_pointer_cast<LeftClickEvent>(e);
                auto wpos = ecast->wpos;
                curve.AddPoint(glm::vec3(ecast->wpos, 0));
            }
        };
        std::shared_ptr<PlacePointHandler> m_place_point_handler;

        struct RotateHandler : public EventHandler
        {
            Curve& curve;
            Mesh& mesh;

            RotateHandler (Curve& curve_, Mesh& mesh_) : curve{curve_}, mesh{mesh_} {}
            virtual void Handle (std::shared_ptr<Event> e) override 
            {
                if(!mesh.GetPositions().empty())
                    mesh.Set({}, {});

                // Rotate curve positions around Y axis.
                auto const& curve_pos = curve.GetPositions(); 
                int n_incs = 1000;
                double inc = 2 * M_PI / n_incs;
                std::vector<glm::vec3> mesh_pos(2 * n_incs * curve_pos.size());
                std::vector<unsigned> mesh_inds(6 * mesh_pos.size());
                auto pos_it = mesh_pos.begin();
                auto ind_it = mesh_inds.begin();
                for(int i = 0; i < n_incs; ++i)
                {
                    double t{inc * i};
                    for(int j = 0; j < curve_pos.size(); ++j)
                    {
                        auto& p = curve_pos[j];
                        double r = 1;
                        *pos_it++ = {p.x * r * cos(t), p.y, p.x * r * sin(t)};

                        *ind_it++ = j + i * curve_pos.size();
                        *ind_it++ = j + ((i + 1) % n_incs) * curve_pos.size();
                        *ind_it++ = (j + 1) % curve_pos.size() + i * curve_pos.size();

                        *ind_it++ = (j + 1) % curve_pos.size() + ((i + 1) % n_incs) * curve_pos.size();
                        *ind_it++ = (j + 1) % curve_pos.size() + i * curve_pos.size();
                        *ind_it++ = j + ((i + 1) % n_incs) * curve_pos.size();

                    }
                }

                // Set normals.
                auto norm_it = pos_it;
                ind_it = mesh_inds.begin();
                for(int i = 0; i < n_incs; ++i)
                {
                    for(int j = 0; j < curve_pos.size(); ++j)
                    {
                        auto p1 = mesh_pos[*ind_it++];
                        auto p2 = mesh_pos[*ind_it++];
                        auto p3 = mesh_pos[*ind_it++];
                        ind_it += 3;

                        auto v1 = p2 - p1;
                        auto v2 = p3 - p1;
                        *norm_it++ = glm::normalize(glm::cross(v1, v2));

                    }
                }
                mesh.Set(std::move(mesh_pos), std::move(mesh_inds));
            }
        };
        std::shared_ptr<RotateHandler> m_rotate_handler;

    public:
        CustomExample ()
            : curve(),
              rotate{false},
              center_line(),
              m_place_point_handler{new PlacePointHandler(curve)},
              m_rotate_handler{new RotateHandler(curve, mesh)}
            {
                EventBus::Subscribe(EventBus::GetID<LeftClickEvent>(), m_place_point_handler);
                EventBus::Subscribe(EventBus::GetID<RightClickEvent>(), m_place_point_handler);
                EventBus::Subscribe(EventBus::GetID<RButtonEvent>(), m_rotate_handler);

                center_line.SetPositions({{0,0,0}, {0, 5, 0}});

                // We need to add a few more lines to the shaders
                gl::ShaderSource vs_source;
                vs_source.set_source(R"""(
      #version 330 core
      in vec3 inPos;
      in vec3 inNorm;
      out vec3 normal;

      uniform mat4 mvp;

      void main() {
        gl_Position = mvp * vec4(inPos, 1.0);
        normal = inNorm;
      })""");
                vs_source.set_source_file("example_shader.vert");
                gl::Shader vs(gl::kVertexShader, vs_source);

                gl::ShaderSource fs_source;
                fs_source.set_source(R"""(
      #version 330 core

      out vec4 fragColor;
      in vec3 normal;

      void main() {
        vec3 lightPos = normalize(vec3(0.3, 1, 0.2));
        float diffuseLighting = max(dot(lightPos, normalize(normal)), 0.0);
        vec3 color = vec3(1.0, 0.0, 0.0);
        fragColor = vec4(diffuseLighting * color, 1.0);
      })""");
                fs_source.set_source_file("example_shader.frag");
                gl::Shader fs(gl::kFragmentShader, fs_source);

                // Create a shader program
                prog_.attachShader(vs);
                prog_.attachShader(fs);
                prog_.link();
                gl::Use(prog_);

                // Bind the attribute locations
                (prog_ | "inPos").bindLocation(CustomShape::kPosition);
                (prog_ | "inNormal").bindLocation(CustomShape::kNormal);

                gl::Enable(gl::kDepthTest);

                // Set the clear color
                gl::ClearColor(0.1f, 0.2f, 0.3f, 1.0f);
            }

    protected:
        virtual void Render() override 
        {
            float t = glfwGetTime();
            camPos.x = sin(t);
            camPos.z = cos(t);
            glm::mat4 camera_mat = glm::lookAt(camPos, lookPos, glm::vec3{0.0f, 1.0f, 0.0f});
            glm::mat4 proj_mat = glm::perspectiveFov<float>(M_PI/3.0, kScreenWidth, kScreenHeight, 0.1, 100);
            if(!rotate)
                gl::Uniform<glm::mat4>(prog_, "mvp") = glm::mat4(1.0f);
            else
                gl::Uniform<glm::mat4>(prog_, "mvp") = proj_mat * camera_mat;
            HandleMouse();
            HandleKeys();
            curve.Render();
            mesh.Render();
            center_line.Render();
        }

        void HandleMouse()
        {
            // Get cursor position. 
            double xpos, ypos;
            glfwGetCursorPos(window_, &xpos, &ypos);

            // Convert xpos, ypos from screen coordinates to [-1,1] coordinates.
            glm::vec2 wpos = {(float)xpos, (float)ypos};
            wpos = glm::vec2(2) * wpos / glm::vec2(kScreenWidth, -kScreenHeight);
            wpos += glm::vec2(-1, 1);

            // Left mouse button down.
            if(glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
            {
                std::cout << "Left click event at: (" << wpos.x << ',' << wpos.y << ")" << std::endl;
                std::shared_ptr<LeftClickEvent> e{new LeftClickEvent};
                e->wpos = wpos;
                EventBus::Publish(e, EventBus::GetID<LeftClickEvent>());
            }

            // Right mouse button down.
            if(glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
            {
                std::cout << "Right click event at: (" << wpos.x << ',' << wpos.y << ")" << std::endl;
                std::shared_ptr<RightClickEvent> e{new RightClickEvent};
                e->wpos = wpos;
                EventBus::Publish(e, EventBus::GetID<RightClickEvent>());
            }
        }

        void HandleKeys()
        {
            if(glfwGetKey(window_, GLFW_KEY_P) == GLFW_PRESS)
                rotate = !rotate;

            if(glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS)
                camPos.y += 0.01;

            if(glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS)
                camPos.y -= 0.01;

            if(glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS)
                lookPos.y += 0.01;

            if(glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS)
                lookPos.y -= 0.01;

            if(glfwGetKey(window_, GLFW_KEY_R) == GLFW_PRESS)
            {
                std::cout << "R button press event" << std::endl;
                std::shared_ptr<RButtonEvent> e{new RButtonEvent};
                EventBus::Publish(e, EventBus::GetID<RButtonEvent>());
            }
        }
        
};

int main() {
    EventBus::CreateSingleton();
    TestWrapper wrapper;
    CustomExample().RunMainLoop();
}

