#include <cstdlib>
#include "our_gl.h"
#include "model.h"

extern mat<4, 4> ModelView, Perspective; // "OpenGL" state matrices and
extern std::vector<double> zbuffer;     // the depth buffer

struct PhongShader : IShader {
    const Model& model;
    vec3 light_dir;
    vec3 eye_pos;
    TGAColor color = {};
    vec3 tri[3];
    vec3 normals[3]; // 存储三角形三个顶点的法线
    vec3 face_normal; // 面法线

    PhongShader(const Model& m, const vec3& light, const vec3& eye)
        : model(m), light_dir(light), eye_pos(eye) {
    }

    virtual vec4 vertex(const int face, const int vert) {
        vec3 v = model.vert(face, vert);
        vec4 gl_Position = ModelView * vec4{ v.x, v.y, v.z, 1. };
        tri[vert] = gl_Position.xyz();
        normals[vert] = model.normal(face, vert);//获取顶点法线
        return Perspective * gl_Position;
    }

    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
        vec3 n = normalized(normals[0] * bar.x + normals[1] * bar.y + normals[2] * bar.z);
        vec3 pos = tri[0] * bar.x + tri[1] * bar.y + tri[2] * bar.z;
        vec3 view_dir = normalized(eye_pos - pos);
        vec3 reflect_dir = normalized(2.f * n * (n * light_dir) - light_dir);

        float ambient = 0.1f;
        float diffuse = std::max(0.0f, float(n * light_dir));
        float spec = std::pow(std::max(0.0f, float(view_dir * reflect_dir)), 16);
        float intensity = ambient + 0.6f * diffuse + 0.3f * spec;
        intensity = std::clamp(intensity, 0.0f, 1.0f);

        TGAColor shaded = color;
        shaded[0] = static_cast<std::uint8_t>(intensity * 255);
        shaded[1] = shaded[2] = 0;
        return { false, shaded };
    }
};

struct RandomShader : IShader {
    const Model& model;
    TGAColor color = {};
    vec3 tri[3];  // triangle in eye coordinates

    RandomShader(const Model& m) : model(m) {
    }

    virtual vec4 vertex(const int face, const int vert) {
        vec3 v = model.vert(face, vert);                          // current vertex in object coordinates
        vec4 gl_Position = ModelView * vec4{ v.x, v.y, v.z, 1. };
        tri[vert] = gl_Position.xyz();                            // in eye coordinates
        return Perspective * gl_Position;                         // in clip coordinates
    }

    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
        return { false, color };                                    // do not discard the pixel
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }

    constexpr int width =800;      // output image size
    constexpr int height =800;
    constexpr vec3    eye{ -1, 0,1 }; // camera position
    constexpr vec3 center{ 0, 0, 0 }; // camera direction
    constexpr vec3     up{ 0, 1, 0 }; // camera up vector

    lookat(eye, center, up);                                   // build the ModelView   matrix
    init_perspective(norm(eye - center));                        // build the Perspective matrix
    init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8); // build the Viewport    matrix
    init_zbuffer(width, height);
    TGAImage framebuffer(width, height, TGAImage::GRAYSCALE, { 128, 0, 0, 255 });

    for (int m = 1; m < argc; m++) {                    // iterate through all input objects
        Model model(argv[m]);        // load the data
        vec3 light_dir = normalized(vec3{ 0,0, 1 });
        PhongShader shader(model, light_dir, eye);
        for (int f = 0; f < model.nface(); f++) {      // iterate through all facets
            shader.color = { 200, 0, 0, 255 };
            Triangle clip = { shader.vertex(f, 0),  // assemble the primitive
                              shader.vertex(f, 1),
                              shader.vertex(f, 2) };
            rasterize(clip, shader, framebuffer);   // rasterize the primitive
        }
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}