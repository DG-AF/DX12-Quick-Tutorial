
// (12) RenderGLTFLightingModel: 加入光照，使用 DirectX 12 + Assimp 渲染《Ave Mujica》的 丰川祥子 gltf 骨骼模型

struct VSInput      // VS 阶段输入顶点数据
{
	float4 position : POSITION;			// 输入顶点的位置
	float4 normal : NORMAL;				// 顶点法线，用于光照
	float2 texcoordUV : TEXCOORD;		// 输入顶点的纹理坐标
	float4 color : COLOR;				// 对于自发光纹理，可能带有的颜色
	uint4 BoneIndices : BLENDINDICES;	// 骨骼索引
	float4 BoneWeights : BLENDWEIGHT;	// 骨骼权重
};

struct VSOutput     // VS 阶段输出顶点数据
{
	float4 position : SV_Position;		// 输出顶点的位置
	float4 normal : NORMAL;				// 顶底法线，用于光照
	float2 texcoordUV : TEXCOORD;		// 输出顶点纹理坐标时
	float4 color : COLOR;				// 对于自发光纹理，可能带有的颜色
};

// Constant Buffer 常量缓冲，常量缓冲是预先分配的一段高速显存，存放每一帧都要变换的数据，例如我们这里的 MVP 变换矩阵
// 常量缓冲对所有着色器都是只读的，着色器不可以修改常量缓冲里面的内容
cbuffer GlobalData : register(b0, space0)
{
	row_major float4x4 MVP; // MVP 矩阵
	
	// 骨骼偏移矩阵组，每个矩阵对应一块骨骼，这里仅设置了最多 512 个骨骼，实际可以更多
	row_major float4x4 BoneTransformMatrixGroup[512];
}

// 第二个常量缓冲，用于表示各种光源数据，进而实现 Blinn-Phong 光照模型
cbuffer GlobalLightData : register(b1, space0)
{
	row_major float4x4 WorldMatrix; // 世界矩阵，用于顶点法线变换，注意要用 row_major 修饰
	float4 CameraPosition;			// 摄像机在世界空间的坐标

	float4 WorldLightDirection;		// 世界空间上的光线方向 (点光源)
	float4 WorldLightColor;			// 世界空间上的光线颜色 (亮度)，默认白色
	float4 AmbientLightColor;		// 环境光的颜色 (亮度)
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
	
	// 法线变换的公式是 N' = N * Inv(Transpose(Matrix)) 与原变换矩阵的逆转置矩阵相乘，此公式网上有推导过程
	// 因为非均匀放缩变换，会破坏法线与面的垂直关系，导致变换错误，所以需要修正后的法线变换公式
	// 我们这里直接乘原变换矩阵，因为逐顶点矩阵求逆会产生高额开销，而且 HLSL 上没有原生的矩阵求逆函数
	// 即使自己实现了 HLSL 矩阵求逆，效果和原矩阵是几乎相同的 (骨骼动画几乎没有非均匀放缩变换)，得不偿失
	
	output.normal = mul(input.normal, BoneMatrix);		// 顶点法线也要做骨骼变换
	output.normal = mul(output.normal, WorldMatrix);	// 法线做完骨骼变换，还要做一次世界变换
	
	output.texcoordUV = input.texcoordUV;				// 纹理 UV 不用变换，照常输出即可
	
	output.color = input.color;							// color 也是
    
	return output;	// 返回处理后的顶点，接下来进行光栅化操作
}


// 第三个常量缓冲，用于表示材质纹理使用到的光照信息，此缓冲绑定到了 C++ 端的根常数，所以不占用显存
// 虽然根参数不需要绑定缓冲资源，但仍然需要将数据映射到 shader 视角的常量缓冲区内
cbuffer SpecificMaterialData : register(b2, space0)
{
	float4 DiffuseAlbedoLight;	// 贴图的反照率分量，表示贴图对光的反射能力
	float4 SpecularLight;		// 贴图反射高光部分的颜色
	float Glossiness;			// 贴图的光泽度，用于镜面光照
}

Texture2D m_DiffuseMap : register(t0, space0);	// 纹理
SamplerState m_sampler : register(s0, space0);	// 纹理采样器



