// 需要 C++ 17
#include <unknwn.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.Search.h>
#include <ShObjIdl.h>
#include <wincodec.h>
#include <dxgi1_6.h>
#include <conio.h>
#include <iostream>
#include <locale>
#include <map>
#include <wrl.h>

#pragma comment(lib,"windowsapp")
#pragma comment(lib,"dxgi")	
#pragma comment(lib,"dxguid")
#pragma comment(lib,"windowscodecs.lib")

using namespace Microsoft;
using namespace Microsoft::WRL;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Pickers;
using namespace winrt::Windows::Storage::Search;

namespace DX12TextureHelper
{
	struct WICTranslate
	{
		GUID wic;
		DXGI_FORMAT format;
	};

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

	struct WICConvert
	{
		GUID source;
		GUID target;
	};

	static WICConvert g_WICConvert[] =
	{
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
		return false;
	}

	DXGI_FORMAT GetDXGIFormatFromPixelFormat(const GUID* pPixelFormat)
	{
		for (size_t i = 0; i < _countof(g_WICFormats); ++i)
		{
			if (InlineIsEqualGUID(g_WICFormats[i].wic, *pPixelFormat))
			{
				return g_WICFormats[i].format;
			}
		}
		return DXGI_FORMAT_UNKNOWN;
	}

	const std::map<DXGI_FORMAT, winrt::hstring> RESULT_MAP =
	{
		{DXGI_FORMAT_R32G32B32A32_FLOAT, winrt::to_hstring("DXGI_FORMAT_R32G32B32A32_FLOAT")},
		{DXGI_FORMAT_R16G16B16A16_FLOAT, winrt::to_hstring("DXGI_FORMAT_R16G16B16A16_FLOAT")},
		{DXGI_FORMAT_R16G16B16A16_UNORM, winrt::to_hstring("DXGI_FORMAT_R16G16B16A16_UNORM")},
		{DXGI_FORMAT_R8G8B8A8_UNORM, winrt::to_hstring("DXGI_FORMAT_R8G8B8A8_UNORM")},
		{DXGI_FORMAT_B8G8R8A8_UNORM, winrt::to_hstring("DXGI_FORMAT_B8G8R8A8_UNORM")},
		{DXGI_FORMAT_B8G8R8X8_UNORM, winrt::to_hstring("DXGI_FORMAT_B8G8R8X8_UNORM")},
		{DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, winrt::to_hstring("DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM")},
		{DXGI_FORMAT_R10G10B10A2_UNORM, winrt::to_hstring("DXGI_FORMAT_R10G10B10A2_UNORM")},
		{DXGI_FORMAT_B5G5R5A1_UNORM, winrt::to_hstring("DXGI_FORMAT_B5G5R5A1_UNORM")},
		{DXGI_FORMAT_B5G6R5_UNORM, winrt::to_hstring("DXGI_FORMAT_B5G6R5_UNORM")},
		{DXGI_FORMAT_R32_FLOAT, winrt::to_hstring("DXGI_FORMAT_R32_FLOAT")},
		{DXGI_FORMAT_R16_FLOAT, winrt::to_hstring("DXGI_FORMAT_R16_FLOAT")},
		{DXGI_FORMAT_R16_UNORM, winrt::to_hstring("DXGI_FORMAT_R16_UNORM")},
		{DXGI_FORMAT_R8_UNORM, winrt::to_hstring("DXGI_FORMAT_R8_UNORM")},
		{DXGI_FORMAT_A8_UNORM, winrt::to_hstring("DXGI_FORMAT_A8_UNORM")}
	};
}

ComPtr<IWICImagingFactory> m_WICFactory;
ComPtr<IWICBitmapDecoder> m_WICBitmapDecoder;
ComPtr<IWICBitmapFrameDecode> m_WICBitmapDecodeFrame;
ComPtr<IWICBitmapSource> m_WICBitmapSource;

#define check(hr) if(hr != S_OK) throw hr;

winrt::hstring GetFormat(winrt::hstring path)
{
	try
	{
		HRESULT hr;
		WICPixelFormatGUID SourceFormat = {};
		GUID TargetFormat = {};
		DXGI_FORMAT TextureFormat = DXGI_FORMAT_UNKNOWN;

		hr = m_WICFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &m_WICBitmapDecoder);
		check(hr);
		hr = m_WICBitmapDecoder->GetFrame(0, &m_WICBitmapDecodeFrame);
		check(hr);
		hr = m_WICBitmapDecodeFrame->GetPixelFormat(&SourceFormat);
		check(hr);

		if (DX12TextureHelper::GetTargetPixelFormat(&SourceFormat, &TargetFormat))
		{
			TextureFormat = DX12TextureHelper::GetDXGIFormatFromPixelFormat(&TargetFormat);
		}
		else
		{
			throw WINCODEC_ERR_COMPONENTNOTFOUND;
		}
		return DX12TextureHelper::RESULT_MAP.at(TextureFormat);
	}
	catch (HRESULT hr)
	{
		return winrt::to_hstring("DXGI_FORMAT_UNKNOWN");
	}
}


