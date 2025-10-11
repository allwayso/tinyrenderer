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
constexpr int width = 20;
constexpr int height = 20;
void line(int ax, int ay, int bx, int by, TGAImage& framebuffer, TGAColor color) {
    bool steep = std::abs(ax - bx) < std::abs(ay - by);
    if (steep) { 
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    //避免出现除零情况，画竖直线
    if (ax == bx) {
        if (ay < by) {
            for (int y = ay; y < by; y++) {
                framebuffer.set(ax, y, color);
            }
        }
        else {
            for (int y = by; y < ay; y++) {
                framebuffer.set(ax, y, color);
            }
        }
        return;
    }
    //确保ax<ay
    if (ax > bx) {
        swap(ax, bx);
        swap(ay, by);
    }
    int y = ay;
    int ierror = 0;
    for (int x = ax; x < bx; x += 1) {
        if (!steep) {
            framebuffer.set(x, y, color);
        }
        if (steep) {
            framebuffer.set(y, x, color);
        }
        ierror += 2 * abs(by - ay);
        if (ierror > bx - ax) {
            y += by > ay ? 1 : -1;
            ierror -= 2 * (bx - ax);
        }
    }
}

void flat_line(int ax, int ay, int bx, int by, TGAImage& framebuffer, TGAColor color) {//水平于x轴的线段绘制
    for (int x = min(ax, bx); x < max(ax, bx); x++) {
        framebuffer.set(x, ay, color);
    }
}

tuple<int, int> project(vec3 v) {//二维坐标缩放函数
    int y = (v.y + 1) * height / 2;
    int x = (v.x + 1) * width / 2;
    return { x,y };
}


bool cmp(vec2& p1, vec2& p2) {
    return p1.y < p2.y;
}


// 保证 ay = by
void flat_triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage& framebuffer, TGAColor color) {
    // 三角形退化成一条线或一个点
    if (cy == ay) return;
    int y_start, y_end;
    float lx, rx;
    float slope_ac = (float)(cx - ax) / (cy - ay);
    float slope_bc = (float)(cx - bx) / (cy - by);
    if (ay < cy) {
        y_start = ay,y_end = cy,lx = ax,rx = bx;
    }
    else {
        y_start = cy, y_end = ay, lx = cx,rx = cx;
    }
    for (int y = y_start; y < y_end; y++) {
        flat_line(lx, y, rx, y, framebuffer, color);
        lx += slope_ac;
        rx += slope_bc;
    }
}

void old_triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage& framebuffer, TGAColor color) {
    line(ax, ay, bx, by, framebuffer, color);
    line(bx, by, cx, cy, framebuffer, color);
    line(cx, cy, ax, ay, framebuffer, color);
    vec2 a = { ax,ay };
    vec2 b = { bx,by };
    vec2 c = { cx,cy };
    vector < vec2 >points= {a, b, c};
    sort(points.begin(), points.end(), cmp);
    if (points[0].y != points[1].y&&points[1].y!=points[2].y) {//一般情况
        double k02 = (float)(points[2].x - points[0].x) / (points[2].y - points[0].y);
        double b02 = points[0].x - k02 * points[0].y;
        int x02 = k02 * points[1].y + b02;
        flat_triangle(points[1].x, points[1].y, x02, points[1].y,points[0].x,points[0].y, framebuffer, color);
        flat_triangle(points[1].x, points[1].y, x02, points[1].y, points[2].x, points[2].y, framebuffer,color);
    }
    else if (points[0].y == points[1].y) {
        flat_triangle(points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, framebuffer, color);
    }
    else if (points[1].y == points[2].y) {
        flat_triangle(points[1].x, points[1].y, points[2].x, points[2].y, points[0].x, points[0].y, framebuffer, color);
    }
}

