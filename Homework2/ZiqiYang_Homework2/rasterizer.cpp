// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f>& positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return { id };
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i>& indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return { id };
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f>& cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return { id };
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}


static bool insideTriangle(float x, float y, const Vector3f* _v) // need to change the parameter as floating point numbers
{
    // TODO : Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
    Eigen::Vector2f pixel = { x, y };
    Eigen::Vector2f a, b, c;
    a = _v[0].head(2) - _v[1].head(2);  // a = A - B = B to A
    b = _v[1].head(2) - _v[2].head(2);  // b = B - C = C to B
    c = _v[2].head(2) - _v[0].head(2);  // c = C - A = A to C

    Eigen::Vector2f AP, BP, CP;
    AP = pixel - _v[0].head(2);
    BP = pixel - _v[1].head(2);
    CP = pixel - _v[2].head(2);

    // all we need is z = xa*yb-ya*xb
    return (AP[0] * c[1] - AP[1] * c[0] > 0)//AP x AC
        && (BP[0] * a[1] - BP[1] * a[0] > 0)//BP x BA
        && (CP[0] * b[1] - CP[1] * b[0] > 0);//CP x CB
}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
    float c1 = (x * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * y + v[1].x() * v[2].y() - v[2].x() * v[1].y()) / (v[0].x() * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * v[0].y() + v[1].x() * v[2].y() - v[2].x() * v[1].y());
    float c2 = (x * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * y + v[2].x() * v[0].y() - v[0].x() * v[2].y()) / (v[1].x() * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * v[1].y() + v[2].x() * v[0].y() - v[0].x() * v[2].y());
    float c3 = (x * (v[0].y() - v[1].y()) + (v[1].x() - v[0].x()) * y + v[0].x() * v[1].y() - v[1].x() * v[0].y()) / (v[2].x() * (v[0].y() - v[1].y()) + (v[1].x() - v[0].x()) * v[2].y() + v[0].x() * v[1].y() - v[1].x() * v[0].y());
    return { c1,c2,c3 };
}

void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type)
{
    auto& buf = pos_buf[pos_buffer.pos_id];
    auto& ind = ind_buf[ind_buffer.ind_id];
    auto& col = col_buf[col_buffer.col_id];

    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Matrix4f mvp = projection * view * model;
    for (auto& i : ind)
    {
        Triangle t;
        Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)
        };

        //Homogeneous division
        for (auto& vec : v) {
            vec /= vec.w();
        }

        //Viewport transformation
        for (auto& vert : v)
        {
            vert.x() = 0.5 * width * (vert.x() + 1.0);
            vert.y() = 0.5 * height * (vert.y() + 1.0);
            vert.z() = vert.z() * f1 + f2;
        }

        for (int i = 0; i < 3; ++i)
        {
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
        }

        auto col_x = col[i[0]];
        auto col_y = col[i[1]];
        auto col_z = col[i[2]];

        t.setColor(0, col_x[0], col_x[1], col_x[2]);
        t.setColor(1, col_y[0], col_y[1], col_y[2]);
        t.setColor(2, col_z[0], col_z[1], col_z[2]);

        rasterize_triangle(t);
    }
}

//Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t) {
    auto v = t.toVector4();

    // TODO : Find out the bounding box of current triangle.
    float bbleft = std::floor(std::min(v[0][0], std::min(v[1][0], v[2][0])));
    float bbright = std::floor(std::max(v[0][0], std::max(v[1][0], v[2][0])));
    float bbtop = std::floor(std::min(v[0][1], std::min(v[1][1], v[2][1])));
    float bbbottom = std::floor(std::max(v[0][1], std::max(v[1][1], v[2][1])));


    if (usingMSAA)
    {
        std::cout << "using MSAA" << std::endl;
        std::vector<Eigen::Vector2f> pos
        {
            {0.25,0.25}, // left-top  
            {0.75,0.25}, // right-top
            {0.25,0.75}, // left-bottom
            {0.75,0.75}  // right-bottom
        };

        for (int i = bbleft; i <= bbright; ++i)
        {
            for (int j = bbtop; j <= bbbottom; ++j)
            {
                int count = 0;
                int ind = get_index(i, j);
                frame_buf[ind] = { 0,0,0 };

                for (int MSAA_4 = 0; MSAA_4 < 4; ++MSAA_4)
                {
                    if (insideTriangle(i + pos[MSAA_4][0], j + pos[MSAA_4][1], t.v))
                    {
                        auto [alpha, beta, gamma] = computeBarycentric2D(i + pos[MSAA_4][0], j + pos[MSAA_4][1], t.v);
                        float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                        float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                        z_interpolated *= w_reciprocal;

                        if (depth_buf[get_index_msaa((i + pos[MSAA_4][0] - 0.25) * 2, (j + pos[MSAA_4][1] - 0.25) * 2)] > z_interpolated) {
                            depth_buf[get_index_msaa((i + pos[MSAA_4][0] - 0.25) * 2, (j + pos[MSAA_4][1] - 0.25) * 2)] = z_interpolated;//update depth
                            color_buf[get_index_msaa((i + pos[MSAA_4][0] - 0.25) * 2, (j + pos[MSAA_4][1] - 0.25) * 2)] = t.getColor();
                            count++;
                        }
                    }
                    // recalculate the avg color from the MSAA color_buf maintained
                    frame_buf[ind] += color_buf[get_index_msaa((i + pos[MSAA_4][0] - 0.25) * 2, (j + pos[MSAA_4][1] - 0.25) * 2)] / 4;
                }
            }
        }
    }
    else
    {
        // iterate through the pixel and find if the current pixel is inside the triangle
        for (int i = bbleft; i <= bbright; i++)
        {
            for (int j = bbtop; j <= bbbottom; j++)
            {
                if (insideTriangle(i + 0.5, j + 0.5, t.v)) {
                    // If so, use the following code to get the interpolated z value.
                    auto [alpha, beta, gamma] = computeBarycentric2D(i + 0.5, j + 0.5, t.v);
                    float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                    float z_interpolated = 
                        alpha * v[0].z() / v[0].w() + 
                        beta * v[1].z() / v[1].w() + 
                        gamma * v[2].z() / v[2].w();

                    z_interpolated *= w_reciprocal;
                    // TODO : set the current pixel (use the set_pixel function) to the color of the triangle (use getColor function) if it should be painted.
                    if (depth_buf[get_index(i, j)] > z_interpolated) {//update
                        depth_buf[get_index(i, j)] = z_interpolated;
                        set_pixel({ static_cast<float>(i), static_cast<float>(j), z_interpolated }, t.getColor());
                    }

                }
            }
        }
    }
}

void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
    view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
    projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{ 0, 0, 0 });
        std::fill(color_buf.begin(), color_buf.end(), Eigen::Vector3f{ 0, 0, 0 });
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}
rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);

    if (usingMSAA) { // check if use MSAA
        w = w * 4 + 100;
    }

    color_buf.resize(w * h);
    depth_buf.resize(w * h);
}

int rst::rasterizer::get_index(int x, int y)
{
    return (height - 1 - y) * width + x;
}

int rst::rasterizer::get_index_msaa(int x, int y)
{
    return (height * 2 - 1 - y) * width * 2 + x;
}


void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height - 1 - point.y()) * width + point.x();
    frame_buf[ind] = color;
}

// clang-format on

