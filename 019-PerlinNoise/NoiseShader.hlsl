
// (19) PerlinNoise: 初步学习计算着色器、UAV Resource 无序访问资源、Readback Heap 回读堆，理解并运用柏林噪声生成简单的地形网格
// NoiseShader.hlsl: 用柏林噪声生成高度图的 shader


// 用于计算着色器的常量缓冲
cbuffer CSGlobalData : register(b0, space0)
{
	// 类似原版游戏的世界种子，在 cpp 端通过根常量传递进来，实现噪声生成的可控随机化
	// 在 Minecraft 中，世界生成的核心就是确定性 —— 相同的种子，无论在何时何地生成，都会得到完全相同的地形
	// 要实现这一点，关键在于让所有随机过程 (包括噪声生成) 都依赖于一个固定的初始值 (种子)
	// 而不依赖任何外部不可控因素 (如时间、线程调度顺序等)
	uint WorldSeed;
}


// 带种子版本的哈希函数，将二维整数映射到 [0, 1) 的伪随机浮点数，可以通过上面的 WorldSeed 控制
// 下面的算法说人话，就是通过一系列数学变换，把一个二维坐标"搅拌"成一个看似随机的数
float HashWithSeed(int2 position)
{
	// 用于混合的位掩码，0xff = 255 = 11111111，意思是只取 8 个比特位的数字
	const uint BlendMask = 0xff;
	
	// 将种子转换为二维浮点数，uint 是 32 位整数，我们只取它的 0-15 位
	// WorldSeed & BlendMask: 只取 WorldSeed 的 0-7 位
	// (WorldSeed >> 8) & BlendMask: 只取 WorldSeed 的 8-15 位
	float2 BlendSeed = float2(WorldSeed & BlendMask, (WorldSeed >> 8) & BlendMask);
	
	// 将 position 强制转换为 float2，并与种子混合生成哈希因子
	float2 MixedHashFactor = float2(position) + BlendSeed;
	
	// HLSL 没有 cpp 端生成随机数的内置函数，我们需要自己写算法，在着色器中生成伪随机数
	// (那个内置的 noise 函数在 HLSL 2.0 就被弃了，补全代码看到了也别用，绝大部分硬件都不支持它)
	// 下面这几行算法叫"正弦哈希"，是一种在着色器中常见的伪随机数生成算法
	// 它利用正弦函数的非线性特性，将一个二维坐标映射到一个看似随机的 [0,1) 范围内的浮点数
	
	// 下面 127.1，311.7，43758.5453 这些魔法数字最早流行于 ShaderToy (一个在线着色器编程社区)
	// 当时 GPU 编程仍属新兴领域，开发者们需要在没有纹理或复杂算法的情况下快速生成随机数
	// 正弦函数因其非线性且计算快 (GPU 硬件加速支持)，成了理想的"混淆器"
	// 通过反复试验，人们发现将输入坐标乘以一组精心挑选的大常数，再取正弦的小数部分，可以产生分布相当均匀的伪随机数
	// 常数的选择通常遵循几个原则：
	// 1.使用无理数或大质数，可以打破输入坐标的规律性，避免周期性条纹；
	// 2.常数之间互质，混合后不易产生重复模式；
	// 3.数值大小适中，既能放大差异，又不会导致浮点数溢出
	// 127.1、311.7、43758.5453 这套组合具体是谁首次提出的已难考证，但它很快成为社区公认的"标准配方"
	// 在 ShaderToy 上千个作品中反复出现，并逐渐渗透到各种教程和代码库中
	
	// 哈希因子与常数向量进行点积，让输入值在点积运算中充分混合，产生一个足够混乱的标量
	// (127.1, 311.7) 是两个精心选择的近似质数
	// Scrambling (Scramble 扰乱，指随机分布)，Scalar 标量，ScramblingScalar 混淆标量
	// 点积的结果是一个浮点数，它综合了 MixedHashFactor.x 和 MixedHashFactor.y 的信息，
	// 并且因为常数不是整数，结果对 MixedHashFactor 的微小变化非常敏感
	float ScramblingScalar = dot(MixedHashFactor, float2(127.1, 311.7));
	
	// 由于点积的结果通常很大，正弦函数会输出一个看似不相关的值，起到"混淆"作用
	// 正弦的周期性可能会引入重复模式，但通过选择适当的常数可以尽量降低这种模式的可视性
	// 43758.5453 是一个很大的近似无理数，同样经过精心选择
	// 乘法进一步放大了数值，将正弦值的微小变化扩散到更大的范围，使得后续取小数部分时分布更均匀
	float SinScramblingValue = sin(ScramblingScalar) * 43758.5453;

	// frac() 取结果的小数部分，截断整数部分，得到 [0, 1) 范围内的浮点数
	// 这一步最终输出一个伪随机数：对于相同的输入，输出永远相同；对于不同的输入，输出看起来是随机的
	return frac(SinScramblingValue);
	
	// 为什么这样能产生随机数？
	// 1.正弦函数对输入非常敏感：输入微小的变化会导致输出剧烈波动；
	// 2.大常数乘法将这种波动放大，使得小数部分在 [0, 1) 内几乎均匀分布
	// 3.整个过程是确定性的，所以它本质上是一个哈希函数，而非真正的随机数生成器，但在着色器中足够我们使用了
	// 不过这种方法并非完美，如果输入是规整的整数网格，可能会产生轻微的周期性图案，不过这没有什么大问题
}


