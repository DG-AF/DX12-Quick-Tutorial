
// (7) RenderMatchbox：渲染一个火柴盒，初步认识 Depth Stencil Buffer 深度模板缓冲，进一步认识顶点、模型与模型矩阵

struct VSInput      // VS 阶段输入顶点数据
{
    float4 position : POSITION;         // 输入顶点的位置，POSITION 语义对应 C++ 端输入布局中的 POSITION
    float2 texcoordUV : TEXCOORD;       // 输入顶点的纹理坐标，TEXCOORD 语义对应 C++ 端输入布局中的 TEXCOORD
    
    // 如果我们需要向 IA 阶段传递矩阵，矩阵太大没法直接传，我们可以把矩阵分割成一个一个行向量，再到 VS 阶段重新组装
    // MATRIX 是自定义语义，语义后面的数字表示同一个输入槽下，同语义名 (MATRIX) 的第 i 号数据
    float4 Matrix_Row0 : MATRIX0;
    float4 Matrix_Row1 : MATRIX1;
    float4 Matrix_Row2 : MATRIX2;
    float4 Matrix_Row3 : MATRIX3;
    
    // 其实语义只是个标识东西的字符串...
};

struct VSOutput     // VS 阶段输出顶点数据
{
    float4 position : SV_Position;      // 输出顶点的位置，SV_POSITION 是系统语义，指定顶点坐标已经位于齐次裁剪空间，通知光栅化阶段对顶点进行透视除法和屏幕映射
    float2 texcoordUV : TEXCOORD;       // 输出顶点纹理坐标时，仍然需要 TEXCOORD 语义
};

// Constant Buffer 常量缓冲，常量缓冲是预先分配的一段高速显存，存放每一帧都要变换的数据，例如我们这里的 MVP 变换矩阵
// 常量缓冲对所有着色器都是只读的，着色器不可以修改常量缓冲里面的内容
cbuffer GlobalData : register(b0, space0) // 常量缓冲，b 表示 buffer 缓冲，b0 表示 0 号 CBV 寄存器，space0 表示使用 b0 的 0 号空间
{
    row_major float4x4 MVP; // MVP 矩阵，用于将顶点坐标从模型空间变换到齐次裁剪空间，HLSL 默认按列存储，row_major 表示数据按行存储
}


// Vertex Shader 顶点着色器入口函数 (逐顶点输入)，接收来自 IA 阶段输入的顶点数据，处理并返回齐次裁剪空间下的顶点坐标
// 上一阶段：Input Assembler 输入装配阶段
// 下一阶段：Rasterization 光栅化阶段
VSOutput VSMain(VSInput input)
{
    float4x4 ModelMatrix;   // VS 阶段要用到的模型矩阵
    VSOutput output;        // 输出给光栅化阶段的结构体变量
    
    // 将 IA 阶段得到的行数据组装成矩阵
    ModelMatrix[0] = input.Matrix_Row0;
    ModelMatrix[1] = input.Matrix_Row1;
    ModelMatrix[2] = input.Matrix_Row2;
    ModelMatrix[3] = input.Matrix_Row3;
    
    // 注意 cbuffer 常量缓冲对着色器是只读的！所以我们不能在这里对常量缓冲进行修改！
    output.position = mul(input.position, ModelMatrix);     // 先乘 模型矩阵
    output.position = mul(output.position, MVP);            // 再乘 观察矩阵 和 投影矩阵，注意 mul 左操作数是 output.position
    output.texcoordUV = input.texcoordUV;                   // 纹理 UV 不用变化，照常输出即可
    
    return output;
}

// register(*#，spaceN) *表示资源类型，#表示所用的寄存器编号，spaceN 表示使用的 N 号寄存器空间

Texture2D m_texure : register(t0, space0);      // 纹理，t 表示 SRV 着色器资源，t0 表示 0 号 SRV 寄存器，space0 表示使用 t0 的 0 号空间
SamplerState m_sampler : register(s0, space0);  // 纹理采样器，s 表示采样器，s0 表示 0 号 sampler 寄存器，space0 表示使用 s0 的 0 号空间

// Pixel Shader 像素着色器入口函数 (逐像素输入)，接收来自光栅化阶段经过插值后的每个片元，返回像素颜色
// 上一阶段：Rasterization 光栅化阶段
// 下一阶段：Output Merger 输出合并阶段
float4 PSMain(VSOutput input) : SV_Target // SV_Target 也是系统语义，通知输出合并阶段将 PS 阶段返回的颜色写入到渲染目标(颜色缓冲)上
{
    return m_texure.Sample(m_sampler, input.texcoordUV); // 在像素着色器根据光栅化插值得到的 UV 坐标对纹理进行采样
}

