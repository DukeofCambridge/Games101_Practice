#include "Triangle.hpp"
#include "rasterizer.hpp"
#include <Eigen/Eigen>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <cmath>

constexpr double MY_PI = 3.1415926;  //180 degrees equals PI in radian

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0], 0, 1, 0, -eye_pos[1], 0, 0, 1,
        -eye_pos[2], 0, 0, 0, 1;

    view = translate * view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

    // Create the model matrix for rotating the triangle around the Z axis.
    const double rotation_angle_radian = rotation_angle * MY_PI / 180;
    Eigen::Matrix4f rotate = Eigen::Matrix4f::Identity();
    rotate(0, 0) = cos(rotation_angle_radian);
    rotate(0, 1) = -sin(rotation_angle_radian);
    rotate(1, 0) = sin(rotation_angle_radian);
    rotate(1, 1) = cos(rotation_angle_radian);
    model = rotate * model;

    return model;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
                                      float zNear, float zFar)
{
    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();
    // create the orthographic projection matrix
    float t = tan(eye_fov * MY_PI / 180 / 2) * fabs(zNear);  //half of the height of the near plane
    float r = t * aspect_ratio;   // half of the width of the near plane
    float l = -r;
    float b = -t;
    Eigen::Matrix4f translate = Eigen::Matrix4f::Identity();
    translate(0, 3) = -(r + l) / 2;
    translate(1, 3) = -(t + b) / 2;
    translate(2, 3) = -(zNear + zFar) / 2;
    Eigen::Matrix4f scale = Eigen::Matrix4f::Identity();
    scale(0, 0) = 2 / (r - l);
    scale(1, 1) = 2 / (t - b);
    scale(2, 2) = 2 / (zNear - zFar);
    Eigen::Matrix4f ortho = scale * translate;
    // create the persp2ortho matrix
    Eigen::Matrix4f persp2ortho = Eigen::Matrix4f::Zero();
    persp2ortho(0, 0) = zNear;
    persp2ortho(1, 1) = zNear;
    persp2ortho(2, 2) = zFar + zNear;
    persp2ortho(2, 3) = -zFar * zNear;
    persp2ortho(3, 2) = 1;

    projection = ortho * persp2ortho;

    return projection;
}

int main(int argc, const char** argv)
{
    float angle = 0;
    bool command_line = false;
    std::string filename = "output.png";

    if (argc >= 3) {
        command_line = true;
        angle = std::stof(argv[2]); // -r by default
        if (argc == 4) {
            filename = std::string(argv[3]);
        }
        else
            return 0;
    }

    rst::rasterizer r(700, 700);
    //position of camera
    Eigen::Vector3f eye_pos = {0, 0, 5};
    //a hard-coded triangle 
    std::vector<Eigen::Vector3f> pos{{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}};
    //indices of coordinates
    std::vector<Eigen::Vector3i> ind{{0, 1, 2}};

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);
    //ascii code of input key
    int key = 0;
    int frame_count = 0;

    if (command_line) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);
        // CV_32FC3 32:one pixel takes up 32 bits; F:float; C3:three channels,namely RGB
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        // CV_8UC3 8:one pixel takes up 8 bits; U:unsigned int; C3:three channels,namely RGB
        image.convertTo(image, CV_8UC3, 1.0f);

        cv::imwrite(filename, image);

        return 0;
    }

    while (key != 27) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::imshow("image", image);
        key = cv::waitKey(10);

        std::cout << "frame count: " << frame_count++ << '\n';

        if (key == 'a') {
            angle += 10;
        }
        else if (key == 'd') {
            angle -= 10;
        }
    }

    return 0;
}