// Pixel Shader 像素着色器入口函数 (逐像素输入)，接收来自光栅化阶段经过插值后的每个片元，返回像素颜色
float4 PSMain(VSOutput input) : SV_Target
{
	// DiffuseMapColor 漫反射材质贴图 (纹理) 本身的颜色
	float4 DiffuseMapColor = float4(1, 1, 1, 0);

	
	// 如果是默认纹理或自发光纹理 (在 C++ 端的纹理 UV 坐标会设置成 -1)，进行特殊处理
	if (input.texcoordUV.x == -1 && input.texcoordUV.y == -1)
	{
		// DiffuseColor = input.color; // 直接赋值自带颜色
		
		clip(-1);	// 没纹理的基本上是一些描边外轮廓，这一回我们直接在 PS 阶段裁剪像素，不显示它们，也不会进入后续流程
	}
	else
	{
		// 在像素着色器根据光栅化插值得到的 UV 坐标对纹理进行采样
		DiffuseMapColor = m_DiffuseMap.Sample(m_sampler, input.texcoordUV);
		// 还需要乘上自带颜色 (虽然说除了 emissive 自发光贴图以外 color 一般都是白色)
		DiffuseMapColor *= input.color;
	}
	
	
	// 1. Ambient 环境光，模拟真实环境下的微弱间接光照 (例如来自月光、远处光源的反射等)
	float4 Ambient = AmbientLightColor;
	
	
	// 2. Diffuse 漫反射光，模拟真实环境下的漫反射现象，漫反射是影响物体亮度最主要的因素
	// 像素携带的法线信息，光栅化也会进行法线插值，法线计算光照非常重要，注意要进行单位化
	float3 Normal = normalize(input.normal.xyz);
	// 光源发出光线的方向向量，这里也要进行单位化
	float3 LightDirection = normalize(WorldLightDirection.xyz);
	// 漫反射光 = 入射光线 * 材质对光线的反射程度 * 像素法线与光源法线的点乘
	// 这里的原理是 Lambert's Cosine Law 兰伯特余弦定律
	// 兰伯特余弦定律是由德国数学家约翰·海因里希·兰伯特提出的光学基本定律，主要描述理想漫反射表面或兰伯特辐射体的光辐射特性
	// 该定律指出：辐射体在任意观察方向上的发光强度与观察方向和表面法线夹角的余弦值成正比
	// 上文对两向量进行单位化了，根据向量点乘公式 a·b = |a||b| cosΘ，|a||b| 均为 1，那么点乘结果就是 cosΘ 了
	float4 Diffuse = WorldLightColor * DiffuseAlbedoLight * max(0, dot(Normal, LightDirection));
	
	
	// 3. Specular 高光，模拟真实环境下，有光泽的物体经光线反射后上面出现的亮点
	// 摄像机对当前像素的观察向量
	float3 ViewDirection = normalize(CameraPosition.xyz - input.position.xyz);
	// 半程向量，传统 Phong 模型虽然得到了高光，但这个高光总是偏向于某个方向，而不是真实环境下一片区域下的高亮
	// 原因是传统 Phong 模型只考虑了视线与反射光线的夹角，当夹角大于 90° 时，高光为 0，在镜面高光区域的边缘出现了明显的断层
	// 1977 年 Blinn 引入了半程向量来改进这一点，半程向量是光线与视线方向向量相加的一个单位向量
	// 当半程向量和法线向量越接近时，镜面高光辐射度越大，该区域下的高光越亮，有效解决了断层的问题
	float3 HalfwayVector = normalize(LightDirection + ViewDirection);
	// 高光 = 入射光线 * 高光反射程度 * pow(半程向量与像素法线的点乘, 光泽度)
	float4 Specular = WorldLightColor * SpecularLight * pow(max(0, dot(HalfwayVector, Normal)), Glossiness);
	
	
	// 最终颜色 = 纹理材质贴图颜色 * Blinn-Phong 光照的相加混合
	// 注意最后相加混合的光照颜色 a 分量要设置成 1，我们需要保留原贴图颜色的 alpha 值
	float4 FinalColor = DiffuseMapColor * float4((Ambient + Diffuse + Specular).rgb, 1);
	
	return FinalColor; // 返回像素颜色，接下来会进行深度模板测试与混合
}