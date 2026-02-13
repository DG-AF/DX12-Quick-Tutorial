
// (10) RenderGLTFModel: 使用 DirectX 12 + Assimp 渲染一个苦力怕 gltf 模型 (此模型没有骨骼)

struct VSInput      // VS 阶段输入顶点数据
{
	float4 position : POSITION;		// 输入顶点的位置
	float2 texcoordUV : TEXCOORD;	// 输入顶点的纹理坐标
};


struct VSOutput     // VS 阶段输出顶点数据
{
	float4 position : SV_Position;	// 输出顶点的位置
	float2 texcoordUV : TEXCOORD;	// 输出顶点纹理坐标时
};


// Constant Buffer 常量缓冲，常量缓冲是预先分配的一段高速显存，存放每一帧都要变换的数据，例如我们这里的 MVP 变换矩阵
// 常量缓冲对所有着色器都是只读的，着色器不可以修改常量缓冲里面的内容
cbuffer GlobalData : register(b0, space0)
{
	row_major float4x4 MVP; // MVP 矩阵
}


// Vertex Shader 顶点着色器入口函数 (逐顶点输入)，接收来自 IA 阶段输入的顶点数据，处理并返回齐次裁剪空间下的顶点坐标
VSOutput VSMain(VSInput input)
{
	VSOutput output;

	output.position = mul(input.position, MVP);		// 注意这里！顶点坐标还需要经过一次 MVP 变换！
	
	output.texcoordUV = input.texcoordUV;			// 纹理 UV 不用变换，照常输出即可
    
	return output; // 返回处理后的顶点，接下来进行光栅化操作
}

// register(*#，spaceN) *表示资源类型，#表示所用的寄存器编号，spaceN 表示使用的 N 号寄存器空间

Texture2D m_texture : register(t0, space0);		// 纹理
SamplerState m_sampler : register(s0, space0);	// 纹理采样器


// Pixel Shader 像素着色器入口函数 (逐像素输入)，接收来自光栅化阶段经过插值后的每个片元，返回像素颜色
float4 PSMain(VSOutput input) : SV_Target
{
	// DiffuseColor 漫反射发出的颜色，用于最终输出像素颜色，漫反射主要和第 12 章的光照有关，现在简单了解一下即可
	float4 DiffuseColor = float4(1, 1, 1, 0);

	
	// 如果是默认纹理 (在 C++ 端的纹理 UV 坐标会设置成 -1)，进行特殊处理
	if (input.texcoordUV.x == -1 && input.texcoordUV.y == -1)
	{
		clip(-1);	// 裁剪当前像素
	}
	else // 在像素着色器根据光栅化插值得到的 UV 坐标对纹理进行采样
	{
		DiffuseColor = m_texture.Sample(m_sampler, input.texcoordUV);
	}
	
	return DiffuseColor; // 返回像素颜色，接下来会进行深度模板测试与混合
}