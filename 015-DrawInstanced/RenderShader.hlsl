
// (15) DrawInstanced: 学会 DirectX 12 的纹理数组、SRV Structured Buffer 结构化缓冲的创建与使用，以及多实例渲染的应用，一次性快速渲染大量方块
// RenderShader.hlsl: 渲染方块的 shader

// 用于 MVP 矩阵的常量缓冲
cbuffer GlobalData : register(b0, space0)
{
	row_major float4x4 MVPMatrix;	// 摄像机提供 MVP 矩阵，将顶点从世界空间变换到齐次裁剪空间
}

// 立方体面结构体，数组索引表示对应的立方体面，数组元素值表示 对应面所用纹理 指向 纹理数组 的索引
struct CUBEFACE
{
	// 数组索引 0-5 分别对应右面 (+X)，左面 (-X)，前面 (+Z)，后面 (-Z)，上面 (+Y)，下面 (-Y)
	uint FaceTexture_InArrayIndex[6];
};


// GPU 上的方块类型-纹理索引组 (SRV Structured Buffer)
// HLSL 也有类似 C++ 一样的模板类，StructuredBuffer<Type> 相当于 C++ 的 std::array<type> 静态数组
// 数组索引表示不同的方块类型，数组元素值表示对应方块六个面的纹理数据索引数据
StructuredBuffer<CUBEFACE> BlockCubeTexture_IndexGroup : register(t0, space0);


// (IA 输入装配 -> VS 顶点着色器) 的 VS 输入参数结构体，注意语义要逐一锚定，不然参数会传递失败！
struct IA_To_VS
{
	// 输入槽 0 (顶点流)
	float4 Position : POSITION;		// 顶点位置
	float2 TexcoordUV : TEXCOORD;	// 纹理 UV
	uint FaceIndex : FACEINDEX;		// 顶点所属的立方体面索引 (在 FaceTextureInArrayIndex 上的索引)
	
	// 输入槽 1 (实例流)
	float3 BlockOffset : BLOCKOFFSET;	// 每个方块实例距离世界中心 (0, 0, 0) 的位移
	uint BlockType : BLOCKTYPE;			// 方块实例类型 (在 BlockCubeTextureIndexGroup 上的索引)
};


// (VS 顶点着色器 -> PS 像素着色器) 的 VS 输出，PS 输入参数结构体，注意语义要逐一锚定，不然参数会传递失败！
struct VS_To_PS
{
	float4 NDCPosition : SV_Position;	// NDC 空间坐标
	float2 TexcoordUV : TEXCOORD;		// 纹理 UV
	
	// 像素最终要采样的纹理，在纹理数组的索引 (nointerpolation 表示此参数禁止在光栅化阶段插值)
	nointerpolation uint FinalSampleTexture_InArrayIndex : ARRAYINDEX;
};


// 顶点着色器 (逐顶点输入，IA 输入装配 -> VS 顶点着色器 -> RS 光栅化)
// 任务：将顶点变换到齐次裁剪空间，并根据 当前实例 和 面索引 计算最终用于纹理采样的索引，之后都传入到下一个阶段
// 输入顶点的数量由 IA 阶段决定：
// 我们现在用的三角形列表，有索引缓冲区的情况下，顶点数量 = 索引数量；没有索引缓冲区，顶点数量 = 顶点缓冲区的顶点数量
// 如果用的是其他图元拓扑类型，IA 顶点传递会更加复杂，以后用到再讨论
VS_To_PS VSMain(IA_To_VS VSInput)
{
	// VS 输出到 PS 的结构体
	VS_To_PS VSOutput;
	
	// 顶点累加偏移，这样就得到了实例顶点相对世界空间的坐标 (其实这个位移是丐版 ModelMatrix)，注意 w 分量不累加
	// (实例混合，将副本的 Position 数据与实例部分数据 BlockOffset 混合)
	VSInput.Position.xyz += VSInput.BlockOffset;
	// 顶点累乘 MVP 矩阵，变换到齐次裁剪空间，光栅化会进行透视除法变换到 NDC 空间，然后插值
	VSOutput.NDCPosition = mul(VSInput.Position, MVPMatrix);
	
	// 纹理 UV 不变，直接赋值，光栅化会进行插值 
	// (实例混合，直接使用副本的 TexcoordUV 数据)
	VSOutput.TexcoordUV = VSInput.TexcoordUV;
	
	// 得到该顶点最终用于采样的纹理索引，我们指定 nointerpolation 之后光栅化就不会对这个参数插值了 
	// (实例混合，将副本数据 FaceIndex 与实例部分数据 BlockType 混合)
	VSOutput.FinalSampleTexture_InArrayIndex =
		BlockCubeTexture_IndexGroup[VSInput.BlockType].FaceTexture_InArrayIndex[VSInput.FaceIndex];
	
	// 输出顶点到光栅化阶段
	return VSOutput;
}


// 纹理数组 2D Texture Array，实际上只是一个记录特殊信息，合并多个纹理的大纹理资源，UVW 这三个分量都用到了
Texture2DArray m_TextureArray : register(t1, space0);
// 采样器 (邻近点过滤)
SamplerState m_sampler : register(s0, space0);


// 像素着色器 (逐像素输入，RS 光栅化 -> PS 像素着色器 -> OM 输出合并)
// 任务：采样对应纹理并得到颜色，之后连带像素的其他信息传入到输出合并阶段
float4 PSMain(VS_To_PS PSInput) : SV_Target
{
	// 根据采样器和纹理 UVW 进行纹理采样，W 坐标是纹理在数组中的索引 (Slice Index 切片索引)
	// 如果 W 坐标有小数部分，HLSL 会强制截断取整，不会进行插值或报错
	return m_TextureArray.Sample(m_sampler, float3(PSInput.TexcoordUV, PSInput.FinalSampleTexture_InArrayIndex));
}


