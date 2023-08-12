// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}


static bool insideTriangle(float x, float y, const Vector3f* _v, float size_x, float size_y)
{
    // check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
    float x_mid = x + size_x / 2.0;
    float y_mid = y + size_y / 2.0;
    Eigen::Vector2f CA(_v[0].x() - _v[2].x(), _v[0].y() - _v[2].y());
    Eigen::Vector2f AP(x_mid - _v[0].x(), y_mid - _v[0].y());
    Eigen::Vector2f AB(_v[1].x() - _v[0].x(), _v[1].y() - _v[0].y());
    Eigen::Vector2f BP(x_mid - _v[1].x(), y_mid - _v[1].y());
    Eigen::Vector2f BC(_v[2].x() - _v[1].x(), _v[2].y() - _v[1].y());
    Eigen::Vector2f CP(x_mid - _v[2].x(), y_mid - _v[2].y());

    // cross product have equal sign
    if ((CA.x() * AP.y() - CA.y() * AP.x() >= 0) == (AB.x() * BP.y() - AB.y() * BP.x() >= 0) && (AB.x() * BP.y() - AB.y() * BP.x() >= 0) == (BC.x() * CP.y() - BC.y() * CP.x() >= 0)) {
        return true;
    }
    else {
        return false;
    }

}


static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
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
        for (auto & vert : v)
        {
            vert.x() = 0.5*width*(vert.x()+1.0);
            vert.y() = 0.5*height*(vert.y()+1.0);
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
    // Find out the bounding box of current triangle.
    int box_x_min = std::min(v[0].x(), std::min(v[1].x(), v[2].x()));
    int box_x_max = std::max(v[0].x(), std::max(v[1].x(), v[2].x()));
    int box_y_min = std::min(v[0].y(), std::min(v[1].y(), v[2].y()));
    int box_y_max = std::max(v[0].y(), std::max(v[1].y(), v[2].y()));
#pragma region normal version
//    // iterate through the pixel and find if the current pixel is inside the triangle
//    for(int x=box_x_min;x<=box_x_max;++x)
//    {
//	    for(int y=box_y_min;y<=box_y_max;++y)
//	    {
//            // If so, use the following code to get the interpolated z value.
//            if (insideTriangle(x, y, t.v, 1, 1)) {
//                auto [alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
//                float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
//                float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
//                z_interpolated *= w_reciprocal;
//                // set the current pixel (use the set_pixel function) to the color of the triangle (use getColor function) if it should be painted.
//                if (z_interpolated > depth_buf[get_index(x, y)]) {
//                    depth_buf[get_index(x, y)] = z_interpolated;
//                    set_pixel(Eigen::Vector3f(x, y, z_interpolated), t.getColor());
//                }
//            }
//	    }
//    }
#pragma endregion
#pragma region super-sampling
    int n = 2;
    int m = 2;
    for (auto x = box_x_min; x <= box_x_max; ++x) {
        for (auto y = box_y_min; y <= box_y_max; ++y) {
            int blockinTriangle = 0;
            auto [alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
            float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
            float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
            z_interpolated *= w_reciprocal;
            // there is at least one sub-pixel out of four that is in the triangle 
            if ((blockinTriangle = MSAA(x, y, t.v, n, m, z_interpolated, t.getColor())) > 0) {
                int idx = get_index(x, y);
                Vector3f c = (msaa_frame_buf[idx * 4] + msaa_frame_buf[idx * 4 + 1] + msaa_frame_buf[idx * 4 + 2] + msaa_frame_buf[idx * 4 + 3]) / 4.0;
                /*if(c[0]!=217.0&&c[1]!=238.0&&c[2]!=185.0&&c[0]!= 185.0&&c[1]!=217.0&&c[2]!=238.0)
					std::cout <<"appear"<< std::endl;*/
                set_pixel(Eigen::Vector3f(x, y, z_interpolated), (msaa_frame_buf[idx * 4] + msaa_frame_buf[idx * 4 + 1] + msaa_frame_buf[idx * 4 + 2] + msaa_frame_buf[idx * 4 + 3]) / 4.0);
            }
        }
    }
#pragma endregion
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
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), -std::numeric_limits<float>::infinity()+1);
    }
    std::fill(msaa_frame_buf.begin(), msaa_frame_buf.end(), Eigen::Vector3f{ 0, 0, 0 });
    std::fill(msaa_depth_buf.begin(), msaa_depth_buf.end(), -std::numeric_limits<float>::infinity() + 1);
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
    msaa_frame_buf.resize(4 * w * h);
    msaa_depth_buf.resize(4 * w * h);
}

int rst::rasterizer::get_index(int x, int y)
{
    return (height-1-y)*width + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height-1-point.y())*width + point.x();
    frame_buf[ind] = color;

}

int rst::rasterizer::MSAA(int x, int y, const Vector3f* _v, int n, int m, float z, Eigen::Vector3f color)
{
    float size_x = 1.0 / n; // the size_x of every super sample pixel
    float size_y = 1.0 / m;

    int blocksinTriangle = 0;
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < m; ++j) {
            if (z > msaa_depth_buf[get_index(x, y) * 4 + i * 2 + j] && insideTriangle(x + i * size_x, y + j * size_y, _v, size_x, size_y)) {
                msaa_depth_buf[get_index(x, y) * 4 + i * 2 + j] = z;
                msaa_frame_buf[get_index(x, y) * 4 + i * 2 + j] = color;
                blocksinTriangle++;
            }
            std::cout<<blocksinTriangle<<std::endl;

        }

    return blocksinTriangle;
}


// clang-format on