// 根据上面的哈希函数，生成随机单位向量 (均匀分布在单位圆上)
// 在 cpp 端的注释就已经说过，柏林噪声的核心是随机梯度向量
// 你有可能纳闷，值噪声有块状结构，那我把原来的线性插值，改为高阶函数插值不就行了吗？为什么说柏林噪声比值噪声更自然？
// 作者用 matlab 对这两个噪声做了一维剖面图和频谱分析，发现即使在高阶函数的插值下，值噪声消除了块状结构，变成了曲线
// 但高峰和低谷实在是太多了，很容易就能看出来，一个高峰后面马上就可能来一个落差超级大的低谷，此起彼伏，视觉上会很刺眼
// 归根结底是因为即使在高阶函数的加持下，值噪声虽然保持了"曲线本身"和"一阶导数"的连续性，但"二阶导数"往往不连续
// 为什么说这个"二阶导数的连续性"是区分值噪声和柏林噪声的关键呢？
// 曲线本身相当于一座连绵不断的山脉，山脉是此起彼伏的，没有出现诸如裂谷这些断层，连续
// 一阶导数相当于山脉每座山的坡度，地形的倾斜方向在边界处也一致，所以不会出现一个陡坎或折痕，连续
// 问题出在二阶导数 —— 坡度的变化率，地形的倾斜速度会发生突变，这个是不连续的
// Minecraft 的地形是由无数个 16x16 的区块拼接而成的，每个区块内部的计算只用到自己周围的顶点
// 如果 Minecraft 使用值噪声，当你翻山越岭，刚刚好跨过一个区块，你会发现左边的区块变陡的速度越来越快，而右边的突然越来越慢
// 走起来会很怪，而柏林噪声使用的基于梯度向量插值，保证了二阶甚至更高阶导数连续，坡度变化率是连续的，用柏林噪声走起来会非常舒服
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
	// return 6 * pow(t, 5) - 15 * pow(t, 4) + 10 * pow(t, 3);
	// 这里不用 pow 是因为这玩意开销大，它在 GPU 上通常通过 exp(y * log(x)) 来实现，需要调用昂贵的超越函数
	// pow 可能还会有更大的舍入误差，对于这种小整数指数乘法，为了确保最高效的代码，显式写出乘法是最可靠的
	// GPU 通用计算对算法性能是很敏感的，这样做有利于减少函数调用和分支、提升线程束利用率、更好地加速计算
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
	// 注意是小数部分！用完整网格坐标会使平滑曲线函数的输入远大于预期的 [0,1] 范围，从而产生极大的噪声值，高度暴增
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



// 计算着色器是渲染管线一个独立阶段，常用于 GPU 通用计算，各种应用场景概念上的"万金油"，非常重要
// CSMain 是计算着色器的入口函数，当我们在 CPU 端调用 ID3D12GraphicsCommandList::Dispatch 的时候
// GPU 会启动大量的线程并行执行这个函数，每个线程负责处理一个独立的任务

// 用于 GPU 读写的 UAV 高度图资源，每个像素存储一个 32 位 float，UAV 资源都会带一个 RW 可读可写前缀
// 其实我们在前面没有说到的是，SRV 的 Texture2D 也是一个模板类，只不过我们默认它是 float4...
RWTexture2D<float> m_HeightMap : register(u0, space0);


// [numsthreads(10, 10, 1)] 这种写在函数声明前，用方括号包裹的语法，在 HLSL 中叫 Attribute 属性
// 它类似于 C#/java 中的 Annotation 注解 或者是 C/C++ 的 #pragma 指令
// 它可以定义行为约束，告诉编译器此函数应被如何特殊处理，或者是启动特定功能与优化
// [numsthreads(10, 10, 1)] 表示一个 ThreadGroup 线程组包含 10x10x1 = 100 个子线程

