
// (10) RenderGLTFModel: ʹ�� DirectX 12 + Assimp ��Ⱦһ�������� gltf ģ�� (��ģ��û�й���)

struct VSInput      // VS �׶����붥������
{
	float4 position : POSITION;		// ���붥���λ��
	float2 texcoordUV : TEXCOORD;	// ���붥�����������
};


struct VSOutput     // VS �׶������������
{
	float4 position : SV_Position;	// ��������λ��
	float2 texcoordUV : TEXCOORD;	// ���������������ʱ
};


// Constant Buffer �������壬����������Ԥ�ȷ����һ�θ����Դ棬���ÿһ֡��Ҫ�任�����ݣ�������������� MVP �任����
// ���������������ɫ������ֻ���ģ���ɫ���������޸ĳ����������������
cbuffer GlobalData : register(b0, space0)
{
	row_major float4x4 MVP; // MVP ����
}


// Vertex Shader ������ɫ����ں��� (�𶥵�����)���������� IA �׶�����Ķ������ݣ�����������βü��ռ��µĶ�������
VSOutput VSMain(VSInput input)
{
	VSOutput output;

	output.position = mul(input.position, MVP);		// ע������������껹��Ҫ����һ�� MVP �任��
	
	output.texcoordUV = input.texcoordUV;			// ���� UV ���ñ任���ճ��������
    
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

	
	// �����Ĭ������ (�� C++ �˵����� UV ��������ó� -1)���������⴦��
	if (input.texcoordUV.x == -1 && input.texcoordUV.y == -1)
	{
		clip(-1);	// �ü���ǰ����
	}
	else // ��������ɫ�����ݹ�դ����ֵ�õ��� UV �����������в���
	{
		DiffuseColor = m_texure.Sample(m_sampler, input.texcoordUV);
	}
	
	return DiffuseColor; // ����������ɫ����������������ģ���������
}