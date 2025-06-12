
// (10) RenderGLTFModel: ʹ�� DirectX 12 + Assimp ��Ⱦ��Minecraft���е� ������ gltf ģ�� (��ģ��û�й���)
// ��лԭ���ߴ��: Vincent Yanez (https://sketchfab.com/vinceyanez) 
// ģ����Ŀ��ַ: https://sketchfab.com/3d-models/minecraft-creeper-fd66182f07e5408eb04fa5a88ae16055


// windows.h ���׼����� min/max �����������³�ͻ�ˣ����� windows.h ����� min/max ����
#define NOMINMAX
// C++ 17 ��ʼ�� std::codecvt_utf8 ��ȡ���ˣ�ֱ��ʹ�ûᱨ�� (�� scanf һ��)�������������ƹ���������ʹ��
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

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
#include<fstream>			// C++ �ļ��������
#include<vector>			// C++ STL vector ������
#include<codecvt>			// C++ �ַ�����ת���⣬���� string ת wstring


#include<assimp/Importer.hpp>				// Assimp Importer ģ�͵����������ڵ���ģ��,��ȡģ������
#include<assimp/postprocess.h>				// PostProcess �����ṩ���ֱ�־ (aiProcess_xxx)�����ڸ���ģ�͵ĵ�������������
#include<assimp/scene.h>					// Scene ��������⣬���ڴ洢�������� 3D ģ�͵���������


#pragma comment(lib,"d3d12.lib")			// ���� DX12 ���� DLL
#pragma comment(lib,"dxgi.lib")				// ���� DXGI DLL
#pragma comment(lib,"dxguid.lib")			// ���� DXGI ��Ҫ���豸 GUID
#pragma comment(lib,"d3dcompiler.lib")		// ���� DX12 ��Ҫ����ɫ������ DLL
#pragma comment(lib,"windowscodecs.lib")	// ���� WIC DLL
#pragma comment(lib,"assimp-vc143-mtd.lib")	// ���� Assimp DLL

using namespace Microsoft;
using namespace Microsoft::WRL;		// ʹ�� wrl.h ����������ռ䣬������Ҫ�õ������ Microsoft::WRL::ComPtr COM����ָ��
using namespace DirectX;			// DirectX �����ռ�


// 1.��Ŀ -> ���� -> VC++ Ŀ¼ -> ����Ŀ¼ -> ��� Assimp/include (����д $(SolutionDir)Assimp/include )
// 2.��Ŀ -> ���� -> VC++ Ŀ¼ -> ��Ŀ¼ -> ��� Assimp/lib (����д $(SolutionDir)Assimp/lib )
// 3.�� Assimp/lib ����� assimp-vc143-mtd.lib �� assimp-vc143-mtd.dll ���Ƶ�
//   exe ���ڵ�Ŀ¼�� (x64/Debug �ļ���)�����ɵ� exe ��Ҫ�������������ӿ�



// ---------------------------------------------------------------------------------------------------------------



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

	XMVECTOR EyePosition = XMVectorSet(1, 1, 1, 1);			// �����������ռ��µ�λ��
	XMVECTOR FocusPosition = XMVectorSet(0, 0, 0, 1);		// �����������ռ��¹۲�Ľ���λ��
	XMVECTOR UpDirection = XMVectorSet(0, 1, 0, 0);			// ����ռ䴹ֱ���ϵ�����

	// ������۲췽��ĵ�λ����������ǰ���ƶ�
	XMVECTOR ViewDirection = XMVector3Normalize(FocusPosition - EyePosition);

	// ���࣬�����ԭ���뽹��ľ��룬XMVector3Length ��ʾ������ȡģ
	float FocalLength = XMVectorGetX(XMVector3Length(FocusPosition - EyePosition));

	// ��������ҷ���ĵ�λ���������������ƶ���XMVector3Cross �����������
	XMVECTOR RightDirection = XMVector3Normalize(XMVector3Cross(ViewDirection, UpDirection));

	POINT LastCursorPoint = {};								// ��һ������λ��

	float FovAngleY = XM_PIDIV4;							// ��ֱ�ӳ���
	float AspectRatio = 16.0 / 9.0;							// ͶӰ���ڿ�߱�
	float NearZ = 0.1;										// ��ƽ�浽ԭ��ľ���
	float FarZ = 1000;										// Զƽ�浽ԭ��ľ���

	XMMATRIX ModelMatrix;									// ģ�;���ģ�Ϳռ� -> ����ռ�
	XMMATRIX ViewMatrix;									// �۲��������ռ� -> �۲�ռ�
	XMMATRIX ProjectionMatrix;								// ͶӰ���󣬹۲�ռ� -> ��βü��ռ�

	XMMATRIX MVPMatrix;										// MVP ����������Ҫ�ù��з��� GetMVPMatrix ��ȡ

public:

	Camera()	// ������Ĺ��캯��
	{
		// ģ�;������������� x ����ת 90�㣬����Ϊģ������ʹ�õĽ�ģ�����ͬ����ֱ���ϵ��������в���
		// ��Щ�� y �ᳯ�Ͻ�ģ�ģ���Щ�� z �ᳯ�Ͻ�ģ�� (���������� Z �᷽���ϣ���Ҫ�� x ����ת 90�� ����)
		ModelMatrix = XMMatrixRotationX(XM_PIDIV2);
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

	// ���������λ��
	inline void SetEyePosition(XMVECTOR pos)
	{
		EyePosition = pos;

		// �ı�λ�ú󣬹۲����������ࡢ�ҷ�������ҲҪ�ı䣬����ᷢ���ӽ�˲��
		ViewDirection = XMVector3Normalize(FocusPosition - EyePosition);
		FocalLength = XMVectorGetX(XMVector3Length(FocusPosition - EyePosition));
		RightDirection = XMVector3Normalize(XMVector3Cross(ViewDirection, UpDirection));
	}

	// �������������
	inline void SetFocusPosition(XMVECTOR pos)
	{
		FocusPosition = pos;

		// �ı�λ�ú󣬹۲����������ࡢ�ҷ�������ҲҪ�ı䣬����ᷢ���ӽ�˲��
		ViewDirection = XMVector3Normalize(FocusPosition - EyePosition);
		FocalLength = XMVectorGetX(XMVector3Length(FocusPosition - EyePosition));
		RightDirection = XMVector3Normalize(XMVector3Cross(ViewDirection, UpDirection));
	}
};



// ---------------------------------------------------------------------------------------------------------------



// DX12 ����
class DX12Engine
{
private:

	int WindowWidth = 1280;		// ���ڿ��
	int WindowHeight = 720;		// ���ڸ߶�
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

	ComPtr<ID3D12DescriptorHeap> m_DSVHeap;					// DSV ��������
	D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle;					// DSV ���������
	ComPtr<ID3D12Resource> m_DepthStencilBuffer;			// DSV ���ģ�建����Դ

	// DSV ��Դ�ĸ�ʽ
	// ���ģ�建��ֻ֧�����ָ�ʽ:
	// DXGI_FORMAT_D24_UNORM_S8_UINT	(ÿ������ռ���ĸ��ֽ� 32 λ��24 λ�޷��Ź�һ���������������ֵ��8 λ��������ģ��ֵ)
	// DXGI_FORMAT_D32_FLOAT_S8X24_UINT	(ÿ������ռ�ð˸��ֽ� 64 λ��32 λ�������������ֵ��8 λ��������ģ��ֵ������ 24 λ������ʹ��)
	// DXGI_FORMAT_D16_UNORM			(ÿ������ռ�������ֽ� 16 λ��16 λ�޷��Ź�һ���������������ֵ����Χ [0,1]����ʹ��ģ��)
	// DXGI_FORMAT_D32_FLOAT			(ÿ������ռ���ĸ��ֽ� 32 λ��32 λ�������������ֵ����ʹ��ģ��)
	// ��������ѡ����õĸ�ʽ DXGI_FORMAT_D24_UNORM_S8_UINT
	DXGI_FORMAT DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	

	// ---------------------------------------------------------------------------------------------------------------



	std::string ModelFileName = "creeper/scene.gltf";				// ģ���ļ���
	std::string ModelTextureFilePath = "creeper/";					// ģ�������ļ���
	Assimp::Importer* m_ModelImporter = new Assimp::Importer;		// ģ�͵�����
	const aiScene* m_ModelScene = nullptr;							// ģ��/��������

