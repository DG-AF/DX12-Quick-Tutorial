
// (21) GPUFrustumCulling: 认识视锥剔除，认识命令签名，UAV 原始缓冲区，学会 GPU 视锥剔除，更好地渲染地图扩大的无限世界
// CullingShader.hlsl: 进行 GPU 视锥剔除，将绘制命令填充到命令缓冲区的 shader


// View Frustum Culling 视锥体剔除，是三维图形渲染中将完全处于视锥外的物体移出渲染流程的技术
// 该技术通过减少不可见物体的渲染计算，提升图形处理效率
// 将摄像机视锥体 (由六个平面构成的类似呈现金字塔形状的 3D 几何体) 与场景中物体的包围盒 (如 AABB 包围盒)
// 逐一进行相交测试，如果物体完全不在视锥体内，说明它不会被摄像机看到，就可以跳过该物体的渲染，从而节省 GPU 资源

// 传统的剔除方式由 CPU 负责 (CPU Culling)，CPU 遍历所有物体，执行相交测试
// 实现比较简单，但是当物体数量巨大时 (例如无限世界的成千上万个区块)，CPU 会成为性能瓶颈

// 我们现在使用的剔除由 GPU 负责 (GPU Culling)，GPU 上的计算着色器遍历所有物体，执行相交测试，CPU 只传递元数据
// CPU 将物体的元数据 (元数据是一种很小量的描述数据，相当于描述符，不存储具体的顶点索引实例数据) 存储在 GPU 缓冲区中
// 利用计算着色器并行测试所有物体，对可见的物体，将绘制命令直接写入间接命令缓冲区
// 最后通过 ExecuteIndirect 一次性提交所有可见物体的绘制



// 在传统渲染中，CPU 需要为每个可见物体/一堆实例调用一次 DrawIndexedInstanced 命令，
// 当物体数量巨大时，CPU 会成为性能瓶颈，即使实例化也只能减轻一点压力

// DirectX 12 的 ExecuteIndirect 间接命令机制改变了这一模式，它让发号施令的部分指挥权，从 CPU 转移到了 GPU
// 命令缓冲区 (Command Buffer) 是一块 GPU 内存，里面存储着绘制命令的参数 (D3D12_DRAW_INDEXED_ARGUMENTS)
// 计算着色器可以像写入数据一样，将可见物体的绘制命令填充到命令缓冲区中
// 之后，CPU 只需调用一次 ExecuteIndirect，GPU 就会自动解析命令缓冲区，并依次执行其中的所有绘制命令
// 这样就可以减轻 CPU 的负载，每帧仅需一次 ExecuteIndirect 调用，不再需要循环发起绘制
// 决定绘制哪个区块，剔除哪个区块，完全由 GPU 决定，剔除与命令生成完全在 GPU 内完成，形成闭环

// GPU 视锥剔除，正是依靠 计算着色器 + ExecuteIndirect 这两个无双组合实现的，它们两个都是 GPU 驱动渲染的核心环节
// 本章也是现代渲染管线中 GPU 驱动渲染初级教程，后面甚至还能实现 GPU 绑定根常量和根描述符，非常强大



// DX12 GPU 端的绘制命令结构体信息，结构必须和 D3D12_DRAW_INDEXED_ARGUMENTS 完全相同
// 每个 D3D12_DRAW_INDEXED_ARGUMENTS，相当于一个 DrawIndexedInstanced 命令
struct D3D12_DRAW_INDEXED_ARGUMENTS
{
	uint IndexCountPerInstance;		// 实例副本索引数量
	uint InstanceCount;				// 实例数量
	uint StartIndexLocation;		// 在索引缓冲的起始索引
	int BaseVertexLocation;			// 在顶点缓冲的起始索引
	uint StartInstanceLocation;		// 在实例缓冲的起始索引
};


// DirectXCollision 的 AABB 包围盒，也是 24 字节 (好奇微软是怎么塞进成员函数，还能保持 24 字节的?)
struct BoundingBox
{
	float3 Center;	// AABB 包围盒中心点
	float3 Extents;	// AABB 包围盒 xyz 轴半边长
};


// 16x16x24 的区块信息 (区块元数据)，和 cpp 端要一致
struct CHUNK
{
	int2 ChunkXZ;				// 整个区块左上角的 xz 轴坐标，这个 shader 不需要用到
	uint InstanceOffset;		// 整个区块在实例缓冲的偏移量 (单位：方块实例)
	uint InstanceCount;			// 整个区块的实例数量 (单位：方块实例)
	int State;					// 整个区块的状态 (传统的枚举都用 int 表示)，不需要用到，CPU 处理
	BoundingBox AABBBox;		// 整个区块的包围盒，用于 GPU 视锥体裁剪 (世界坐标)
};



