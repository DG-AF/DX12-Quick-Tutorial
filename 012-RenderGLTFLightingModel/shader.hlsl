
// (12) RenderGLTFLightingModel: ������գ�ʹ�� DirectX 12 + Assimp ��Ⱦ��Ave Mujica���� �ᴨ���� gltf ����ģ��

struct VSInput      // VS �׶����붥������
{
	float4 position : POSITION;			// ���붥���λ��
	float4 normal : NORMAL;				// ���׷��ߣ����ڹ���
	float2 texcoordUV : TEXCOORD;		// ���붥�����������
	float4 color : COLOR;				// �����Է����������ܴ��е���ɫ
	uint4 BoneIndices : BLENDINDICES;	// ��������
	float4 BoneWeights : BLENDWEIGHT;	// ����Ȩ��
};

struct VSOutput     // VS �׶������������
{
	float4 position : SV_Position;		// ��������λ��
	float4 normal : NORMAL;				// ���׷��ߣ����ڹ���
	float2 texcoordUV : TEXCOORD;		// ���������������ʱ
	float4 color : COLOR;				// �����Է����������ܴ��е���ɫ
};

// Constant Buffer �������壬����������Ԥ�ȷ����һ�θ����Դ棬���ÿһ֡��Ҫ�任�����ݣ�������������� MVP �任����
// ���������������ɫ������ֻ���ģ���ɫ���������޸ĳ����������������
cbuffer GlobalData : register(b0, space0)
{
	row_major float4x4 MVP; // MVP ����
	
	// ����ƫ�ƾ����飬ÿ�������Ӧһ��������������������� 512 ��������ʵ�ʿ��Ը���
	row_major float4x4 BoneTransformMatrixGroup[512];
}

// �ڶ����������壬���ڱ�ʾ���ֹ�Դ���ݣ�����ʵ�� Blinn-Phong ����ģ��
cbuffer GlobalLightData : register(b1, space0)
{
	row_major float4x4 WorldMatrix; // ����������ڶ��㷨�߱任��ע��Ҫ�� row_major ����
	float4 CameraPosition;			// �����������ռ������

	float4 WorldLightDirection;		// ����ռ��ϵĹ��߷��� (���Դ)
	float4 WorldLightColor;			// ����ռ��ϵĹ�����ɫ (����)��Ĭ�ϰ�ɫ
	float4 AmbientLight;			// ���������ɫ (����)
}


// Vertex Shader ������ɫ����ں��� (�𶥵�����)���������� IA �׶�����Ķ������ݣ�����������βü��ռ��µĶ�������
VSOutput VSMain(VSInput input)
{
	VSOutput output;
	
	// ����Ȩ�ؼ�¼��ÿ������Ըö����Ӱ��̶ȣ�ͨ�����������͹���Ȩ�أ����㶥��������ģ�͵���ʵλ��
	// ����λ�� = (ÿ�������Ȩ�� * ÿ�������ȫ��ƫ�ƾ��� ���ۼӺ�) * ���㾲ֹλ������
	float4x4 BoneMatrix = (
		input.BoneWeights[0] * BoneTransformMatrixGroup[input.BoneIndices[0]] +
		input.BoneWeights[1] * BoneTransformMatrixGroup[input.BoneIndices[1]] +
		input.BoneWeights[2] * BoneTransformMatrixGroup[input.BoneIndices[2]] +
		input.BoneWeights[3] * BoneTransformMatrixGroup[input.BoneIndices[3]]
	);
	
	
	output.position = mul(input.position, BoneMatrix); // �����Ȩ�ؾ�����ˣ��õ���ֹ״̬�µ���ʵλ��
	
	output.position = mul(output.position, MVP); // ע������������껹��Ҫ����һ�� MVP �任��
	
	output.normal = mul(input.normal, WorldMatrix);	// ���㷨��ֻ���������任
	
	output.texcoordUV = input.texcoordUV; // ���� UV ���ñ任���ճ��������
	
	output.color = input.color;	// color Ҳ��
    
	return output; // ���ش����Ķ��㣬���������й�դ������
}


// �������������壬���ڱ�ʾ��������ʹ�õ��Ĺ�����Ϣ���˻���󶨵��� C++ �˵ĸ����������Բ�ռ���Դ�
// ��Ȼ����������Ҫ�󶨻�����Դ������Ȼ��Ҫ������ӳ�䵽 shader �ӽǵĳ�����������
cbuffer SpecificMaterialData : register(b2, space0)
{
	float4 DiffuseAlbedoLight;	// ��ͼ�ķ����ʷ�������ʾ��ͼ�Թ�ķ�������
	float4 SpecularLight;		// ��ͼ����߹ⲿ�ֵ���ɫ
	float Glossiness;			// ��ͼ�Ĺ���ȣ����ھ������
}

