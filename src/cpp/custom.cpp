#include "oglwrap_example.hpp"
#include <lodepng.h>

#include <oglwrap/oglwrap.h>
#include "custom_shape.h"
#include <glm/gtc/matrix_transform.hpp>
#include <event_bus.h>
#include <glm/gtx/norm.hpp>

struct LeftClickEvent : public Event
{
    glm::vec2 wpos; 
};

struct RightClickEvent : public Event
{
    glm::vec2 wpos; 
};

struct RButtonEvent : public Event {};
struct PButtonEvent : public Event {};
struct KButtonEvent : public Event {};

std::pair<float, glm::vec2> minimum_distance(glm::vec2 v, glm::vec2 w, glm::vec2 p) 
{
  // Consider the line extending the segment, parameterized as v + t (w - v).
  // We find projection of point p onto the line. 
  // It falls where t = [(p-v) . (w-v)] / |w-v|^2
  // We clamp t from [0,1] to handle points outside the segment vw.
  const float l2 = glm::distance2(v, w);  // i.e. |w-v|^2 -  avoid a sqrt
  const float t = std::max(0.0f, std::min(1.0f, glm::dot(p - v, w - v) / l2));
  const glm::vec2 projection = v + t * (w - v);  // Projection falls on the segment
  return std::make_pair(glm::distance(p, projection), projection);
}

std::pair<float, glm::vec2> minimum_distance(std::vector<glm::vec3> curve_2d, glm::vec2 p)
{
    float min_dist = FLT_MAX;
    std::pair<float, glm::vec2> res;
    for(int i = 0; i < curve_2d.size() - 1; ++i)
    {
        glm::vec2 v(curve_2d[i]); 
        glm::vec2 w(curve_2d[i+1]); 
        auto pr = minimum_distance(v, w, p);
        if(pr.first < min_dist)
		{
            res = pr;
			min_dist = pr.first;
		}
    }
    return res;
}


class CustomExample : public OglwrapExample {
    private:
        Curve curve;
        Curve axis;
        Mesh mesh;
        Curve center_line;
        bool rotate = false;
        bool mode = true; // true = draw, false = set axis.
        float camAng = 0;
        glm::vec3 camPos = {1, 1, 0};
        glm::vec3 lookPos = {0, 0, 0};

        // A 2d texture.
        gl::Texture2D tex_;
        

        // A shader program
        gl::Program prog_;
        
        struct PlacePointHandler : public EventHandler
        {
            Curve& curve;
            Curve& axis;
            bool& mode;

            PlacePointHandler (Curve& curve_, Curve& axis_, bool& mode_) : curve{curve_}, axis{axis_}, mode{mode_} {}
            virtual void Handle (std::shared_ptr<Event> e) override
            {
                auto ecast = std::static_pointer_cast<LeftClickEvent>(e);
                auto wpos = ecast->wpos;
                if(mode)
                    curve.AddPoint(glm::vec3(ecast->wpos, 0));
                else
                    axis.AddPoint(glm::vec3(ecast->wpos, 0));
            }
        };
        std::shared_ptr<PlacePointHandler> m_place_point_handler;

        friend class ViewHandler;
        struct ViewHandler : public EventHandler
        {
            CustomExample& example;
            ViewHandler(CustomExample& example_) : example{example_} {}
            virtual void Handle (std::shared_ptr<Event>) override
            {
                example.rotate = !example.rotate;
            }
        };
        std::shared_ptr<ViewHandler> m_view_handler;

        friend class ModeHandler;
        struct ModeHandler : public EventHandler
        {
            CustomExample& example;
            ModeHandler(CustomExample& example_) : example{example_} {}
            virtual void Handle (std::shared_ptr<Event>) override
            {
                example.mode = !example.mode;
            }
        };
        std::shared_ptr<ModeHandler> m_mode_handler;

        struct RotateHandler : public EventHandler
        {
            Curve& curve;
            Curve& axis;
            Mesh& mesh;

