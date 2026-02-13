
// (14) RenderGLTFWithSkyBox: 加入并渲染天空盒，使用 DirectX 12 + Assimp 渲染《为美好的生活献上祝福》中的 Q版阿库娅 GLTF 模型
// SkyBoxShader.hlsl: 用于渲染天空盒的 shader

// 常量数据，注意这里用的是 b1 寄存器
cbuffer GlobalData : register(b1, space0)
{
	row_major float4x4 Projection_View_InvMatrix;	// 逆视图投影矩阵 (NDC 空间 -> 世界空间)，用于天空盒纹理采样，算天空盒纹理映射位置的
}

struct VSInput	// VS 输入结构体
{
	float4 position : POSITION;			// 天空盒顶点的坐标
};

struct VSOutput	// VS 输出结构体
{
	float4 NDCPosition : SV_Position;	// NDC 空间下的顶点坐标，这个会进入光栅化阶段插值
	float4 SampleMapPos : TEXCOORD;		// 用于天空盒纹理采样的顶点坐标 (世界空间)，这个也会进入光栅化阶段插值
};

// 渲染天空盒的 Vertex Shader
VSOutput VSMain(VSInput input)
{
	// VS 阶段输出的结果，VS -> 光栅化 -> PS
	VSOutput output;
	
	// 输入的坐标已经在 NDC 空间里面了，直接赋值就行
	output.NDCPosition = input.position;
	// 我们需要对天空盒采样，所以这里要逆变换到世界空间，还记得 MVP 变换的相关步骤吗？
	output.SampleMapPos = normalize(mul(input.position, Projection_View_InvMatrix));
	
	return output;
}


// ---------------------------------------------------------------------------------------------------------------


// 用单位化的顶点坐标，经过等距圆柱投影 (Equirectangular Projection，等距柱状投影) 采样后得到纹理 UV 坐标
// 地球仪的球体部分，按纬线竖着切开就会得到一个长方形贴图，这个长方形贴图就是世界地图，把这个世界地图围成圆柱
// 然后再拿一个相同的地球仪，把球体装进圆柱里，会发现地球赤道和圆柱相切
// 把一个点光源放进球心，球心发出的光线穿过球表面某一点，恰好击中圆柱的一个地方 (两极除外)，光会在这个地方形成光斑
// 而且这两个地方 (球体和圆柱) 指向的是同一个地点，这就是等距柱面投影
// 我们可以通过投影将 3D 坐标 (这个坐标必须在单位球上) 转化为 2D 纹理 UV 坐标，本质上是获取经纬度然后进行比例计算
float2 SampleSphericalMap(float3 position)
{
	// 圆周率
	const float PI = 3.1415926;
	
	// 先单位化坐标向量，让它在单位球上
	position = normalize(position);
	// 获得坐标的纬度，纬度受 y 影响
	float theta = acos(position.y);
	// 获得坐标的经度，经度受 x 和 z 的影响，注意 atan2 的结果范围是 [-PI, PI] ([0, 0.5] -> [0, PI]，[0.5, 1] -> [-PI, 0])
	float phi = atan2(position.x, position.z);
	
	// 经度除以 2*pi 获得 U 坐标
	float U = phi / (2 * PI);
	// 纬度除以 pi 获得 V 坐标
	float V = theta / PI;
	
	// 返回纹理 UV，注意这里 UV 可能会有负数 (原因见 atan2)，cpp 端的 sampler 需要设置成 WRAP 环绕模式，这样才能正确处理负数 UV
	return float2(U, V);
}


Texture2D m_SkyBoxTexture : register(t0, space0);		// 天空盒纹理
SamplerState m_LinearSampler : register(s1, space0);	// 纹理采样器 (注意这里是 s1 寄存器，线性过滤，同样要在 cpp 端设置)


// 渲染天空盒的 Pixel Shader
float4 PSMain(VSOutput output): SV_Target
{
	// 用世界空间下顶点坐标，进行等距圆柱投影，得到真正用于天空盒的纹理 UV 坐标
	// 天空本质上是一个大球体，进行投影，本质上是对球体展开成二维平面，然后在平面上找到对应的位置
	float2 SkyBoxMapUV = SampleSphericalMap(output.SampleMapPos.xyz);
	
	// 返回采样后得到的颜色，作为天空盒，原 HDR 纹理的透明度不用管它，直接设 1
	return float4(m_SkyBoxTexture.Sample(m_LinearSampler, SkyBoxMapUV).rgb, 1.0f);
}

