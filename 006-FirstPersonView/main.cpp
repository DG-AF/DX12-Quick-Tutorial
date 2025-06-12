
// (6) FirstPersonView��������ʶ Camera ��������� DirectX 12 �Ϲ�����һ�˳��ӽ�

#include<Windows.h>			// Windows ���ڱ�̺���ͷ�ļ�
#include<d3d12.h>			// DX12 ����ͷ�ļ�
#include<dxgi1_6.h>			// DXGI ͷ�ļ������ڹ����� DX12 �������������Ҫ�豸���� DXGI ������ ������
#include<DirectXColors.h>	// DirectX ��ɫ��
#include<DirectXMath.h>		// DirectX ��ѧ��
#include<d3dcompiler.h>		// DirectX Shader ��ɫ�������
#include<wincodec.h>		// WIC ͼ�����ܣ����ڽ������ת��ͼƬ�ļ�

#include<wrl.h>				// COM ���ģ��⣬����д DX12 �� DXGI ��صĽӿ�
#include<string>			// C++ ��׼ string ��
#include<sstream>			// C++ �ַ����������
#include<functional>		// C++ ��׼��������⣬�������ĵ� std::function ������װ���� std::bind �󶨻ص�����

#pragma comment(lib,"d3d12.lib")			// ���� DX12 ���� DLL
#pragma comment(lib,"dxgi.lib")				// ���� DXGI DLL
#pragma comment(lib,"dxguid.lib")			// ���� DXGI ��Ҫ���豸 GUID
#pragma comment(lib,"d3dcompiler.lib")		// ���� DX12 ��Ҫ����ɫ������ DLL
#pragma comment(lib,"windowscodecs.lib")	// ���� WIC DLL

using namespace Microsoft;
using namespace Microsoft::WRL;		// ʹ�� wrl.h ����������ռ䣬������Ҫ�õ������ Microsoft::WRL::ComPtr COM����ָ��
using namespace DirectX;			// DirectX �����ռ�


// �����ռ� DX12TextureHelper �����˰�������ת������ͼƬ��ʽ�Ľṹ���뺯��
namespace DX12TextureHelper
{
	// ����ת���ã����� DX12 ��֧�ֵĸ�ʽ��DX12 û����

	// Standard GUID -> DXGI ��ʽת���ṹ��
	struct WICTranslate
	{
		GUID wic;
		DXGI_FORMAT format;
	};

	// WIC ��ʽ�� DXGI ���ظ�ʽ�Ķ�Ӧ���ñ��еĸ�ʽΪ��֧�ֵĸ�ʽ
	static WICTranslate g_WICFormats[] =
	{
		{ GUID_WICPixelFormat128bppRGBAFloat,       DXGI_FORMAT_R32G32B32A32_FLOAT },
		{ GUID_WICPixelFormat64bppRGBAHalf,         DXGI_FORMAT_R16G16B16A16_FLOAT },
		{ GUID_WICPixelFormat64bppRGBA,             DXGI_FORMAT_R16G16B16A16_UNORM },
		{ GUID_WICPixelFormat32bppRGBA,             DXGI_FORMAT_R8G8B8A8_UNORM },
		{ GUID_WICPixelFormat32bppBGRA,             DXGI_FORMAT_B8G8R8A8_UNORM },
		{ GUID_WICPixelFormat32bppBGR,              DXGI_FORMAT_B8G8R8X8_UNORM },
		{ GUID_WICPixelFormat32bppRGBA1010102XR,    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM },
		{ GUID_WICPixelFormat32bppRGBA1010102,      DXGI_FORMAT_R10G10B10A2_UNORM },
		{ GUID_WICPixelFormat16bppBGRA5551,         DXGI_FORMAT_B5G5R5A1_UNORM },
		{ GUID_WICPixelFormat16bppBGR565,           DXGI_FORMAT_B5G6R5_UNORM },
		{ GUID_WICPixelFormat32bppGrayFloat,        DXGI_FORMAT_R32_FLOAT },
		{ GUID_WICPixelFormat16bppGrayHalf,         DXGI_FORMAT_R16_FLOAT },
		{ GUID_WICPixelFormat16bppGray,             DXGI_FORMAT_R16_UNORM },
		{ GUID_WICPixelFormat8bppGray,              DXGI_FORMAT_R8_UNORM },
		{ GUID_WICPixelFormat8bppAlpha,             DXGI_FORMAT_A8_UNORM }
	};

	// GUID -> Standard GUID ��ʽת���ṹ��
	struct WICConvert
	{
		GUID source;
		GUID target;
	};

	// WIC ���ظ�ʽת����
	static WICConvert g_WICConvert[] =
	{
		// Ŀ���ʽһ������ӽ��ı�֧�ֵĸ�ʽ
		{ GUID_WICPixelFormatBlackWhite,            GUID_WICPixelFormat8bppGray },			// DXGI_FORMAT_R8_UNORM
		{ GUID_WICPixelFormat1bppIndexed,           GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat2bppIndexed,           GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat4bppIndexed,           GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat8bppIndexed,           GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat2bppGray,              GUID_WICPixelFormat8bppGray },			// DXGI_FORMAT_R8_UNORM
		{ GUID_WICPixelFormat4bppGray,              GUID_WICPixelFormat8bppGray },			// DXGI_FORMAT_R8_UNORM
		{ GUID_WICPixelFormat16bppGrayFixedPoint,   GUID_WICPixelFormat16bppGrayHalf },		// DXGI_FORMAT_R16_FLOAT
		{ GUID_WICPixelFormat32bppGrayFixedPoint,   GUID_WICPixelFormat32bppGrayFloat },	// DXGI_FORMAT_R32_FLOAT
		{ GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat16bppBGRA5551 },		// DXGI_FORMAT_B5G5R5A1_UNORM
		{ GUID_WICPixelFormat32bppBGR101010,        GUID_WICPixelFormat32bppRGBA1010102 },	// DXGI_FORMAT_R10G10B10A2_UNORM
		{ GUID_WICPixelFormat24bppBGR,              GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat24bppRGB,              GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat32bppPBGRA,            GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat32bppPRGBA,            GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat48bppRGB,              GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat48bppBGR,              GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat64bppBGRA,             GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat64bppPRGBA,            GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat64bppPBGRA,            GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat48bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat48bppBGRFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat64bppRGBAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat64bppBGRAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat64bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat48bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat64bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat128bppPRGBAFloat,      GUID_WICPixelFormat128bppRGBAFloat },	// DXGI_FORMAT_R32G32B32A32_FLOAT
		{ GUID_WICPixelFormat128bppRGBFloat,        GUID_WICPixelFormat128bppRGBAFloat },	// DXGI_FORMAT_R32G32B32A32_FLOAT
		{ GUID_WICPixelFormat128bppRGBAFixedPoint,  GUID_WICPixelFormat128bppRGBAFloat },	// DXGI_FORMAT_R32G32B32A32_FLOAT
		{ GUID_WICPixelFormat128bppRGBFixedPoint,   GUID_WICPixelFormat128bppRGBAFloat },	// DXGI_FORMAT_R32G32B32A32_FLOAT
		{ GUID_WICPixelFormat32bppRGBE,             GUID_WICPixelFormat128bppRGBAFloat },	// DXGI_FORMAT_R32G32B32A32_FLOAT
		{ GUID_WICPixelFormat32bppCMYK,             GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat64bppCMYK,             GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat40bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat80bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat32bppRGB,              GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat64bppRGB,              GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat64bppPRGBAHalf,        GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT

		{ GUID_WICPixelFormat128bppRGBAFloat,       GUID_WICPixelFormat128bppRGBAFloat },	// DXGI_FORMAT_R32G32B32A32_FLOAT
		{ GUID_WICPixelFormat64bppRGBAHalf,         GUID_WICPixelFormat64bppRGBAHalf },		// DXGI_FORMAT_R16G16B16A16_FLOAT
		{ GUID_WICPixelFormat64bppRGBA,             GUID_WICPixelFormat64bppRGBA },			// DXGI_FORMAT_R16G16B16A16_UNORM
		{ GUID_WICPixelFormat32bppRGBA,             GUID_WICPixelFormat32bppRGBA },			// DXGI_FORMAT_R8G8B8A8_UNORM
		{ GUID_WICPixelFormat32bppBGRA,             GUID_WICPixelFormat32bppBGRA },			// DXGI_FORMAT_B8G8R8A8_UNORM
		{ GUID_WICPixelFormat32bppBGR,              GUID_WICPixelFormat32bppBGR },			// DXGI_FORMAT_B8G8R8X8_UNORM
		{ GUID_WICPixelFormat32bppRGBA1010102XR,    GUID_WICPixelFormat32bppRGBA1010102XR },// DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM
		{ GUID_WICPixelFormat32bppRGBA1010102,      GUID_WICPixelFormat32bppRGBA1010102 },	// DXGI_FORMAT_R10G10B10A2_UNORM
		{ GUID_WICPixelFormat16bppBGRA5551,         GUID_WICPixelFormat16bppBGRA5551 },		// DXGI_FORMAT_B5G5R5A1_UNORM
		{ GUID_WICPixelFormat16bppBGR565,           GUID_WICPixelFormat16bppBGR565 },		// DXGI_FORMAT_B5G6R5_UNORM
		{ GUID_WICPixelFormat32bppGrayFloat,        GUID_WICPixelFormat32bppGrayFloat },	// DXGI_FORMAT_R32_FLOAT
		{ GUID_WICPixelFormat16bppGrayHalf,         GUID_WICPixelFormat16bppGrayHalf },		// DXGI_FORMAT_R16_FLOAT
		{ GUID_WICPixelFormat16bppGray,             GUID_WICPixelFormat16bppGray },			// DXGI_FORMAT_R16_UNORM
		{ GUID_WICPixelFormat8bppGray,              GUID_WICPixelFormat8bppGray },			// DXGI_FORMAT_R8_UNORM
		{ GUID_WICPixelFormat8bppAlpha,             GUID_WICPixelFormat8bppAlpha }			// DXGI_FORMAT_A8_UNORM
	};


	// ���ȷ�����ݵ���ӽ���ʽ���ĸ�
	bool GetTargetPixelFormat(const GUID* pSourceFormat, GUID* pTargetFormat)
	{
		*pTargetFormat = *pSourceFormat;
		for (size_t i = 0; i < _countof(g_WICConvert); ++i)
		{
			if (InlineIsEqualGUID(g_WICConvert[i].source, *pSourceFormat))
			{
				*pTargetFormat = g_WICConvert[i].target;
				return true;
			}
		}
		return false;		// �Ҳ������ͷ��� false
	}

	// ���ȷ�����ն�Ӧ�� DXGI ��ʽ����һ��
	DXGI_FORMAT GetDXGIFormatFromPixelFormat(const GUID* pPixelFormat)
	{
		for (size_t i = 0; i < _countof(g_WICFormats); ++i)
		{
			if (InlineIsEqualGUID(g_WICFormats[i].wic, *pPixelFormat))
			{
				return g_WICFormats[i].format;
			}
		}
		return DXGI_FORMAT_UNKNOWN;		// �Ҳ������ͷ��� UNKNOWN
	}
}


// ���ڰ󶨻ص��������м��
class CallBackWrapper
{
public:

	// ���ڱ��� DX12Engine ��ĳ�Ա�ص������İ�װ��
	// ǰ 5 ������һֱ��������Ļص��������ó� static ��̬����������Ϊ WIN32 API ���ô� C ���д��
	// WNDCLASS �� lpfnWndProc �� C-Style �ĺ���ָ�룬�� DX12Engine::CallBackFunc �����Ա����������Ҫ����һ�� this ָ��
	// ��� this ָ�뻹��������ʵ���Ķ�����Ϣ (���Ա,�麯����,��̳й�ϵ)������ lpfnWndProc ��������� this ָ��
	// �������������ݣ�����û��ֱ�ӽ� DX12Engine::CallBackFunc ��ֵ�� lpfnWndProc�������ǿ��ת�������� (reinterpret_cast Ҳ����)
	// ���ǿ������� C++11 �ĺ�����װ�� std::function �������������� DX12Engine::CallBackFunc
	// [���õ���ģ��+�º��� (��������) �հ������ԣ�����Ȥ���Բ�����ϣ����˽�һ�¡����������Կ���Դ�룬�������ڲ�ʵ�ַǳ�����]
	// Ȼ����ͨ�����ĵľ�̬���� CallBackWrapper::CallBackFunc ��ӵ��ã������ CallBackWrapper::CallBackFunc ���� lpfnWndProc
	// ������ʵ���� ��ص����� -> C-Style ��ͨ�ص����� ��ת��
	// �� C++17 �� inline static ԭ���� static ��̬�ǳ�����ԱҪ���������������ⶨ�壬�������ⶨ��Ļ��ͻᱨ�������Ӵ���
	// ���ڵľ�̬��Ա����������һ��������Ҫ�����ʱ��ŷ����ڴ棬�ڶ���֮ǰ���ǲ��ɷ��ʵģ����Ծ�̬�ǳ�����Ա���������ڳ�ʼ��
	// [������Կ���https://www.cnblogs.com/lixuejian/p/13215271]
	// ���ȥ�� inline����Ҫ�ں�����϶��塣��������������ļ����룬ÿ���ļ���Ҫ�����������ʱ�򣬾ͻᷢ�����Ӵ��󣬷ǳ��鷳
	// inline static ����̬��Ա��������ֱ�����ڳ�ʼ��������������������������鷳������������ǲ�ʹ����
	inline static std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> Broker_Func;
	
	// ���ڴ��ݵ� lpfnWndProc �ľ�̬��Ա�������ڲ����ñ��� DX12Engine::CallBackFunc �ĺ�����װ��
	// ��̬��Ա���������࣬��������ʵ����������û�� this ָ�룬����ֱ�Ӹ�ֵ�� C-Style �ĺ���ָ��
	static LRESULT CALLBACK CallBackFunc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return Broker_Func(hwnd, msg, wParam, lParam);
	}

};


// �������
class Camera
{
private:

	XMVECTOR EyePosition = XMVectorSet(4, 3, 4, 1);			// �����������ռ��µ�λ��
	XMVECTOR FocusPosition = XMVectorSet(0, 1, 1, 1);		// �����������ռ��¹۲�Ľ���λ��
	XMVECTOR UpDirection = XMVectorSet(0, 1, 0, 0);			// ����ռ䴹ֱ���ϵ�����

	// ������۲췽��ĵ�λ����������ǰ���ƶ�
	XMVECTOR ViewDirection = XMVector3Normalize(FocusPosition - EyePosition);

	// ���࣬�����ԭ���뽹��ľ��룬XMVector3Length ��ʾ������ȡģ
	float FocalLength = XMVectorGetX(XMVector3Length(FocusPosition - EyePosition));

	// ��������ҷ���ĵ�λ���������������ƶ���XMVector3Cross �����������
	XMVECTOR RightDirection = XMVector3Normalize(XMVector3Cross(ViewDirection, UpDirection));

	POINT LastCursorPoint = {};								// ��һ������λ��

	float FovAngleY = XM_PIDIV4;							// ��ֱ�ӳ���
	float AspectRatio = 4.0 / 3.0;							// ͶӰ���ڿ�߱�
	float NearZ = 0.1;										// ��ƽ�浽ԭ��ľ���
	float FarZ = 1000;										// Զƽ�浽ԭ��ľ���

	XMMATRIX ModelMatrix;									// ģ�;���ģ�Ϳռ� -> ����ռ�
	XMMATRIX ViewMatrix;									// �۲��������ռ� -> �۲�ռ�
	XMMATRIX ProjectionMatrix;								// ͶӰ���󣬹۲�ռ� -> ��βü��ռ�

	XMMATRIX MVPMatrix;										// MVP ����������Ҫ�ù��з��� GetMVPMatrix ��ȡ

public:

	Camera()	// ������Ĺ��캯��
	{
		// ģ�;�������������ģ����ת 30�� ���У�ע������ֻ��һ��ʾ�����������ǻὫ���Ƴ���ÿ��ģ�Ͷ�Ӧ��ӵ����Զ�����ģ�;���
		ModelMatrix = XMMatrixRotationY(30.0f);
		// �۲����ע��ǰ���������ǵ㣬������������������
		ViewMatrix = XMMatrixLookAtLH(EyePosition, FocusPosition, UpDirection);
		// ͶӰ���� (ע���ƽ���Զƽ����벻�� <= 0!)
		ProjectionMatrix = XMMatrixPerspectiveFovLH(FovAngleY, AspectRatio, NearZ, FarZ);
	}

	// �����ǰ���ƶ������� Stride ���ƶ��ٶ� (����)��������ǰ�ƶ�����������ƶ�
	void Walk(float Stride)
	{
		EyePosition += Stride * ViewDirection;
		FocusPosition += Stride * ViewDirection;
	}

	// ����������ƶ������� Stride ���ƶ��ٶ� (����)�����������ƶ������������ƶ�
	void Strafe(float Stride)
	{
		EyePosition += Stride * RightDirection;
		FocusPosition += Stride * RightDirection;
	}

	// �������Ļ�ռ� y �����ƶ����൱������������ҵ����� RightDirection ����������ת�����������¿�
	void RotateByY(float angleY)
	{
		// ����������Ϊ�ṹ����ת������ת ViewDirection �� UpDirection
		XMMATRIX R = XMMatrixRotationAxis(RightDirection, -angleY);

		UpDirection = XMVector3TransformNormal(UpDirection, R);
		ViewDirection = XMVector3TransformNormal(ViewDirection, R);

		// ���� ViewDirection �۲�������FocalLength ���࣬���½���λ��
		FocusPosition = EyePosition + ViewDirection * FocalLength;
	}

	// �������Ļ�ռ� x �����ƶ����൱�������������ռ�� y ������������ת�����������ҿ�
	void RotateByX(float angleX)
	{
		// ����������ϵ�µ� y �� (0,1,0,0) ������ת������������ ViewDirection, UpDirection, RightDirection ��Ҫ��ת
		XMMATRIX R = XMMatrixRotationY(angleX);

		UpDirection = XMVector3TransformNormal(UpDirection, R);
		ViewDirection = XMVector3TransformNormal(ViewDirection, R);
		RightDirection = XMVector3TransformNormal(RightDirection, R);

		// ���� ViewDirection �۲�������FocalLength ���࣬���½���λ��
		FocusPosition = EyePosition + ViewDirection * FocalLength;
	}

	// ������һ�ε����λ��
	void UpdateLastCursorPos()
	{
		GetCursorPos(&LastCursorPoint);
	}

	// ���������������ƶ�ʱ����ת������ӽ�
	void CameraRotate()
	{
		POINT CurrentCursorPoint = {};
		GetCursorPos(&CurrentCursorPoint);	// ��ȡ��ǰ���λ��

		// �����������Ļ����ϵ�� x,y ���ƫ�����������������ת��
		float AngleX = XMConvertToRadians(0.25 * static_cast<float>(CurrentCursorPoint.x - LastCursorPoint.x));
		float AngleY = XMConvertToRadians(0.25 * static_cast<float>(CurrentCursorPoint.y - LastCursorPoint.y));

		// ��ת�����
		RotateByY(AngleY);
		RotateByX(AngleX);

		UpdateLastCursorPos();		// ��ת��ϣ�������һ�ε����λ��
	}

	// ���� MVP ����
	void UpdateMVPMatrix()
	{
		// ��Ҫ�Ǹ��¹۲����
		ViewMatrix = XMMatrixLookAtLH(EyePosition, FocusPosition, UpDirection);
		MVPMatrix = ModelMatrix * ViewMatrix * ProjectionMatrix;
	}

	// ��ȡ MVP ����
	inline XMMATRIX& GetMVPMatrix()
	{
		// ÿ�η���ǰ��������һ��
		UpdateMVPMatrix();
		return MVPMatrix;
	}
};


// DX12 ����
class DX12Engine
{
private:

	int WindowWidth = 640;		// ���ڿ��
	int WindowHeight = 480;		// ���ڸ߶�
	HWND m_hwnd;				// ���ھ��

	ComPtr<ID3D12Debug> m_D3D12DebugDevice;					// D3D12 ���Բ��豸
	UINT m_DXGICreateFactoryFlag = NULL;					// ���� DXGI ����ʱ��Ҫ�õ��ı�־

	ComPtr<IDXGIFactory5> m_DXGIFactory;					// DXGI ����
	ComPtr<IDXGIAdapter1> m_DXGIAdapter;					// ��ʾ������ (�Կ�)
	ComPtr<ID3D12Device4> m_D3D12Device;					// D3D12 �����豸

	ComPtr<ID3D12CommandQueue> m_CommandQueue;				// �������
	ComPtr<ID3D12CommandAllocator> m_CommandAllocator;		// ���������
	ComPtr<ID3D12GraphicsCommandList> m_CommandList;		// �����б�

	ComPtr<IDXGISwapChain3> m_DXGISwapChain;				// DXGI ������
	ComPtr<ID3D12DescriptorHeap> m_RTVHeap;					// RTV ��������
	ComPtr<ID3D12Resource> m_RenderTarget[3];				// ��ȾĿ�����飬ÿһ����ȾĿ���Ӧһ�����ڻ�����
	D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle;					// RTV ���������
	UINT RTVDescriptorSize = 0;								// RTV �������Ĵ�С
	UINT FrameIndex = 0;									// ֡��������ʾ��ǰ��Ⱦ�ĵ� i ֡ (�� i ����ȾĿ��)