            RotateHandler (Curve& curve_, Curve& axis_, Mesh& mesh_) : curve{curve_}, axis{axis_}, mesh{mesh_} {}
            virtual void Handle (std::shared_ptr<Event> e) override 
            {
                if(!mesh.GetPositions().empty())
                    mesh.Set({}, {});

                // Rotate curve positions around Y axis.
                auto const& curve_pos = curve.GetPositions(); 
                auto const& axis_pos = axis.GetPositions(); 
                int n_incs = 100;
                double inc = 2 * M_PI / n_incs;
                std::vector<glm::vec3> mesh_pos(3 * n_incs * curve_pos.size());
                std::vector<unsigned> mesh_inds(6 * n_incs * curve_pos.size());
                auto pos_it = mesh_pos.begin();
                auto norm_it = mesh_pos.begin() + mesh_pos.size() / 3;
                auto uv_it = mesh_pos.begin() + 2 * mesh_pos.size() / 3;
                auto ind_it = mesh_inds.begin();
                for(int j = 0; j < curve_pos.size(); ++j)
                {
                    auto const& p = curve_pos[j];
                    auto const& p_next = curve_pos[(j + 1) % curve_pos.size()];
                    auto const& p_prev = curve_pos[(j - 1) % curve_pos.size()];

                    // Get normal vector to p - p_next and p_prev - p by rotating by 90 degrees (x, y) -> (-y, x).
                    auto nm_next = glm::vec2(p_next.y - p.y, p.x - p_next.x);
                    auto nm_prev = glm::vec2(p.y - p_prev.y, p_prev.x - p.x);

                    // Average normals to get this point's normal.
                    auto nm = 0.5f * (nm_next + nm_prev);

                    // Get vector v = p - proj where proj is the projection of p onto axis.
                    auto pr = minimum_distance(axis_pos, glm::vec2(p.x, p.y));
                    float dist = pr.first;
                    glm::vec3 proj = glm::vec3(pr.second, 0);
                    glm::vec3 v = p - proj;

                    // Get angle between y axis and v.
                    float ang = -std::atan2(v.x, v.y) + M_PI_2;
                    
                    for(int i = 0; i < n_incs; ++i)
                    {
                        double t{inc * i};
                        double r = 3 + (1/4.0) * std::tanh(4 * sin(12 * t));
//                        double r = 1;

                        // First rotate around y, then rotate by ang, then add proj back.
                        glm::vec3 rot = glm::vec3(r * cos(t), 0, r * sin(t)) * dist;
                        rot = {rot.x * cos(ang) - rot.y * sin(ang), 
                               rot.x * sin(ang) + rot.y * cos(ang),
                               rot.z};
                        *pos_it++ = rot + proj;

                        // Set normal by applying rotation to 2d normal.
                        *norm_it++ = glm::normalize(glm::vec3{nm.x * r * cos(t), nm.y, nm.x * r * sin(t)});

                        *uv_it++ = glm::vec3(5 + 10 * i / float(n_incs), 5 + 10 * j / float(curve_pos.size() - 1), 0);

                        #define T2to1(i, j) ((j) % curve_pos.size()) * n_incs + (i) % n_incs
                        *ind_it++ = T2to1(i, j);
                        *ind_it++ = T2to1(i+1, j);
                        *ind_it++ = T2to1(i, j+1);

                        *ind_it++ = T2to1(i+1, j+1);
                        *ind_it++ = T2to1(i, j+1);
                        *ind_it++ = T2to1(i+1, j);
                        #undef T2to1
                    }
                }
                mesh.Set(std::move(mesh_pos), std::move(mesh_inds));
            }
        };
        std::shared_ptr<RotateHandler> m_rotate_handler;