Texture2D m_DiffuseMap : register(t0, space0);	// ����
SamplerState m_sampler : register(s0, space0);	// ���������



// Pixel Shader ������ɫ����ں��� (����������)���������Թ�դ���׶ξ�����ֵ���ÿ��ƬԪ������������ɫ
float4 PSMain(VSOutput input) : SV_Target
{
	// DiffuseMapColor �����������ͼ (����) �������ɫ
	float4 DiffuseMapColor = float4(1, 1, 1, 0);

	
	// �����Ĭ��������Է������� (�� C++ �˵����� UV ��������ó� -1)���������⴦��
	if (input.texcoordUV.x == -1 && input.texcoordUV.y == -1)
	{
		// DiffuseColor = input.color; // ֱ�Ӹ�ֵ�Դ���ɫ
		
		clip(-1);	// û����Ļ�������һЩ�������������һ������ֱ���� PS �׶βü����أ�����ʾ���ǣ�Ҳ��������������
	}
	else
	{
		// ��������ɫ�����ݹ�դ����ֵ�õ��� UV �����������в���
		DiffuseMapColor = m_DiffuseMap.Sample(m_sampler, input.texcoordUV);
		// ����Ҫ�����Դ���ɫ (��Ȼ˵���� emissive �Է�����ͼ���� color һ�㶼�ǰ�ɫ)
		DiffuseMapColor *= input.color;
	}
	
	
	// 1. Ambient �����⣬ģ����ʵ�����µ�΢����ӹ��� (���������¹⡢Զ����Դ�ķ����)
	float4 Ambient = AmbientLight;
	
	
	// 2. Diffuse ������⣬ģ����ʵ�����µ�������������������Ӱ��������������Ҫ������
	// ����Я���ķ�����Ϣ����դ��Ҳ����з��߲�ֵ�����߼�����շǳ���Ҫ��ע��Ҫ���е�λ��
	float3 Normal = normalize(input.normal.xyz);
	// ��Դ�������ߵķ�������������ҲҪ���е�λ��
	float3 LightDirection = normalize(WorldLightDirection.xyz);
	// ������� = ������� * ���ʶԹ��ߵķ���̶� * ���ط������Դ���ߵĵ��
	// �����ԭ���� Lambert's Cosine Law ���������Ҷ���
	// ���������Ҷ������ɵ¹���ѧ��Լ����������ϣ������������Ĺ�ѧ�������ɣ���Ҫ�����������������������ط�����Ĺ��������
	// �ö���ָ����������������۲췽���ϵķ���ǿ����۲췽��ͱ��淨�߼нǵ�����ֵ������
	// ���Ķ����������е�λ���ˣ�����������˹�ʽ a��b = |a||b| cos����|a||b| ��Ϊ 1����ô��˽������ cos�� ��
	float4 Diffuse = WorldLightColor * DiffuseAlbedoLight * max(0, dot(Normal, LightDirection));
	
	
	// 3. Specular �߹⣬ģ����ʵ�����£��й�������徭���߷����������ֵ�����
	// ������Ե�ǰ���صĹ۲�����
	float3 ViewDirection = normalize(CameraPosition.xyz - input.position.xyz);
	// �����������ͳ Phong ģ����Ȼ�õ��˸߹⣬������߹�����ƫ����ĳ�����򣬶�������ʵ������һƬ�����µĸ���
	// ԭ���Ǵ�ͳ Phong ģ��ֻ�����������뷴����ߵļнǣ����нǴ��� 90�� ʱ���߹�Ϊ 0���ھ���߹�����ı�Ե���������ԵĶϲ�
	// 1977 �� Blinn �����˰���������Ľ���һ�㣬��������ǹ��������߷���������ӵ�һ����λ����
	// ����������ͷ�������Խ�ӽ�ʱ������߹�����Խ�󣬸������µĸ߹�Խ������Ч����˶ϲ������
	float3 HalfwayVector = normalize(LightDirection + ViewDirection);
	// �߹� = ������� * �߹ⷴ��̶� * pow(������������ط��ߵĵ��, �����)
	float4 Specular = WorldLightColor * SpecularLight * pow(max(0, dot(HalfwayVector, Normal)), Glossiness);
	
	
	// ������ɫ = ���������ͼ��ɫ * Blinn-Phong ���յ���ӻ��
	// ע�������ӻ�ϵĹ�����ɫ a ����Ҫ���ó� 1��������Ҫ����ԭ��ͼ��ɫ�� alpha ֵ
	float4 FinalColor = DiffuseMapColor * float4((Ambient + Diffuse + Specular).rgb, 1);
	
	return FinalColor; // ����������ɫ����������������ģ���������
}