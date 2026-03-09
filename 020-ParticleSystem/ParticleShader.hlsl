
// (20) ParticleSystem: 进一步学习计算着色器，学习粒子的动态生成与销毁，模拟 Minecraft 的粒子破坏效果
// ParticleShader.hlsl: 生成、管理、更新并销毁粒子的 shader


// 粒子实例结构体
struct Particle
{
	float3 Position;		// 粒子的世界坐标
	float3 Velocity;		// 粒子的速度
	float life;				// 剩余生命 (单位：秒)，大于 0 表示存活
	uint BlockType;			// 所属方块类型
};


// 管理所有粒子 (包括存活和已死亡的) 的粒子缓冲区，它是一个数据有"空泡"的非连续缓冲
// UAV RWStructuredBuffer 是一个可读写的结构体缓冲区，类似上一章的 RWTexture2D
RWStructuredBuffer<Particle> m_ParticlesBuffer : register(u0, space0);


// HLSL 没有类似 std::vector 那样的顺序存储结构，我们需要自行管理数据的新增与删除
// 我们借助了 C++26 新数据结构 std::hive 的"空泡"和"空闲列表"思想，粒子死亡后不会立即像 vector 那样删除
// 而是原地变成"空泡"，新增的数据可以直接覆盖这个"空泡"，实现数据空位的复用，这样就不需要 vector 那样循环删除了
// 记录"空泡"的空闲栈 (缓冲区)，栈中每个元素记录上面 m_ParticlesBuffer 可用位置的索引
// CPU 端最开始创建时，将栈按顺序初始化为 0,1,2,3,4... (表示粒子缓冲区为空，全部都是"空泡")
RWStructuredBuffer<uint> m_IdleParticlesStack : register(u1, space0);


// 我们还需要一个 4 字节的栈顶指针，来计数最近可用"空泡"的索引，这样就很方便数据空位的复用了
// 这个栈顶指针同时还表示存活粒子数，CPU 需要通过 回读堆 和 围栏同步 持续向 GPU 获取这个数据
// 如果有新粒子，栈顶指针++，找到 m_ParticlesBuffer 新的"空泡"索引，将新粒子添加到"空泡"
// 如果检测到粒子死亡，会原地产生"空泡"，栈顶指针--，栈顶记录新的"空泡"索引
// UAV RWByteAddressBuffer 是一个可读写的原始缓冲区，相当于 C/C++ 的 void*，支持原子操作
// 原子操作是指在多线程 (或多线程组) 并发访问同一内存地址时，能够完整地、不可分割地完成读-改-写整个过程的操作
// 在 GPU 中，成百上千个线程可能同时执行同一段代码，并试图修改同一个变量。如果没有原子操作，就会出现数据竞争
// 原子操作保证这三个步骤一气呵成，中间不会被其他线程打断
// 硬件会锁住该内存地址，确保只有一个线程能完成整个操作，其他线程必须等待
// 原子操作是并发环境下安全共享数据的基础，在 GPU 粒子系统中，正是靠它来实现空闲栈的并发管理
RWByteAddressBuffer m_StackTopPointer : register(u2, space0);


// 来自 CPU 端的待生成列表 (上传堆资源，着色器只读，计算着色器不是常规管线阶段，不能通过 IA 阶段传递实例数据)
// SRV StructuredBuffer 也可以当上传堆资源，直接被 CPU 写入，不过没显存快
// 这个结构化缓冲区的作用是：用来暂存那些 CPU 产生的、还没来得及交给 GPU 的粒子数据
// 用待生成列表先把粒子"攒起来"，等到每帧的固定更新点时，再一次性把这一批粒子都交给 GPU，效率高得多
// 为什么不直接将新数据写入到 m_ParticlesBuffer？因为 m_ParticlesBuffer 里面的数据会有"空泡"，数据不是连续的
// 替代方案非常的麻烦，还不如直接 StructuredBuffer 然后 GPU 一个个写入清晰方便的多
StructuredBuffer<Particle> m_SpawnParticlesList : register(t0, space0);



// ---------------------------------------------------------------------------------------------------------------



