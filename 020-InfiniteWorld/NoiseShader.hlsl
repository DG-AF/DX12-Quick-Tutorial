
// (20) InfiniteWorld: 进一步学习计算着色器，掌握动态资源的管理，认识 UAV 纹理数组与结构化缓冲区，模拟 MC 无限世界的生成
// NoiseShader.hlsl: 用柏林噪声生成 UAV 区块高度纹理数组的 shader


// 用于计算着色器的常量缓冲
cbuffer CSGlobalData : register(b0, space0)
{
	// 类似原版游戏的世界种子，在 cpp 端通过根常量传递进来，实现噪声生成的可控随机化
	uint WorldSeed;
}


// 带种子版本的正弦哈希函数，将二维整数映射到 [0, 1) 的伪随机浮点数，可以通过上面的 WorldSeed 控制
float HashWithSeed(int2 position)
{
	// 用于混合的位掩码，0xff = 255 = 11111111，意思是只取 8 个比特位的数字
	const uint BlendMask = 0xff;
	
	// 将种子转换为二维浮点数，
	float2 BlendSeed = float2(WorldSeed & BlendMask, (WorldSeed >> 8) & BlendMask);
	
	// 将 position 强制转换为 float2，并与种子混合生成哈希因子
	float2 MixedHashFactor = float2(position) + BlendSeed;
	
	// 哈希因子与常数向量进行点积，让输入值在点积运算中充分混合，产生一个足够混乱的标量
	float ScramblingScalar = dot(MixedHashFactor, float2(127.1, 311.7));
	
	// 由于点积的结果通常很大，正弦函数会输出一个看似不相关的值，起到"混淆"作用，大质数相乘会放大范围
	float SinScramblingValue = sin(ScramblingScalar) * 43758.5453;

	// frac() 取结果的小数部分，截断整数部分，得到 [0, 1) 范围内的浮点数
	return frac(SinScramblingValue);
}


// 根据上面的哈希函数，生成随机单位向量 (均匀分布在单位圆上)
float2 RandomGradientVector(int2 ConnerPointPosition)
{
	// 圆周率
	const float PI = 3.14159265359;
	
	// 生成随机角度，范围 [0, 2PI)
	float RandomAngle = HashWithSeed(ConnerPointPosition) * 2 * PI;
	
	// 返回一个在单位圆的单位向量
	return float2(cos(RandomAngle), sin(RandomAngle));
}


// 高阶平滑曲线函数 f(t) = 6t^5 - 15t^4 + 10t^3，用于权重插值，这个也是 Perlin 本人推荐的函数
float SmoothstepCurve(float t)
{
	return t * t * t * (t * (t * 6 - 15) + 10);
}


// 柏林噪声，用于更平滑自然的地形生成，根据输入的网格点 (采样点) 坐标，生成对应的高度，范围大概是 [-0.7, 0.7]
float PerlinNoise(float2 GridPointPosition)
{
	// 1.计算这个点所属网格四个角的整数坐标
	// floor() 向下取整，L = LEFT、R = RIGHT、U = UP、D = DOWN，LU = LEFT-UP 左上角，其他以此类推
	int2 GridConner_LU = int2(floor(GridPointPosition.x), floor(GridPointPosition.y));
	int2 GridConner_RU = GridConner_LU + int2(1, 0);
	int2 GridConner_LD = GridConner_LU + int2(0, 1);
	int2 GridConner_RD = GridConner_LU + int2(1, 1);
	
	
	// 2.获取四个角点的随机梯度向量，这个是柏林噪声的核心
	float2 GradientVector_LU = RandomGradientVector(GridConner_LU);
	float2 GradientVector_RU = RandomGradientVector(GridConner_RU);
	float2 GradientVector_LD = RandomGradientVector(GridConner_LD);
	float2 GradientVector_RD = RandomGradientVector(GridConner_RD);
	
	
	// 3.计算每个角点到网格点的偏移向量，偏移向量是指从网格角点指向采样点的向量
	float2 OffsetVector_LU = GridPointPosition - float2(GridConner_LU);
	float2 OffsetVector_RU = GridPointPosition - float2(GridConner_RU);
	float2 OffsetVector_LD = GridPointPosition - float2(GridConner_LD);
	float2 OffsetVector_RD = GridPointPosition - float2(GridConner_RD);
	
	
	// 4.计算每个角点的贡献，贡献是指每个角点 梯度向量 和 偏移向量 的点积
	// 如果向量方向一致，点积结果越大，越有可能出现"山峰"; 向量方向相反，点积结果越小，越有可能出现"山谷"
	// 这种思路模仿了自然界中物质状态的相互作用：某一点的状态不仅取决于它本身，还受到周围物质运动方向的影响
	float Weight_LU = dot(GradientVector_LU, OffsetVector_LU);
	float Weight_RU = dot(GradientVector_RU, OffsetVector_RU);
	float Weight_LD = dot(GradientVector_LD, OffsetVector_LD);
	float Weight_RD = dot(GradientVector_RD, OffsetVector_RD);
	
	
	// 5.用上面的高阶平滑函数，计算采样点的小数部分对应的插值权重，这样就得到了平滑的曲线
	// 注意是小数部分！用完整网格坐标会使平滑曲线函数的输入远大于预期的 [0, 1) 范围，从而产生极大的噪声值，高度暴增
	float2 GridFracPos = frac(GridPointPosition);
	float Smoothstep_GridU_LerpFactor = SmoothstepCurve(GridFracPos.x);
	float Smoothstep_GridV_LerpFactor = SmoothstepCurve(GridFracPos.y);
	
	
	// 6.最后对噪声生成的贡献，进行类似纹理那样的双线性插值，得到最终的高度值
	// 先对上下两个横轴 (TextureU) 的网格角点对应的贡献，根据 GridU 分别进行一次插值
	float Weight_GridUpConner = lerp(Weight_LU, Weight_RU, Smoothstep_GridU_LerpFactor);
	float Weight_GridDownConner = lerp(Weight_LD, Weight_RD, Smoothstep_GridU_LerpFactor);
	// 再对这两个插值点，在竖轴方向 (TextureV) 上根据 GridV 进行一次插值
	float FinalHeight = lerp(Weight_GridUpConner, Weight_GridDownConner, Smoothstep_GridV_LerpFactor);
	
	
	// 返回最终高度，原始范围大概是 [-0.7, 0.7]，这个范围是经典 2D 柏林噪声的理论近似极值，实际可能会有波动
	// 它来源于算法中每个步骤数学运算的极限情况，这个结果的推导要看其他资料，这里不涉及
	return FinalHeight;
}



