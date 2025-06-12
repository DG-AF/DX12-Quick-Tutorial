
// (3) DrawRectangle���� DirectX 12 ��һ������

struct VSInput      // VS �׶����붥������
{
    float4 position : POSITION;     // ���붥���λ�ã�POSITION �����Ӧ C++ �����벼���е� POSITION
    float4 color : COLOR;           // ���붥�����ɫ��COLOR �����Ӧ C++ �����벼���е� COLOR
};

struct VSOutput     // VS �׶������������
{
    float4 position : SV_Position;  // ��������λ�ã�SV_POSITION ��ϵͳ���壬ָ�����������Ѿ�λ����βü��ռ䣬֪ͨ��դ���׶ζԶ������͸�ӳ�������Ļӳ��
    float4 color : COLOR;           // ���������ɫʱ����Ȼ��Ҫ COLOR ����
};

// Vertex Shader ������ɫ����ں��� (�𶥵�����)���������� IA �׶�����Ķ������ݣ�����������βü��ռ��µĶ�������
// ��һ�׶Σ�Input Assembler ����װ��׶�
// ��һ�׶Σ�Rasterization ��դ���׶�
VSOutput VSMain(VSInput input)
{
    VSOutput output;    // ����ֱ���� IA �׶����붥���� NDC �ռ��µ����꣬��������任��ֱ�Ӹ�ֵ���ؾ���
    output.position = input.position;
    output.color = input.color;
    
    return output;
}

// Pixel Shader ������ɫ����ں��� (����������)���������Թ�դ���׶ξ�����ֵ���ÿ��ƬԪ������������ɫ
// ��һ�׶Σ�Rasterization ��դ���׶�
// ��һ�׶Σ�Output Merger ����ϲ��׶�
float4 PSMain(VSOutput input) : SV_Target   // SV_Target Ҳ��ϵͳ���壬֪ͨ����ϲ��׶ν� PS �׶η��ص���ɫд�뵽��ȾĿ��(��ɫ����)��
{
    return input.color;     // ��������ֱ�ӷ���ÿ�����ص���ɫ����
}

