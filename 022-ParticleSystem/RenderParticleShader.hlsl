
// (22) ParticleSystem: 进一步学习计算着色器，学习粒子的动态生成与销毁，模拟 Minecraft 的粒子破坏效果
// RenderParticleShader.hlsl: 渲染粒子的 shader


// 用于 MVP 矩阵、方块破坏阶段、方块朝向的常量缓冲
cbuffer GlobalData : register(b0, space0)
{
	// 摄像机提供 MVP 矩阵，将顶点从世界空间变换到齐次裁剪空间
	row_major float4x4 MVPMatrix;
	// 方块朝向 (0-5 分别对应 右左前后上下，6-8 用于正面朝上的特殊方块)，存储的旋转到对应朝向的旋转矩阵
	row_major float4x4 BlockFaceForwardMatrix[9];
	// 方块破坏阶段使用纹理的索引，在 DestroyStageShader.hlsl 会用到，这个 shader 不需要管
	uint DestroyStage;
}


// 立方体面结构体，数组索引表示对应的立方体面，数组元素值表示 对应面所用纹理 指向 纹理数组 的索引
struct CUBEFACE
{
	// 数组索引 0-5 分别对应右面 (+X)，左面 (-X)，前面 (+Z)，后面 (-Z)，上面 (+Y)，下面 (-Y)
	uint FaceTexture_InArrayIndex[6];
};


// GPU 上的方块类型-纹理索引组 (SRV Structured Buffer)
StructuredBuffer<CUBEFACE> BlockCubeTexture_IndexGroup : register(t0, space0);


// 粒子实例结构体
struct Particle
{
	float3 Position;		// 粒子的世界坐标
	float3 Velocity;		// 粒子的速度
	float life;				// 剩余生命 (单位：秒)，大于 0 表示存活
	uint BlockType;			// 所属方块类型
};


// 完成计算准备渲染的粒子缓冲区，此时这个资源是 SRV 资源，由 SRV Descriptor 描述
StructuredBuffer<Particle> m_ParticlesBuffer : register(t2, space0);


// (IA 输入装配 -> VS 顶点着色器) 的 VS 输入参数结构体，注意语义要逐一锚定，不然参数会传递失败！
struct IA_To_VS
{
	// 输入槽 0 (顶点流)
	float4 Position : POSITION;		// 方块副本的顶点位置，以 (0, 0, 0) 为中心
	float2 TexcoordUV : TEXCOORD;	// 纹理 UV
	uint FaceIndex : FACEINDEX;		// 顶点所属的立方体面索引
	
	// 粒子实例在 m_ParticlesBuffer 的索引，我们靠它获取每个粒子实例的数据
	// SV_InstanceID 是一个系统值，指定每个粒子实例索引 (从 0 开始) 由 GPU 自动生成
	// 这个值不是从顶点缓冲区或实例缓冲区读取的，因此不需要在 PSO 输入布局中描述
	uint ParticleInstanceIndex_InBuffer : SV_InstanceID;
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
// 任务：从 m_ParticlesBuffer 获取实例数据，将顶点变换到齐次裁剪空间，对粒子进行特殊处理，之后进入光栅化
VS_To_PS VSMain(IA_To_VS VSInput)
{
	// VS 输出到 PS 的结构体
	VS_To_PS VSOutput;
	
	// 从粒子缓冲中获取当前要处理的粒子
	Particle p = m_ParticlesBuffer[VSInput.ParticleInstanceIndex_InBuffer];
	
	// 如果粒子存活，就渲染
	if(p.life > 0)
	{
		// 粒子是非常小的几何体，先进行缩小，0.07 是作者试出来的经验值，让粒子接近原版的小碎块
		VSInput.Position.xyz *= 0.07;
		// 实例混合，粒子偏移到对应的世界坐标
		VSInput.Position.xyz += p.Position;
		// 与 MVP 矩阵相乘，得到齐次裁剪空间下的坐标
		VSOutput.NDCPosition = mul(VSInput.Position, MVPMatrix);
		
		// 粒子的 UV 坐标只采样纹理左下角一部分区域
		// clamp: 将输入的 UV 值，TexcoordU 截断到 [0.0, 0.875]，TexcoordV 截断到 [0.125, 1.0]
		VSOutput.TexcoordUV = clamp(VSInput.TexcoordUV, float2(0.0, 0.875), float2(0.125, 1.0));
		
		// 得到该顶点最终用于采样的纹理索引
		VSOutput.FinalSampleTexture_InArrayIndex =
			BlockCubeTexture_IndexGroup[p.BlockType].FaceTexture_InArrayIndex[VSInput.FaceIndex];
	}
	else	// 否则说明粒子死亡或未使用，直接返回空结构体，将所有顶点设置为同一位置，光栅化会进行裁剪
	{
		VSOutput.NDCPosition = float4(0, 0, 0, 1);
		VSOutput.TexcoordUV = float2(0, 0);
		VSOutput.FinalSampleTexture_InArrayIndex = 0;
	}
	
	// 输出顶点到光栅化阶段
	return VSOutput;
}



// 纹理数组 2D Texture Array
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