// 线程组是 GPU 执行的基本调度单元，这种设计能最大限度地利用硬件资源、隐藏内存访问延迟，并允许线程间高效协作
// GPU 的核心是大量的 ALU (Arithmetic Logic Unit) 算术逻辑单元，但它们并非独立运行
// 现代 GPU 实际上是由多个 SM (Streaming Multiprocessor) 流多处理器 组成的
// GPU 并不是每次调度一个线程，而是将线程组内的线程进一步打包成更小的单元 (线程束)
// 这些线程在同一时刻必须执行完全相同的指令，只是处理的数据不同
// 硬件调度器直接操作的是这些线程束，而线程组正是这些线程束的集合

// 同一个线程组内的线程运行在同一个 SM 上，这意味着同一个线程组的不同线程，可以互相访问，共享内存
// 组内的线程可以通过 groupshared 共享内存进行极低延迟的数据交换
// 不仅如此，组内的线程可以通过 GroupMemoryBarrierWithGroupSync() 等函数进行组内同步，
// 确保所有线程都执行到某一点后再继续，这对于需要分步计算的算法 (如归约、扫描) 至关重要 (不得不说计算着色器是真的强大)

// 那如何区分线程组每个线程呢? SV_DispatchThreadID 这个系统语义就可以区分
// 它表示 唯一标识当前线程在所有被调度的线程中的全局唯一 ID (注意不是单个线程组，是全局被调度的线程)
// 它的 xyz 分量代表当前线程在整个调度范围内的三维全局索引
// xyz = 线程组索引 * 被调度的尺寸 + 单个线程在组内的索引 (在我们这里 z = 0，2D 纹理不用这个 z 坐标)
// 在我们这里，每个 GPU 线程，都逐一对应 UAV 纹理 (m_HeightMap 高度图) 中相同 UV 坐标的像素
// CurrentGPUThreadID.xy = m_HeightMap.uv


// 计算着色器 (每个分配的线程组包含 10x10x1=100 个线程)
// 任务：获取当前线程对应的 ID (网格坐标)，然后进行柏林噪声获取并处理高度，最后将高度写入 UAV 高度图对应像素
[numthreads(10, 10, 1)]
void CSMain(uint3 CurrentGPUThreadID : SV_DispatchThreadID)
{
	// 地形起伏频率，表示地形变化的快慢，这个可以调，取值范围 [0.005, 0.5]
	// 0.005 ~ 0.01：	地形极其平缓，起伏非常缓慢 (大型大陆、草原、海洋盆地)
	// 0.02 ~ 0.05：		适中的起伏，出现明显的丘陵和小山包 (普通的野外环境、乡村地形)
	// 0.1 ~ 0.2：		地形崎岖，山峰和山谷密集，变化剧烈 (山地、高原、峡谷)
	// 0.3 ~ 0.5：		噪声频率过高，地形破碎，出现大量细小尖峰 (特殊地貌，例如岩石纹理、珊瑚礁)
	const float TerrainFrequency = 0.03;
	
	
	// 1.将当前线程 ID 映射到地形坐标，与 TerrainFrequency 相乘可以控制地形
	float2 WorldPosition = float2(CurrentGPUThreadID.x, CurrentGPUThreadID.y) * TerrainFrequency;
	
	
	// 2.调用柏林噪声随机获取地形高度，注意这个原始噪声高度不能直接用，后面还要进行一次处理
	float RawNoiseHeight = PerlinNoise(WorldPosition);
	
	
	// 3.归一化高度，将原始噪声高度进行一次映射处理，从 [-0.7, 0.7] 映射到 [0, 1]
	// 为什么是 raw * 0.5 + 0.5? 精确映射实际上大概是 raw / 1.4 + 0.5 这个式子
	// 但许多实现 (尤其是改进版，包括我们这个) 会使实际输出更接近 [-1, 1]，且视觉上差异极小
	// 我们要把高度先进行一次归一化，这样后续好做实际高度范围的映射
	float NormalizedHeight = RawNoiseHeight * 0.5 + 0.5;
	
	
	// 4.将高度放大到我们想要的实际范围，我们的范围是 [0, 24] 这一波动区间
	// 公式其实就是线性插值公式: Height * (max - min) + min，这里我们直接用内置函数 lerp 就直观方便多了
	float Height = lerp(0, 24, NormalizedHeight);
	
	
	// 5.每个线程都对应高度图的一个像素，将计算得到的高度，写入对应坐标的纹理像素上
	m_HeightMap[CurrentGPUThreadID.xy] = Height;
}