	ComPtr<ID3D12Fence> m_Fence;							// Χ��
	UINT64 FenceValue = 0;									// ����Χ���ȴ���Χ��ֵ
	HANDLE RenderEvent = NULL;								// GPU ��Ⱦ�¼�
	D3D12_RESOURCE_BARRIER beg_barrier = {};				// ��Ⱦ��ʼ����Դ���ϣ����� -> ��ȾĿ��
	D3D12_RESOURCE_BARRIER end_barrier = {};				// ��Ⱦ��������Դ���ϣ���ȾĿ�� -> ����

	std::wstring TextureFilename = L"diamond_ore.png";		// �����ļ��� (�����õ������·��)
	ComPtr<IWICImagingFactory> m_WICFactory;				// WIC ����
	ComPtr<IWICBitmapDecoder> m_WICBitmapDecoder;			// λͼ������
	ComPtr<IWICBitmapFrameDecode> m_WICBitmapDecodeFrame;	// �ɽ������õ��ĵ���λͼ֡
	ComPtr<IWICFormatConverter> m_WICFormatConverter;		// λͼת����
	ComPtr<IWICBitmapSource> m_WICBitmapSource;				// WIC λͼ��Դ�����ڻ�ȡλͼ����
	UINT TextureWidth = 0;									// ������
	UINT TextureHeight = 0;									// ����߶�
	UINT BitsPerPixel = 0;									// ͼ����ȣ�ͼƬÿ������ռ�õı�����
	UINT BytePerRowSize = 0;								// ����ÿ�����ݵ���ʵ�ֽڴ�С�����ڶ�ȡ�������ݡ��ϴ�������Դ
	DXGI_FORMAT TextureFormat = DXGI_FORMAT_UNKNOWN;		// �����ʽ

	ComPtr<ID3D12DescriptorHeap> m_SRVHeap;					// SRV ��������
	D3D12_CPU_DESCRIPTOR_HANDLE SRV_CPUHandle;				// SRV ������ CPU ���
	D3D12_GPU_DESCRIPTOR_HANDLE SRV_GPUHandle;				// SRV ������ GPU ���

	ComPtr<ID3D12Resource> m_UploadTextureResource;			// �ϴ�����Դ��λ�ڹ����ڴ棬������ת������Դ
	ComPtr<ID3D12Resource> m_DefaultTextureResource;		// Ĭ�϶���Դ��λ���Դ棬���ڷ�����
	UINT TextureSize = 0;									// �������ʵ��С (��λ���ֽ�)
	UINT UploadResourceRowSize = 0;							// �ϴ�����Դÿ�еĴ�С (��λ���ֽ�)
	UINT UploadResourceSize = 0;							// �ϴ�����Դ���ܴ�С (��λ���ֽ�)


	ComPtr<ID3D12Resource> m_CBVResource;		// ����������Դ�����ڴ�� MVP ����MVP ����ÿ֡��Ҫ���£�������Ҫ�洢�ڳ�����������
	struct CBuffer								// ��������ṹ��
	{
		XMFLOAT4X4 MVPMatrix;		// MVP �������ڽ��������ݴӶ���ռ�任����βü��ռ�
	};
	CBuffer* MVPBuffer = nullptr;	// ��������ṹ��ָ�룬����洢���� MVP ������Ϣ������ Map ��ָ���ָ�� CBVResource �ĵ�ַ

	Camera m_FirstCamera;			// ��һ�˳������

	ComPtr<ID3D12RootSignature> m_RootSignature;			// ��ǩ��
	ComPtr<ID3D12PipelineState> m_PipelineStateObject;		// ��Ⱦ����״̬


	ComPtr<ID3D12Resource> m_VertexResource;				// ������Դ
	struct VERTEX											// �������ݽṹ��
	{
		XMFLOAT4 position;									// ����λ��
		XMFLOAT2 texcoordUV;								// ������������
	};
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView;				// ���㻺��������


	ComPtr<ID3D12Resource> m_IndexResource;					// ������Դ
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};			// ��������������

	// �ӿ�
	D3D12_VIEWPORT viewPort = D3D12_VIEWPORT{ 0, 0, float(WindowWidth), float(WindowHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	// �ü�����
	D3D12_RECT ScissorRect = D3D12_RECT{ 0, 0, WindowWidth, WindowHeight };

public:

	// ��ʼ������
	void InitWindow(HINSTANCE hins)
	{
		WNDCLASS wc = {};					// ���ڼ�¼��������Ϣ�Ľṹ��
		wc.hInstance = hins;				// ��������Ҫһ��Ӧ�ó����ʵ����� hinstance

		// �󶨻ص����������� std::bind���� DX12Engine::CallBackFunc �󶨵� CallBackWrapper �ĺ�����װ����
		// std::bind �������ǽ� CallBackFunc �������󶨵�һ�������ϣ������ɶ�Ӧ�ĺ�����װ��
		// ��һ��������ʾ����ĳ�Ա������ָ�룬��Ϊ���������Ὣ����ĳ�Ա������ʽת���ɺ���ָ�룬���Ա�����ǰ����� &
		// �ڶ���������ʾ����ĵ�ַ����������ͱ�ʾ���Ա������Ҫ���ݵ� this ָ��
		// ֮��ı�ʾ��Ҫ�󶨵Ĳ�����std::placeholders ��ʾռλ�����û����ú���������ʵ��ʱ�����ռλ���Ὣʵ��һһ��Ӧ
		CallBackWrapper::Broker_Func = std::bind(&DX12Engine::CallBackFunc, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

		wc.lpfnWndProc = CallBackWrapper::CallBackFunc;		// ��������Ҫһ���ص����������ڴ����ڲ�������Ϣ��ע�����ﴫ�ݵ����м��Ļص�����
		wc.lpszClassName = L"DX12 Game";					// �����������

		RegisterClass(&wc);					// ע�ᴰ���࣬��������¼�뵽����ϵͳ��

		// ʹ�����ĵĴ����ഴ������
		m_hwnd = CreateWindow(wc.lpszClassName, L"DX12����ʯԭ��", WS_SYSMENU | WS_OVERLAPPED,
			10, 10, WindowWidth, WindowHeight,
			NULL, NULL, hins, NULL);

		// ��Ϊָ���˴��ڴ�С���ɱ�� WS_SYSMENU �� WS_OVERLAPPED��Ӧ�ò����Զ���ʾ���ڣ���Ҫʹ�� ShowWindow ǿ����ʾ����
		ShowWindow(m_hwnd, SW_SHOW);
	}

	// �������Բ�
	void CreateDebugDevice()
	{
		::CoInitialize(nullptr);	// ע�����DX12 �������豸�ӿڶ��ǻ��� COM �ӿڵģ�������Ҫ��ȫ����ʼ��Ϊ nullptr

#if defined(_DEBUG)		// ����� Debug ģʽ�±��룬��ִ������Ĵ���

		// ��ȡ���Բ��豸�ӿ�
		D3D12GetDebugInterface(IID_PPV_ARGS(&m_D3D12DebugDevice));
		// �������Բ�
		m_D3D12DebugDevice->EnableDebugLayer();
		// �������Բ�󣬴��� DXGI ����Ҳ��Ҫ Debug Flag
		m_DXGICreateFactoryFlag = DXGI_CREATE_FACTORY_DEBUG;

#endif
	}

	// �����豸
	bool CreateDevice()
	{
		// ���� DXGI ����
		CreateDXGIFactory2(m_DXGICreateFactoryFlag, IID_PPV_ARGS(&m_DXGIFactory));

		// DX12 ֧�ֵ����й��ܰ汾������Կ������Ҫ֧�� 11.0
		const D3D_FEATURE_LEVEL dx12SupportLevel[] =
		{
			D3D_FEATURE_LEVEL_12_2,		// 12.2
			D3D_FEATURE_LEVEL_12_1,		// 12.1
			D3D_FEATURE_LEVEL_12_0,		// 12.0
			D3D_FEATURE_LEVEL_11_1,		// 11.1
			D3D_FEATURE_LEVEL_11_0		// 11.0
		};


		// �� EnumAdapters1 �ȱ��������ϵ�ÿһ���Կ�
		// ÿ�ε��� EnumAdapters1 �ҵ��Կ����Զ����� DXGIAdapter �ӿڣ������� S_OK
		// �Ҳ����Կ��᷵�� ERROR_NOT_FOUND

		for (UINT i = 0; m_DXGIFactory->EnumAdapters1(i, &m_DXGIAdapter) != ERROR_NOT_FOUND; i++)
		{
			// �ҵ��Կ����ʹ��� D3D12 �豸���Ӹߵ��ͱ������й��ܰ汾�������ɹ�������
			for (const auto& level : dx12SupportLevel)
			{
				// ���� D3D12 ���Ĳ��豸�������ɹ��ͷ��� true
				if (SUCCEEDED(D3D12CreateDevice(m_DXGIAdapter.Get(), level, IID_PPV_ARGS(&m_D3D12Device))))
				{
					DXGI_ADAPTER_DESC1 adap = {};
					m_DXGIAdapter->GetDesc1(&adap);
					OutputDebugStringW(adap.Description);
					return true;
				}
			}
		}

		// ����Ҳ����κ���֧�� DX12 ���Կ������˳�����
		if (m_D3D12Device == nullptr)
		{
			MessageBox(NULL, L"�Ҳ����κ���֧�� DX12 ���Կ��������������ϵ�Ӳ����", L"����", MB_OK | MB_ICONERROR);
			return false;
		}
	}

	// ��������������
	void CreateCommandComponents()
	{
		// ������Ϣ�ṹ�壬����ֻ��Ҫ����е����� type ������
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		// D3D12_COMMAND_LIST_TYPE_DIRECT ��ʾ�����ֱ�ӷŽ������������������
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		// �����������
		m_D3D12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue));

		// ������������������������ǿ����ڴ棬�洢�����б��ϵ����ע����������Ҫһ��
		m_D3D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator));

		// ����ͼ�������б�ע����������Ҫһ��
		m_D3D12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator.Get(),
			nullptr, IID_PPV_ARGS(&m_CommandList));

		// �����б���ʱ���� Record ¼��״̬��������Ҫ�ر������������ĵ� Reset ���ܳɹ�
		m_CommandList->Close();
	}

	// ������ȾĿ�꣬����ȾĿ������Ϊ����
	void CreateRenderTarget()
	{
		// ���� RTV �������� (Render Target View����ȾĿ��������)
		D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc = {};
		RTVHeapDesc.NumDescriptors = 3;							// ��ȾĿ�������
		RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;		// �������ѵ����ͣ�RTV
		// ����һ�� RTV �������ѣ������ɹ��󣬻��Զ������������������ڴ�
		m_D3D12Device->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(&m_RTVHeap));


		// ���� DXGI �����������ڽ����ڻ���������ȾĿ���
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
		swapchainDesc.BufferCount = 3;								// ����������
		swapchainDesc.Width = WindowWidth;							// ������ (����) ���
		swapchainDesc.Height = WindowHeight;						// ������ (����) �߶�
		swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			// ��������ʽ��ָ��������ÿ�����صĴ�С
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// ���������ͣ��� FILP �� BITBLT ��������
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;// ����������;�������ʾ�ѻ�����������ȾĿ������
		swapchainDesc.SampleDesc.Count = 1;							// ���������ز�������

		// ��ʱ�Ͱ汾�������ӿڣ����ڴ����߰汾����������Ϊ���ĵ� CreateSwapChainForHwnd ����ֱ�����ڴ����߰汾�ӿ�
		ComPtr<IDXGISwapChain1> _temp_swapchain;

		// ����������������������ȾĿ���
		// ע�⣺��������Ҫ�󶨵����������ˢ�£����Ե�һ������Ҫ��������У�����ᴴ��ʧ�ܣ�
		m_DXGIFactory->CreateSwapChainForHwnd(m_CommandQueue.Get(), m_hwnd,
			&swapchainDesc, nullptr, nullptr, &_temp_swapchain);

		// ͨ�� As ���������Ͱ汾�ӿڵ���Ϣ���ݸ��߰汾�ӿ�
		_temp_swapchain.As(&m_DXGISwapChain);


		// �����꽻���������ǻ���Ҫ�� RTV ������ ָ�� ��ȾĿ��
		// ��Ϊ ID3D12Resource ������ֻ��һ�����ݣ�������û�ж������÷���˵��
		// ����Ҫ�ó���֪�����������һ����ȾĿ�꣬�͵ô�����ʹ�� RTV ������

		// ��ȡ RTV ��ָ�����������ľ��
		RTVHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
		// ��ȡ RTV �������Ĵ�С
		RTVDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (UINT i = 0; i < 3; i++)
		{
			// �ӽ������л�ȡ�� i �����ڻ��壬������ i �� RenderTarget ��ȾĿ��
			m_DXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTarget[i]));

			// ���� RTV ������������ȾĿ��󶨵���������
			m_D3D12Device->CreateRenderTargetView(m_RenderTarget[i].Get(), nullptr, RTVHandle);

			// ƫ�Ƶ���һ�� RTV ���
			RTVHandle.ptr += RTVDescriptorSize;
		}
	}

	// ����Χ������Դ���ϣ����� CPU-GPU ��ͬ��
	void CreateFenceAndBarrier()
	{
		// ���� CPU �ϵĵȴ��¼�
		RenderEvent = CreateEvent(nullptr, false, true, nullptr);

		// ����Χ�����趨��ʼֵΪ 0
		m_D3D12Device->CreateFence(FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));


		// ������Դ����
		// beg_barrier ��ʼ���ϣ�Present ����״̬ -> Render Target ��ȾĿ��״̬
		beg_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;					// ָ������Ϊת������		
		beg_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		beg_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		// end_barrier ��ֹ���ϣ�Render Target ��ȾĿ��״̬ -> Present ����״̬
		end_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		end_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		end_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	}

	// ���������ڴ���
	bool LoadTextureFromFile()
	{
		// �����û���� WIC ���������½�һ�� WIC ����ʵ����ע�⣡WIC �����������ظ��ͷ��봴����
		if (m_WICFactory == nullptr) CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_WICFactory));

		// ����ͼƬ������������ͼƬ���뵽��������
		HRESULT hr = m_WICFactory->CreateDecoderFromFilename(TextureFilename.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &m_WICBitmapDecoder);

		std::wostringstream output_str;		// ���ڸ�ʽ���ַ���
		switch (hr)
		{
		case S_OK: break;	// ����ɹ���ֱ�� break ������һ������

		case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):	// �ļ��Ҳ���
			output_str << L"�Ҳ����ļ� " << TextureFilename << L" �������ļ�·���Ƿ�����";
			MessageBox(NULL, output_str.str().c_str(), L"����", MB_OK | MB_ICONERROR);
			return false;

		case HRESULT_FROM_WIN32(ERROR_FILE_CORRUPT):	// �ļ�������ڱ���һ��Ӧ�ý���ռ��
			output_str << L"�ļ� " << TextureFilename << L" �Ѿ�����һ��Ӧ�ý��̴򿪲�ռ���ˣ����ȹر��Ǹ�Ӧ�ý��̣�";
			MessageBox(NULL, output_str.str().c_str(), L"����", MB_OK | MB_ICONERROR);
			return false;

		case WINCODEC_ERR_COMPONENTNOTFOUND:			// �Ҳ����ɽ���������˵���ⲻ����Ч��ͼ���ļ�
			output_str << L"�ļ� " << TextureFilename << L" ������Ч��ͼ���ļ����޷����룡�����ļ��Ƿ�Ϊͼ���ļ���";
			MessageBox(NULL, output_str.str().c_str(), L"����", MB_OK | MB_ICONERROR);
			return false;

		default:			// ��������δ֪����
			output_str << L"�ļ� " << TextureFilename << L" ����ʧ�ܣ��������������󣬴����룺" << hr << L" �������΢��ٷ��ĵ���";
			MessageBox(NULL, output_str.str().c_str(), L"����", MB_OK | MB_ICONERROR);
			return false;
		}

		// ��ȡͼƬ���ݵĵ�һ֡����� GetFrame �������� gif ���ֶ�֡��ͼ
		m_WICBitmapDecoder->GetFrame(0, &m_WICBitmapDecodeFrame);


		// ��ȡͼƬ��ʽ��������ת��Ϊ DX12 �ܽ��ܵ������ʽ
		// ���������ʽ�޷�֧�ֵĴ��󣬿�����΢���ṩ�� ��ͼ3D ��ת����ǿ���Ƽ�!
		WICPixelFormatGUID SourceFormat = {};				// Դͼ��ʽ
		GUID TargetFormat = {};								// Ŀ���ʽ

		m_WICBitmapDecodeFrame->GetPixelFormat(&SourceFormat);						// ��ȡԴͼ��ʽ

		if (DX12TextureHelper::GetTargetPixelFormat(&SourceFormat, &TargetFormat))	// ��ȡĿ���ʽ
		{
			TextureFormat = DX12TextureHelper::GetDXGIFormatFromPixelFormat(&TargetFormat);	// ��ȡ DX12 ֧�ֵĸ�ʽ
		}
		else	// ���û�п�֧�ֵ�Ŀ���ʽ
		{
			::MessageBox(NULL, L"��������֧��!", L"��ʾ", MB_OK);
			return false;
		}


		// ��ȡĿ���ʽ�󣬽�����ת��ΪĿ���ʽ��ʹ���ܱ� DX12 ʹ��
		m_WICFactory->CreateFormatConverter(&m_WICFormatConverter);		// ����ͼƬת����
		// ��ʼ��ת������ʵ�����ǰ�λͼ������ת��
		m_WICFormatConverter->Initialize(m_WICBitmapDecodeFrame.Get(), TargetFormat, WICBitmapDitherTypeNone,
			nullptr, 0.0f, WICBitmapPaletteTypeCustom);
		// ��λͼ���ݼ̳е� WIC λͼ��Դ������Ҫ����� WIC λͼ��Դ�ϻ�ȡ��Ϣ
		m_WICFormatConverter.As(&m_WICBitmapSource);



		m_WICBitmapSource->GetSize(&TextureWidth, &TextureHeight);		// ��ȡ������

		ComPtr<IWICComponentInfo> _temp_WICComponentInfo = {};			// ���ڻ�ȡ BitsPerPixel ����ͼ�����
		ComPtr<IWICPixelFormatInfo> _temp_WICPixelInfo = {};			// ���ڻ�ȡ BitsPerPixel ����ͼ�����
		m_WICFactory->CreateComponentInfo(TargetFormat, &_temp_WICComponentInfo);
		_temp_WICComponentInfo.As(&_temp_WICPixelInfo);
		_temp_WICPixelInfo->GetBitsPerPixel(&BitsPerPixel);				// ��ȡ BitsPerPixel ͼ�����

		return true;
	}

	// ���� SRV Descriptor Heap ��ɫ����Դ��������
	void CreateSRVHeap()
	{
		// ���� SRV �������� (Shader Resource View����ɫ����Դ������)
		D3D12_DESCRIPTOR_HEAP_DESC SRVHeapDesc = {};
		SRVHeapDesc.NumDescriptors = 1;									// ����ֻ��һ������ֻ��Ҫ��һ�� SRV ������
		SRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		// �����������ͣ�CBV��SRV��UAV ���������������Է���ͬһ������������
		SRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// �������ѱ�־��Shader-Visible ��ʾ����ɫ���ɼ�

		// ���� SRV ��������
		m_D3D12Device->CreateDescriptorHeap(&SRVHeapDesc, IID_PPV_ARGS(&m_SRVHeap));
	}

	// ��ȡ���㷨���� A ����ȡ�����ж�����Ҫ���ٸ�����Ϊ B �Ŀռ�������� A�������ڴ����
	inline UINT Ceil(UINT A, UINT B)
	{
		return (A + B - 1) / B;
	}

	// ���������ϴ��� UploadResource �����ڷ������ DefaultResource
	void CreateUploadAndDefaultResource()
	{
		// ��������ÿ�����ݵ���ʵ���ݴ�С (��λ��Byte �ֽ�)����Ϊ����ͼƬ���ڴ��������Դ洢��
		// ���ȡ�������ʵ��С����ȷ��ȡ�������ݡ��ϴ��� GPU�������Ȼ�ȡ����� BitsPerPixel ͼ����ȣ���Ϊ��ͬλͼ��ȿ��ܲ�ͬ
		// Ȼ���ټ���ÿ������ռ�õ��ֽڣ����� 8 ����Ϊ 1 Byte = 8 bits
		BytePerRowSize = TextureWidth * BitsPerPixel / 8;

		// �������ʵ��С (��λ���ֽ�)
		TextureSize = BytePerRowSize * TextureHeight;

		// �ϴ�����Դÿ�еĴ�С (��λ���ֽ�)��ע������Ҫ���� 256 �ֽڶ��룡
		// ��Ϊ GPU �� CPU �ܹ���ͬ��GPU ע�ز��м��㣬ע�ؽṹ�����ݵĿ��ٶ�ȡ����ȡ���ݶ����� 256 �ֽ�Ϊһ��������
		// ���Ҫ��Ҫ�� BytePerRowSize ���ж��룬�ж���Ҫ�ж����������������ÿ�����أ�������Ļ����ݻ����ġ�
		UploadResourceRowSize = Ceil(BytePerRowSize, 256) * 256;

		// �ϴ�����Դ���ܴ�С (��λ���ֽ�)������ռ����ֻ�಻�٣�����ᱨ D3D12 MinimumAlloc Error ��Դ�ڴ洴������
		// ע�����һ�в����ڴ���� (��Ϊ����û�������ˣ������ڴ����Ҳ����ȷ��ȡ)������Ҫ (TextureHeight - 1) �ټ� BytePerRowSize
		UploadResourceSize = UploadResourceRowSize * (TextureHeight - 1) + BytePerRowSize;


		// ������ת������ϴ�����Դ�ṹ��
		D3D12_RESOURCE_DESC UploadResourceDesc = {};
		UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		// ��Դ���ͣ��ϴ��ѵ���Դ���Ͷ��� buffer ����
		UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;			// ��Դ���֣�ָ����Դ�Ĵ洢��ʽ���ϴ��ѵ���Դ���� row major �������Դ洢
		UploadResourceDesc.Width = UploadResourceSize;						// ��Դ��ȣ��ϴ��ѵ���Դ�������Դ���ܴ�С��ע����Դ��С����ֻ�಻��
		UploadResourceDesc.Height = 1;										// ��Դ�߶ȣ��ϴ��ѽ����Ǵ���������Դ�ģ����Ը߶ȱ���Ϊ 1
		UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// ��Դ��ʽ���ϴ�����Դ�ĸ�ʽ����Ϊ UNKNOWN
		UploadResourceDesc.DepthOrArraySize = 1;							// ��Դ��ȣ������������������� 3D ����ģ��ϴ�����Դ����Ϊ 1
		UploadResourceDesc.MipLevels = 1;									// Mipmap �ȼ����������������ģ��ϴ�����Դ����Ϊ 1
		UploadResourceDesc.SampleDesc.Count = 1;							// ��Դ�����������ϴ�����Դ������ 1

		// �ϴ������ԵĽṹ�壬�ϴ���λ�� CPU �� GPU �Ĺ����ڴ�
		D3D12_HEAP_PROPERTIES UploadHeapDesc = { D3D12_HEAP_TYPE_UPLOAD };

		// �����ϴ�����Դ
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE, &UploadResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_UploadTextureResource));


		// ���ڷ������Ĭ�϶���Դ�ṹ��
		D3D12_RESOURCE_DESC DefaultResourceDesc = {};
		DefaultResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// ��Դ���ͣ�����ָ��Ϊ Texture2D 2D����
		DefaultResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;			// ������Դ�Ĳ��ֶ��� UNKNOWN
		DefaultResourceDesc.Width = TextureWidth;							// ��Դ��ȣ�������������
		DefaultResourceDesc.Height = TextureHeight;							// ��Դ�߶ȣ�����������߶�
		DefaultResourceDesc.Format = TextureFormat;							// ��Դ��ʽ�������������ʽ��Ҫ������һ��
		DefaultResourceDesc.DepthOrArraySize = 1;							// ��Դ��ȣ�����ֻ��һ������������ 1
		DefaultResourceDesc.MipLevels = 1;									// Mipmap �ȼ���������ʱ��ʹ�� Mipmap�������� 1
		DefaultResourceDesc.SampleDesc.Count = 1;							// ��Դ�������������������� 1 ����

		// Ĭ�϶����ԵĽṹ�壬Ĭ�϶�λ���Դ�
		D3D12_HEAP_PROPERTIES DefaultHeapDesc = { D3D12_HEAP_TYPE_DEFAULT };

		// ����Ĭ�϶���Դ
		m_D3D12Device->CreateCommittedResource(&DefaultHeapDesc, D3D12_HEAP_FLAG_NONE, &DefaultResourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_DefaultTextureResource));
	}

	// ��������з���������������ݸ��Ƶ� DefaultResource
	void CopyTextureDataToDefaultResource()
	{
		// ������ʱ�洢�������ݵ�ָ�룬����Ҫ�� malloc ����ռ�
		BYTE* TextureData = (BYTE*)malloc(TextureSize);

		// �������������ݶ��� TextureData �У�������ĵ� memcpy ���Ʋ���
		m_WICBitmapSource->CopyPixels(nullptr, BytePerRowSize, TextureSize, TextureData);

		// ���ڴ�����Դ��ָ��
		BYTE* TransferPointer = nullptr;

		// Map ��ʼӳ�䣬Map ������õ��ϴ�����Դ�ĵ�ַ (�ڹ����ڴ���)�����ݸ�ָ�룬�������Ǿ���ͨ�� memcpy ��������������
		m_UploadTextureResource->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));

		// ��������Ҫ���и������ݣ�ע������ָ��ƫ�Ƶĳ��Ȳ�ͬ��
		for (UINT i = 0; i < TextureHeight; i++)
		{
			// ���ϴ�����Դ���и����������� (CPU ���ٻ��� -> �����ڴ�)
			memcpy(TransferPointer, TextureData, BytePerRowSize);
			// ����ָ��ƫ�Ƶ���һ��
			TextureData += BytePerRowSize;
			// �ϴ�����Դָ��ƫ�Ƶ���һ�У�ע��ƫ�Ƴ��Ȳ�ͬ��
			TransferPointer += UploadResourceRowSize;
		}

		// Unmap ����ӳ�䣬��Ϊ�����޷�ֱ�Ӷ�дĬ�϶���Դ����Ҫ�ϴ��Ѹ��Ƶ�����ڸ���֮ǰ��������Ҫ�Ƚ���ӳ�䣬���ϴ��Ѵ���ֻ��״̬
		m_UploadTextureResource->Unmap(0, nullptr);

		TextureData -= TextureSize;		// ������Դָ��ƫ�ƻس�ʼλ��
		free(TextureData);				// �ͷ����� malloc ����Ŀռ䣬���������ò���������Ҫ����ռ�ڴ�

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint = {};						// ��Դ�ű�����������Ҫ���Ƶ���Դ
		D3D12_RESOURCE_DESC DefaultResourceDesc = m_DefaultTextureResource->GetDesc();	// Ĭ�϶���Դ�ṹ��

		// ��ȡ�����ƽű����������ĵ�������
		m_D3D12Device->GetCopyableFootprints(&DefaultResourceDesc, 0, 1, 0, &PlacedFootprint, nullptr, nullptr, nullptr);

		D3D12_TEXTURE_COPY_LOCATION DstLocation = {};						// ����Ŀ��λ�� (Ĭ�϶���Դ) �ṹ��
		DstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;		// ���������ͣ��������ָ������
		DstLocation.SubresourceIndex = 0;									// ָ��Ҫ���Ƶ�����Դ����
		DstLocation.pResource = m_DefaultTextureResource.Get();				// Ҫ���Ƶ�����Դ

		D3D12_TEXTURE_COPY_LOCATION SrcLocation = {};						// ����Դλ�� (�ϴ�����Դ) �ṹ��
		SrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;		// ���������ͣ��������ָ�򻺳���
		SrcLocation.PlacedFootprint = PlacedFootprint;						// ָ��Ҫ���Ƶ���Դ�ű���Ϣ
		SrcLocation.pResource = m_UploadTextureResource.Get();				// ���������ݵĻ���

		// ������Դ��Ҫʹ�� GPU �� CopyEngine �������棬������Ҫ��������з�����������
		m_CommandAllocator->Reset();								// ���������������
		m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);	// �����������б����������Ҫ PSO ״̬�����Եڶ��������� nullptr

		// ��¼������Դ��Ĭ�϶ѵ����� (�����ڴ� -> �Դ�) 
		m_CommandList->CopyTextureRegion(&DstLocation, 0, 0, 0, &SrcLocation, nullptr);
		// �ر������б�
		m_CommandList->Close();

		// ���ڴ��������õ���ʱ ID3D12CommandList ����
		ID3D12CommandList* _temp_cmdlists[] = { m_CommandList.Get() };

		// �ύ�������GPU ��ʼ���ƣ�
		m_CommandQueue->ExecuteCommandLists(1, _temp_cmdlists);


		// ��Χ��Ԥ��ֵ�趨Ϊ��һ֡��ע�⸴����ԴҲ��ҪΧ���ȴ�������ᷢ����Դ��ͻ
		FenceValue++;
		// ��������� (��������� GPU ��) ����Χ��Ԥ��ֵ�����������뵽���������
		// �������ִ�е�������޸�Χ��ֵ����ʾ��������ɣ�"����"Χ��
		m_CommandQueue->Signal(m_Fence.Get(), FenceValue);
		// ����Χ����Ԥ���¼������������ʱ��Χ����"����"������Ԥ���¼������¼������ź�״̬ת�������ź�״̬
		m_Fence->SetEventOnCompletion(FenceValue, RenderEvent);
	}

	// ���մ��� SRV ��ɫ����Դ���������������� DefaultResource Ϊһ������
	void CreateSRV()
	{
		// SRV ��������Ϣ�ṹ��
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDescriptorDesc = {};
		// SRV ���������ͣ���������ָ�� Texture2D 2D����
		SRVDescriptorDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		// SRV �������ĸ�ʽҲҪ�������ʽ
		SRVDescriptorDesc.Format = TextureFormat;
		// ���������ÿ���������� RGBA ������˳��D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING ��ʾ������������˳�򲻸ı�
		SRVDescriptorDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		// �������ǲ�ʹ�� Mipmap�������� 1
		SRVDescriptorDesc.Texture2D.MipLevels = 1;

		// ��ȡ SRV �������� CPU ӳ���������ڴ�����Դ
		SRV_CPUHandle = m_SRVHeap->GetCPUDescriptorHandleForHeapStart();

		// ���� SRV ������
		m_D3D12Device->CreateShaderResourceView(m_DefaultTextureResource.Get(), &SRVDescriptorDesc, SRV_CPUHandle);

		// ��ȡ SRV �������� GPU ӳ���������������б����� SRVHeap �������ѣ���ɫ������ SRV ��������������Դ
		SRV_GPUHandle = m_SRVHeap->GetGPUDescriptorHandleForHeapStart();
	}

	// ���� Constant Buffer Resource ����������Դ������������һ��Ԥ�ȷ���ĸ����Դ棬���ڴ洢ÿһ֡��Ҫ�任����Դ����������Ҫ�洢 MVP ����
	void CreateCBVResource()
	{
		// ������Դ��ȣ������������ṹ��Ĵ�С��ע�⣡Ӳ��Ҫ�󣬳���������Ҫ 256 �ֽڶ��룡��������Ҫ���� Ceil ����ȡ���������ڴ���룡
		UINT CBufferWidth = Ceil(sizeof(CBuffer), 256) * 256;

		D3D12_RESOURCE_DESC CBVResourceDesc = {};						// ����������Դ��Ϣ�ṹ��
		CBVResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;	// �ϴ�����Դ���ǻ���
		CBVResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;		// �ϴ�����Դ���ǰ��д洢���ݵ� (һά���Դ洢)
		CBVResourceDesc.Width = CBufferWidth;							// ������������Դ��� (Ҫ�����Դ���ܴ�С)
		CBVResourceDesc.Height = 1;										// �ϴ�����Դ���Ǵ洢һά������Դ�����Ը߶ȱ���Ϊ 1
		CBVResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// �ϴ�����Դ�ĸ�ʽ����Ϊ DXGI_FORMAT_UNKNOWN
		CBVResourceDesc.DepthOrArraySize = 1;							// ��Դ��ȣ������������������� 3D ����ģ��ϴ�����Դ����Ϊ 1
		CBVResourceDesc.MipLevels = 1;									// Mipmap �ȼ����������������ģ��ϴ�����Դ����Ϊ 1
		CBVResourceDesc.SampleDesc.Count = 1;							// ��Դ�����������ϴ�����Դ������ 1

		// �ϴ������ԵĽṹ�壬�ϴ���λ�� CPU �� GPU �Ĺ����ڴ�
		D3D12_HEAP_PROPERTIES UploadHeapDesc = { D3D12_HEAP_TYPE_UPLOAD };

		// ��������������Դ
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE, &CBVResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_CBVResource));



		// ��������ֱ�� Map ӳ�䵽�ṹ��ָ����м��ɣ����� Unmap �ر�ӳ�䣬Map-Unmap �Ǻ�ʱ���������ڶ�̬�������Ƕ�ֻ��Ҫ Map һ�ξ��У�Ȼ��ֱ�Ӷ�ָ���޸����ݣ�������ʵ���˳����������ݵ��޸�
		// ��Ϊ����ÿ֡��Ҫ�任 MVP ����ÿ֡��Ҫ�� MVPBuffer �����޸ģ���������ֱ�ӽ��ϴ�����Դ��ַӳ�䵽�ṹ��ָ��
		// ÿֱ֡�Ӷ�ָ������޸Ĳ����������ٽ��� Unmap �ˣ�������ɫ��ÿ֡���ܶ�ȡ���޸ĺ��������
		m_CBVResource->Map(0, nullptr, reinterpret_cast<void**>(&MVPBuffer));

	}


	// ������ǩ��
	void CreateRootSignature()
	{
		ComPtr<ID3DBlob> SignatureBlob;			// ��ǩ���ֽ���
		ComPtr<ID3DBlob> ErrorBlob;				// �����ֽ��룬��ǩ������ʧ��ʱ�� OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer()); ���Ի�ȡ������Ϣ

		D3D12_ROOT_PARAMETER RootParameters[2] = {};							// ����������

		// ��һ������������������ (Range: SRV)

		D3D12_DESCRIPTOR_RANGE SRVDescriptorRangeDesc = {};						// Range ��������Χ�ṹ�壬һ�� Range ��ʾһ��������ͬ����������
		SRVDescriptorRangeDesc.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;		// Range ���ͣ�����ָ�� SRV ���ͣ�CBV_SRV_UAV ���������
		SRVDescriptorRangeDesc.NumDescriptors = 1;								// Range ��������������� N��һ�ο��԰󶨶��������������Ĵ�������
		SRVDescriptorRangeDesc.BaseShaderRegister = 0;							// Range Ҫ�󶨵���ʼ�Ĵ����۱�� i���󶨷�Χ�� [s(i),s(i+N)]�����ǰ� s0
		SRVDescriptorRangeDesc.RegisterSpace = 0;								// Range Ҫ�󶨵ļĴ����ռ䣬���� Range ����󶨵�ͬһ�Ĵ����ռ��ϣ����ǰ� space0
		SRVDescriptorRangeDesc.OffsetInDescriptorsFromTableStart = 0;			// Range ����������ͷ��ƫ���� (��λ��������)����ǩ����Ҫ������Ѱ�� Range �ĵ�ַ���������� 0 ����

		D3D12_ROOT_DESCRIPTOR_TABLE RootDescriptorTableDesc = {};				// RootDescriptorTable ����������Ϣ�ṹ�壬һ�� Table �����ж�� Range
		RootDescriptorTableDesc.pDescriptorRanges = &SRVDescriptorRangeDesc;	// Range ��������Χָ��
		RootDescriptorTableDesc.NumDescriptorRanges = 1;						// ���������� Range ������

		RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;				// ����������ɫ���еĿɼ��ԣ�����ָ������������ɫ���ɼ� (ֻ��������ɫ���õ�������)
		RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	// ���������ͣ���������ѡ Table ��������һ����������ռ�� 1 DWORD
		RootParameters[0].DescriptorTable = RootDescriptorTableDesc;					// ������ָ��


		// �ڶ�����������CBV ���������������������������������������İ󶨸�����ʱ��ֻ��Ҫ���ݳ���������Դ�ĵ�ַ����

		D3D12_ROOT_DESCRIPTOR CBVRootDescriptorDesc = {};					// �����������������Ϣ�ṹ��
		CBVRootDescriptorDesc.ShaderRegister = 0;							// Ҫ�󶨵ļĴ�����ţ������Ӧ HLSL �� b0 �Ĵ���
		CBVRootDescriptorDesc.RegisterSpace = 0;							// Ҫ�󶨵������ռ䣬�����Ӧ HLSL �� space0

		RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// ���������������Ⱦ���߶��ɼ�
		RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// �ڶ��������������ͣ�CBV ��������
		RootParameters[1].Descriptor = CBVRootDescriptorDesc;				// �����ĵĽṹ��


		D3D12_STATIC_SAMPLER_DESC StaticSamplerDesc = {};						// ��̬�������ṹ�壬��̬����������ռ�ø�ǩ��
		StaticSamplerDesc.ShaderRegister = 0;									// Ҫ�󶨵ļĴ����ۣ���Ӧ s0
		StaticSamplerDesc.RegisterSpace = 0;									// Ҫ�󶨵ļĴ����ռ䣬��Ӧ space0
		StaticSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;		// ��̬����������ɫ���еĿɼ��ԣ�����ָ������������ɫ���ɼ� (ֻ��������ɫ���õ����������)
		StaticSamplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;	// ����������ͣ���������ֱ��ѡ �ڽ������ ����
		StaticSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;			// �� U �����ϵ�����Ѱַ��ʽ
		StaticSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;			// �� V �����ϵ�����Ѱַ��ʽ
		StaticSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;			// �� W �����ϵ�����Ѱַ��ʽ (3D ������õ�)
		StaticSamplerDesc.MinLOD = 0;											// ��С LOD ϸ�ڲ�Σ���������Ĭ���� 0 ����
		StaticSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;							// ��� LOD ϸ�ڲ�Σ���������Ĭ���� D3D12_FLOAT32_MAX (û�� LOD ����)
		StaticSamplerDesc.MipLODBias = 0;										// ���� Mipmap ����ƫ������������������ֱ���� 0 ����
		StaticSamplerDesc.MaxAnisotropy = 1;									// �������Թ��˵ȼ������ǲ�ʹ�ø������Թ��ˣ���ҪĬ���� 1
		StaticSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;			// �����������Ӱ��ͼ�ģ����ǲ���Ҫ������������ D3D12_COMPARISON_FUNC_NEVER


		D3D12_ROOT_SIGNATURE_DESC rootsignatureDesc = {};			// ��ǩ����Ϣ�ṹ�壬���� 64 DWORD����̬��������ռ�ø�ǩ��
		rootsignatureDesc.NumParameters = 2;						// ����������
		rootsignatureDesc.pParameters = RootParameters;				// ������ָ��
		rootsignatureDesc.NumStaticSamplers = 1;					// ��̬����������
		rootsignatureDesc.pStaticSamplers = &StaticSamplerDesc;		// ��̬������ָ��
		// ��ǩ����־������������Ⱦ���߲�ͬ�׶��µ��������״̬��ע���������Ҫ�� IA �׶����붥�����ݣ�����Ҫͨ����ǩ����������Ⱦ��������� IA �׶ζ�������
		rootsignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// �����ǩ�����ø�ǩ���ȱ���� GPU �ɶ��Ķ������ֽ���
		D3D12SerializeRootSignature(&rootsignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &SignatureBlob, &ErrorBlob);
		if (ErrorBlob)		// �����ǩ���������ErrorBlob �����ṩ������Ϣ
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}


		// ������������ֽ��봴����ǩ������
		m_D3D12Device->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
	}

	// ������Ⱦ����״̬���� (Pipeline State Object, PSO)
	void CreatePSO()
	{
		// PSO ��Ϣ�ṹ��
		D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

		// Input Assembler ����װ��׶�
		D3D12_INPUT_LAYOUT_DESC InputLayoutDesc = {};			// ������ʽ��Ϣ�ṹ��
		D3D12_INPUT_ELEMENT_DESC InputElementDesc[2] = {};		// ����Ԫ����Ϣ�ṹ������

		InputElementDesc[0].SemanticName = "POSITION";					// Ҫê��������
		InputElementDesc[0].SemanticIndex = 0;							// ����������Ŀǰ������ 0 ����
		InputElementDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;	// �����ʽ
		InputElementDesc[0].InputSlot = 0;								// ����۱�ţ�Ŀǰ������ 0 ����
		InputElementDesc[0].AlignedByteOffset = 0;						// ��������е�ƫ��
		// ���������ͣ�һ�������������õ� D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA �𶥵�������,����һ�ֽ���ʵ����������������ѧ
		InputElementDesc[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InputElementDesc[0].InstanceDataStepRate = 0;					// ʵ�����ݲ����ʣ�Ŀǰ����û���õ�ʵ�������� 0


		InputElementDesc[1].SemanticName = "TEXCOORD";										// Ҫê��������
		InputElementDesc[1].SemanticIndex = 0;												// ��������
		InputElementDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;								// �����ʽ
		InputElementDesc[1].InputSlot = 0;													// ����۱��
		// ��������е�ƫ�ƣ���Ϊ position �� texcoord ��ͬһ�����(0�������)
		// position �� float4���� 4 �� float ��ÿ�� float ռ 4 ���ֽڣ�����Ҫƫ�� 4*4=16 ���ֽڣ���������ȷ�� texcoord ������λ�ã���Ȼװ���ʱ��Ḳ��ԭ�� position ������
		InputElementDesc[1].AlignedByteOffset = 16;											// ��������е�ƫ��
		InputElementDesc[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;	// ����������
		InputElementDesc[1].InstanceDataStepRate = 0;										// ʵ�����ݲ�����


		InputLayoutDesc.NumElements = 2;						// ����Ԫ�ظ���
		InputLayoutDesc.pInputElementDescs = InputElementDesc;	// ����Ԫ�ؽṹ������ָ��
		PSODesc.InputLayout = InputLayoutDesc;					// ������Ⱦ���� IA �׶ε�������ʽ




		ComPtr<ID3DBlob> VertexShaderBlob;		// ������ɫ���������ֽ���
		ComPtr<ID3DBlob> PixelShaderBlob;		// ������ɫ���������ֽ���
		ComPtr<ID3DBlob> ErrorBlob;				// �����ֽ��룬��ǩ������ʧ��ʱ�� OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer()); ���Ի�ȡ������Ϣ

		// ���붥����ɫ�� Vertex Shader
		D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", NULL, NULL, &VertexShaderBlob, &ErrorBlob);
		if (ErrorBlob)		// �����ɫ���������ErrorBlob �����ṩ������Ϣ
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		// ����������ɫ�� Pixel Shader
		D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", NULL, NULL, &PixelShaderBlob, &ErrorBlob);
		if (ErrorBlob)		// �����ɫ���������ErrorBlob �����ṩ������Ϣ
		{
			OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer());
			OutputDebugStringA("\n");
		}

		PSODesc.VS.pShaderBytecode = VertexShaderBlob->GetBufferPointer();		// VS �ֽ�������ָ��
		PSODesc.VS.BytecodeLength = VertexShaderBlob->GetBufferSize();			// VS �ֽ������ݳ���
		PSODesc.PS.pShaderBytecode = PixelShaderBlob->GetBufferPointer();		// PS �ֽ�������ָ��
		PSODesc.PS.BytecodeLength = PixelShaderBlob->GetBufferSize();			// PS �ֽ������ݳ���

		// Rasterizer ��դ��
		PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;		// �޳�ģʽ��ָ���Ƿ�������/����/���޳�������ѡ�����޳�
		PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;		// ���ģʽ��ָ���Ƿ�����ɫ/�߿���䣬����ѡ��ɫ���

		// ��һ�����ø�ǩ�������������ǽ���ǩ���� PSO �󶨣�������Ⱦ���ߵ��������״̬
		PSODesc.pRootSignature = m_RootSignature.Get();

		// ���û���ͼԪ����������������������
		PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		// ������ȾĿ������������ֻ��һ����ȾĿ�� (��ɫ����) ��Ҫ������Ⱦ�������� 1
		PSODesc.NumRenderTargets = 1;
		// ������ȾĿ��ĸ�ʽ������Ҫ�ͽ�����ָ�����ڻ���ĸ�ʽһ�£������ 0 ָ������ȾĿ�������
		PSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		// ���û�Ͻ׶� (����ϲ��׶�) �� RGBA ��ɫͨ���Ŀ����͹رգ�D3D12_COLOR_WRITE_ENABLE_ALL ��ʾ RGBA ��ɫͨ��ȫ������
		PSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		// ���ò������������������� 1 ����
		PSODesc.SampleDesc.Count = 1;
		// ���ò������룬��������ڶ��ز����ģ�����ֱ����ȫ���� (UINT_MAX�����ǽ� UINT ���еı���λȫ�����Ϊ 1) ����
		PSODesc.SampleMask = UINT_MAX;

		// ���մ��� PSO ����
		m_D3D12Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&m_PipelineStateObject));
	}

	// ����������Դ
	void CreateVertexResource()
	{

		// CPU ���ٻ����ϵĶ�����Ϣ���飬ע�� DirectX ʹ�õ�����������ϵ��д������Ϣʱ�����һ��������֣�
		VERTEX vertexs[24] =
		{
			// ����
			{{0,2,0,1},{0,0}},
			{{2,2,0,1},{1,0}},
			{{2,0,0,1},{1,1}},
			{{0,0,0,1},{0,1}},

			// ����
			{{2,2,2,1},{0,0}},
			{{0,2,2,1},{1,0}},
			{{0,0,2,1},{1,1}},
			{{2,0,2,1},{0,1}},

			// ����
			{{0,2,2,1},{0,0}},
			{{0,2,0,1},{1,0}},
			{{0,0,0,1},{1,1}},
			{{0,0,2,1},{0,1}},

			// ����
			{{2,2,0,1},{0,0}},
			{{2,2,2,1},{1,0}},
			{{2,0,2,1},{1,1}},
			{{2,0,0,1},{0,1}},

			// ����
			{{0,2,2,1},{0,0}},
			{{2,2,2,1},{1,0}},
			{{2,2,0,1},{1,1}},
			{{0,2,0,1},{0,1}},

			// ����
			{{0,0,0,1},{0,0}},
			{{2,0,0,1},{1,0}},
			{{2,0,2,1},{1,1}},
			{{0,0,2,1},{0,1}}
		};


		D3D12_RESOURCE_DESC VertexDesc = {};						// D3D12Resource ��Ϣ�ṹ��
		VertexDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		// ��Դ���ͣ��ϴ��ѵ���Դ���Ͷ��� buffer ����
		VertexDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;			// ��Դ���֣�ָ����Դ�Ĵ洢��ʽ���ϴ��ѵ���Դ���� row major �������Դ洢
		VertexDesc.Width = sizeof(vertexs);							// ��Դ��ȣ��ϴ��ѵ���Դ�������Դ���ܴ�С
		VertexDesc.Height = 1;										// ��Դ�߶ȣ��ϴ��ѽ����Ǵ���������Դ�ģ����Ը߶ȱ���Ϊ 1
		VertexDesc.Format = DXGI_FORMAT_UNKNOWN;					// ��Դ��ʽ���ϴ�����Դ�ĸ�ʽ����Ϊ UNKNOWN
		VertexDesc.DepthOrArraySize = 1;							// ��Դ��ȣ������������������� 3D ����ģ��ϴ�����Դ����Ϊ 1
		VertexDesc.MipLevels = 1;									// Mipmap �ȼ����������������ģ��ϴ�����Դ����Ϊ 1
		VertexDesc.SampleDesc.Count = 1;							// ��Դ�����������ϴ�����Դ������ 1

		// �ϴ������ԵĽṹ�壬�ϴ���λ�� CPU �� GPU �Ĺ����ڴ�
		D3D12_HEAP_PROPERTIES UploadHeapDesc = { D3D12_HEAP_TYPE_UPLOAD };

		// ������Դ��CreateCommittedResource ��Ϊ��Դ�Զ�����һ���ȴ�С����ʽ�ѣ������ʽ�ѵ�����Ȩ�ɲ���ϵͳ���������߲��ɿ���
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE,
			&VertexDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_VertexResource));



		// ���ڴ�����Դ��ָ��
		BYTE* TransferPointer = nullptr;
		// Map ��ʼӳ�䣬Map ������õ���� D3D12Resource �ĵ�ַ (�ڹ����ڴ���)�����ݸ�ָ�룬�������Ǿ���ͨ�� memcpy ��������������
		m_VertexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));
		// �� CPU ���ٻ����ϵĶ������� ���Ƶ� �����ڴ��ϵ� D3D12Resource ��CPU ���ٻ��� -> �����ڴ�
		memcpy(TransferPointer, vertexs, sizeof(vertexs));
		// Unmap ����ӳ�䣬D3D12Resource ���ֻ��״̬���������ܼ��� GPU �ķ���
		m_VertexResource->Unmap(0, nullptr);


		// ��д VertexBufferView VBV ���㻺������������������� D3D12Resource���� GPU ֪������һ�����㻺��
		VertexBufferView.BufferLocation = m_VertexResource->GetGPUVirtualAddress();		// ���㻺����Դ�ĵ�ַ
		VertexBufferView.SizeInBytes = sizeof(vertexs);									// �������㻺����ܴ�С
		VertexBufferView.StrideInBytes = sizeof(VERTEX);								// ÿ������Ԫ�صĴ�С (����)
	}

	// ����������Դ���������������ظ�ʹ�ö�����Դ������Ҫ���ݵĶ�����������ʡ�Դ�
	void CreateIndexResource()
	{
		// �����������飬ע������� UINT == UINT32��������ĸ�ʽ (����) ������ DXGI_FORMAT_R32_UINT����������
		UINT IndexArray[36] =
		{
			// ����
			0,1,2,0,2,3,
			// ����
			4,5,6,4,6,7,
			// ����
			8,9,10,8,10,11,
			// ����
			12,13,14,12,14,15,
			// ����
			16,17,18,16,18,19,
			// ����
			20,21,22,20,22,23
		};

		D3D12_RESOURCE_DESC IndexResDesc = {};						// D3D12Resource ��Ϣ�ṹ��
		IndexResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;	// ��Դ���ͣ��ϴ��ѵ���Դ���Ͷ��� buffer ����
		IndexResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;		// ��Դ���֣�ָ����Դ�Ĵ洢��ʽ���ϴ��ѵ���Դ���� row major �������Դ洢
		IndexResDesc.Width = sizeof(IndexArray);					// ��Դ��ȣ��ϴ��ѵ���Դ�������Դ���ܴ�С
		IndexResDesc.Height = 1;									// ��Դ�߶ȣ��ϴ��ѽ����Ǵ���������Դ�ģ����Ը߶ȱ���Ϊ 1
		IndexResDesc.Format = DXGI_FORMAT_UNKNOWN;					// ��Դ��ʽ���ϴ�����Դ�ĸ�ʽ����Ϊ UNKNOWN
		IndexResDesc.DepthOrArraySize = 1;							// ��Դ��ȣ������������������� 3D ����ģ��ϴ�����Դ����Ϊ 1
		IndexResDesc.MipLevels = 1;									// Mipmap �ȼ����������������ģ��ϴ�����Դ����Ϊ 1
		IndexResDesc.SampleDesc.Count = 1;							// ��Դ�����������ϴ�����Դ������ 1

		// �ϴ������ԵĽṹ�壬�ϴ���λ�� CPU �� GPU �Ĺ����ڴ�
		D3D12_HEAP_PROPERTIES UploadHeapDesc = { D3D12_HEAP_TYPE_UPLOAD };

		// ������Դ��CreateCommittedResource ��Ϊ��Դ�Զ�����һ���ȴ�С����ʽ�ѣ������ʽ�ѵ�����Ȩ�ɲ���ϵͳ���������߲��ɿ���
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE,
			&IndexResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_IndexResource));



		// ���ڴ�����Դ��ָ��
		BYTE* TransferPointer = nullptr;
		// Map ��ʼӳ�䣬Map ������õ���� D3D12Resource �ĵ�ַ (�ڹ����ڴ���)�����ݸ�ָ�룬�������Ǿ���ͨ�� memcpy ��������������
		m_IndexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));
		// �� CPU ���ٻ����ϵ��������� ���Ƶ� �����ڴ��ϵ� D3D12Resource ��CPU ���ٻ��� -> �����ڴ�
		memcpy(TransferPointer, IndexArray, sizeof(IndexArray));
		// Unmap ����ӳ�䣬D3D12Resource ���ֻ��״̬���������ܼ��� GPU �ķ���
		m_IndexResource->Unmap(0, nullptr);


		// ��д IndexBufferView IBV ������������������������� D3D12Resource���� GPU ֪������һ����������
		IndexBufferView.BufferLocation = m_IndexResource->GetGPUVirtualAddress();	// ����������Դ�ĵ�ַ
		IndexBufferView.SizeInBytes = sizeof(IndexArray);							// ��������������ܴ�С
		IndexBufferView.Format = DXGI_FORMAT_R32_UINT;								// ÿ�������Ĵ�С (����)����ͬ���͵����ݣ������ĸ�ʽ�ǹ̶���
	}

	// ���³�������������ÿ֡�µ� MVP ���󴫵ݵ������������У��������ܿ�����̬�� 3D ������
	void UpdateConstantBuffer()
	{
		// �����º�ľ��󣬴洢�������ڴ��ϵĳ������壬���� GPU �Ϳ��Է��ʵ� MVP ������
		XMStoreFloat4x4(&MVPBuffer->MVPMatrix, m_FirstCamera.GetMVPMatrix());
	}

	// ��Ⱦ
	void Render()
	{
		// ÿ֡��Ⱦ��ʼǰ������ UpdateConstantBuffer() ���³���������
		UpdateConstantBuffer();


		// ��ȡ RTV ���׾��
		RTVHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
		// ��ȡ��ǰ��Ⱦ�ĺ�̨�������
		FrameIndex = m_DXGISwapChain->GetCurrentBackBufferIndex();
		// ƫ�� RTV ������ҵ���Ӧ�� RTV ������
		RTVHandle.ptr += FrameIndex * RTVDescriptorSize;


		// ���������������
		m_CommandAllocator->Reset();
		// �����������б�Close �ر�״̬ -> Record ¼��״̬
		m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);

		// ����ʼת�����ϵ���Դָ��Ϊ��ǰ��ȾĿ��
		beg_barrier.Transition.pResource = m_RenderTarget[FrameIndex].Get();
		// ������Դ���ϣ�����ȾĿ���� Present ����(ֻ��) ת���� RenderTarget ��ȾĿ��(ֻд)
		m_CommandList->ResourceBarrier(1, &beg_barrier);

		// �ڶ������ø�ǩ�����������ý����� ��Ⱦ���߰󶨵ĸ�ǩ�� �� ����ĸ�ǩ�� �Ƿ�ƥ��
		// �Լ���ǩ��ָ������Դ�Ƿ���ȷ�󶨣������Ϻ����м򵥵�ӳ��
		m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
		// ������Ⱦ����״̬������������ m_CommandList->Reset() ��ʱ��ֱ���ڵڶ����������� PSO
		m_CommandList->SetPipelineState(m_PipelineStateObject.Get());

		// �����ӿ� (��դ���׶�)�����ڹ�դ�������Ļӳ��
		m_CommandList->RSSetViewports(1, &viewPort);
		// ���òü����� (��դ���׶�)
		m_CommandList->RSSetScissorRects(1, &ScissorRect);

		// �� RTV ���������ȾĿ��
		m_CommandList->OMSetRenderTargets(1, &RTVHandle, false, nullptr);

		// ��յ�ǰ��ȾĿ��ı���Ϊ����ɫ
		m_CommandList->ClearRenderTargetView(RTVHandle, DirectX::Colors::SkyBlue, 0, nullptr);

		// �����������������õ���ʱ ID3D12DescriptorHeap ����
		ID3D12DescriptorHeap* _temp_DescriptorHeaps[] = { m_SRVHeap.Get() };
		// ������������
		m_CommandList->SetDescriptorHeaps(1, _temp_DescriptorHeaps);
		// ���� SRV ��� (��һ��������)
		m_CommandList->SetGraphicsRootDescriptorTable(0, SRV_GPUHandle);

		// ���ó������� (�ڶ���������)�����Ǹ��������ݵ� CBVResource �󣬾Ϳ�������ɫ����ȡ���Զ������ MVP �任��
		m_CommandList->SetGraphicsRootConstantBufferView(1, m_CBVResource->GetGPUVirtualAddress());

		// ����ͼԪ���� (����װ��׶�)���������������������б�
		m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// ���� VBV ���㻺�������� (����װ��׶�) 
		m_CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);

		// ���� IBV �������������� (����װ��׶�) 
		m_CommandList->IASetIndexBuffer(&IndexBufferView);

		// Draw Call! ���Ʒ���
		m_CommandList->DrawIndexedInstanced(36, 1, 0, 0, 0);


		// ����ֹת�����ϵ���Դָ��Ϊ��ǰ��ȾĿ��
		end_barrier.Transition.pResource = m_RenderTarget[FrameIndex].Get();
		// ��ͨ��һ����Դ���ϣ�����ȾĿ���� RenderTarget ��ȾĿ��(ֻд) ת���� Present ����(ֻ��)
		m_CommandList->ResourceBarrier(1, &end_barrier);

		// �ر������б�Record ¼��״̬ -> Close �ر�״̬�������б�ֻ�йرղſ����ύ
		m_CommandList->Close();

		// ���ڴ��������õ���ʱ ID3D12CommandList ����
		ID3D12CommandList* _temp_cmdlists[] = { m_CommandList.Get() };

		// ִ�����ĵ���Ⱦ���
		m_CommandQueue->ExecuteCommandLists(1, _temp_cmdlists);

		// ��������з������������������������뵽��������У��������ִ�е�������ʱ����֪ͨ��������������
		m_DXGISwapChain->Present(1, NULL);




		// ��Χ��Ԥ��ֵ�趨Ϊ��һ֡
		FenceValue++;
		// ��������� (��������� GPU ��) ����Χ��Ԥ��ֵ�����������뵽���������
		// �������ִ�е�������޸�Χ��ֵ����ʾ��Ⱦ����ɣ�"����"Χ��
		m_CommandQueue->Signal(m_Fence.Get(), FenceValue);
		// ����Χ����Ԥ���¼�������Ⱦ���ʱ��Χ����"����"������Ԥ���¼������¼������ź�״̬ת�������ź�״̬
		m_Fence->SetEventOnCompletion(FenceValue, RenderEvent);
	}

	// ��Ⱦѭ��
	void RenderLoop()
	{
		bool isExit = false;	// �Ƿ��˳�
		MSG msg = {};			// ��Ϣ�ṹ��

		while (isExit != true)
		{
			// MsgWaitForMultipleObjects ���ڶ���̵߳��������ȴ�������ֵ�Ǽ����¼� (�߳�) �� ID
			// �����ú����� RenderEvent Ҳ���Զ�����Ϊ���ź�״̬����Ϊ���Ǵ����¼���ʱ��ָ���˵ڶ�������Ϊ false
			DWORD ActiveEvent = ::MsgWaitForMultipleObjects(1, &RenderEvent, false, INFINITE, QS_ALLINPUT);

			switch (ActiveEvent - WAIT_OBJECT_0)
			{
			case 0:				// ActiveEvent �� 0��˵����Ⱦ�¼��Ѿ�����ˣ�������һ����Ⱦ
			{
				Render();
				Sleep(10);
			}
			break;


			case 1:				// ActiveEvent �� 1��˵����Ⱦ�¼�δ��ɣ�CPU ���߳�ͬʱ��������Ϣ����ֹ�������
			{
				// �鿴��Ϣ�����Ƿ�����Ϣ������оͻ�ȡ�� PM_REMOVE ��ʾ��ȡ����Ϣ�������̽�����Ϣ����Ϣ�������Ƴ�
				while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					// �������û���յ��˳���Ϣ���������ϵͳ�����ɷ���Ϣ������
					if (msg.message != WM_QUIT)
					{
						TranslateMessage(&msg);		// ������Ϣ�������̰��������ź� (WM_KEYDOWN)�������ⰴ��ֵת��Ϊ��Ӧ�� ASCII �룬ͬʱ���� WM_CHAR ��Ϣ
						DispatchMessage(&msg);		// �ɷ���Ϣ��֪ͨ����ϵͳ���ûص�����������Ϣ
					}
					else
					{
						isExit = true;							// �յ��˳���Ϣ�����˳���Ϣѭ��
					}
				}
			}
			break;


			case WAIT_TIMEOUT:	// ��Ⱦ��ʱ
			{

			}
			break;

			}
		}
	}

	// �ص������������ڲ�������Ϣ
	// WASD �� ���� WM_CHAR �ַ���Ϣ ���� �����ǰ�������ƶ�
	// ��곤������ƶ� ���� WM_MOUSEMOVE ����ƶ���Ϣ ���� ������ӽ���ת
	// �رմ��� ���� WM_DESTROY ����������Ϣ ���� ���ڹرգ���������˳�
	LRESULT CALLBACK CallBackFunc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		// �� switch ���ڶ�������������ÿ�� case �ֱ��Ӧһ��������Ϣ
		switch (msg)
		{
		case WM_DESTROY:			// ���ڱ����� (���������Ͻ� X �رմ���ʱ)
		{
			PostQuitMessage(0);		// �����ϵͳ�����˳����� (WM_QUIT)��������Ϣѭ��
		}
		break;


		case WM_CHAR:	// ��ȡ���̲������ַ���Ϣ��TranslateMessage �Ὣ������뷭����ַ��룬ͬʱ���� WM_CHAR ��Ϣ
		{
			switch (wParam)		// wParam �ǰ�����Ӧ���ַ� ASCII ��
			{
			case 'w':
			case 'W':	// ��ǰ�ƶ�
				m_FirstCamera.Walk(0.1);
				break;

			case 's':
			case 'S':	// ����ƶ�
				m_FirstCamera.Walk(-0.1);
				break;

			case 'a':
			case 'A':	// �����ƶ�
				m_FirstCamera.Strafe(0.1);
				break;

			case 'd':
			case 'D':	// �����ƶ�
				m_FirstCamera.Strafe(-0.1);
				break;
			}
		}
		break;


		case WM_MOUSEMOVE:	// ��ȡ����ƶ���Ϣ
		{
			switch (wParam)	// wParam ����갴����״̬
			{
			case MK_LBUTTON:	// ���û�������������ͬʱ�ƶ���꣬�������ת
				m_FirstCamera.CameraRotate();
				break;

			// ����û�������ֻ���ƶ�ҲҪ���£�����ͻᷢ��������ӽ�˲��
			default: m_FirstCamera.UpdateLastCursorPos();
			}
		}
		break;


		// ������յ�������Ϣ��ֱ��Ĭ�Ϸ�����������
		default: return DefWindowProc(hwnd, msg, wParam, lParam);

		}

		return 0;	// ע�����default ����ķ�֧�������е���������Ҫ return 0������ͻ᷵��ϵͳ���ֵ�����´����޷�������ʾ
	}

	// ���д���
	static void Run(HINSTANCE hins)
	{
		DX12Engine engine;
		engine.InitWindow(hins);
		engine.CreateDebugDevice();
		engine.CreateDevice();
		engine.CreateCommandComponents();
		engine.CreateRenderTarget();
		engine.CreateFenceAndBarrier();

		engine.LoadTextureFromFile();
		engine.CreateSRVHeap();
		engine.CreateUploadAndDefaultResource();
		engine.CopyTextureDataToDefaultResource();
		engine.CreateSRV();

		engine.CreateCBVResource();

		engine.CreateRootSignature();
		engine.CreatePSO();

		engine.CreateVertexResource();
		engine.CreateIndexResource();

		engine.RenderLoop();
	}
};


// ������
int WINAPI WinMain(HINSTANCE hins, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow)
{
	DX12Engine::Run(hins);
}