// 输入：所有已加载区块的元数据 (SRV 结构化缓冲区，上传堆)
StructuredBuffer<CHUNK> m_KeepChunksBuffer : register(t0, space0);

// 输出：间接绘制命令缓冲区 (UAV 结构化缓冲区，默认堆)
RWStructuredBuffer<D3D12_DRAW_INDEXED_ARGUMENTS> m_IndirectCommandBuffer : register(u0, space0);

// 输出：可见区块 (间接绘制命令) 计数器 (UAV 原始缓冲区，默认堆 4 字节资源)
// UAV RWByteAddressBuffer 是一个可读写的原始缓冲区，相当于 C/C++ 的 void*，支持原子操作
// 原子操作是指在多线程 (或多线程组) 并发访问同一内存地址时，能够完整地、不可分割地完成读-改-写整个过程的操作
// 在 GPU 中，成百上千个线程可能同时执行同一段代码，并试图修改同一个变量。如果没有原子操作，就会出现数据竞争
// 原子操作保证这三个步骤一气呵成，中间不会被其他线程打断
// 硬件会锁住该内存地址，确保只有一个线程能完成整个操作，其他线程必须等待
// 原子操作是并发环境下安全共享数据的基础，在 GPU 粒子系统中，正是靠它来实现线程的按顺序写入
RWByteAddressBuffer m_IndirectCommandCounter : register(u1, space0);


// 常量缓冲区，存储视锥体 6 个平面方程系数 和 已加载区块总数
cbuffer CSGlobalData : register(b0, space0)
{
	float4 FrustumPlanes[6];	// 组成视锥体的六个平面方程 (ax, by, cz, d)，法线指向视锥体内部
	uint KeepChunkCount;		// 已加载 (处于保持状态) 的区块数量
}



// 对区块 (AABB 包围盒) 进行保守视锥剔除的函数，如果可见返回 true，不可见返回 false
// "保守" 的意思是 "不会错误地剔除一个实际可见的物体，但可能会保留一些实际不可见的物体，宁可多画，不能少画"
// 原理是利用围成视锥体的 6 个平面，它们的平面方程可以表示点与平面的相交情况
// (x, y, z, 1) 是点的坐标，(a, b, c) 表示平面的法向量
// ax + by + cz + d >= 0 表示点在平面内部/恰好在平面上，否则表示在背面
// 选择包围盒上在平面法线方向上最远的角点，对这个角点算平面方程，如果结果 < 0，那可以确定包围盒在视锥体外面了
// 你有可能好奇，为什么不计算平面法线与包围盒表面相交的点，而是选择包围盒 8 个角点之一呢？
// 因为包围盒是一个凸多面体，它在任何方向上的极值点必然出现在顶点上
// 平面法线方向就是一个特定的方向，所以该方向上的最远点必然是某个角点
// 很多人的惯性思维，让它们认为平面法线 "固定" 在了平面中心，实际上一个平面的任何地方都是有法线的
// 所以总会有一条可以穿过包围盒角点的法线，选择方向上的最远角点，如果这个最远角点也 < 0，那么说明就在视锥体外面
bool isChunkVisible(uint ChunkIndex)
{
	CHUNK CurrentChunk = m_KeepChunksBuffer[ChunkIndex];	// 当前区块元信息
	float3 center = CurrentChunk.AABBBox.Center;			// 包围盒中心
	float3 extents = CurrentChunk.AABBBox.Extents;			// 包围盒 xyz 轴半边长
	
	
	// [unroll] 是一个循环展开属性，用于告诉编译器将循环完全展开，用重复的代码序列直接替代循环结构
	// 使用得当 (当循环迭代次数很少且固定时，类似下面这样)，展开后可以避免分支预测失败，提高执行效率，消除开销
	// 不过这个 [unroll] 会导致 fxc 编译器误报:
	// warning X4000: use of potentially uninitialized variable (isChunkVisible)
	// 这其实是编译器的一个解析混淆，根源在于 [unroll] 属性改变了编译器对代码的早期处理方式，不用管它
	[unroll]
	for (uint i = 0; i < 6; i++)
	{
		float4 plane = FrustumPlanes[i];	// 平面方程
		float3 normal = plane.xyz;			// 平面法线
		
		// 计算 AABB 包围盒在平面法线上的最远角点 (中心点 + 法线方向上的半边长)
		float3 FarthestConner = center + float3(
			(normal.x > 0) ? extents.x : -extents.x,
			(normal.y > 0) ? extents.y : -extents.y,
			(normal.z > 0) ? extents.z : -extents.z
		);

		
		// 将角点带入对应的平面方程 ax + by + cz + d，如果 < 0，说明测试失败，包围盒就在视锥体外面，直接剔除
		if(dot(normal, FarthestConner) + plane.w < 0)
		{
			return false;
		}
	}
	
	
	// 所有 6 个视锥体平面检测通过，说明就在视锥体内部/与视锥体相交
	return true;
}



