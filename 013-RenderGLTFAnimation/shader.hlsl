
// (13) RenderGLTFAnimation: 使用 DirectX 12 + Assimp 渲染《超次元游戏 海王星》中的 Neptune 涅普顿(绀紫之心) GLTF模型 的骨骼动画

struct VSInput      // VS 阶段输入顶点数据
{
	float4 position : POSITION;			// 输入顶点的位置
	float2 texcoordUV : TEXCOORD;		// 输入顶点的纹理坐标
	float4 color : COLOR;				// 对于自发光纹理，可能带有的颜色
	uint4 BoneIndices : BLENDINDICES;	// 骨骼索引
	float4 BoneWeights : BLENDWEIGHT;	// 骨骼权重
};

struct VSOutput     // VS 阶段输出顶点数据
{
	float4 position : SV_Position;		// 输出顶点的位置
	float2 texcoordUV : TEXCOORD;		// 输出顶点纹理坐标时
	float4 color : COLOR;				// 对于自发光纹理，可能带有的颜色
};

// Constant Buffer 常量缓冲，常量缓冲是预先分配的一段高速显存，存放每一帧都要变换的数据，例如我们这里的 MVP 变换矩阵
// 常量缓冲对所有着色器都是只读的，着色器不可以修改常量缓冲里面的内容
cbuffer GlobalData : register(b0, space0)
{
	row_major float4x4 MVP; // MVP 矩阵
	
	row_major float4x4 BoneTransformMatrixGroup[512]; // 骨骼偏移矩阵组，每个矩阵对应一块骨骼，这里仅设置了最多 512 个骨骼，实际可以更多
}


// Vertex Shader 顶点着色器入口函数 (逐顶点输入)，接收来自 IA 阶段输入的顶点数据，处理并返回齐次裁剪空间下的顶点坐标
VSOutput VSMain(VSInput input)
{
	VSOutput output;
	
	// 骨骼权重记录了每块骨骼对该顶点的影响程度，通过骨骼索引和骨骼权重，计算顶点在整个模型的真实位置
	// 顶点位置 = (每块骨骼的权重 * 每块骨骼的全局偏移矩阵 的累加和) * 顶点静止位置坐标
	float4x4 BoneMatrix = (
		input.BoneWeights[0] * BoneTransformMatrixGroup[input.BoneIndices[0]] +
		input.BoneWeights[1] * BoneTransformMatrixGroup[input.BoneIndices[1]] +
		input.BoneWeights[2] * BoneTransformMatrixGroup[input.BoneIndices[2]] +
		input.BoneWeights[3] * BoneTransformMatrixGroup[input.BoneIndices[3]]
	);
	
	
	output.position = mul(input.position, BoneMatrix);	// 与骨骼权重矩阵相乘，得到静止状态下的真实位置
	
	output.position = mul(output.position, MVP);		// 注意这里！顶点坐标还需要经过一次 MVP 变换！
	
	output.texcoordUV = input.texcoordUV;				// 纹理 UV 不用变换，照常输出即可
	
	output.color = input.color;							// color 也是
    
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

	
	// 如果是默认纹理或自发光纹理 (在 C++ 端的纹理 UV 坐标会设置成 -1)，进行特殊处理
	if (input.texcoordUV.x == -1 && input.texcoordUV.y == -1)
	{
		DiffuseColor = input.color; // 直接赋值自带颜色
	}
	else
	{
		// 在像素着色器根据光栅化插值得到的 UV 坐标对纹理进行采样
		DiffuseColor = m_texture.Sample(m_sampler, input.texcoordUV);
	}
	
	// 还要进行颜色混合，因为对于 Emissive Texture 自发光纹理而言，顶点会自带 color 信息的，需要将纹理颜色相混合
	DiffuseColor = input.color * DiffuseColor;
	
	return DiffuseColor; // 返回像素颜色，接下来会进行深度模板测试与混合
}