// 用于生成新粒子的常量缓冲 (着色器只读)
cbuffer SpawnParticlesData : register(b0, space0)
{
	// 新粒子在 m_SpawnParticlesList 的数量，等会要将它们添加到 m_ParticlesBuffer 中
	uint SpawnParticlesCount;
}


// 向 m_ParticlesBuffer 添加新粒子的计算着色器，一个线程组调用 16 个 GPU 线程
// 从 CPU 上传的 m_SpawnParticlesList 中读取新粒子数据，从空闲栈分配索引，写入粒子缓冲区
// 这里 CPU 调度线程组的数量由 SpawnParticlesCount 决定
[numthreads(16, 1, 1)]
void SpawnCSMain(uint3 CurrentGPUThreadID : SV_DispatchThreadID)
{
	// 如果当前 GPU 线程的索引已经超过了新粒子的数量，后续不处理，直接跳过
	if (CurrentGPUThreadID.x >= SpawnParticlesCount)
	{
		return;
	}
	
	// 每个 GPU 线程的 id 表示 m_SpawnParticlesList 一个新粒子索引
	uint NewParticleIndex = CurrentGPUThreadID.x;
	
	// 新空泡在 m_IdleParticlesStack 的索引，这个索引会指向空闲栈的栈顶
	uint NewIdleSpaceIndex_InStack = 0;
	
	// 栈顶指针 m_StackTopPointer 进行原子操作，查询栈顶可用的空泡 (已死亡粒子产生的数据空位)
	// 返回新空泡在 m_IdleParticlesStack 的索引，同时栈顶指针原子递增，移动到下一个空泡索引
	m_StackTopPointer.InterlockedAdd(0, 1, NewIdleSpaceIndex_InStack);
	
	// 新空泡在 m_ParticlesBuffer 的索引 (本质上用的是空闲链表的思想，在 HLSL 上用"空闲栈"代替链表)
	// 这个索引会指向已死亡粒子产生的数据空位，或者是还没使用过的新数据空位
	uint NewIdleSpaceIndex_InBuffer = m_IdleParticlesStack[NewIdleSpaceIndex_InStack];

	// 向 m_ParticlesBuffer 的空泡写入新粒子数据，这样就能新增粒子
	m_ParticlesBuffer[NewIdleSpaceIndex_InBuffer] = m_SpawnParticlesList[NewParticleIndex];
}



// ---------------------------------------------------------------------------------------------------------------



// 用于更新粒子状态的常量缓冲 (着色器只读)
cbuffer UpdateParticlesData : register(b1, space0)
{
	float PerFrameDataTime;				// 相比上一帧，当前帧过去的时间，用于计算速度与位移
	float Gravity;						// 重力，用于计算速度，呈现粒子自然掉落效果
	uint MaxAllocParticleBufferSize;	// 粒子缓冲可以存储的最大粒子数量
}



