
// (4) DrawTexture：用 DirectX 12 画一个钻石原矿

struct VSInput      // VS 阶段输入顶点数据
{
    float4 position : POSITION;         // 输入顶点的位置，POSITION 语义对应 C++ 端输入布局中的 POSITION
    float2 texcoordUV : TEXCOORD;       // 输入顶点的纹理坐标，TEXCOORD 语义对应 C++ 端输入布局中的 TEXCOORD
};

struct VSOutput     // VS 阶段输出顶点数据
{
    float4 position : SV_Position;      // 输出顶点的位置，SV_POSITION 是系统语义，指定顶点坐标已经位于齐次裁剪空间，通知光栅化阶段对顶点进行透视除法和屏幕映射
    float2 texcoordUV : TEXCOORD;       // 输出顶点纹理坐标时，仍然需要 TEXCOORD 语义
};

// Vertex Shader 顶点着色器入口函数 (逐顶点输入)，接收来自 IA 阶段输入的顶点数据，处理并返回齐次裁剪空间下的顶点坐标
// 上一阶段：Input Assembler 输入装配阶段
// 下一阶段：Rasterization 光栅化阶段
VSOutput VSMain(VSInput input)
{
    VSOutput output; // 我们直接向 IA 阶段输入顶点在 NDC 空间下的坐标，所以无需变换，直接赋值返回就行
    output.position = input.position;
    output.texcoordUV = input.texcoordUV;
    
    return output;
}

// register(*#，spaceN) *表示资源类型，#表示所用的寄存器编号，spaceN 表示使用的 N 号寄存器空间

Texture2D m_texure : register(t0, space0);          // 纹理，t 表示 SRV 着色器资源，t0 表示 0 号 SRV 寄存器，space0 表示使用 t0 的 0 号空间
SamplerState m_sampler : register(s0, space0);      // 纹理采样器，s 表示采样器，s0 表示 0 号 sampler 寄存器，space0 表示使用 s0 的 0 号空间

// Pixel Shader 像素着色器入口函数 (逐像素输入)，接收来自光栅化阶段经过插值后的每个片元，返回像素颜色
// 上一阶段：Rasterization 光栅化阶段
// 下一阶段：Output Merger 输出合并阶段
float4 PSMain(VSOutput input) : SV_Target   // SV_Target 也是系统语义，通知输出合并阶段将 PS 阶段返回的颜色写入到渲染目标(颜色缓冲)上
{
    return m_texure.Sample(m_sampler, input.texcoordUV);    // 在像素着色器根据光栅化插值得到的 UV 坐标对纹理进行采样
}

