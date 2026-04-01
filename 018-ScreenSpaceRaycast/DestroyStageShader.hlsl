
// (18) ScreenSpaceRaycast: 认识屏幕射线相交检测，学会方块的破坏与放置，理解 PSO 的 IA 输入布局复用，学会如何利用 PSO 解决深度冲突
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
// 不用担心硬件寻址问题，只要输入布局与顶点/实例缓冲区的实际数据布局严格一致，即使着色器不填某些没用到的输入参数
// 也是完全没有问题的！下面我们没有填 FaceIndex，BlockType，RotateIndex
// 你可能很好奇，为什么这里不用像上面的 BlockFaceForwardMatrix 那样需要占位，不填会不会越界/错位寻址
// 我可以明确告诉你不会，因为 GPU 端是按输入布局读的，IA 输入参数是靠 Semantic 语义精准传递参数的
// HLSL 编译器会对这些输入参数进行优化，所以即使你把下面的输入参数顺序全部打乱了，语义对上了也照样正常渲染
// 那输入布局的 AlignedByteOffset 输入槽偏移又是什么意思呢? 它其实指的是 CPU 端的输入数据的布局
// 硬件正确寻址真要靠 AlignedByteOffset 和 CPU 顶点/实例数据的排列形式完全对上才行
// 至于着色器填不填这些没用到的输入参数，与我硬件寻址无关，你不填 FaceIndex 我照样偏移 28 个字节正确寻址
// 你不填 BlockType，RotateIndex 我照样偏移 16 字节。着色器无论是不填、填了，顺序打乱了结果都是一样的
// 就是会有微不足道的带宽开销，不过没有关系，该省略的也可以省略。虽然说可以打乱，但还是建议按顺序写，可读性更好
// 记住一句话：GPU 根据输入布局从缓冲区读取数据，与着色器是否使用这些数据完全无关
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


