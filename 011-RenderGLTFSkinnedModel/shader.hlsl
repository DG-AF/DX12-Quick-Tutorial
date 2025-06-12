
// (11) RenderGLTFSkinnedModel: ʹ�� DirectX 12 + Assimp ��Ⱦһ����������ʱ�����ģ��

struct VSInput      // VS �׶����붥������
{
	float4 position : POSITION;			// ���붥���λ��
	float2 texcoordUV : TEXCOORD;		// ���붥�����������
	float4 color : COLOR;				// �����Է����������ܴ��е���ɫ
	uint4 BoneIndices : BLENDINDICES;	// ��������
	float4 BoneWeights : BLENDWEIGHT;	// ����Ȩ��
};

struct VSOutput     // VS �׶������������
{
	float4 position : SV_Position;		// ��������λ��
	float2 texcoordUV : TEXCOORD;		// ���������������ʱ
	float4 color : COLOR;				// �����Է����������ܴ��е���ɫ
};

// Constant Buffer �������壬����������Ԥ�ȷ����һ�θ����Դ棬���ÿһ֡��Ҫ�任�����ݣ�������������� MVP �任����
// ���������������ɫ������ֻ���ģ���ɫ���������޸ĳ����������������
cbuffer GlobalData : register(b0, space0)
{
	row_major float4x4 MVP; // MVP ����
	
	row_major float4x4 BoneTransformMatrixGroup[512]; // ����ƫ�ƾ����飬ÿ�������Ӧһ��������������������� 512 ��������ʵ�ʿ��Ը���
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
	
	output.texcoordUV = input.texcoordUV; // ���� UV ���ñ任���ճ��������
	
	output.color = input.color; // color Ҳ��
    
	return output; // ���ش����Ķ��㣬���������й�դ������
}


// register(*#��spaceN) *��ʾ��Դ���ͣ�#��ʾ���õļĴ�����ţ�spaceN ��ʾʹ�õ� N �żĴ����ռ�

Texture2D m_texure : register(t0, space0); // ����
SamplerState m_sampler : register(s0, space0); // ���������


// Pixel Shader ������ɫ����ں��� (����������)���������Թ�դ���׶ξ�����ֵ���ÿ��ƬԪ������������ɫ
float4 PSMain(VSOutput input) : SV_Target
{
	// DiffuseColor �����䷢������ɫ�������������������ɫ����������Ҫ�͵� 12 �µĹ����йأ����ڼ��˽�һ�¼���
	float4 DiffuseColor = float4(1, 1, 1, 0);

	
	// �����Ĭ��������Է������� (�� C++ �˵����� UV ��������ó� -1)���������⴦��
	if (input.texcoordUV.x == -1 && input.texcoordUV.y == -1)
	{
		DiffuseColor = input.color; // ֱ�Ӹ�ֵ�Դ���ɫ
	}
	else
	{
		// ��������ɫ�����ݹ�դ����ֵ�õ��� UV �����������в���
		DiffuseColor = m_texure.Sample(m_sampler, input.texcoordUV);
	}
	
	// ��Ҫ������ɫ��ϣ���Ϊ���� Emissive Texture �Է���������ԣ�������Դ� color ��Ϣ�ģ���Ҫ��������ɫ����
	DiffuseColor = input.color * DiffuseColor;
	
	return DiffuseColor; // ����������ɫ����������������ģ���������
}