// 每帧更新 m_ParticlesBuffer 所有存活粒子的计算着色器，一个线程组调用 16 个 GPU 线程
// 这里 CPU 调度线程组的数量是整个 m_ParticlesBuffer 可以存储的最大粒子数量 (就是上面的 MaxAllocParticleBufferSize)
// 为什么和上面的 SpawnCSMain 不同？因为 m_ParticlesBuffer 的粒子死亡后会原地标记"空泡"
// m_ParticlesBuffer 不像 std::vector 那样删除一个元素后会自动往前移，而是变成"空泡"方便数据复用
// 所以 m_ParticlesBuffer 的数据不是连续的，"空泡"大部分情况都和存活粒子交错排列
// 那你可能也会说：我可以再用一个 UAV 缓冲存储存活粒子的最大索引，CPU 实时获取就能减少 GPU 调度线程组的数量了
// 然而再添加这个 UAV 缓冲真的好吗？读者可以自行尝试，孰优孰劣，实践出真知！
[numthreads(16, 1, 1)]
void UpdateCSMain(uint3 CurrentGPUThreadID : SV_DispatchThreadID)
{
	// 当前线程的索引表示 m_ParticlesBuffer 中的粒子索引 (包括存活、已死亡和从未使用的)
	uint ParticleIndex = CurrentGPUThreadID.x;
	
	// 如果当前 GPU 线程的索引已经超过了缓冲的最大数量，后续不处理，直接跳过
	if (ParticleIndex >= MaxAllocParticleBufferSize)
	{
		return;
	}
	
	// 当前线程要处理的粒子
	Particle p = m_ParticlesBuffer[ParticleIndex];
	
	// 只处理存活的粒子，如果当前粒子的生命 <= 0，说明该粒子已死亡/未使用，直接跳过
	// 死亡粒子会变成"空泡"，仍然占用 m_ParticlesBuffer 的槽位，等待新粒子数据覆盖
	if (p.life > 0)
	{
		// 更新粒子的速度、位置和生命
		// S = vt * 0.5gt^2  我们在高中物理学习的这个匀加速公式叫"解析解"，它在数学上是准确的
		// 下面这个我们要使用的叫 "半隐式欧拉积分" (Semi-implicit Euler)
		// 它是一些游戏/物理引擎 (例如 physx 物理引擎) 常见的计算加速/减速运动的方法
		// 它的核心思想：利用微分的思想，先用加速度更新速度，再用新速度更新位置
		
		// 微分：把时间切成无数个极小的片段 (比如每帧)，在每个片段里，我们假设运动是简单的线性变化，
		//      这就像观察一辆飞驰的赛车，我们不是一直盯着它，而是每隔一秒拍一张照片，
		//      每一张照片就是一次“微分”，它记录了那一瞬间的状态 (位置、速度)
		//		上面的 PerFrameDataTime 就是微分变量，表示当前帧与上一帧 (片段) 相隔的时间
		
		// 积分：把所有极小片段的变化累加起来，还原出完整的运动轨迹，
		//      就像把刚才拍的所有照片连续播放，就能看到赛车的完整奔跑过程
		
		// 粒子系统往往不只受重力影响，还可能受到风力、阻力、随机扰动等变力的作用
		// 如果使用匀加速公式，一旦力发生变化 (例如增加了空气阻力)，公式就需要重新推导，且无法精确解析求解
		// 用了解析解，我们可能会看到粒子是直接加速飞出去的，而不是随着各种阻力减小速度/达到恒定速度，不符合视觉
		// 半隐式欧拉积分可以自然地处理任意力场，只需在每帧根据当前受力计算加速度，然后更新速度和位置，代码结构统一且易于扩展
		// 并且半隐式欧拉误差很小，很接近现实物理的能量守恒，不需要乘法，性能很高效
		// 这种"先更新速度，再用新速度更新位置"的顺序，就是"半隐式"的含义，它更遵循能量守恒定律，也更符合直觉，非常好用
		
		p.Velocity.y -= Gravity * PerFrameDataTime;		// 微分：在这一小段时间内，速度改变量 = 重力 x 时间
		p.Position += p.Velocity * PerFrameDataTime;	// 积分：用更新后的速度算出小段位移，再累加到总位置
		p.life -= PerFrameDataTime;						// 生命也像时间一样，每帧减去一小段

		
		// 如果粒子生命结束，标记死亡
		if(p.life <= 0)
		{
			// 标记死亡
			p.life = 0;
			
			
			// 死亡后的粒子会产生"空泡"，栈顶指针递减，这个变量是栈顶指针原先指向的位置
			uint StackTopOriginalIndex;
			// 栈顶指针 m_StackTopPointer 进行原子操作，进行原子递减，同时返回递减前指向的位置
			m_StackTopPointer.InterlockedAdd(0, -1, StackTopOriginalIndex);
			// 将新空泡索引推入到 m_IdleParticlesStack 空闲栈中，表示该死亡粒子所占的空间可以被复用
			// InterlockedAdd 返回的操作前的原值，-1 才是栈顶新位置，空闲栈本质是数组栈，新空泡在这里推入
			m_IdleParticlesStack[StackTopOriginalIndex - 1] = ParticleIndex;
		}
		
		// 将粒子的新状态写回 m_ParticlesBuffer 中
		m_ParticlesBuffer[ParticleIndex] = p;
	}
}