    public:
        CustomExample ()
            : curve(), axis(),
              rotate{false},
              center_line(),
              m_place_point_handler{new PlacePointHandler(curve, axis, mode)},
              m_view_handler{new ViewHandler(*this)},
              m_mode_handler{new ModeHandler(*this)},
              m_rotate_handler{new RotateHandler(curve, axis, mesh)}
            {
//                for(int i = 0; i < 100; ++i)
//                {
//                    float t = M_PI * 2 * i / 100.0;
////                    axis.AddPoint(glm::vec3{cos(t), sin(t), 0} * 0.1f);
//                    curve.AddPoint(glm::vec3{cos(t), sin(t), 0} * 0.5f);
//                }
//                axis.AddPoint({0,-0.1,0});
//                axis.AddPoint({0,0.1,0});
//                axis.AddPoint({-0.1,0,0});
//                axis.AddPoint({0.1,0,0});
                EventBus::Subscribe(EventBus::GetID<LeftClickEvent>(), m_place_point_handler);
                EventBus::Subscribe(EventBus::GetID<RightClickEvent>(), m_place_point_handler);
                EventBus::Subscribe(EventBus::GetID<RButtonEvent>(), m_rotate_handler);
                EventBus::Subscribe(EventBus::GetID<PButtonEvent>(), m_view_handler);
                EventBus::Subscribe(EventBus::GetID<KButtonEvent>(), m_mode_handler);

//                center_line.SetPositions({{0,0,0}, {0, 5, 0}});

                // We need to add a few more lines to the shaders
                gl::ShaderSource vs_source;
                vs_source.set_source(R"""(
      #version 330 core
      in vec3 inPos;
      in vec3 inNorm;
      in vec3 inUV;
      out vec3 normal;
      out vec3 position;
      out vec2 uv;

      uniform mat4 mvp;

      void main() {
        gl_Position = mvp * vec4(inPos, 1.0);
        position = vec3(gl_Position);
        normal = inNorm;
        uv = vec2(inUV);
      })""");
                vs_source.set_source_file("example_shader.vert");
                gl::Shader vs(gl::kVertexShader, vs_source);

                gl::ShaderSource fs_source;
                fs_source.set_source(R"""(
      #version 330 core

      out vec4 fragColor;
      in vec3 normal;
      in vec3 position;
      in vec2 uv;
      uniform vec3 lightPos1;
      uniform vec3 lightPos2;
      uniform sampler2D tex;

      void main() {
        vec3 lightVec1 = normalize(lightPos1 - position);
        vec3 lightVec2 = normalize(lightPos2 - position);
        float d1 = abs(dot(lightVec1, normal)); // Note the abs means normal sign does not matter.
        float d2 = abs(dot(lightVec2, normal)); 
        vec3 c1 = vec3(1);
        vec3 c2 = vec3(1);
        vec3 tex_col = vec3(texture(tex, uv));
		vec3 ambient = 0.0 * vec3(1.0);
        fragColor = vec4(abs(normal) * (ambient + d1 * c1 + d2 * c2), 1.0);
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

                glfwSetKeyCallback(window_, KeyGLFWCallback);

                // Setup texture.
                {
                    gl::Bind(tex_);
                    unsigned width, height;
                    std::vector<unsigned char> data;
                    std::string path = "../sand.png";
                    unsigned error = lodepng::decode(data, width, height, path, LCT_RGBA, 8);
                    if (error) {
                        std::cerr << "Image decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
                        std::terminate();
                    }
                    tex_.upload(gl::kSrgb8Alpha8, width, height,
                            gl::kRgba, gl::kUnsignedByte, data.data());
                    tex_.generateMipmap();
                    tex_.minFilter(gl::kLinearMipmapLinear);
                    tex_.magFilter(gl::kLinear);
                    tex_.wrapS(gl::kRepeat);
                    tex_.wrapT(gl::kRepeat);
                }
            }

    protected:
        virtual void Render() override 
        {
            float t = glfwGetTime();
            glm::mat4 camera_mat = glm::lookAt(camPos, lookPos, glm::vec3{0.0f, 1.0f, 0.0f});
            glm::mat4 model_mat = glm::rotate(glm::mat4(1.0f), 0 * glm::radians(t) * 100, glm::vec3(0,1,0));
            glm::mat4 proj_mat = glm::perspectiveFov<float>(M_PI/3.0, kScreenWidth, kScreenHeight, 0.1, 100);
            if(!rotate)
                gl::Uniform<glm::mat4>(prog_, "mvp") = glm::mat4(1.0f);
            else
                gl::Uniform<glm::mat4>(prog_, "mvp") = proj_mat * camera_mat * model_mat;

            glm::vec3 lightPos1 = {1, 1, 0};
            glm::vec3 lightPos2 = {0, sin(t / 3 * 2 * M_PI) - 1, 0};
            gl::Uniform<glm::vec3>(prog_, "lightPos1") = lightPos1;
            gl::Uniform<glm::vec3>(prog_, "lightPos2") = lightPos2;
            HandleMouse();
            HandleKeys();
            curve.Render();
            axis.Render();
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
            if(glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS)
                camPos.y += 0.05;

            if(glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS)
                camPos.y -= 0.05;

            auto gA = glfwGetKey(window_, GLFW_KEY_A);
            auto gD = glfwGetKey(window_, GLFW_KEY_D);
            if(gA == GLFW_PRESS || gD == GLFW_PRESS)
            {
                if(gA == GLFW_PRESS)
                    camAng += 0.1;
                if(gD == GLFW_PRESS)
                    camAng -= 0.1;
                camPos.x = cos(camAng);
                camPos.z = sin(camAng);
            }

            if(glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS)
                lookPos.y += 0.01;

            if(glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS)
                lookPos.y -= 0.01;
        }

        // Handles keys on events (i.e., hold/unhold, not just is/isn't pressed).
        static void KeyGLFWCallback (GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            if(key == GLFW_KEY_P && action == GLFW_PRESS)
            {
                std::shared_ptr<PButtonEvent> e{new PButtonEvent};
                EventBus::Publish(e, EventBus::GetID<PButtonEvent>());
            }

            if(key == GLFW_KEY_K && action == GLFW_PRESS)
            {
                std::shared_ptr<KButtonEvent> e{new KButtonEvent};
                EventBus::Publish(e, EventBus::GetID<KButtonEvent>());
            }

            if(key == GLFW_KEY_R && action == GLFW_PRESS)
            {
                std::shared_ptr<RButtonEvent> e{new RButtonEvent};
                EventBus::Publish(e, EventBus::GetID<RButtonEvent>());
            }

            if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            {
                std::cout << " Bye bye :)" << std::endl;
                glfwTerminate();
                exit(0);
            }
        }
        
};

int main() {
    EventBus::CreateSingleton();
    TestWrapper wrapper;
    CustomExample().RunMainLoop();
}

