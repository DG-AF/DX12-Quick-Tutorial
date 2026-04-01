
// (22) ParticleSystem: 进一步学习计算着色器，学习粒子的动态生成与销毁，模拟 Minecraft 的粒子破坏效果
// ParticleShader.hlsl: 生成、管理、更新并销毁粒子的 shader


// 粒子实例结构体
struct Particle
{
	float3 Position;	// 粒子的世界坐标
	float3 Velocity;	// 粒子的速度
	float life;			// 剩余生命 (单位：秒)，大于 0 表示存活
	uint BlockType;		// 所属方块类型
};


// 管理所有粒子 (包括存活和已死亡的) 的粒子缓冲区，它是一个有数据"空泡"的 HiveBuffer 非连续缓冲
// 粒子死亡后会原地产生空泡，新粒子填充优先使用最近的空泡，这样能提高数据复用率
RWStructuredBuffer<Particle> m_ParticlesHiveBuffer : register(u0, space0);


// HLSL 没有类似 std::stack 那样的内置栈结构，我们需要自行用普通数组 (缓冲) 模拟空闲栈
// 这个数组空闲栈存储了 HiveBuffer 可用的空泡索引
RWStructuredBuffer<uint> m_ParticlesFreeStack : register(u1, space0);


// 我们还需要一个 4 字节的栈顶指针，来计数最近可用"空泡"的索引
// 这里需要用到 UAV 原始缓冲区，因为会有很多 GPU 线程几乎同时来访问空闲栈
// 我们需要利用原始缓冲区支持原子操作的特性，控制线程的访问，实现访问同步，避免资源被乱序访问
// 如果有新粒子，栈顶指针++，找到 m_ParticlesBuffer 新的"空泡"索引，将新粒子添加到"空泡"
// 如果检测到粒子死亡，会原地产生"空泡"，栈顶指针--，栈顶记录新的"空泡"索引
RWByteAddressBuffer m_StackTopPointer : register(u2, space0);


// 来自 CPU 端的粒子待生成列表 (上传堆资源，着色器只读)
// 这个结构化缓冲区的作用是：用来暂存那些 CPU 产生的、还没来得及交给 GPU 的粒子数据
// 用待生成列表先把粒子"攒起来"，等到每帧的固定更新点时，再一次性把这一批粒子都交给 GPU，效率高得多
// 为什么不直接将新数据写入到 m_ParticlesBuffer？因为 m_ParticlesBuffer 里面的数据会有"空泡"，数据不是连续的
// 替代方案非常的麻烦，还不如直接 StructuredBuffer 然后 GPU 一个个写入清晰方便的多
StructuredBuffer<Particle> m_SpawnParticlesList : register(t0, space0);



// ---------------------------------------------------------------------------------------------------------------



// 用于生成新粒子的常量缓冲 (着色器只读)
cbuffer SpawnParticlesData : register(b0, space0)
{
	// 新粒子在 m_SpawnParticlesList 的数量，等会要将它们添加到 m_ParticlesHiveBuffer 中
	uint SpawnParticlesCount;
}


// 向 m_ParticlesHiveBuffer 添加新粒子的计算着色器 (一个线程组调用 16 个 GPU 线程，每个线程对应一个新粒子)
// SV_DispatchThreadID: 系统语义，表示在全局线程组调度内，当前线程的全局索引
// 任务：从 CPU 上传的 m_SpawnParticlesList 中读取新粒子数据，从空闲栈分配索引，写入粒子缓冲区
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
	
	
	// 新空泡在 m_ParticlesFreeStack 的索引，这个索引会指向空闲栈的栈顶
	uint NewIdleSpaceIndex_InStack = 0;
	
	
	// 栈顶指针 m_StackTopPointer 进行原子操作，查询栈顶可用的空泡 (已死亡粒子产生的数据空位)
	// 返回新空泡在 m_ParticlesFreeStack 的索引，同时栈顶指针原子递增，移动到下一个空泡索引
	// InterlockedAdd 是一个原子操作，向指定的地址增加值，第三个参数返回操作前的原值
	// 由于空闲栈是数组栈，栈顶指针总是指向下一个可写入的空闲位置
	// 因此需要先获取递增前的索引，才能拿到当前可用的空闲槽
	m_StackTopPointer.InterlockedAdd(0, 1, NewIdleSpaceIndex_InStack);
	
	
	// 新空泡在 m_ParticlesHiveBuffer 的索引
	// 这个索引会指向已死亡粒子产生的数据空位，或者是还没使用过的新数据空位
	uint NewIdleSpaceIndex_InBuffer = m_ParticlesFreeStack[NewIdleSpaceIndex_InStack];

	
	// 向 m_ParticlesHiveBuffer 的空泡写入新粒子数据，这样就能新增粒子
	m_ParticlesHiveBuffer[NewIdleSpaceIndex_InBuffer] = m_SpawnParticlesList[NewParticleIndex];
}



