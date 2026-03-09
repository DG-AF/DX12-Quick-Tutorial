
// (20) ParticleSystem: 进一步学习计算着色器，学习粒子的动态生成与销毁，模拟 Minecraft 的粒子破坏效果
// DestroyStageShader.hlsl: 渲染方块表面破坏纹理的 shader


// 用于 MVP 矩阵、方块破坏阶段、方块朝向的常量缓冲
cbuffer GlobalData : register(b0, space0)
{
	// 摄像机提供 MVP 矩阵，将顶点从世界空间变换到齐次裁剪空间
	row_major float4x4 MVPMatrix;
	// 方块朝向，在 RenderShader.hlsl 中会用到，这个 shader 不需要管朝向，在这里做占位用的
	row_major float4x4 BlockFaceForwardMatrix[9];
	// 方块破坏阶段使用纹理的索引，每个面用的破坏阶段纹理索引是一样的，所以下面不需要用到面索引了
	uint DestroyStage;
}


// (IA 输入装配 -> VS 顶点着色器) 的 VS 输入参数结构体，注意语义要逐一锚定，不然参数会传递失败！
struct IA_To_VS
{
	// 输入槽 0 (顶点流)
	float4 Position : POSITION;		// 顶点位置
	float2 TexcoordUV : TEXCOORD;	// 纹理 UV
	
	// 输入槽 1 (实例流)
	float3 BlockOffset : BLOCKOFFSET;	// 每个方块实例距离世界中心 (0, 0, 0) 的位移
};


// (VS 顶点着色器 -> PS 像素着色器) 的 VS 输出，PS 输入参数结构体，注意语义要逐一锚定，不然参数会传递失败！
struct VS_To_PS
{
	float4 NDCPosition : SV_Position;	// NDC 空间坐标
	float2 TexcoordUV : TEXCOORD;		// 纹理 UV
};


// 顶点着色器 (逐顶点输入，IA 输入装配 -> VS 顶点着色器 -> RS 光栅化)
// 任务：将顶点变换到齐次裁剪空间，之后传入到下一个阶段
VS_To_PS VSMain(IA_To_VS VSInput)
{
	// VS 输出到 PS 的结构体
	VS_To_PS VSOutput;
	
	// 顶点累加偏移，偏移到对应要破坏的方块上
	VSInput.Position.xyz += VSInput.BlockOffset;
	// 顶点累乘 MVP 矩阵，变换到齐次裁剪空间，光栅化会进行透视除法变换到 NDC 空间，然后插值
	VSOutput.NDCPosition = mul(VSInput.Position, MVPMatrix);
	
	// 纹理 UV 不变，直接赋值，光栅化会进行插值
	VSOutput.TexcoordUV = VSInput.TexcoordUV;
	
	// 输出顶点到光栅化阶段
	return VSOutput;
}



// 纹理数组 2D Texture Array
Texture2DArray m_TextureArray : register(t1, space0);
// 采样器 (邻近点过滤)
SamplerState m_sampler : register(s0, space0);


// 像素着色器 (逐像素输入，RS 光栅化 -> PS 像素着色器 -> OM 输出合并)
// 任务：采样对应纹理并得到颜色，之后连带像素的其他信息传入到输出合并阶段，破坏纹理与方块表面原纹理混合
float4 PSMain(VS_To_PS PSInput) : SV_Target
{
	// 采样对应阶段的破坏纹理
	return m_TextureArray.Sample(m_sampler, float3(PSInput.TexcoordUV, DestroyStage));
}


