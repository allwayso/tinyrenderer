#define _USE_MATH_DEFINES
#include "tgaimage.h"
#include"model.h"
#include"geometry.h"
#include<cmath>
#include<tuple>
#include<algorithm>
using namespace std;
constexpr TGAColor white = { 255, 255, 255, 255 }; // attention, BGRA order
constexpr TGAColor green = { 0, 255,   0, 255 };
constexpr TGAColor red = { 0,   0, 255, 255 };
constexpr TGAColor blue = { 255, 128,  64, 255 };
constexpr TGAColor yellow = { 0, 200, 255, 255 };
constexpr int width = 4096;
constexpr int height = 4096;


mat<4, 4> ModelView, Viewport, Perspective;

//--------------变换函数---------------

//视口变换+深度赋值
tuple<int, int, int> project(vec3 v) {
    return { (v.x + 1.) * width / 2,
             (v.y + 1.) * height / 2,
             (v.z + 1.) * 255. / 2 };
}

//视口矩阵：从归一化二维坐标转换为屏幕坐标，xywh分别为屏幕坐标系的原点xy坐标和屏幕宽高
void viewport(const int x, const int y, const int w, const int h) {
    Viewport = { {{w / 2., 0, 0, x + w / 2.}, {0, h / 2., 0, y + h / 2.}, {0,0,1,0}, {0,0,0,1}} };
}

//绕y轴旋转矩阵
vec3 rot(vec3 v) {
    constexpr double a = M_PI / 6;
    mat<3, 3> Ry = { {{std::cos(a), 0, std::sin(a)}, {0,1,0}, {-std::sin(a), 0, std::cos(a)}} };
    return Ry * v;
}

//透视变换
vec3 persp(vec3 v) {
    constexpr double c = 3.;
    return v / (1 - v.z / c);
}

//透视矩阵：从相机视角将对象投影到z=0平面上
void perspective(const double f) {
    Perspective = { {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0, -1 / f,1}} };
}

//观察矩阵：平移至相机坐标系原点+基向量转换（新坐标向量=原向量点乘基向量）
void lookat(const vec3 eye, const vec3 center, const vec3 up) {
    //相机坐标系的三个基向量
    vec3 n = normalized(eye - center);
    vec3 l = normalized(cross(up, n));
    vec3 m = normalized(cross(n, l));
    ModelView = mat<4, 4>{ {{l.x,l.y,l.z,0}, {m.x,m.y,m.z,0}, {n.x,n.y,n.z,0}, {0,0,0,1}} } *
        mat<4, 4>{{{1, 0, 0, -center.x}, { 0,1,0,-center.y }, { 0,0,1,-center.z }, { 0,0,0,1 }}};
}

void rasterize(const vec4 clip[3], std::vector<double>& zbuffer, TGAImage& framebuffer, const TGAColor color) {
    vec4 ndc[3] = { clip[0] / clip[0].w, clip[1] / clip[1].w, clip[2] / clip[2].w };                // 归一化至[1,1,1]空间
    vec2 screen[3] = { (Viewport * ndc[0]).xy(), (Viewport * ndc[1]).xy(), (Viewport * ndc[2]).xy() }; //视口变换，取xy坐标

    mat<3, 3> ABC = { { {screen[0].x, screen[0].y, 1.}, {screen[1].x, screen[1].y, 1.}, {screen[2].x, screen[2].y, 1.} } };
    if (ABC.det() < 1) return; // 消隐

    auto [bbminx, bbmaxx] = std::minmax({ screen[0].x, screen[1].x, screen[2].x }); // 矩形框
    auto [bbminy, bbmaxy] = std::minmax({ screen[0].y, screen[1].y, screen[2].y }); 
    #pragma omp parallel for
    for (int x = std::max<int>(bbminx, 0); x <= std::min<int>(bbmaxx, framebuffer.width() - 1); x++) { // 用屏幕裁剪这个矩形，省略超出屏幕的部分
        for (int y = std::max<int>(bbminy, 0); y <= std::min<int>(bbmaxy, framebuffer.height() - 1); y++) {
            vec3 bc = ABC.invert_transpose() * vec3 { static_cast<double>(x), static_cast<double>(y), 1. }; // 权重计算
            if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;                                                    
            double z = bc * vec3{ ndc[0].z, ndc[1].z, ndc[2].z };
            if (z <= zbuffer[x + y * framebuffer.width()]) continue;
            zbuffer[x + y * framebuffer.width()] = z;
            framebuffer.set(x, y, color);
        }
    }
}


int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }

    constexpr vec3    eye{ -1,0,2 }; // camera position
    constexpr vec3 center{ 0,0,0 };  // camera direction
    constexpr vec3     up{ 0,1,0 };  // camera up vector

    lookat(eye, center, up);                              // build the ModelView   matrix
    perspective(norm(eye - center));                        // build the Perspective matrix
    viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8); // build the Viewport matrix

    TGAImage framebuffer(width, height, TGAImage::RGB);
    std::vector<double> zbuffer(width * height, -std::numeric_limits<double>::max());

    for (int m = 1; m < argc; m++) { // iterate through all input objects
        Model model(argv[m]);
        for (int i = 0; i < model.nface(); i++) { // iterate through all triangles
            vec4 clip[3];
            for (int d : {0, 1, 2}) {            // assemble the primitive
                vec3 v = model.vert(i, d);
                clip[d] = Perspective * ModelView * vec4{ v.x, v.y, v.z, 1. };
            }
            TGAColor rnd;
            for (int c = 0; c < 3; c++) rnd[c] = std::rand() % 255;
            rasterize(clip, zbuffer, framebuffer, rnd); // rasterize the primitive
        }
    }

    framebuffer.write_tga_file("camera_devil.tga");
    return 0;
}