// 用于 GPU 视锥剔除的计算着色器 (每个分配的线程组包含 64 个线程，每个线程处理一个区块)
// SV_DispatchThreadID: 系统语义，表示在全局线程组调度内，当前线程的全局索引
// 任务：获取当前线程对应的 ID (区块索引)，对当前处理的区块进行视锥体包围检测，处于可见范围就填充一条绘制指令
[numthreads(64, 1, 1)]
void CullingCSMain(uint3 GlobalThreadID : SV_DispatchThreadID)
{
	// 如果当前线程索引已经超过区块数量了，不处理，直接返回
	if(GlobalThreadID.x >= KeepChunkCount)
	{
		return;
	}
	
	// 获取当前要处理的区块坐标
	uint ChunkIndex = GlobalThreadID.x;
	
	
	// 如果当前区块实例数量为 0，说明这个区块是 "空泡"，不处理，直接返回
	if (m_KeepChunksBuffer[ChunkIndex].InstanceCount == 0)
	{
		return;
	}
	
	
	// 如果区块 AABB 包围盒通过视锥剔除测试，说明区块可见，将区块加入到绘制指令，否则不处理 (视锥剔除)
	if(isChunkVisible(ChunkIndex))
	{
		// 新绘制指令 (DrawIndexedInstanced)
		D3D12_DRAW_INDEXED_ARGUMENTS NewGPUDrawCall;
		// 当前区块
		CHUNK CurrentChunk = m_KeepChunksBuffer[ChunkIndex];
		
		NewGPUDrawCall.IndexCountPerInstance = 36;							// 每个方块 36 个索引
		NewGPUDrawCall.InstanceCount = CurrentChunk.InstanceCount;			// 区块内方块实例数量
		NewGPUDrawCall.StartIndexLocation = 0;								// 起始索引为 0
		NewGPUDrawCall.BaseVertexLocation = 0;								// 起始顶点索引为 0
		NewGPUDrawCall.StartInstanceLocation = CurrentChunk.InstanceOffset;	// 区块在实例缓冲的索引

		
		// 绘制命令将要添加到命令缓冲的索引
		uint CommandIndex;
		
		// 命令缓冲计数器进行原子递增 (m_IndirectCommandCounter++)，先返回该地址下原来的值，然后递增 1
		// 命令缓冲进行原子操作时，会锁定该 GPU 线程对资源的持有权，其他 GPU 线程要等操作完成，才能继续访问
		// 这样就保证了 GPU 线程对计数器 (包括命令缓冲) 的访问同步，不怕资源访问乱序了
		// 第一个参数 dest: 从缓冲区起始位置，要写入的偏移地址 (单位：字节)，它必须是 4 的倍数
		// 第二个参数 value: 增加值
		// 第三个参数 originalValue: 输出进行加法前的原值
		m_IndirectCommandCounter.InterlockedAdd(0, 1, CommandIndex);
		
		// 向命令缓冲添加新命令，同步访问已经由上面的 m_IndirectCommandCounter 保证了
		m_IndirectCommandBuffer[CommandIndex] = NewGPUDrawCall;
	}
}


// 用于清空 m_IndirectCommandCounter 的计算着色器 (只分配一个线程)
// 任务：清零计数器
[numthreads(1, 1, 1)]
void ClearCounterCSMain()
{
	// 清空区块计数器，准备下一帧 GPU 视锥剔除
	// Store 用于向缓冲区一个 4 字节对齐的位置，写入一个 4 字节的值，它不是原子操作
	// 第一个参数 address 表示从缓冲区起始位置的偏移地址 (单位：字节)，它必须是 4 的倍数，必须 4 字节对齐
	// 第二个参数 value 是要写入的值
	m_IndirectCommandCounter.Store(0, 0);
}