bool in_triangle(int ax, int ay, int bx, int by, int cx, int cy, int px, int py) {
    vec2 lpa = { ax - px,ay - py };
    vec2 lpb = { bx - px,by - py };
    vec2 lpc = { cx - px,cy - py };
    float Sabp = (lpa.x * lpb.y - lpa.y * lpb.x);
    float Sbcp = (lpb.x * lpc.y - lpb.y * lpc.x);
    float Sacp = (lpc.x * lpa.y - lpc.y * lpa.x);
    if ((Sabp >= 0 && Sbcp >= 0 && Sacp >= 0)||(Sabp<=0&&Sbcp<=0&&Sacp<=0)) {
        return true;
    }
    else return false;
}
void new_triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage& framebuffer, TGAColor color) {
    int x_min = std::min({ ax, bx, cx });
    int x_max = std::max({ ax, bx, cx });
    int y_min = std::min({ ay, by, cy });
    int y_max = std::max({ ay, by, cy });
    int total_area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    //这是由于obj文件中规定当从某个面的“正面”看过去时，其结点总是逆时针排列，所以面积为负数的面一定背朝当前视角，可以剔除
    if (total_area < 1) return; // backface culling + discarding triangles that cover less than a pixel
    #pragma omp parallel for
    for (int px = x_min; px <= x_max; px++) {
        for (int py = y_min; py <= y_max; py++) {
            if (in_triangle(ax,ay,bx,by,cx,cy,px,py)) {
                framebuffer.set(px, py, color);
            }
        }
    }
}
void colorful_triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage& framebuffer) {
    int x_min = std::min({ ax, bx, cx });
    int x_max = std::max({ ax, bx, cx });
    int y_min = std::min({ ay, by, cy });
    int y_max = std::max({ ay, by, cy });
    float total_area = (float)(bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    //这是由于obj文件中规定当从某个面的“正面”看过去时，其结点总是逆时针排列，所以面积为负数的面一定背朝当前视角，可以剔除
    if (total_area < 1) return; // backface culling + discarding triangles that cover less than a pixel
    #pragma omp parallel for
    for (int px = x_min; px <= x_max; px++) {
        for (int py = y_min; py <= y_max; py++) {
            vec2 lpa = { ax - px,ay - py };
            vec2 lpb = { bx - px,by - py };
            vec2 lpc = { cx - px,cy - py };
            float Sabp = (lpa.x * lpb.y - lpa.y * lpb.x);
            float Sbcp = (lpb.x * lpc.y - lpb.y * lpc.x);
            float Sacp = (lpc.x * lpa.y - lpc.y * lpa.x);
            if (Sbcp < 0 || Sabp < 0 || Sacp < 0) {
                continue;
            }
            float A_weight = Sbcp / total_area;
            float B_weight = Sacp / total_area;
            float C_weight = Sabp / total_area;
            int blue = A_weight * 255;
            int green = B_weight * 255;
            int red = C_weight * 255;
            TGAColor color = { blue,green,red,255 };
            framebuffer.set(px, py, color);

        }
    }
}

/*void gray_triangle(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz, TGAImage& framebuffer) {
    int x_min = std::min({ ax, bx, cx });
    int x_max = std::max({ ax, bx, cx });
    int y_min = std::min({ ay, by, cy });
    int y_max = std::max({ ay, by, cy });
    float total_area = (float)(bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    //这是由于obj文件中规定当从某个面的“正面”看过去时，其结点总是逆时针排列，所以面积为负数的面一定背朝当前视角，可以剔除
    if (total_area < 1) return; // backface culling + discarding triangles that cover less than a pixel
    #pragma omp parallel for
    for (int px = x_min; px <= x_max; px++) {
        for (int py = y_min; py <= y_max; py++) {
            vec2 lpa = { ax - px,ay - py };
            vec2 lpb = { bx - px,by - py };
            vec2 lpc = { cx - px,cy - py };
            float Sabp = (lpa.x * lpb.y - lpa.y * lpb.x);
            float Sbcp = (lpb.x * lpc.y - lpb.y * lpc.x);
            float Sacp = (lpc.x * lpa.y - lpc.y * lpa.x);
            if (Sbcp < 0 || Sabp < 0 || Sacp < 0) {
                continue;
            }
            float A_weight = Sbcp / total_area;
            float B_weight = Sacp / total_area;
            float C_weight = Sabp / total_area;
            float gray_scale = az * A_weight + bz * B_weight + cz * C_weight;
            unsigned char z = static_cast<unsigned char> (gray_scale);
            framebuffer.set(px, py, { z });

        }
    }
}
*/

/*扫描线算法测试用例
int main(int argc, char** argv) {
    TGAImage framebuffer(width, height, TGAImage::RGB);
    //old-school扫描线算法
    old_triangle(7, 45, 35, 100, 45, 60, framebuffer, red);
    old_triangle(120, 35, 90, 5, 45, 110, framebuffer, white);
    old_triangle(115, 83, 80, 90, 85, 120, framebuffer, green);
    
new_triangle(7, 45, 35, 100, 45, 60, framebuffer, red);
new_triangle(120, 35, 90, 5, 45, 110, framebuffer, white);
new_triangle(115, 83, 80, 90, 85, 120, framebuffer, green);

framebuffer.write_tga_file("framebuffer.tga");
return 0;
}
*/

/*灰色三角形测试用例
* int main(int argc, char** argv) {
    constexpr int width = 64;
    constexpr int height = 64;
    TGAImage framebuffer(width, height, TGAImage::RGB);

    int ax = 17, ay = 4, az = 50;
    int bx = 55, by = 39, bz = 100;
    int cx = 23, cy = 59, cz = 150;

    gray_triangle(ax, ay, az, bx, by, bz, cx, cy, cz, framebuffer);
    framebuffer.write_tga_file("gray_triangle.tga");
    return 0;
}
*/
int main(int argc, char** argv) {
    
    TGAImage framebuffer(width, height, TGAImage::RGBA);

    vec2 p1 = { 1,1 };
    vec2 p2 = { 6,5 };
    line(p1.x, p1.y, p2.x, p2.y, framebuffer, red);
    framebuffer.write_tga_file("homework.tga");
    return 0;
}