	// ����ģ��ʹ�õı�־
	// aiProcess_ConvertToLeftHanded: Assimp �����ģ������ OpenGL ������ϵͳΪ�����ģ���ģ��ת���� DirectX ������ϵͳ
	// aiProcess_Triangulate��ģ�����ʦ����ʹ�ö���ζ�ģ�ͽ��н�ģ�ģ������ö���ν�ģ��ģ�ͣ������Ƕ�ת���ɻ��������ν�ģ
	// aiProcess_FixInfacingNormals����ģ�������˫����ʾ�ģ��������ʦ�������ⶥ�������򣬲�����ᱻ�޳��޷�������ʾ����Ҫ��ת����
	// aiProcess_LimitBoneWeights: ��������Ĺ���Ȩ�����Ϊ 4 ��������Ȩ�����账��
	// aiProcess_GenBoundBoxes: ��ÿ�����񣬶�����һ�� AABB �����
	// aiProcess_JoinIdenticalVertices: ��λ����ͬ�Ķ���ϲ�Ϊһ�����㣬�Ӷ�����ģ�͵Ķ����������Ż��ڴ�ʹ�ú�������ȾЧ�ʡ�
	UINT ModelImportFlag = aiProcess_ConvertToLeftHanded | aiProcess_Triangulate | aiProcess_FixInfacingNormals |
		aiProcess_LimitBoneWeights | aiProcess_GenBoundingBoxes | aiProcess_JoinIdenticalVertices;


	// ����/����ṹ��
	struct Material
	{
		std::string FilePath;						// �����ļ�·��
		aiTextureType Type;							// ��������
		ComPtr<ID3D12Resource> UploadTexture;		// λ���ϴ��ѵ�����
		ComPtr<ID3D12Resource> DefaultTexture;		// λ��Ĭ�϶ѵ�����

		D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;		// CPU ���
		D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;		// GPU ���
	};

	std::vector<Material> MaterialGroup;			// ģ���õ��Ĳ�����


	struct VERTEX									// �������ݽṹ��
	{
		XMFLOAT4 position;							// ����λ��
		XMFLOAT2 texcoordUV;						// ������������
	};

	struct MESH										// �������ݽṹ��
	{
		UINT MaterialIndex;							// Mesh �õ��Ĳ�������

		UINT VertexGroupOffset;						// ������Ϣ�� VertexGroup ��ƫ��
		UINT IndexGroupOffset;						// ������Ϣ�� IndexGroup ��ƫ��
		UINT VertexCount;							// ��������
		UINT IndexCount;							// ��������
	};


