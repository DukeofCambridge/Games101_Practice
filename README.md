# Games101_Practice

## 环境配置
为VS添加 OpenCV 和 Eigen 库：<br/>
<div align="center">
<img src="./MD_ImageBed/env1.png" width=45%><img src="MD_ImageBed/env2.png" width=45%> </div>

## Assignment 1 Rotation&Projection 旋转与投影
- 核心：MVP变换
- 描述：实现以下几个函数，模拟三角形绕过原点的任意轴进行旋转并经光栅化绘制在屏幕上的过程。
  * model_transformation(Vector3f axis, float rotation_angle)  // 输入旋转轴与角度，得到对应的模型变换矩阵
  * view_transformation(Vector3f eye_pos)  // 输入摄像机位置，得到对应的观测变换矩阵
  * project_transformation(float eye_fov, float aspect_ratio, float zNear, float zFar) // 输入field of view，视野长宽比，近远平面距离，得到对应的透视投影变换矩阵
  * draw(std::map<int, std::vector<Eigen::Vector3f>> pos_buf, std::map<int, std::vector<Eigen::Vector3i>> indice_buf) // 输入三角形顶点坐标，应用MVP变换，然后进行viewport transformation，将三角形转换到指定大小的屏幕上

- 代码细节：
```c++
//用一个vector<Eigen::Vector3f>存储颜色信息，纵坐标设为 height-point.y() 是遵守opencv左上角为原点的规范，用cv库从frame_buf生成图像
void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    if (point.x() < 0 || point.x() >= width ||
        point.y() < 0 || point.y() >= height) return;
    auto ind = (height-point.y())*width + point.x();
    frame_buf[ind] = color;
}
```
- 实现效果：


https://github.com/DukeofCambridge/Games101_Practice/assets/68137344/090e2952-7c5d-4c45-bd05-d8d9d2ce304e




https://github.com/DukeofCambridge/Games101_Practice/assets/68137344/8814a175-2453-4c8d-b3ee-7ce5bdac3b8b


- 参考资料：
  * Bresenham's line algorithm: https://blog.csdn.net/qq_41883085/article/details/102706471
  * Rodrigues' rotation formula: https://blog.csdn.net/qq_36162042/article/details/115488168

## Assignment 2 Z-buffering 深度缓存
- 核心：fragment-level z-test
- 描述：实现以下几个方法，对三角形内部进行插值填充（颜色与深度）
  * 对视口变换后的三角形生成包围盒(bounding box)
  * 判断某点是否在三角形内部
  * 计算三角形内部点的重心坐标，用于插值
  * 完成z-test的逻辑
  * 利用超采样的方法抗锯齿(MSAA)，将每个像素分成四份，分别计算是否位于三角形内，像素的颜色设定为四个子像素的平均值

- 实现效果：
<div align="center">
<img src="./MD_ImageBed/depthTest.png" width=60%></div><br/>
采用MSAA抗锯齿后的效果对比（右图为超采样处理后，本质就是模糊了边界）: <br/><br/>
<div align="center">
<img src="./MD_ImageBed/msaa_contrast.png" width=60%></div>

## Assignment 3 Shading 着色
- 核心：fragment shader
- 描述：实现以下几个方法，熟悉几种常见的片元着色的逻辑
  * 在assignment2的基础上完善着色逻辑，将深度、法线、纹理坐标、view-space坐标等插值信息传递给fragment shader计算每个像素的颜色值
  * 实现基于blinn-phong 光照模型的 phong shader
  * 实现纹理采样的 texture shader
  * 实现基于凹凸贴图技术的 bump shader
  * 实现基于位移贴图技术的 displacement shader
  * 实现对texture mapping进行双线性插值

- 实现效果：
<div align="center">
<img src="./MD_ImageBed/phong.png" width=40%><img src="./MD_ImageBed/normal_fragment_shader.png" width=40%><br/>phong shader &nbsp &nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp normal shader</div><br/>
<div align="center">
<img src="./MD_ImageBed/bump_hmap.png" width=40%><img src="./MD_ImageBed/displacement.png" width=40%><br/>bump shader &nbsp &nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp displacement shader</div>

texture双线性插值采样前后效果对比：<br/>
<div align="center">
<img src="./MD_ImageBed/contrast_sampling.png" width=60%></div>

- 参考资料：
  * Bump Mapping: https://blog.csdn.net/qq_38065509/article/details/106050879
  * DownSamping and UpSampling: https://blog.csdn.net/zhibing_ding/article/details/125254670
