
// (3) DrawRectangle：用 DirectX 12 画一个矩形

struct VSInput      // VS 阶段输入顶点数据
{
    float4 position : POSITION;     // 输入顶点的位置，POSITION 语义对应 C++ 端输入布局中的 POSITION
    float4 color : COLOR;           // 输入顶点的颜色，COLOR 语义对应 C++ 端输入布局中的 COLOR
};

struct VSOutput     // VS 阶段输出顶点数据
{
    float4 position : SV_Position;  // 输出顶点的位置，SV_POSITION 是系统语义，指定顶点坐标已经位于齐次裁剪空间，通知光栅化阶段对顶点进行透视除法和屏幕映射
    float4 color : COLOR;           // 输出顶点颜色时，仍然需要 COLOR 语义
};

// Vertex Shader 顶点着色器入口函数 (逐顶点输入)，接收来自 IA 阶段输入的顶点数据，处理并返回齐次裁剪空间下的顶点坐标
// 上一阶段：Input Assembler 输入装配阶段
// 下一阶段：Rasterization 光栅化阶段
VSOutput VSMain(VSInput input)
{
    VSOutput output;    // 我们直接向 IA 阶段输入顶点在 NDC 空间下的坐标，所以无需变换，直接赋值返回就行
    output.position = input.position;
    output.color = input.color;
    
    return output;
}

// Pixel Shader 像素着色器入口函数 (逐像素输入)，接收来自光栅化阶段经过插值后的每个片元，返回像素颜色
// 上一阶段：Rasterization 光栅化阶段
// 下一阶段：Output Merger 输出合并阶段
float4 PSMain(VSOutput input) : SV_Target   // SV_Target 也是系统语义，通知输出合并阶段将 PS 阶段返回的颜色写入到渲染目标(颜色缓冲)上
{
    return input.color;     // 我们这里直接返回每个像素的颜色就行
}