void menu()
{
	system("cls");
	SetWindowText(::GetConsoleWindow(), L"DirectX 12 纹理遍历统计工具 - by DGAF 2026.2.11");
	std::wcout << L"1. 选择几个图片文件进行统计" << std::endl;
	std::wcout << L"2. 选择一层文件夹遍历统计" << std::endl;
	std::wcout << L"0. 退出" << std::endl;
}


void Action1()
{
	system("cls");
	SetWindowText(::GetConsoleWindow(), L"请选择一个或多个图片 (可用 Ctrl+A 全选或 Ctrl长按+鼠标左键 多选)，它会自动输出图像的 DXGI 格式");
	auto OpenPicker = FileOpenPicker();
	OpenPicker.as<::IInitializeWithWindow>()->Initialize(::GetConsoleWindow());
	OpenPicker.FileTypeFilter().ReplaceAll({ winrt::to_hstring(".bmp"), winrt::to_hstring(".png"), winrt::to_hstring(".jpg"),
		winrt::to_hstring(".jpeg") , winrt::to_hstring(".gif"), winrt::to_hstring(".adng"), winrt::to_hstring(".webp"),
		winrt::to_hstring(".ico") , winrt::to_hstring(".heif") , winrt::to_hstring(".jxr"), winrt::to_hstring(".wmp"),
		winrt::to_hstring(".tiff"), winrt::to_hstring(".dds"), winrt::to_hstring(".raw") });
	auto Files = OpenPicker.PickMultipleFilesAsync().get();

	if (Files != nullptr)
	{
		std::wcout << L"一共选择了 " << Files.Size() << L" 个文件：\n\n";

		for (const auto file : Files)
		{
			auto result = GetFormat(file.Path());
			if (result != winrt::to_hstring("DXGI_FORMAT_UNKNOWN"))
			{
				std::wcout << L"文件：" << file.Name().c_str() << L" ( " << result.c_str() << L" ) " << std::endl;
			}
			else
			{
				std::wcout << L"文件：" << file.Name().c_str() << L" ( 无法解析 ) " << std::endl;
			}
		}

		std::wcout << std::endl;
		system("pause");
	}
}


void Action2()
{
	std::map<winrt::hstring, std::vector<winrt::hstring>> TypeFileMap;

	system("cls");
	SetWindowText(::GetConsoleWindow(), L"请选择一个有图片的文件夹，它会自动遍历并输出 DirectX 12 可以使用的图像文件名与 DXGI 格式 (只会遍历一层)");
	auto OpenPicker = FolderPicker();
	OpenPicker.as<::IInitializeWithWindow>()->Initialize(::GetConsoleWindow());
	OpenPicker.FileTypeFilter().Append(L"*");
	auto folder = OpenPicker.PickSingleFolderAsync().get();

	if (folder != nullptr)
	{
		for (const auto& file : folder.GetFilesAsync().get())
		{
			auto result = GetFormat(file.Path());
			if (result != winrt::to_hstring("DXGI_FORMAT_UNKNOWN"))
			{
				if (TypeFileMap.find(result) != TypeFileMap.end())
				{
					TypeFileMap[result].push_back(file.Name());
				}
				else
				{
					TypeFileMap[result] = std::vector<winrt::hstring>();
					TypeFileMap[result].push_back(file.Name());
				}
			}
		}

		std::wcout << L"该文件夹一共有 " << TypeFileMap.size() << L" 种类型的图像文件\n\n";

		for (const auto& TypeGroup : TypeFileMap)
		{
			std::wcout << TypeGroup.first.c_str() << L" 类型的图像文件有 " << TypeGroup.second.size() << L" 个：" << std::endl;

			for (const auto& FileName : TypeGroup.second)
			{
				std::wcout << FileName.c_str() << std::endl;
			}
			std::wcout << std::endl;
		}

		std::wcout << std::endl;
		system("pause");
	}
}


int main()
{
	winrt::init_apartment();
	std::locale::global(std::locale(""));
	std::wcout.imbue(std::locale(""));

	CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_WICFactory));

	while (1)
	{
		menu();
		switch (_getch())
		{
			case '1': Action1(); break;
			case '2': Action2(); break;
			case '0': return 0;
		}
	}
	return 0;
}