// ---------------------------------------------------------------------------------------------------------------



// 用于更新粒子状态的常量缓冲 (着色器只读)
cbuffer UpdateParticlesData : register(b1, space0)
{
	float PerFrameDataTime;				// 相比上一帧，当前帧过去的时间，用于计算速度与位移
	float Gravity;						// 重力，用于计算速度，呈现粒子自然掉落效果
}


// 每帧更新 m_ParticlesHiveBuffer 所有存活粒子的计算着色器 (一个线程组调用 16 个线程，每个线程对应一个粒子)
// SV_DispatchThreadID: 系统语义，表示在全局线程组调度内，当前线程的全局索引
// 任务：每帧检测并更新 HiveBuffer 所有活跃粒子的状态，并回收死亡粒子产生的 "空泡" 到空闲栈
[numthreads(16, 1, 1)]
void UpdateCSMain(uint3 CurrentGPUThreadID : SV_DispatchThreadID)
{
	// 当前线程的索引表示 m_ParticlesBuffer 中的粒子索引 (包括存活、已死亡和从未使用的)
	uint ParticleIndex = CurrentGPUThreadID.x;
	
	
	// 当前线程要处理的粒子
	Particle p = m_ParticlesHiveBuffer[ParticleIndex];
	
	
	// 只处理存活的粒子，如果当前粒子的生命 <= 0，说明该粒子已死亡/未使用，直接跳过
	// 死亡粒子会变成"空泡"，仍然占用 m_ParticlesHiveBuffer 的槽位，等待新粒子数据覆盖
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
		p.life -= PerFrameDataTime;						// 时间流逝，粒子生命每帧也减去一小段

		
		// 如果粒子生命结束，标记死亡
		if (p.life <= 0)
		{
			// 标记死亡
			p.life = 0;
			
			
			// 死亡后的粒子会产生"空泡"，栈顶指针递减，这个变量是栈顶指针原先指向的位置
			uint StackTopOriginalIndex;
			
			
			// 栈顶指针 m_StackTopPointer 进行原子操作，进行原子递减，同时返回递减前指向的位置
			m_StackTopPointer.InterlockedAdd(0, -1, StackTopOriginalIndex);
			
			
			// 将新空泡索引推入到 m_ParticlesFreeStack 空闲栈中，表示该死亡粒子所占的空间可以被复用
			// InterlockedAdd 返回的操作前的原值，-1 才是栈顶新位置，空闲栈本质是数组栈，新空泡在这里推入
			m_ParticlesFreeStack[StackTopOriginalIndex - 1] = ParticleIndex;
		}
		
		
		// 将粒子的新状态写回 m_ParticlesHiveBuffer 中
		m_ParticlesHiveBuffer[ParticleIndex] = p;
	}
}



// ---------------------------------------------------------------------------------------------------------------



// 用于初始化粒子所用数据结构的计算着色器，只在开始时调用一次 (一个线程组调用 16 个线程)
// SV_DispatchThreadID: 系统语义，表示在全局线程组调度内，当前线程的全局索引
// 任务：初始化粒子使用的 UAV 资源 (HiveBuffer、FreeStack)，防止资源创建遗留的脏数据污染上面两个着色器
[numthreads(16, 1, 1)]
void InitCSMain(uint3 CurrentGPUThreadID : SV_DispatchThreadID)
{
	// 将栈顶指针重置为 0，只让线程组中第一个线程执行栈顶指针初始化，避免多线程同时写入造成竞争
	// 将这个 if 注释掉，会出现一个 X3583 的警告，非常有意思，感兴趣可以看看
	if(CurrentGPUThreadID.x == 0)
	{
		m_StackTopPointer.Store(0, 0);
	}

	
	// 根据线程索引，将对应位置下空闲数组栈填充空泡索引
	m_ParticlesFreeStack[CurrentGPUThreadID.x] = CurrentGPUThreadID.x;
	
	
	// 粒子空泡实例
	Particle EmptySpace;
	EmptySpace.BlockType = 0;
	EmptySpace.life = 0;
	EmptySpace.Position = float3(0, 0, 0);
	EmptySpace.Velocity = float3(0, 0, 0);
	
	
	// 根据线程索引，将对应位置下粒子缓冲填充粒子空泡
	m_ParticlesHiveBuffer[CurrentGPUThreadID.x] = EmptySpace;
}