// ---------------------------------------------------------------------------------------------------------------



// 待加载区块坐标信息的结构体缓冲，存储需要加载的区块左上角坐标信息 (上传堆资源)
// 上传堆资源 (普通缓冲可以，纹理不行) 可以做 SRV 描述符，并且可以做持续化映射，动态加载，不用一直 Map-Unmap
StructuredBuffer<int2> m_ReadyCreateChunkBuffer : register(t0, space0);

// 用于输出到 CPU 的高度图纹理数组，每个元素表示一个 16x16 一个区块的高度图纹理
RWTexture2DArray<float> m_HeightTextureArray : register(u0, space0);


// 用于地形生成的计算着色器 (每个分配的线程组包含 16x16x1=256 个线程，相当于一个区块的大小)
// SV_GroupID: 系统语义，表示当前线程所属线程组的索引 (线程组索引 = 纹理数组元素索引)
// SV_GroupThreadID: 系统语义，表示当前线程在线程组中的索引 (组内线程索引 = 纹理像素索引)
// 任务：获取当前线程对应的 ID (网格坐标)，然后进行柏林噪声获取并处理高度，最后将高度写入 UAV 高度图对应像素
[numthreads(16, 16, 1)]
void NoiseCSMain(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
	// 地形起伏频率，表示地形变化的快慢，这个可以调，取值范围 [0.005, 0.5]
	// 0.005 ~ 0.01：	地形极其平缓，起伏非常缓慢 (大型大陆、草原、海洋盆地)
	// 0.02 ~ 0.05：		适中的起伏，出现明显的丘陵和小山包 (普通的野外环境、乡村地形)
	// 0.1 ~ 0.2：		地形崎岖，山峰和山谷密集，变化剧烈 (山地、高原、峡谷)
	// 0.3 ~ 0.5：		噪声频率过高，地形破碎，出现大量细小尖峰 (特殊地貌，例如岩石纹理、珊瑚礁)
	const float TerrainFrequency = 0.03;
	
	
	// 获取当前线程组索引，线程组索引 = 要输出的 UAV 纹理数组索引 = SRV 结构体缓冲元素索引
	uint HeightMapIndex = GroupID.x;
	
	// 获取组内线程索引，组内线程索引 = 纹理内每一个像素的坐标
	uint2 HeightMapTexcoord = GroupThreadID.xy;
	
	
	// 1.根据待加载区块坐标信息的结构体缓冲，将当前线程 ID 映射到区块地形坐标
	// 注意这里！要将 uint 转换成 int！不然处理负数坐标的区块会出现巨大的正整数，导致噪声失效！
	int2 ChunkPosition = int2(HeightMapTexcoord) + m_ReadyCreateChunkBuffer[HeightMapIndex];
	
	// 2.获取当前坐标对应的网格坐标，与 TerrainFrequency 相乘可以控制地形
	float2 WorldPosition = float2(ChunkPosition) * TerrainFrequency;
	
	// 3.调用柏林噪声随机获取地形高度，注意这个原始噪声高度不能直接用，后面还要进行一次处理
	float RawNoiseHeight = PerlinNoise(WorldPosition);
	
	// 4.将原始噪声值转换成地形高度
	// 乘 8 表示放大噪声振幅，地形起伏更大，更明显
	// 加 8 表示把整体高度基线抬到 8 以上，地形平均高度为 8
	// clamp 表示截断地形高度，限制地形高度在 [0, 24] 之间，不让越界
	float Height = clamp(8 + RawNoiseHeight * 8, 0, 24);
	
	// 5.将计算完成的高度写入纹理数组的像素中，全部线程完成后进行围栏同步，CPU 回读高度图纹理数组，生成方块柱
	m_HeightTextureArray[uint3(HeightMapTexcoord, HeightMapIndex)] = Height;
}