	ComPtr<ID3D12Resource> UploadVertexResource;	// �ϴ��Ѷ�����Դ
	ComPtr<ID3D12Resource> UploadIndexResource;		// �ϴ���������Դ
	ComPtr<ID3D12Resource> DefaultVertexResource;	// Ĭ�϶Ѷ�����Դ
	ComPtr<ID3D12Resource> DefaultIndexResource;	// Ĭ�϶�������Դ

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {};	// ������Ϣ������
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};	// ������Ϣ������


	std::vector<VERTEX> VertexGroup;		// �����飬�洢����ģ�͵Ķ�����Ϣ
	std::vector<UINT> IndexGroup;			// �����飬�洢����ģ�͵�������Ϣ

	std::vector<MESH> MeshGroup;			// ������Ϣ��


	struct AABB				// AABB �нṹ��
	{
		float minBoundsX;	// ��С����� X ֵ
		float minBoundsY;	// ��С����� Y ֵ
		float minBoundsZ;	// ��С����� Z ֵ

		float maxBoundsX;	// �������� X ֵ
		float maxBoundsY;	// �������� Y ֵ
		float maxBoundsZ;	// �������� Z ֵ
	};

	AABB ModelBoundingBox;	// ģ�� AABB �У����ڵ����������Ұ����ֹģ�����������Ұ��ɳ�ȥ



	// ---------------------------------------------------------------------------------------------------------------



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

	UINT TextureSize = 0;									// �������ʵ��С (��λ���ֽ�)
	UINT UploadResourceRowSize = 0;							// �ϴ�����Դÿ�еĴ�С (��λ���ֽ�)
	UINT UploadResourceSize = 0;							// �ϴ�����Դ���ܴ�С (��λ���ֽ�)

	UINT SRVDescriptorSize;									// SRV �������Ĵ�С


	ComPtr<ID3D12Resource> m_CBVResource;		// ����������Դ�����ڴ�� MVP ����MVP ����ÿ֡��Ҫ���£�������Ҫ�洢�ڳ�����������
	struct CBuffer								// ��������ṹ��
	{
		XMFLOAT4X4 MVPMatrix;					// MVP �������ڽ��������ݴӶ���ռ�任����βü��ռ�
	};
	CBuffer* MVPBuffer = nullptr;	// ��������ṹ��ָ�룬����洢���� MVP ������Ϣ������ Map ��ָ���ָ�� CBVResource �ĵ�ַ

	Camera m_FirstCamera;			// ��һ�˳������

	ComPtr<ID3D12RootSignature> m_RootSignature;			// ��ǩ��
	ComPtr<ID3D12PipelineState> m_PipelineStateObject;		// ��Ⱦ����״̬


	// �ӿ�
	D3D12_VIEWPORT viewPort = D3D12_VIEWPORT{ 0, 0, float(WindowWidth), float(WindowHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	// �ü�����
	D3D12_RECT ScissorRect = D3D12_RECT{ 0, 0, WindowWidth, WindowHeight };



public:

	// ��ʼ������
	void STEP1_InitWindow(HINSTANCE hins)
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
		m_hwnd = CreateWindow(wc.lpszClassName, L"Creeper", WS_SYSMENU | WS_OVERLAPPED,
			10, 10, WindowWidth, WindowHeight,
			NULL, NULL, hins, NULL);

		// ��Ϊָ���˴��ڴ�С���ɱ�� WS_SYSMENU �� WS_OVERLAPPED��Ӧ�ò����Զ���ʾ���ڣ���Ҫʹ�� ShowWindow ǿ����ʾ����
		ShowWindow(m_hwnd, SW_SHOW);
	}

	// �������Բ�
	void STEP2_CreateDebugDevice()
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
	bool STEP3_CreateDevice()
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
					OutputDebugStringW(L"��ǰʹ�õ��Կ���");
					OutputDebugStringW(adap.Description);
					OutputDebugStringW(L"\n");
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
	void STEP4_CreateCommandComponents()
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
	void STEP5_CreateRenderTarget()
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
	void STEP6_CreateFenceAndBarrier()
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

	// ���� DSV ���ģ���������� (Non-Shader Visible)
	void STEP7_CreateDSVHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC DSVHeapDesc = {};		// DSV �������ѽṹ��
		DSVHeapDesc.NumDescriptors = 1;						// ������ֻ�� 1 ������Ϊ����ֻ��һ����ȾĿ��
		DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;	// ������������

		// ���� DSV �������� (Depth Stencil View�����ģ��������)��������Ȳ�����ģ�����
		m_D3D12Device->CreateDescriptorHeap(&DSVHeapDesc, IID_PPV_ARGS(&m_DSVHeap));

		// ��ȡ DSV �� CPU ���
		DSVHandle = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
	}

	// ���������ģ�建�壬���ڿ�����Ȳ��ԣ���Ⱦ������ȷ��������ڵ���ϵ
	void STEP8_CreateDepthStencilBuffer()
	{
		D3D12_RESOURCE_DESC DSVResourceDesc = {};							// ���ģ�建����Դ��Ϣ�ṹ��
		DSVResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;		// ��Ȼ�����ʵҲ��һ������
		DSVResourceDesc.Format = DSVFormat;									// ��Դ�����ʽ
		DSVResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;				// ��Ȼ���Ĳ���Ҳ�� UNKNOWN
		DSVResourceDesc.Width = WindowWidth;								// ��Ⱥ���ȾĿ��һ��
		DSVResourceDesc.Height = WindowHeight;								// �߶Ⱥ���ȾĿ��һ��
		DSVResourceDesc.MipLevels = 1;										// Mipmap �㼶������Ϊ 1 ����
		DSVResourceDesc.DepthOrArraySize = 1;								// ���������С (3D �������),����Ϊ 1 ����
		DSVResourceDesc.SampleDesc.Count = 1;								// ��������������Ϊ 1 ����
		DSVResourceDesc.SampleDesc.Quality = 0;								// ��������������Ϊ 0 ����
		// ��Դ��־���������ģ�建�壬����Ҫ�� D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL������ᴴ��ʧ��
		DSVResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE DepthStencilBufferClearValue = {};				// ���������Ȼ������Ϣ�ṹ�壬DX12 �ܶ���������Ż�
		DepthStencilBufferClearValue.DepthStencil.Depth = 1.0f;				// Ҫ��յ������ֵ����պ�����õ���ֵ
		DepthStencilBufferClearValue.DepthStencil.Stencil = 0;				// Ҫ��յ���ģ��ֵ����պ�����õ���ֵ
		DepthStencilBufferClearValue.Format = DSVFormat;					// Ҫ��ջ���ĸ�ʽ��Ҫ������һ��

		// Ĭ�϶����ԣ���Ȼ���Ҳ��һ������������Ĭ�϶�
		D3D12_HEAP_PROPERTIES DefaultProperties = { D3D12_HEAP_TYPE_DEFAULT };

		// ������Դ����Ȼ���ֻ��ռ�ú�����Դ������ֱ�� CreateCommittedResource ��ʽ�Ѵ������ɣ��ò���ϵͳ�����ǹ���
		m_D3D12Device->CreateCommittedResource(&DefaultProperties, D3D12_HEAP_FLAG_NONE, &DSVResourceDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, &DepthStencilBufferClearValue, IID_PPV_ARGS(&m_DepthStencilBuffer));
	}

	// ���� DSV ��������DSV �����������������ģ�建���������������������Ⱦ����Ҫ���õĶ���
	void STEP9_CreateDSV()
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC DSVViewDesc = {};
		DSVViewDesc.Format = DSVFormat;								// DSV ��������ʽҪ����Դһ��
		DSVViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// ��Ȼ��屾��Ҳ��һ�� 2D ����
		DSVViewDesc.Flags = D3D12_DSV_FLAG_NONE;					// ��� Flag ���������ö�дȨ�޵ģ����ֵ��ģ��ֵ�����Զ�д

		// ���� DSV ������ (Depth Stencil View�����ģ��������)
		m_D3D12Device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &DSVViewDesc, DSVHandle);
	}



	// ---------------------------------------------------------------------------------------------------------------



	// ��ȡģ�����ݣ����ݻ�洢�� aiScene ����
	bool STEP10_OpenModelFile()
	{
		// ʹ�� ReadFile ����ֱ�Ӵ����ļ�·�����ļ���·������������������Щ utf-8 �ַ� (Assimp ����޸������ bug)
		m_ModelScene = m_ModelImporter->ReadFile(ModelFileName, ModelImportFlag);

		// ���ģ��û�гɹ����� (�޷����룬����δ��ɣ�������޸��ڵ�)
		if (!m_ModelScene || m_ModelScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_ModelScene->mRootNode)
		{
			// Assimp ����ģ�͵Ĵ�����Ϣ
			std::string Assimp_error_msg = m_ModelImporter->GetErrorString();

			std::string errorMsg = "�����ļ� ";
			errorMsg += ModelFileName;
			errorMsg += " ʧ�ܣ�����ԭ��";
			errorMsg += Assimp_error_msg;
			MessageBoxA(NULL, errorMsg.c_str(), "����", MB_ICONERROR | MB_OK);
			return false;
		}

		return true;
	}


	// ���ģ�Ͳ���
	void STEP11_AddModelMaterials()
	{
		// ����ģ���е����в���
		for (UINT i = 0; i < m_ModelScene->mNumMaterials; i++)
		{
			// Assimp ����������ģ�Ͳ���
			aiMaterial* material = m_ModelScene->mMaterials[i];

			// ģ���ļ�Я���Ĳ���/�����ļ�·��
			aiString materialPath;


			// ��Ⲣ���� Diffuse Material �����������ͼ����������ͼ�Ǳ�ʾ���������ɫ�Ļ�������������������������
			// һ����Ի���� _diffuse��_base_color��_albedo ��Щ��׺��һ��������������ǵ�ͬ��
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &materialPath) == aiReturn_SUCCESS)
			{
				Material mt = {};						// �²���
				mt.FilePath = ModelTextureFilePath;		// �²������ڵ��ļ���
				mt.FilePath += materialPath.C_Str();	// �²��ʵ��ļ���
				mt.Type = aiTextureType_DIFFUSE;		// �²��ʵ�����
				MaterialGroup.push_back(mt);			// ����²���
			}
			// ������� DIFFUSE ��ͼ��������Ĭ����ͼ����Щ�����Ӧ��ȷʵ��Ĭ����ͼ��Ĭ����ͼ����ʾ��ɫ
			// һ����ͼ���������ã�������ģ���ļ��޷��ҵ��������
			else
			{
				Material mt = {};						// �²���
				mt.Type = aiTextureType_NONE;			// �²��ʵ�����
				MaterialGroup.push_back(mt);			// ����²���
			}
		}
	}


	// ���ģ����������
	// Mesh �����൱��ģ�͵�Ƥ�������洢��ģ��Ҫ��Ⱦ�Ķ�����Ϣ
	void STEP12_AddModelData()
	{
		// ��ǰ Mesh �� VertexGroup ������
		UINT CurrentMeshVertexGroupOffset = 0;
		// ��ǰ Mesh �� IndexGroup ������
		UINT CurrentMeshIndexGroupOffset = 0;

		// ����ģ�����е� Mesh ��Ϣ���� Assimp �У�Mesh �� Bone �Ƿֿ��洢�ģ�һ�� Mesh ���ж������
		for (UINT i = 0; i < m_ModelScene->mNumMeshes; i++)
		{
			// ��ǰ����
			const aiMesh* mesh = m_ModelScene->mMeshes[i];

			// ��� Mesh ���������� 0��ֱ������
			if (mesh->mNumVertices == 0) continue;

			// Ҫ��Ⱦ���� Mesh��ע�������ô����ų�ʼ���������г�Ա�� 0 ��
			MESH new_mesh = {};

			
			// ����ģ�����е� Mesh ��Ϣ��һ�� Mesh ���ж������
			for (UINT j = 0; j < mesh->mNumVertices; j++)
			{
				// �¶���
				VERTEX new_vertex = {};

				// �¶���λ�� (����ڵ�ǰ Mesh)
				new_vertex.position = XMFLOAT4(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z, 1);


				// �½ڵ����� UV������о���ӣ�û�о�Ĭ�� (-1, -1)
				// ע����� 0 ָ���ǵ� 0 �� UV ͨ�� (������� UE5 �ĵ�: UV ͨ��)
				// ����ͬһ�����㣬��ͬ�� UV ͨ�������в�ͬ�� UV ���꣬�����ڹ��գ����������ﲻ�漰��ֱ�ӻ�ȡ�� 0 ������ UV ����
				if (mesh->HasTextureCoords(0))
				{
					new_vertex.texcoordUV = XMFLOAT2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y);
				}
				else
				{
					new_vertex.texcoordUV = XMFLOAT2(-1, -1);	// Ĭ������ UV ���꣬Pixel Shader ����д���
				}

				// ����½ڵ�
				VertexGroup.push_back(new_vertex);
			}


			// ��� Mesh �����ж���������Ϣ���� Assimp �У�����ͨ������ Face Ϊ��λ�洢��
			// ��Ϊģ�����ʦ���ܻ��ö���ν��н�ģ������ָ�� aiProcess_Triangulate ����Ϊ�˰ѿ��ܵĶ����ȫ���ָ��������
			// ������������ֱ��ָ�� mesh->mFaces[j].mIndices ��ӽڵ㼴��
			for (UINT j = 0; j < mesh->mNumFaces; j++)
			{
				// ��ÿ�� Face��������������� (aiProcess_Triangulate �Ѿ���֤��ÿ��������ֻ������������)
				IndexGroup.push_back(mesh->mFaces[j].mIndices[0]);
				IndexGroup.push_back(mesh->mFaces[j].mIndices[1]);
				IndexGroup.push_back(mesh->mFaces[j].mIndices[2]);
			}


			// Mesh ��Ӧ������������Assimp ��֤ÿ�� Mesh ����Ӧһ�����������Ӧ�˶����������ģ��ʱ���Զ�����ָ�
			new_mesh.MaterialIndex = mesh->mMaterialIndex;


			// ���� Mesh �Ķ�������������ƫ��
			new_mesh.VertexGroupOffset = CurrentMeshVertexGroupOffset;		// ����ƫ��
			new_mesh.IndexGroupOffset = CurrentMeshIndexGroupOffset;		// ����ƫ��
			new_mesh.VertexCount = mesh->mNumVertices;						// ��������
			new_mesh.IndexCount = mesh->mNumFaces * 3;						// ��������

			// ���µ�ǰ�����������ƫ��
			CurrentMeshVertexGroupOffset = VertexGroup.size();
			CurrentMeshIndexGroupOffset = IndexGroup.size();
			

			// ����� Mesh
			MeshGroup.push_back(new_mesh);
		}
	}


	// ����ģ�͵� AABB ��Χ�У����������������ֹģ�����������Ұ��ɳ�ȥ
	void STEP13_CalcModelBoundingBox()
	{
		// ���ó�ʼֵ
		ModelBoundingBox =
		{
			m_ModelScene->mMeshes[0]->mAABB.mMin.x,
			m_ModelScene->mMeshes[0]->mAABB.mMin.y,
			m_ModelScene->mMeshes[0]->mAABB.mMin.z,

			m_ModelScene->mMeshes[0]->mAABB.mMax.x,
			m_ModelScene->mMeshes[0]->mAABB.mMax.y,
			m_ModelScene->mMeshes[0]->mAABB.mMax.z
		};

		// �������������������ģ�͵� AABB ��Χ�У���ע�⵼��ģ��ʱҪָ�� aiProcess_GenBoundingBoxes������ mAABB ��Ա��û������
		for (UINT i = 1; i < m_ModelScene->mNumMeshes; i++)
		{
			// ��ǰ����
			const aiMesh* mesh = m_ModelScene->mMeshes[i];

			// �����ܰ�Χ��
			ModelBoundingBox.minBoundsX = std::min(mesh->mAABB.mMin.x, ModelBoundingBox.minBoundsX);
			ModelBoundingBox.minBoundsY = std::min(mesh->mAABB.mMin.y, ModelBoundingBox.minBoundsY);
			ModelBoundingBox.minBoundsZ = std::min(mesh->mAABB.mMin.z, ModelBoundingBox.minBoundsZ);

			ModelBoundingBox.maxBoundsX = std::max(mesh->mAABB.mMax.x, ModelBoundingBox.maxBoundsX);
			ModelBoundingBox.maxBoundsY = std::max(mesh->mAABB.mMax.y, ModelBoundingBox.maxBoundsY);
			ModelBoundingBox.maxBoundsZ = std::max(mesh->mAABB.mMax.z, ModelBoundingBox.maxBoundsZ);
		}


		// ����ģ�����ĵ�
		XMFLOAT3 CenterPoint = {};
		CenterPoint.x = (ModelBoundingBox.maxBoundsX + ModelBoundingBox.minBoundsX) / 2.0;
		CenterPoint.y = (ModelBoundingBox.maxBoundsY + ModelBoundingBox.minBoundsY) / 2.0;
		CenterPoint.z = (ModelBoundingBox.maxBoundsZ + ModelBoundingBox.minBoundsZ) / 2.0;

		
		// ����ģ�� AABB �а뾶
		float RadiusX = (ModelBoundingBox.maxBoundsX - ModelBoundingBox.minBoundsX) / 2.0;	// AABB �а뾶 x ����
		float RadiusY = (ModelBoundingBox.maxBoundsY - ModelBoundingBox.minBoundsY) / 2.0;	// AABB �а뾶 y ����
		float RadiusZ = (ModelBoundingBox.maxBoundsZ - ModelBoundingBox.minBoundsZ) / 2.0;	// AABB �а뾶 z ����

		// ģ�� AABB �а뾶 (�����뾶)���� 1 ��Ϊ���������ģ����������
		float Radius = std::sqrt(RadiusX * RadiusX + RadiusY * RadiusY + RadiusZ * RadiusZ) / 2.0;


		// �������������
		m_FirstCamera.SetFocusPosition(XMVectorSet(CenterPoint.x, CenterPoint.y, CenterPoint.z, 1.0));

		// ���������λ��
		m_FirstCamera.SetEyePosition(XMVectorSet(CenterPoint.x, CenterPoint.y, CenterPoint.z - Radius, 1.0));
	}



	// ---------------------------------------------------------------------------------------------------------------



	// ���� SRV Descriptor Heap ��ɫ����Դ�������� (Shader Visible)
	void CreateSRVHeap()
	{
		// ���� SRV �������� (Shader Resource View����ɫ����Դ������)
		D3D12_DESCRIPTOR_HEAP_DESC SRVHeapDesc = {};
		SRVHeapDesc.NumDescriptors = MaterialGroup.size();					// �������ѵ�����
		SRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;			// �����������ͣ�CBV��SRV��UAV ���������������Է���ͬһ������������
		SRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;		// �������ѱ�־��Shader-Visible ��ʾ����ɫ���ɼ�

		// ���� SRV ��������
		m_D3D12Device->CreateDescriptorHeap(&SRVHeapDesc, IID_PPV_ARGS(&m_SRVHeap));
	}

	// ���������б�׼��¼�Ƹ�������
	void StartCommandRecord()
	{
		// ������Դ��Ҫʹ�� GPU �� CopyEngine �������棬������Ҫ��������з�����������

		m_CommandAllocator->Reset();								// ���������������
		m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);	// �����������б����������Ҫ PSO ״̬�����Եڶ��������� nullptr
	}

	// ��ȡ���㷨���� A ����ȡ�����ж�����Ҫ���ٸ�����Ϊ B �Ŀռ�������� A�������ڴ����
	inline UINT Ceil(UINT A, UINT B)
	{
		return (A + B - 1) / B;
	}

	// ����Ĭ��������Դ����Щ Mesh ������Դģ���ļ��в������ڵ� Default Teature Ĭ�������򵥴�������
	void CreateDefaultTexture(const UINT index)
	{
		// ������ת������ϴ�����Դ�ṹ��
		D3D12_RESOURCE_DESC UploadResourceDesc = {};
		UploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;		// ��Դ���ͣ��ϴ��ѵ���Դ���Ͷ��� buffer ����
		UploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;			// ��Դ���֣�ָ����Դ�Ĵ洢��ʽ���ϴ��ѵ���Դ���� row major �������Դ洢
		UploadResourceDesc.Width = 512;										// ��Դ��ȣ��ϴ��ѵ���Դ�������Դ���ܴ�С��ע����Դ��С����ֻ�಻��
		UploadResourceDesc.Height = 1;										// ��Դ�߶ȣ��ϴ��ѽ����Ǵ���������Դ�ģ����Ը߶ȱ���Ϊ 1
		UploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// ��Դ��ʽ���ϴ�����Դ�ĸ�ʽ����Ϊ UNKNOWN
		UploadResourceDesc.DepthOrArraySize = 1;							// ��Դ��ȣ������������������� 3D ����ģ��ϴ�����Դ����Ϊ 1
		UploadResourceDesc.MipLevels = 1;									// Mipmap �ȼ����������������ģ��ϴ�����Դ����Ϊ 1
		UploadResourceDesc.SampleDesc.Count = 1;							// ��Դ�����������ϴ�����Դ������ 1

		// �ϴ������ԵĽṹ�壬�ϴ���λ�� CPU �� GPU �Ĺ����ڴ�
		D3D12_HEAP_PROPERTIES UploadHeapDesc = { D3D12_HEAP_TYPE_UPLOAD };

		// �����ϴ�����Դ
		m_D3D12Device->CreateCommittedResource(&UploadHeapDesc, D3D12_HEAP_FLAG_NONE, &UploadResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&MaterialGroup[index].UploadTexture));


		// ע�⣡Ĭ������������ʽҪѡ DXGI_FORMAT_R8G8B8A8_UNORM��
		TextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		// ���ڷ������Ĭ�϶���Դ�ṹ��
		D3D12_RESOURCE_DESC DefaultResourceDesc = {};
		DefaultResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// ��Դ���ͣ�����ָ��Ϊ Texture2D 2D����
		DefaultResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;			// ������Դ�Ĳ��ֶ��� UNKNOWN
		DefaultResourceDesc.Width = 2;										// ��Դ��ȣ�������������
		DefaultResourceDesc.Height = 2;										// ��Դ�߶ȣ�����������߶�
		DefaultResourceDesc.Format = TextureFormat;							// ��Դ��ʽ��Ĭ������ѡ DXGI_FORMAT_R8G8B8A8_UNORM ����
		DefaultResourceDesc.DepthOrArraySize = 1;							// ��Դ��ȣ�����ֻ��һ������������ 1
		DefaultResourceDesc.MipLevels = 1;									// Mipmap �ȼ���������ʱ��ʹ�� Mipmap�������� 1
		DefaultResourceDesc.SampleDesc.Count = 1;							// ��Դ�������������������� 1 ����

		// Ĭ�϶����ԵĽṹ�壬Ĭ�϶�λ���Դ�
		D3D12_HEAP_PROPERTIES DefaultHeapDesc = { D3D12_HEAP_TYPE_DEFAULT };

		// ����Ĭ�϶���Դ
		m_D3D12Device->CreateCommittedResource(&DefaultHeapDesc, D3D12_HEAP_FLAG_NONE, &DefaultResourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&MaterialGroup[index].DefaultTexture));
	}

	// CommandList ¼�����¼�ƽ�Ĭ���������ݸ��Ƶ�Ĭ�϶���Դ������
	void CopyDefaultTextureToDefaultResource(const UINT index)
	{
		// ����Ĭ����ͼ���ݣ�һ�� 2 �У�ÿ������ DXGI_R8G8B8A8_UNROM ���ݣ�ÿ������ռ 8 Byte (Byte = UINT8)
		UINT8 DefaultTextureData[2 * 2 * 4];

		// ������ɫ����
		for (UINT i = 0; i < 2; i++)
		{
			DefaultTextureData[i * 4 + 0] = 255;	// R (��ɫ)
			DefaultTextureData[i * 4 + 1] = 255;	// G (��ɫ)
			DefaultTextureData[i * 4 + 2] = 255;	// B (��ɫ)
			DefaultTextureData[i * 4 + 3] = 128;	// A (͸����)
		}

		// �������ݵ�ָ��
		BYTE* TransferPointer = nullptr;

		// Map ��ʼӳ�䣬Map ������õ��ϴ�����Դ�ĵ�ַ (�ڹ����ڴ���)�����ݸ�ָ�룬�������Ǿ���ͨ�� memcpy ��������������
		MaterialGroup[index].UploadTexture->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));

		// ���и������ݣ�Ĭ�������ϴ���һ�� 256 Byte
		for (UINT i = 0; i < 2; i++)
		{
			// ���ϴ�����Դ���и����������� (CPU ���ٻ��� -> �����ڴ�)
			memcpy(TransferPointer, DefaultTextureData + i * 16, 16);
			// �ϴ�����Դָ��ƫ�Ƶ���һ��
			TransferPointer += 256;
		}

		// Unmap ����ӳ�䣬��Ϊ�����޷�ֱ�Ӷ�дĬ�϶���Դ����Ҫ�ϴ��Ѹ��Ƶ�����ڸ���֮ǰ��������Ҫ�Ƚ���ӳ�䣬���ϴ��Ѵ���ֻ��״̬
		MaterialGroup[index].UploadTexture->Unmap(0, nullptr);

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint = {};									// ��Դ�ű�����������Ҫ���Ƶ���Դ
		D3D12_RESOURCE_DESC DefaultResourceDesc = MaterialGroup[index].DefaultTexture->GetDesc();	// Ĭ�϶���Դ�ṹ��

		// ��ȡ�����ƽű����������ĵ�������
		m_D3D12Device->GetCopyableFootprints(&DefaultResourceDesc, 0, 1, 0, &PlacedFootprint, nullptr, nullptr, nullptr);

		D3D12_TEXTURE_COPY_LOCATION DstLocation = {};						// ����Ŀ��λ�� (Ĭ�϶���Դ) �ṹ��
		DstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;		// ���������ͣ��������ָ������
		DstLocation.SubresourceIndex = 0;									// ָ��Ҫ���Ƶ�����Դ����
		DstLocation.pResource = MaterialGroup[index].DefaultTexture.Get();	// Ҫ���Ƶ�����Դ

		D3D12_TEXTURE_COPY_LOCATION SrcLocation = {};						// ����Դλ�� (�ϴ�����Դ) �ṹ��
		SrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;		// ���������ͣ��������ָ�򻺳���
		SrcLocation.PlacedFootprint = PlacedFootprint;						// ָ��Ҫ���Ƶ���Դ�ű���Ϣ
		SrcLocation.pResource = MaterialGroup[index].UploadTexture.Get();	// ���������ݵĻ���



		// ��¼������Դ��Ĭ�϶ѵ����� (�����ڴ� -> �Դ�) 
		m_CommandList->CopyTextureRegion(&DstLocation, 0, 0, 0, &SrcLocation, nullptr);
	}

	// ���������ڴ���
	bool LoadTextureFromFile(const std::wstring TextureFilename)
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

	// ���������ϴ��� UploadResource �����ڷ������ DefaultResource
	void CreateUploadAndDefaultResource(const UINT index)
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
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&MaterialGroup[index].UploadTexture));



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
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&MaterialGroup[index].DefaultTexture));
	}

	// CommandList ¼�����¼�ƽ��������ݸ��Ƶ�Ĭ�϶���Դ������
	void CopyTextureDataToDefaultResource(const UINT index)
	{
		// ������ʱ�洢�������ݵ�ָ�룬����Ҫ�� malloc ����ռ�
		BYTE* TextureData = (BYTE*)malloc(TextureSize);

		// �������������ݶ��� TextureData �У�������ĵ� memcpy ���Ʋ���
		m_WICBitmapSource->CopyPixels(nullptr, BytePerRowSize, TextureSize, TextureData);

		// ���ڴ�����Դ��ָ��
		BYTE* TransferPointer = nullptr;

		// Map ��ʼӳ�䣬Map ������õ��ϴ�����Դ�ĵ�ַ (�ڹ����ڴ���)�����ݸ�ָ�룬�������Ǿ���ͨ�� memcpy ��������������
		MaterialGroup[index].UploadTexture->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));

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
		MaterialGroup[index].UploadTexture->Unmap(0, nullptr);

		TextureData -= TextureSize;		// ������Դָ��ƫ�ƻس�ʼλ��
		free(TextureData);				// �ͷ����� malloc ����Ŀռ䣬���������ò���������Ҫ����ռ�ڴ�

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint = {};									// ��Դ�ű�����������Ҫ���Ƶ���Դ
		D3D12_RESOURCE_DESC DefaultResourceDesc = MaterialGroup[index].DefaultTexture->GetDesc();	// Ĭ�϶���Դ�ṹ��

		// ��ȡ�����ƽű����������ĵ�������
		m_D3D12Device->GetCopyableFootprints(&DefaultResourceDesc, 0, 1, 0, &PlacedFootprint, nullptr, nullptr, nullptr);

		D3D12_TEXTURE_COPY_LOCATION DstLocation = {};						// ����Ŀ��λ�� (Ĭ�϶���Դ) �ṹ��
		DstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;		// ���������ͣ��������ָ������
		DstLocation.SubresourceIndex = 0;									// ָ��Ҫ���Ƶ�����Դ����
		DstLocation.pResource = MaterialGroup[index].DefaultTexture.Get();	// Ҫ���Ƶ�����Դ

		D3D12_TEXTURE_COPY_LOCATION SrcLocation = {};						// ����Դλ�� (�ϴ�����Դ) �ṹ��
		SrcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;		// ���������ͣ��������ָ�򻺳���
		SrcLocation.PlacedFootprint = PlacedFootprint;						// ָ��Ҫ���Ƶ���Դ�ű���Ϣ
		SrcLocation.pResource = MaterialGroup[index].UploadTexture.Get();	// ���������ݵĻ���


		// ��¼������Դ��Ĭ�϶ѵ����� (�����ڴ� -> �Դ�) 
		m_CommandList->CopyTextureRegion(&DstLocation, 0, 0, 0, &SrcLocation, nullptr);
	}

	// �ر������б�����������У���ʽ��ʼ�������Ƶ�Ĭ�϶���Դ��
	void StartCommandExecute()
	{
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

	// ���մ��� SRV ��ɫ����Դ����������������Ĭ�϶���ԴΪһ������������ SRV ���������Ὣ����������洢������ӳ�����
	void CreateSRV(const UINT index, D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle)
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

		// ���� SRV ��������ע������Ҫ�ò����е� CPUHandle
		m_D3D12Device->CreateShaderResourceView(MaterialGroup[index].DefaultTexture.Get(), &SRVDescriptorDesc, CPUHandle);

		// ����ǰ SRV ����������洢�� ModelManager ������ӳ����У�ע�����Ǵ��������ò���������ֱ�ӶԲ��������޸�
		MaterialGroup[index].CPUHandle = CPUHandle;
		MaterialGroup[index].GPUHandle = GPUHandle;
	}


	// �� std::string ת���� std::wstring
	const std::wstring StringConvertToWString(const std::string str)
	{
		// ���� wstring_convert �������� UTF-8 �� wchar ���ַ���ת��
		static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		// �� str ת���ɿ��ַ��汾 (wstring)
		return converter.from_bytes(str);
	}


	// ��ȡ������������Դ
	void STEP14_CreateModelTextureResource()
	{
		CreateSRVHeap();	// ���� SRV �������ѣ�����ʱ�ͻ�ȷ���������� CPU �� GPU ��ַ�����赣��

		// ��ǰԪ�ص� CPU ���
		D3D12_CPU_DESCRIPTOR_HANDLE CurrentCPUHandle = m_SRVHeap->GetCPUDescriptorHandleForHeapStart();
		// ��ǰԪ�ص� GPU ���
		D3D12_GPU_DESCRIPTOR_HANDLE CurrentGPUHandle = m_SRVHeap->GetGPUDescriptorHandleForHeapStart();
		// SRV �������Ĵ�С
		SRVDescriptorSize = m_D3D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		StartCommandRecord();	// ���������б���ʼ¼������

		// �Բ�������б���
		for (UINT i = 0; i < MaterialGroup.size(); i++)
		{
			// �������Ĭ������
			if (MaterialGroup[i].Type != aiTextureType_NONE)
			{
				// �������ļ��м�������
				LoadTextureFromFile(StringConvertToWString(MaterialGroup[i].FilePath));
				// �����ϴ��Ѻ�Ĭ�϶���Դ
				CreateUploadAndDefaultResource(i);
				// ���������ݸ��Ƶ��ϴ��ѣ�����¼һ���ϴ��Ѹ��Ƶ�Ĭ�϶ѵ�����
				CopyTextureDataToDefaultResource(i);
			}
			else // ����ͽ������⴦��
			{
				// ������Դ
				CreateDefaultTexture(i);
				// ��������
				CopyDefaultTextureToDefaultResource(i);
			}

			// ���մ��� SRV ������
			CreateSRV(i, CurrentCPUHandle, CurrentGPUHandle);

			// CPU �� GPU ���ƫ�ƣ�׼����һ������
			CurrentCPUHandle.ptr += SRVDescriptorSize;
			CurrentGPUHandle.ptr += SRVDescriptorSize;
		}

		StartCommandExecute();	// �ر������б������������ִ��


		// �����߳�ǿ�Ƶȴ�������е���ɣ�WaitForSingleObject ��������ǰ�̣߳�ֱ�� event ���źţ���ﵽָ��ʱ��
		// ��Ϊ���Ǹ�������Դ����Ҫִ�и��ƶ��㵽Ĭ�϶ѵ���������Ҫ�������������
		// ���������������Ҫ���ǣ�������б���ִ������������������ᷢ����Դ����
		// �� CPU��GPU ����ǡ�����첽ִ�еģ�Ҳ����˵ CommandQueue->ExecuteCommandLists() �� CPU ����Ȼ�����ִ��
		// ��������������Ҫ���� Main �������ڵ����̣߳�ͬ�� CPU �� GPU ��ִ��
		// Ȼ�������� WaitForSingleObject �������̲߳�����һ���õ�ѡ�񣬵ڶ������� INFINITE �趨�ȴ�ʱ�����ޣ�����һ�ֲ��õ�����
		// ���Ҫִ�е����������ִ࣬�к�ʱ�������߳�һ���������󶨴��ڵ���Ϣ�ص�ǡ���������߳���ִ�еģ���ô����ͻ᲻�ҵ� gg ��
		// ����Ľ̳����ǻὫ������ж��߳��Ż�������ʹ�� WaitForSingleObject ͬ�������Ǹ��� Render ��� MsgWaitForMultipleObjects
		WaitForSingleObject(RenderEvent, INFINITE);
	}



	// ---------------------------------------------------------------------------------------------------------------



	// ����������������Դ�������ϴ�����Ĭ�϶ѣ�ͬʱ����������
	void STEP15_CreateMeshResourceAndDescriptor()
	{
		// ������� (�ϴ��Ѻ�Ĭ�϶�) ����;���� buffer ���Ի��壬����Ҫ�ٽ��� 256 �ֽڶ�����

		// �ϴ��Ѷ�����Դ�ṹ��
		D3D12_RESOURCE_DESC UploadVertexResourceDesc = {};
		UploadVertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;	// �ϴ�����Դ���Ͷ��� buffer
		UploadVertexResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// buffer �ĸ�ʽ���� DXGI_FORMAT_UNKNOWN
		UploadVertexResourceDesc.Width = sizeof(VERTEX) * VertexGroup.size();	// �ܴ�С�Ƕ����� * ����ṹ���С
		UploadVertexResourceDesc.Height = 1;									// buffer �߶ȶ��� 1
		UploadVertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;		// buffer �Ĳ��ֶ��� row_major ������
		UploadVertexResourceDesc.DepthOrArraySize = 1;							// �� 1
		UploadVertexResourceDesc.MipLevels = 1;									// �� 1
		UploadVertexResourceDesc.SampleDesc.Count = 1;							// �� 1
		UploadVertexResourceDesc.SampleDesc.Quality = 0;						// �� 0

		// Ĭ�϶Ѷ�����Դ�ṹ��
		D3D12_RESOURCE_DESC DefaultVertexResourceDesc = {};
		DefaultVertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;	// ����ѡ buffer
		DefaultVertexResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// buffer �ĸ�ʽ���� DXGI_FORMAT_UNKNOWN
		DefaultVertexResourceDesc.Width = sizeof(VERTEX) * VertexGroup.size();	// �ܴ�С�Ƕ����� * ����ṹ���С
		DefaultVertexResourceDesc.Height = 1;									// buffer �߶ȶ��� 1
		DefaultVertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;		// buffer �Ĳ��ֶ��� row_major ������
		DefaultVertexResourceDesc.DepthOrArraySize = 1;							// �� 1
		DefaultVertexResourceDesc.MipLevels = 1;								// �� 1
		DefaultVertexResourceDesc.SampleDesc.Count = 1;							// �� 1
		DefaultVertexResourceDesc.SampleDesc.Quality = 0;						// �� 0

		// �ϴ���������Դ�ṹ��
		D3D12_RESOURCE_DESC UploadIndexResourceDesc = {};
		UploadIndexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;	// ����ѡ buffer
		UploadIndexResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// buffer �ĸ�ʽ���� DXGI_FORMAT_UNKNOWN
		UploadIndexResourceDesc.Width = sizeof(UINT) * IndexGroup.size();		// �ܴ�С�������� * һ�� 32 λ�����Ĵ�С
		UploadIndexResourceDesc.Height = 1;										// buffer �߶ȶ��� 1
		UploadIndexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;		// buffer �Ĳ��ֶ��� row_major ������
		UploadIndexResourceDesc.DepthOrArraySize = 1;							// �� 1
		UploadIndexResourceDesc.MipLevels = 1;									// �� 1
		UploadIndexResourceDesc.SampleDesc.Count = 1;							// �� 1
		UploadIndexResourceDesc.SampleDesc.Quality = 0;							// �� 0

		// Ĭ�϶�������Դ�ṹ��
		D3D12_RESOURCE_DESC DefaultIndexResourceDesc = {};
		DefaultIndexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;	// ����ѡ buffer
		DefaultIndexResourceDesc.Format = DXGI_FORMAT_UNKNOWN;					// buffer �ĸ�ʽ���� DXGI_FORMAT_UNKNOWN
		DefaultIndexResourceDesc.Width = sizeof(UINT) * IndexGroup.size();		// �ܴ�С�������� * һ�� 32 λ�����Ĵ�С
		DefaultIndexResourceDesc.Height = 1;									// buffer �߶ȶ��� 1
		DefaultIndexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;		// buffer �Ĳ��ֶ��� row_major ������
		DefaultIndexResourceDesc.DepthOrArraySize = 1;							// �� 1
		DefaultIndexResourceDesc.MipLevels = 1;									// �� 1
		DefaultIndexResourceDesc.SampleDesc.Count = 1;							// �� 1
		DefaultIndexResourceDesc.SampleDesc.Quality = 0;						// �� 0

		// �ϴ�������
		D3D12_HEAP_PROPERTIES UploadHeapProperties = { D3D12_HEAP_TYPE_UPLOAD };
		// Ĭ�϶�����
		D3D12_HEAP_PROPERTIES DefaultHeapProperties = { D3D12_HEAP_TYPE_DEFAULT };

		

		// ������Դ��ע��Ĭ�϶���Դ��״̬�� D3D12_RESOURCE_STATE_COPY_DEST
		m_D3D12Device->CreateCommittedResource(&UploadHeapProperties, D3D12_HEAP_FLAG_NONE, &UploadVertexResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&UploadVertexResource));

		m_D3D12Device->CreateCommittedResource(&DefaultHeapProperties, D3D12_HEAP_FLAG_NONE, &DefaultVertexResourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&DefaultVertexResource));

		m_D3D12Device->CreateCommittedResource(&UploadHeapProperties, D3D12_HEAP_FLAG_NONE, &UploadIndexResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&UploadIndexResource));

		m_D3D12Device->CreateCommittedResource(&DefaultHeapProperties, D3D12_HEAP_FLAG_NONE, &DefaultIndexResourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&DefaultIndexResource));


		// ��д VertexBufferView ������Ϣ������ �� IndexBufferView ������Ϣ������
		// ע�⣺������д���� DefaultResource �ĵ�ַ����Ϊ��������������Ҫ�������ʵĻ�����Ĭ�϶���ԴЧ�����ã���Ⱦ�ٶȸ���

		VertexBufferView.BufferLocation = DefaultVertexResource->GetGPUVirtualAddress();	// ��Դ��ַ
		VertexBufferView.SizeInBytes = DefaultVertexResourceDesc.Width;						// ��Դ��С
		VertexBufferView.StrideInBytes = sizeof(VERTEX);									// ��Դ����

		IndexBufferView.BufferLocation = DefaultIndexResource->GetGPUVirtualAddress();		// ��Դ��ַ
		IndexBufferView.SizeInBytes = DefaultIndexResourceDesc.Width;						// ��Դ��С
		IndexBufferView.Format = DXGI_FORMAT_R32_UINT;										// ��Դ��ʽ (����)
	}


	// �� Mesh ����Ķ������������ݸ��Ƶ��ϴ�����Դ����Ϊ�����������ݣ�GPU �����ϴ��ѵ��ٶȽ����൱��
	// �෴������Ĭ�϶ѵ��ٶȾͿ�ö࣬����漰��������̬���ݣ�������ͨ���ϴ����ƶ���Ĭ�϶��ϣ�ֻ���� Copy Engine ���ƻ����ʱ����һ��ʱ��
	void STEP16_CopyMeshToUploadResource()
	{
		// ���ڴ������ݵ�ָ��
		BYTE* TransferPointer = nullptr;

		// ��ȡ�ϴ��Ѷ�����Դ��ַ
		UploadVertexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));
		// �� Mesh �������ݸ��Ƶ��ϴ�����Դ��
		memcpy(TransferPointer, &VertexGroup[0], sizeof(VERTEX) * VertexGroup.size());
		// �ر�ӳ��
		UploadVertexResource->Unmap(0, nullptr);


		// ��ȡ�ϴ���������Դ��ַ
		UploadIndexResource->Map(0, nullptr, reinterpret_cast<void**>(&TransferPointer));
		// �� Mesh �������ݸ��Ƶ��ϴ�����Դ��
		memcpy(TransferPointer, &IndexGroup[0], sizeof(UINT) * IndexGroup.size());
		// �ر�ӳ��
		UploadIndexResource->Unmap(0, nullptr);
	}


	// �����б���Ӹ���������͵��������ִ�У����ϴ����ϵĶ���������Դ���Ƶ�Ĭ�϶���
	void STEP17_CopyMeshToDefaultResource()
	{
		StartCommandRecord();	// ���������б���ʼ¼������

		// CopyBufferRegion ���ƻ�����Դ�������ڲ�ͬ�ѣ�CPU ����ֱ�ӷ��ʵĻ��帴��

		// �����б���Ӹ�����������ϴ��Ѷ�����Դ��Ĭ�϶���
		m_CommandList->CopyBufferRegion(DefaultVertexResource.Get(), 0,
			UploadVertexResource.Get(), 0, sizeof(VERTEX) * VertexGroup.size());
		// �����б���Ӹ�����������ϴ��Ѷ�����Դ��Ĭ�϶���
		m_CommandList->CopyBufferRegion(DefaultIndexResource.Get(), 0,
			UploadIndexResource.Get(), 0, sizeof(UINT) * IndexGroup.size());

		StartCommandExecute();	// �ر������б������������ִ��


		// �����߳�ǿ�Ƶȴ�������е���ɣ������ĵȴ����������һ������
		WaitForSingleObject(RenderEvent, INFINITE);
	}



	// ---------------------------------------------------------------------------------------------------------------



	// ���� Constant Buffer Resource ����������Դ������������һ��Ԥ�ȷ���ĸ����Դ棬���ڴ洢ÿһ֡��Ҫ�任����Դ����������Ҫ�洢 MVP ����
	void STEP18_CreateCBVResource()
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
	void STEP19_CreateRootSignature()
	{
		ComPtr<ID3DBlob> SignatureBlob;			// ��ǩ���ֽ���
		ComPtr<ID3DBlob> ErrorBlob;				// �����ֽ��룬��ǩ������ʧ��ʱ�� OutputDebugStringA((const char*)ErrorBlob->GetBufferPointer()); ���Ի�ȡ������Ϣ

		D3D12_ROOT_PARAMETER RootParameters[2] = {};							// ����������

		// �Ѹ���Ƶ�ʸߵĸ�������ǰ�棬�͵ķź��棬�����Ż����� (΢��ٷ��ĵ�����)
		// ��Ϊ DirectX API �ܶԸ�ǩ������ Version Control �汾���ƣ��ڸ�ǩ��Խǰ��ĸ������������ٶȸ���

		// ��һ����������CBV ���������������������������������������İ󶨸�����ʱ��ֻ��Ҫ���ݳ���������Դ�ĵ�ַ����

		D3D12_ROOT_DESCRIPTOR CBVRootDescriptorDesc = {};					// �����������������Ϣ�ṹ��
		CBVRootDescriptorDesc.ShaderRegister = 0;							// Ҫ�󶨵ļĴ�����ţ������Ӧ HLSL �� b0 �Ĵ���
		CBVRootDescriptorDesc.RegisterSpace = 0;							// Ҫ�󶨵������ռ䣬�����Ӧ HLSL �� space0

		RootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// ���������������Ⱦ���߶��ɼ�
		RootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// �����������ͣ�CBV ��������
		RootParameters[0].Descriptor = CBVRootDescriptorDesc;				// �����ĵĽṹ��


		// �ڶ������������������� (Range: SRV)

		D3D12_DESCRIPTOR_RANGE SRVDescriptorRangeDesc = {};						// Range ��������Χ�ṹ�壬һ�� Range ��ʾһ��������ͬ����������
		SRVDescriptorRangeDesc.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;		// Range ���ͣ�����ָ�� SRV ���ͣ�CBV_SRV_UAV ���������
		SRVDescriptorRangeDesc.NumDescriptors = 1;								// Range ��������������� N��һ�ο��԰󶨶��������������Ĵ�������
		SRVDescriptorRangeDesc.BaseShaderRegister = 0;							// Range Ҫ�󶨵���ʼ�Ĵ����۱�� i���󶨷�Χ�� [s(i),s(i+N)]�����ǰ� s0
		SRVDescriptorRangeDesc.RegisterSpace = 0;								// Range Ҫ�󶨵ļĴ����ռ䣬���� Range ����󶨵�ͬһ�Ĵ����ռ��ϣ����ǰ� space0
		SRVDescriptorRangeDesc.OffsetInDescriptorsFromTableStart = 0;			// Range ����������ͷ��ƫ���� (��λ��������)����ǩ����Ҫ������Ѱ�� Range �ĵ�ַ���������� 0 ����

		D3D12_ROOT_DESCRIPTOR_TABLE RootDescriptorTableDesc = {};				// RootDescriptorTable ����������Ϣ�ṹ�壬һ�� Table �����ж�� Range
		RootDescriptorTableDesc.pDescriptorRanges = &SRVDescriptorRangeDesc;	// Range ��������Χָ��
		RootDescriptorTableDesc.NumDescriptorRanges = 1;						// ���������� Range ������

		RootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;				// ����������ɫ���еĿɼ��ԣ�����ָ������������ɫ���ɼ� (ֻ��������ɫ���õ�������)
		RootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	// ���������ͣ���������ѡ Table ��������һ����������ռ�� 1 DWORD
		RootParameters[1].DescriptorTable = RootDescriptorTableDesc;					// ������ָ��



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
	void STEP20_CreatePSO()
	{
		// PSO ��Ϣ�ṹ��
		D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};

		// Input Assembler ����װ��׶�
		D3D12_INPUT_LAYOUT_DESC InputLayoutDesc = {};			// ������ʽ��Ϣ�ṹ��
		D3D12_INPUT_ELEMENT_DESC InputElementDesc[2] = {};		// ����Ԫ����Ϣ�ṹ������


		// ����λ�� float4 position
		InputElementDesc[0].SemanticName = "POSITION";										// Ҫê��������
		InputElementDesc[0].SemanticIndex = 0;												// ��������	
		InputElementDesc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;						// �����ʽ
		InputElementDesc[0].InputSlot = 0;													// ����۱��	
		InputElementDesc[0].AlignedByteOffset = 0;											// ��������е�ƫ��
		InputElementDesc[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;	// ����������
		InputElementDesc[0].InstanceDataStepRate = 0;										// ʵ�����ݲ�����	


		// ���� UV ���� float2 texcoordUV
		InputElementDesc[1].SemanticName = "TEXCOORD";										// Ҫê��������
		InputElementDesc[1].SemanticIndex = 0;												// ��������
		InputElementDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;								// �����ʽ
		InputElementDesc[1].InputSlot = 0;													// ����۱��
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

		// ������Ȳ���״̬
		PSODesc.DSVFormat = DSVFormat;											// ������Ȼ���ĸ�ʽ
		PSODesc.DepthStencilState.DepthEnable = true;							// ������Ȼ���
		PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;		// ��Ȼ���ıȽϷ�ʽ
		PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	// ��Ȼ���Ķ�дȨ��

		// D3D12_DEPTH_WRITE_MASK_ALL	����ͨ����Ȳ��Ե�����д����Ȼ���			(��Ȼ���ɶ�д)
		// D3D12_DEPTH_WRITE_MASK_ZERO	��ֹ����Ȼ������д���������Կɽ�����Ȳ���	(��Ȼ���ֻ��)
		// ����ֻ��ѡһ�������ɹ��档ָ�� DepthWriteMask ���Կ���������ݵĶ�д��ʵ��ĳЩ��Ч

		/*
			��Ȳ��ԱȽ�������ȵ�α���룬���������͸��������أ������ϾͶ���
			NewPixel:			Ҫд���������
			CurrentPixel:		��ǰ�ڻ�����������
			DepthFunc:			�ȽϷ�ʽ (ʵ���Ͼ��� C/C++ �Ķ�Ԫ���������)

			if (NewPixel.Depth  DepthFunc  CurrentPixel.Depth)
			{
				Accept(NewPixel)			// ������ͨ����Ȳ��ԣ���һ�����Խ��л��
				WriteDepth(NewPixel.Depth)	// �����������д����Ȼ�����
			}
			else
			{
				Reject(NewPixel)			// ����������
			}


			D3D12_COMPARISON_FUNC_LESS �൱��С�ں� <

			if (NewPixel.Depth  <  CurrentPixel.Depth)
			{
				Accept(NewPixel)			// �����������ȸ�С��˵���������������ǰ��ͨ����Ȳ���
				WriteDepth(NewPixel.Depth)	// �����������д����Ȼ�����
			}
			else
			{
				Reject(NewPixel)			// ������������ظ����󣬱���ǰ������ס�ˣ�����������
			}
		*/


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



	// ---------------------------------------------------------------------------------------------------------------



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

		// �� RTV ���������ȾĿ�꣬ͬʱ�� DSV ����������ģ�建�壬������Ȳ���
		m_CommandList->OMSetRenderTargets(1, &RTVHandle, false, &DSVHandle);

		// ��պ�̨�����ģ�建�壬���������Ϊ��ʼֵ 1����ס���Ĵ�����Ȼ�����Դ��ʱ��Ҫ�� ClearValue
		// ����ᱨ D3D12 WARNING: The application did not pass any clear value to resource creation.
		m_CommandList->ClearDepthStencilView(DSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1, 0, 0, nullptr);

		// ��յ�ǰ��ȾĿ��ı���Ϊ����ɫ
		m_CommandList->ClearRenderTargetView(RTVHandle, DirectX::Colors::SkyBlue, 0, nullptr);

		// �����������������õ���ʱ ID3D12DescriptorHeap ����
		ID3D12DescriptorHeap* _temp_DescriptorHeaps[] = { m_SRVHeap.Get() };
		// ������������
		m_CommandList->SetDescriptorHeaps(1, _temp_DescriptorHeaps);

		// ���ó������� (��һ��������)�����Ǹ��������ݵ� CBVResource �󣬾Ϳ�������ɫ����ȡ���Զ������ MVP �任��
		m_CommandList->SetGraphicsRootConstantBufferView(0, m_CBVResource->GetGPUVirtualAddress());

		// ����ͼԪ���� (����װ��׶�)���������������������б�
		m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


		// ���� VBV��IBV ������
		m_CommandList->IASetVertexBuffers(0, 1, &VertexBufferView);
		m_CommandList->IASetIndexBuffer(&IndexBufferView);


		// ��Ⱦȫ������
		for (const auto& Mesh : MeshGroup)
		{
			// �������� SRV
			m_CommandList->SetGraphicsRootDescriptorTable(1, MaterialGroup[Mesh.MaterialIndex].GPUHandle);

			// ��������
			m_CommandList->DrawIndexedInstanced(Mesh.IndexCount, 1, Mesh.IndexGroupOffset, Mesh.VertexGroupOffset, 0);
		}


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
	void STEP21_RenderLoop()
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
				Sleep(20);
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



	// ---------------------------------------------------------------------------------------------------------------



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
				m_FirstCamera.Walk(0.2);
				break;

			case 's':
			case 'S':	// ����ƶ�
				m_FirstCamera.Walk(-0.2);
				break;

			case 'a':
			case 'A':	// �����ƶ�
				m_FirstCamera.Strafe(0.2);
				break;

			case 'd':
			case 'D':	// �����ƶ�
				m_FirstCamera.Strafe(-0.2);
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
		engine.STEP1_InitWindow(hins);
		engine.STEP2_CreateDebugDevice();
		engine.STEP3_CreateDevice();
		engine.STEP4_CreateCommandComponents();
		engine.STEP5_CreateRenderTarget();
		engine.STEP6_CreateFenceAndBarrier();
		engine.STEP7_CreateDSVHeap();
		engine.STEP8_CreateDepthStencilBuffer();
		engine.STEP9_CreateDSV();

		engine.STEP10_OpenModelFile();
		engine.STEP11_AddModelMaterials();
		engine.STEP12_AddModelData();
		engine.STEP13_CalcModelBoundingBox();

		engine.STEP14_CreateModelTextureResource();

		engine.STEP15_CreateMeshResourceAndDescriptor();
		engine.STEP16_CopyMeshToUploadResource();
		engine.STEP17_CopyMeshToDefaultResource();

		engine.STEP18_CreateCBVResource();
		engine.STEP19_CreateRootSignature();
		engine.STEP20_CreatePSO();

		engine.STEP21_RenderLoop();
	}
};


// ������
int WINAPI WinMain(HINSTANCE hins, HINSTANCE hPrev, LPSTR cmdLine, int cmdShow)
{
	DX12Engine::Run(hins);
}
