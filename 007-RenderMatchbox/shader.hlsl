
// (7) RenderMatchbox����Ⱦһ�����У�������ʶ Depth Stencil Buffer ���ģ�建�壬��һ����ʶ���㡢ģ����ģ�;���

struct VSInput      // VS �׶����붥������
{
    float4 position : POSITION;         // ���붥���λ�ã�POSITION �����Ӧ C++ �����벼���е� POSITION
    float2 texcoordUV : TEXCOORD;       // ���붥����������꣬TEXCOORD �����Ӧ C++ �����벼���е� TEXCOORD
    
    // ���������Ҫ�� IA �׶δ��ݾ��󣬾���̫��û��ֱ�Ӵ������ǿ��԰Ѿ���ָ��һ��һ�����������ٵ� VS �׶�������װ
    // MATRIX ���Զ������壬�����������ֱ�ʾͬһ��������£�ͬ������ (MATRIX) �ĵ� i ������
    float4 Matrix_Row0 : MATRIX0;
    float4 Matrix_Row1 : MATRIX1;
    float4 Matrix_Row2 : MATRIX2;
    float4 Matrix_Row3 : MATRIX3;
    
    // ��ʵ����ֻ�Ǹ���ʶ�������ַ���...
};

struct VSOutput     // VS �׶������������
{
    float4 position : SV_Position;      // ��������λ�ã�SV_POSITION ��ϵͳ���壬ָ�����������Ѿ�λ����βü��ռ䣬֪ͨ��դ���׶ζԶ������͸�ӳ�������Ļӳ��
    float2 texcoordUV : TEXCOORD;       // ���������������ʱ����Ȼ��Ҫ TEXCOORD ����
};

// Constant Buffer �������壬����������Ԥ�ȷ����һ�θ����Դ棬���ÿһ֡��Ҫ�任�����ݣ�������������� MVP �任����
// ���������������ɫ������ֻ���ģ���ɫ���������޸ĳ����������������
cbuffer GlobalData : register(b0, space0) // �������壬b ��ʾ buffer ���壬b0 ��ʾ 0 �� CBV �Ĵ�����space0 ��ʾʹ�� b0 �� 0 �ſռ�
{
    row_major float4x4 MVP; // MVP �������ڽ����������ģ�Ϳռ�任����βü��ռ䣬HLSL Ĭ�ϰ��д洢��row_major ��ʾ���ݰ��д洢
}


// Vertex Shader ������ɫ����ں��� (�𶥵�����)���������� IA �׶�����Ķ������ݣ�����������βü��ռ��µĶ�������
// ��һ�׶Σ�Input Assembler ����װ��׶�
// ��һ�׶Σ�Rasterization ��դ���׶�
VSOutput VSMain(VSInput input)
{
    float4x4 ModelMatrix;   // VS �׶�Ҫ�õ���ģ�;���
    VSOutput output;        // �������դ���׶εĽṹ�����
    
    // �� IA �׶εõ�����������װ�ɾ���
    ModelMatrix[0] = input.Matrix_Row0;
    ModelMatrix[1] = input.Matrix_Row1;
    ModelMatrix[2] = input.Matrix_Row2;
    ModelMatrix[3] = input.Matrix_Row3;
    
    // ע�� cbuffer �����������ɫ����ֻ���ģ��������ǲ���������Գ�����������޸ģ�
    output.position = mul(input.position, ModelMatrix);     // �ȳ� ģ�;���
    output.position = mul(output.position, MVP);            // �ٳ� �۲���� �� ͶӰ����ע�� mul ��������� output.position
    output.texcoordUV = input.texcoordUV;                   // ���� UV ���ñ仯���ճ��������
    
    return output;
}

// register(*#��spaceN) *��ʾ��Դ���ͣ�#��ʾ���õļĴ�����ţ�spaceN ��ʾʹ�õ� N �żĴ����ռ�

Texture2D m_texure : register(t0, space0);      // ����t ��ʾ SRV ��ɫ����Դ��t0 ��ʾ 0 �� SRV �Ĵ�����space0 ��ʾʹ�� t0 �� 0 �ſռ�
SamplerState m_sampler : register(s0, space0);  // �����������s ��ʾ��������s0 ��ʾ 0 �� sampler �Ĵ�����space0 ��ʾʹ�� s0 �� 0 �ſռ�

// Pixel Shader ������ɫ����ں��� (����������)���������Թ�դ���׶ξ�����ֵ���ÿ��ƬԪ������������ɫ
// ��һ�׶Σ�Rasterization ��դ���׶�
// ��һ�׶Σ�Output Merger ����ϲ��׶�
float4 PSMain(VSOutput input) : SV_Target // SV_Target Ҳ��ϵͳ���壬֪ͨ����ϲ��׶ν� PS �׶η��ص���ɫд�뵽��ȾĿ��(��ɫ����)��
{
    return m_texure.Sample(m_sampler, input.texcoordUV); // ��������ɫ�����ݹ�դ����ֵ�õ��� UV �����������в���
}

