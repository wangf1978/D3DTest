#include "stdafx.h"
#include "D3DReport.h"
#include <wrl/client.h>
#include <tchar.h>
#include "PCIHDR.h"
#include "D3DTypeNames.h"
#include <string>
#include <Mferror.h>
#include <tuple>
#include <sstream>
#include <iomanip>
#include <stdlib.h>

#define D3DREPORT_LOG	_tprintf

using namespace Microsoft::WRL;

std::ostream& operator<< (std::ostream &os, GUID& guid)
{
	return 	os << std::hex
		<< std::setw(8) << std::setfill('0') << guid.Data1 << '-'
		<< std::setw(4) << std::setfill('0') << guid.Data2 << '-'
		<< std::setw(4) << std::setfill('0') << guid.Data3 << '-'
		<< std::setw(2) << std::setfill('0') << (int)guid.Data4[0]
		<< std::setw(2) << std::setfill('0') << (int)guid.Data4[1] << '-'
		<< std::setw(2) << std::setfill('0') << (int)guid.Data4[2]
		<< std::setw(2) << std::setfill('0') << (int)guid.Data4[3]
		<< std::setw(2) << std::setfill('0') << (int)guid.Data4[4]
		<< std::setw(2) << std::setfill('0') << (int)guid.Data4[5]
		<< std::setw(2) << std::setfill('0') << (int)guid.Data4[6]
		<< std::setw(2) << std::setfill('0') << (int)guid.Data4[7];
}

std::tuple<DXGI_RATIONAL/* frame-rate */, UINT /* width */, UINT /* height */> video_fmts[] =
{
	{ { 30000, 1001 }, 720, 480 },
	{ { 60000, 1001 }, 720, 480 },
	{ { 25, 1 }, 720, 576 },
	{ { 50, 1 }, 720, 576 },
	{ { 30000, 1001 }, 1920, 1080 },
	{ { 60000, 1001 }, 1920, 1080 },
	{ { 25, 1 }, 1920, 1080 },
	{ { 50, 1 }, 1920, 1080 },
	{ { 24000, 1001 }, 1920, 1080 },
	{ { 24, 1 }, 1920, 1080 },
	{ { 60000, 1001 }, 1280, 720 },
	{ { 50, 1 }, 1280, 720 },
	{ { 24000, 1001 }, 1280, 720 },
	{ { 24, 1 }, 1280, 720 },
	{ { 60, 1 }, 1920, 1080 },
	{ { 24000, 1001 }, 3840, 2160 },
	{ { 24, 1 }, 3840, 2160 },
	{ { 25, 1 }, 3840, 2160 },
	{ { 50, 1 }, 3840, 2160 },
	{ { 60000, 1001 }, 3840, 2160 },
	{ { 60, 1 }, 3840, 2160 }
};

std::tuple<UINT, const char*> device_cap_names[] = {
	{ D3D11_VIDEO_PROCESSOR_DEVICE_CAPS_LINEAR_SPACE, "Linear_Space" },
	{ D3D11_VIDEO_PROCESSOR_DEVICE_CAPS_xvYCC, "xvYCC" },
	{ D3D11_VIDEO_PROCESSOR_DEVICE_CAPS_RGB_RANGE_CONVERSION, "RGB_RANGE_CONVERSION" },
	{ D3D11_VIDEO_PROCESSOR_DEVICE_CAPS_YCbCr_MATRIX_CONVERSION, "YcbCr_MATRIX_CONVERSION" },
	{ D3D11_VIDEO_PROCESSOR_DEVICE_CAPS_NOMINAL_RANGE, "Nominal_Range" }
};

std::tuple<UINT, const char*> feature_cap_names[] = {
	{ D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_ALPHA_FILL, "Alpha Fill" },
	{ D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_CONSTRICTION, "Constriction" },
	{ D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_LUMA_KEY, "LumaKey" },
	{ D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_ALPHA_PALETTE, "Alpha Palette" },
	{ D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_LEGACY, "Legacy" },
	{ D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_STEREO, "Stereoscopic" },
	{ D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_ROTATION, "Rotation" },
	{ D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_ALPHA_STREAM, "Alpha Stream" },
	{ D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_PIXEL_ASPECT_RATIO, "Pixel Aspect Ratio" },
	{ D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_MIRROR, "Mirror" },
	{ D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_SHADER_USAGE, "Shader Usage" },
	{ D3D11_VIDEO_PROCESSOR_FEATURE_CAPS_METADATA_HDR10, "HDR10" }
};


std::tuple<UINT, const char*> filter_cap_names[] = {
	{ D3D11_VIDEO_PROCESSOR_FILTER_CAPS_BRIGHTNESS, "Brightness" },
	{ D3D11_VIDEO_PROCESSOR_FILTER_CAPS_CONTRAST, "Contrast" },
	{ D3D11_VIDEO_PROCESSOR_FILTER_CAPS_HUE, "Hue" },
	{ D3D11_VIDEO_PROCESSOR_FILTER_CAPS_SATURATION, "Saturation" },
	{ D3D11_VIDEO_PROCESSOR_FILTER_CAPS_NOISE_REDUCTION, "Noise Reduction" },
	{ D3D11_VIDEO_PROCESSOR_FILTER_CAPS_EDGE_ENHANCEMENT, "Edge Enhancement" },
	{ D3D11_VIDEO_PROCESSOR_FILTER_CAPS_ANAMORPHIC_SCALING, "Anamorphic Scaling" },
	{ D3D11_VIDEO_PROCESSOR_FILTER_CAPS_STEREO_ADJUSTMENT, "Stereo Adjustment" }
};

std::tuple<UINT, const char*> inputformat_cap_names[] = {
	{ D3D11_VIDEO_PROCESSOR_FORMAT_CAPS_RGB_INTERLACED, "RGB Interlaced" },
	{ D3D11_VIDEO_PROCESSOR_FORMAT_CAPS_RGB_PROCAMP, "RGB ProcAmp" },
	{ D3D11_VIDEO_PROCESSOR_FORMAT_CAPS_RGB_LUMA_KEY, "RGB Luma Key" },
	{ D3D11_VIDEO_PROCESSOR_FORMAT_CAPS_PALETTE_INTERLACED, "Palette Interlaced" },
};

std::tuple<UINT, const char*> autostream_cap_names[] = {
	{ D3D11_VIDEO_PROCESSOR_AUTO_STREAM_CAPS_DENOISE, "De-Noise" },
	{ D3D11_VIDEO_PROCESSOR_AUTO_STREAM_CAPS_DERINGING, "Deringing" },
	{ D3D11_VIDEO_PROCESSOR_AUTO_STREAM_CAPS_EDGE_ENHANCEMENT, "Edge Enhancement" },
	{ D3D11_VIDEO_PROCESSOR_AUTO_STREAM_CAPS_COLOR_CORRECTION, "Color Correction" },
	{ D3D11_VIDEO_PROCESSOR_AUTO_STREAM_CAPS_FLESH_TONE_MAPPING, "Flesh Tone Mapping" },
	{ D3D11_VIDEO_PROCESSOR_AUTO_STREAM_CAPS_IMAGE_STABILIZATION, "Image Stabilization" },
	{ D3D11_VIDEO_PROCESSOR_AUTO_STREAM_CAPS_SUPER_RESOLUTION, "Super Resolution" },
	{ D3D11_VIDEO_PROCESSOR_AUTO_STREAM_CAPS_ANAMORPHIC_SCALING, "Anamorphic Scaling" }
};

std::tuple<UINT, const char*> stereo_cap_names[] = {
	{ D3D11_VIDEO_PROCESSOR_STEREO_CAPS_MONO_OFFSET, "Mono Offset" },
	{ D3D11_VIDEO_PROCESSOR_STEREO_CAPS_ROW_INTERLEAVED, "Row Interleaved" },
	{ D3D11_VIDEO_PROCESSOR_STEREO_CAPS_COLUMN_INTERLEAVED, "Column Interleaved" },
	{ D3D11_VIDEO_PROCESSOR_STEREO_CAPS_CHECKERBOARD, "CheckerBoard" },
	{ D3D11_VIDEO_PROCESSOR_STEREO_CAPS_FLIP_MODE, "Flip Mode" }
};


CD3DReport::CD3DReport()
{
	m_xmlPrinter.SetIndent("    ");
}


CD3DReport::~CD3DReport()
{
}

HRESULT CD3DReport::Generate()
{
	HRESULT hr = S_OK;
	// Clean the previous XML object which store D3D report
	m_xmlDoc.Clear();
	
	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "utf-8", "");
	m_xmlDoc.LinkEndChild(decl);

	TiXmlElement* root_element = new TiXmlElement("Report");
	m_xmlDoc.LinkEndChild(root_element);

	// Generate the report for IDXGIAdaptor
	ComPtr<IDXGIFactory> spFactory;
	if (SUCCEEDED(CreateDXGIFactory(IID_IDXGIFactory, (void**)&spFactory)))
	{
		UINT idxAdaptor = 0;
		ComPtr<IDXGIAdapter> spAdaptor;
		while (SUCCEEDED(spFactory->EnumAdapters(idxAdaptor, &spAdaptor)))
		{
			TiXmlElement* adaptor_element = new TiXmlElement("Adapter");
			adaptor_element->SetAttribute("ID", idxAdaptor);
			root_element->LinkEndChild(adaptor_element);

			GenerateAdaptorReport(spAdaptor.Get(), adaptor_element);

			idxAdaptor++;
		}
	}
	else
	{
		D3DREPORT_LOG(_T("[D3DReport] Failed to create DXGI Factory.\n"));
	}
	
	m_xmlDoc.Accept(&m_xmlPrinter);

	return hr;
}

const char* CD3DReport::GetReport()
{
	return m_xmlPrinter.CStr();
}

bool CD3DReport::SaveToFile(const char* str_file_name)
{
	return m_xmlDoc.SaveFile(str_file_name);
}

HRESULT CD3DReport::GenerateAdaptorReport(IDXGIAdapter* pAdater, TiXmlElement* pAdaptorElement)
{
	// Generate the report for IDXGIAdaptor
	TiXmlElement* adaptor_1_0_element = new TiXmlElement("Version");
	adaptor_1_0_element->SetAttribute("number", "1.0");
	pAdaptorElement->LinkEndChild(adaptor_1_0_element);

	// Test IDXGIAdapter::CheckInterfaceSupport 
	LARGE_INTEGER UMDVersion;
	TiXmlElement* element_check_interface_support = new TiXmlElement("ID3D10Device");
	if (SUCCEEDED(pAdater->CheckInterfaceSupport(IID_ID3D10Device, &UMDVersion)))
	{
		char szUMDVersion[128];
		sprintf_s(szUMDVersion, "0X%llX(%lld)", UMDVersion.QuadPart, UMDVersion.QuadPart);
		element_check_interface_support->SetAttribute("Support", "Yes");
		element_check_interface_support->SetAttribute("UMDVersion", szUMDVersion);
	}
	else
	{
		element_check_interface_support->SetAttribute("Support", "No");
	}
	adaptor_1_0_element->LinkEndChild(element_check_interface_support);

	element_check_interface_support = new TiXmlElement("ID3D10Device1");
	if (SUCCEEDED(pAdater->CheckInterfaceSupport(IID_ID3D10Device1, &UMDVersion)))
	{
		char szUMDVersion[128];
		sprintf_s(szUMDVersion, "0X%llX(%lld)", UMDVersion.QuadPart, UMDVersion.QuadPart);
		element_check_interface_support->SetAttribute("Support", "Yes");
		element_check_interface_support->SetAttribute("UMDVersion", szUMDVersion);
	}
	else
	{
		element_check_interface_support->SetAttribute("Support", "No");
	}
	adaptor_1_0_element->LinkEndChild(element_check_interface_support);

	DXGI_ADAPTER_DESC desc;
	TiXmlElement* element_desc = new TiXmlElement("Desc");
	if (SUCCEEDED(pAdater->GetDesc(&desc)))
	{
		TiXmlElement* element_item = new TiXmlElement("Description");
		char szDesc[512];
		memset(szDesc, 0, sizeof(szDesc));
		if (WideCharToMultiByte(CP_UTF8, 0, desc.Description, 128, szDesc, 512, NULL, NULL) > 0)
		{
			TiXmlText* pText = new TiXmlText(szDesc);
			element_item->LinkEndChild(pText);
		}
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("Vendor");
		sprintf_s(szDesc, "%d(0X%X)", desc.VendorId, desc.VendorId);
		element_item->SetAttribute("ID", szDesc);

		for(int idxVendor=0;idxVendor<sizeof(PciVenTable)/sizeof(PciVenTable[0]); idxVendor++)
			if (PciVenTable[idxVendor].VenId == desc.VendorId)
			{
				if (PciVenTable[idxVendor].VenShort[0] != '\0')
					element_item->SetAttribute("VendorShortName", PciVenTable[idxVendor].VenShort);

				if (PciVenTable[idxVendor].VenFull[0] != '\0')
				{
					TiXmlText* pText = new TiXmlText(PciVenTable[idxVendor].VenFull);
					element_item->LinkEndChild(pText);
				}
			}
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("Device");
		sprintf_s(szDesc, "%d(0X%X)", desc.DeviceId, desc.DeviceId);
		element_item->SetAttribute("ID", szDesc);

		for (int idxDevice = 0; idxDevice<sizeof(PciDevTable) / sizeof(PciDevTable[0]); idxDevice++)
			if (PciDevTable[idxDevice].VenId == desc.VendorId && PciDevTable[idxDevice].DevId == desc.DeviceId)
			{
				if (PciDevTable[idxDevice].Chip[0] != '\0')
					element_item->SetAttribute("Chipset", PciDevTable[idxDevice].Chip);

				if (PciDevTable[idxDevice].ChipDesc[0] != '\0')
				{
					TiXmlText* pText = new TiXmlText(PciDevTable[idxDevice].ChipDesc);
					element_item->LinkEndChild(pText);
				}
			}
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("SubSysId");
		sprintf_s(szDesc, "%lu(0X%X)", desc.SubSysId, desc.SubSysId);
		element_item->SetAttribute("ID", szDesc);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("Revision");
		sprintf_s(szDesc, "%d(0X%X)", desc.Revision, desc.Revision);
		element_item->SetAttribute("ID", szDesc);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("DedicatedVideoMemory");
		sprintf_s(szDesc, "%llu(0X%llX) bytes", (unsigned long long)desc.DedicatedVideoMemory, (unsigned long long)desc.DedicatedVideoMemory);
		element_item->SetAttribute("Bytes", szDesc);
		sprintf_s(szDesc, "%.2f GB", desc.DedicatedVideoMemory/1024.f/1024.f);
		element_item->SetAttribute("GB", szDesc);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("DedicatedSystemMemory");
		sprintf_s(szDesc, "%llu(0X%llX) bytes", (unsigned long long)desc.DedicatedSystemMemory, (unsigned long long)desc.DedicatedSystemMemory);
		element_item->SetAttribute("Bytes", szDesc);
		sprintf_s(szDesc, "%.2f GB", desc.DedicatedSystemMemory / 1024.f / 1024.f);
		element_item->SetAttribute("GB", szDesc);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("SharedSystemMemory");
		sprintf_s(szDesc, "%llu(0X%llX) bytes", (unsigned long long)desc.SharedSystemMemory, (unsigned long long)desc.SharedSystemMemory);
		element_item->SetAttribute("Bytes", szDesc);
		sprintf_s(szDesc, "%.2f GB", desc.SharedSystemMemory / 1024.f / 1024.f);
		element_item->SetAttribute("GB", szDesc);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("AdapterLuid");
		sprintf_s(szDesc, "%llu(0X%llX)", ((long long)desc.AdapterLuid.HighPart<<32)|desc.AdapterLuid.LowPart, ((long long)desc.AdapterLuid.HighPart << 32) | desc.AdapterLuid.LowPart);
		element_item->SetAttribute("LUID", szDesc);
		element_desc->LinkEndChild(element_item);
	}
	else
	{
		TiXmlText* pText = new TiXmlText("Failed");
		element_desc->LinkEndChild(pText);
	}
	adaptor_1_0_element->LinkEndChild(element_desc);

	UINT idxOutput = 0;
	ComPtr<IDXGIOutput> spOutput;
	while (SUCCEEDED(pAdater->EnumOutputs(idxOutput, &spOutput)))
	{
		TiXmlElement* output_element = new TiXmlElement("Output");
		output_element->SetAttribute("ID", idxOutput);
		adaptor_1_0_element->LinkEndChild(output_element);

		GenerateOutputReport(spOutput.Get(), output_element);

		idxOutput++;
	}

	// Generate the report for IDXGIAdaptor1


	// Generate HW video decoder report
	ComPtr<ID3D11Device> spD3D11Device;
	D3D_FEATURE_LEVEL FeatureLevel;
	static const D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_10_0,
	};

	UINT flags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT | D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;

	if (SUCCEEDED(D3D11CreateDevice(pAdater,
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		flags,
		levels,
		ARRAYSIZE(levels),
		D3D11_SDK_VERSION,
		&spD3D11Device,
		&FeatureLevel,
		NULL)))
	{
		ComPtr<ID3D11VideoDevice> spVideoDevice;
		if (SUCCEEDED(spD3D11Device.As(&spVideoDevice)))
		{
			TiXmlElement* video_device_element = new TiXmlElement("VideoDevice");
			pAdaptorElement->LinkEndChild(video_device_element);

			GenerateVideoDeviceReport(spVideoDevice.Get(), video_device_element);
		}
	}

	return S_OK;
}

HRESULT CD3DReport::GenerateOutputReport(IDXGIOutput* pOutput, TiXmlElement* pOutputElement)
{
	HRESULT hr = S_OK;

	// Generate the report for IDXGIOutput
	TiXmlElement* output_0_0_element = new TiXmlElement("Version");
	output_0_0_element->SetAttribute("number", "0.0");
	pOutputElement->LinkEndChild(output_0_0_element);

	DXGI_OUTPUT_DESC desc;
	TiXmlElement* element_desc = new TiXmlElement("Desc");
	if (SUCCEEDED(hr = pOutput->GetDesc(&desc)))
	{
		TiXmlElement* element_item = new TiXmlElement("DeviceName");
		char szOutputName[128];
		memset(szOutputName, 0, sizeof(szOutputName));
		if (WideCharToMultiByte(CP_UTF8, 0, desc.DeviceName, sizeof(desc.DeviceName)/sizeof(desc.DeviceName[0]), szOutputName, 128, NULL, NULL) > 0)
		{
			TiXmlText* pText = new TiXmlText(szOutputName);
			element_item->LinkEndChild(pText);
		}
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("DesktopCoordinates");
		element_item->SetAttribute("left", desc.DesktopCoordinates.left);
		element_item->SetAttribute("top", desc.DesktopCoordinates.top);
		element_item->SetAttribute("right", desc.DesktopCoordinates.right);
		element_item->SetAttribute("bottom", desc.DesktopCoordinates.bottom);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("AttachedToDesktop");
		TiXmlText* pText = new TiXmlText(desc.AttachedToDesktop ? "Yes" : "No");
		element_item->LinkEndChild(pText);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("Rotation");
		pText = new TiXmlText(desc.Rotation == DXGI_MODE_ROTATION_IDENTITY ? "No Rotation" : (
			desc.Rotation == DXGI_MODE_ROTATION_ROTATE90 ? "90 degree" : (
				desc.Rotation == DXGI_MODE_ROTATION_ROTATE180 ? "180 degree" : (
					desc.Rotation == DXGI_MODE_ROTATION_ROTATE270 ? "270 degree" : "Unspecified"))));
		element_item->LinkEndChild(pText);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("Monitor");
		char szHandleMonitor[256];
		sprintf_s(szHandleMonitor, "0x%p", desc.Monitor);
		pText = new TiXmlText(szHandleMonitor);
		element_item->LinkEndChild(pText);
		element_desc->LinkEndChild(element_item);
	}
	else
	{
		char szHR[64];
		sprintf_s(szHR, "0X%X(%ld)", hr, hr);
		TiXmlText* pText = new TiXmlText(szHR);
		element_desc->LinkEndChild(pText);
	}
	output_0_0_element->LinkEndChild(element_desc);

	for (int idxFmt = 0; idxFmt <= DXGI_FORMAT_V408; idxFmt++)
	{
		UINT numModes = 0;
		if (SUCCEEDED(pOutput->GetDisplayModeList((DXGI_FORMAT)idxFmt, 0, &numModes, NULL)) && numModes > 0)
		{
			DXGI_MODE_DESC* pDescs = new DXGI_MODE_DESC[numModes];
			if (SUCCEEDED(pOutput->GetDisplayModeList((DXGI_FORMAT)idxFmt, 0, &numModes, pDescs)))
			{
				TiXmlElement* element_modes = new TiXmlElement("DisplayModes");
				element_modes->SetAttribute("Format", DXGI_FORMAT_Name(idxFmt));
				element_modes->SetAttribute("number_of_modes", numModes);
				for (UINT idxDesc = 0; idxDesc < numModes; idxDesc++)
				{
					TiXmlElement* element_mode = new TiXmlElement("DisplayMode");

					element_mode->SetAttribute("Format", DXGI_FORMAT_Name(pDescs[idxDesc].Format));
					element_mode->SetAttribute("Width", pDescs[idxDesc].Width);
					element_mode->SetAttribute("Height", pDescs[idxDesc].Height);

					char szElement[64];
					sprintf_s(szElement, "%.2f(%lu/%lu)", pDescs[idxDesc].RefreshRate.Numerator*1.f / pDescs[idxDesc].RefreshRate.Denominator,
						pDescs[idxDesc].RefreshRate.Numerator, pDescs[idxDesc].RefreshRate.Denominator);
					element_mode->SetAttribute("RefreshRate", szElement);

					element_mode->SetAttribute("ScanlineOrdering", pDescs[idxDesc].ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE?"Progressive":(
													pDescs[idxDesc].ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST?"TopField First":(
													pDescs[idxDesc].ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST ? "BottomField First" : "Unspecified")));
					element_mode->SetAttribute("Scaling", pDescs[idxDesc].Scaling == DXGI_MODE_SCALING_CENTERED?"No scaling":(
													pDescs[idxDesc].Scaling == DXGI_MODE_SCALING_STRETCHED ? "Stretched scaling" : "Unspecified"));

					element_modes->LinkEndChild(element_mode);
				}
				output_0_0_element->LinkEndChild(element_modes);
			}
			delete[] pDescs;
		}
	}

	element_desc = new TiXmlElement("FrameStatistics");

	DXGI_FRAME_STATISTICS frame_stat;
	if (SUCCEEDED(hr = pOutput->GetFrameStatistics(&frame_stat)))
	{
		TiXmlElement* element_item = new TiXmlElement("PresentCount");
		TiXmlText* pText = new TiXmlText(std::to_string(frame_stat.PresentCount).c_str());
		element_item->LinkEndChild(pText);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("PresentRefreshCount");
		pText = new TiXmlText(std::to_string(frame_stat.PresentRefreshCount).c_str());
		element_item->LinkEndChild(pText);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("SyncRefreshCount");
		pText = new TiXmlText(std::to_string(frame_stat.SyncRefreshCount).c_str());
		element_item->LinkEndChild(pText);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("SyncQPCTime");
		pText = new TiXmlText(std::to_string(frame_stat.SyncQPCTime.QuadPart).c_str());
		element_item->LinkEndChild(pText);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("SyncGPUTime");
		pText = new TiXmlText(std::to_string(frame_stat.SyncGPUTime.QuadPart).c_str());
		element_item->LinkEndChild(pText);
		element_desc->LinkEndChild(element_item);
	}
	else
	{
		char szHR[64];
		sprintf_s(szHR, "0X%X(%ld)", hr, hr);
		TiXmlText* pText = new TiXmlText(szHR);
		element_desc->LinkEndChild(pText);
	}

	output_0_0_element->LinkEndChild(element_desc);

	element_desc = new TiXmlElement("GammaControlCapabilities");

	DXGI_GAMMA_CONTROL_CAPABILITIES GammaCaps;
	if (SUCCEEDED(hr = pOutput->GetGammaControlCapabilities(&GammaCaps)))
	{
		TiXmlElement* element_item = new TiXmlElement("ScaleAndOffsetSupported");
		TiXmlText* pText = new TiXmlText(GammaCaps.ScaleAndOffsetSupported?"Yes":"No");
		element_item->LinkEndChild(pText);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("MaxConvertedValue");
		pText = new TiXmlText(std::to_string(GammaCaps.MaxConvertedValue).c_str());
		element_item->LinkEndChild(pText);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("MinConvertedValue");
		pText = new TiXmlText(std::to_string(GammaCaps.MinConvertedValue).c_str());
		element_item->LinkEndChild(pText);
		element_desc->LinkEndChild(element_item);

		element_item = new TiXmlElement("NumGammaControlPoints");
		pText = new TiXmlText(std::to_string(GammaCaps.NumGammaControlPoints).c_str());
		element_item->LinkEndChild(pText);
		element_desc->LinkEndChild(element_item);

		for (UINT idxPoint = 0; idxPoint < GammaCaps.NumGammaControlPoints; idxPoint++)
		{
			element_item = new TiXmlElement("ControlPointPositions");
			pText = new TiXmlText(std::to_string(GammaCaps.ControlPointPositions[idxPoint]).c_str());
			element_item->LinkEndChild(pText);
			element_desc->LinkEndChild(element_item);
		}
	}
	else
	{
		char szHR[64];
		sprintf_s(szHR, "0X%X(%ld)", hr, hr);
		TiXmlText* pText = new TiXmlText(szHR);
		element_desc->LinkEndChild(pText);
	}

	output_0_0_element->LinkEndChild(element_desc);

	element_desc = new TiXmlElement("GammaControl");

	DXGI_GAMMA_CONTROL GammaControl;
	if (SUCCEEDED(hr = pOutput->GetGammaControl(&GammaControl)))
	{
		char szTmp[128];
		TiXmlElement* element = new TiXmlElement("Scale");
		sprintf_s(szTmp, "RGB(%f, %f, %f)", GammaControl.Scale.Red, GammaControl.Scale.Green, GammaControl.Scale.Blue);
		TiXmlText* pText = new TiXmlText(szTmp);
		element->LinkEndChild(pText);
		element_desc->LinkEndChild(element);

		element = new TiXmlElement("Offset");
		sprintf_s(szTmp, "RGB(%f, %f, %f)", GammaControl.Offset.Red, GammaControl.Offset.Green, GammaControl.Offset.Blue);
		pText = new TiXmlText(szTmp);
		element->LinkEndChild(pText);
		element_desc->LinkEndChild(element);

		element = new TiXmlElement("GammaCurve");
		for (int idxGammaCurve = 0; idxGammaCurve < sizeof(GammaControl.GammaCurve) / sizeof(GammaControl.GammaCurve[0]); idxGammaCurve++)
		{
			TiXmlElement* point = new TiXmlElement(std::to_string(idxGammaCurve).c_str());
			sprintf_s(szTmp, "RGB(%f, %f, %f)", GammaControl.Offset.Red, GammaControl.Offset.Green, GammaControl.Offset.Blue);
			pText = new TiXmlText(szTmp);
			point->LinkEndChild(pText);
			element->LinkEndChild(point);
		}
		element_desc->LinkEndChild(element);
	}
	else
	{
		char szHR[64];
		sprintf_s(szHR, "0X%X(%ld)", hr, hr);
		TiXmlText* pText = new TiXmlText(szHR);
		element_desc->LinkEndChild(pText);
	}

	output_0_0_element->LinkEndChild(element_desc);

	// IDXGIOutput1

	// IDXGIOutput2
	ComPtr<IDXGIOutput2> pOutput2;
	pOutput->QueryInterface(IID_IDXGIOutput2, &pOutput2);

	if (pOutput2.Get() != NULL)
	{
		TiXmlElement* output_2_0_element = new TiXmlElement("Version");
		output_2_0_element->SetAttribute("number", "2.0");
		pOutputElement->LinkEndChild(output_2_0_element);

		element_desc = new TiXmlElement("SupportsOverlays");
		TiXmlText* pText = new TiXmlText(pOutput2->SupportsOverlays() ? "Yes" : "No");
		element_desc->LinkEndChild(pText);

		output_2_0_element->LinkEndChild(element_desc);
	}

	return S_OK;
}

HRESULT CD3DReport::GenerateVideoDeviceReport(ID3D11VideoDevice* pVideoDevice, TiXmlElement* pVDElement)
{
	HRESULT hr = S_OK;
	std::tuple<UINT, UINT> sample_sizes[] = { {720, 480}, {720, 576}, {1280, 720}, {1920, 1080}, {3840, 2160} };
	
	UINT nProfileCount = pVideoDevice->GetVideoDecoderProfileCount();
	TiXmlElement* decoder_profiles = new TiXmlElement("VideoDecoderProfiles");
	pVDElement->LinkEndChild(decoder_profiles);
	decoder_profiles->SetAttribute("Count", nProfileCount);

	std::vector<GUID> vProfiles;

	for (UINT idxProfile = 0; idxProfile < nProfileCount; idxProfile++)
	{
		GUID guidProfile = GUID_NULL;
		if (FAILED(pVideoDevice->GetVideoDecoderProfile(idxProfile, &guidProfile)))
			continue;

		char szProfileIdx[64];
		sprintf_s(szProfileIdx, "Profile%lu", idxProfile);
		TiXmlElement* profile_element = new TiXmlElement(szProfileIdx);

		char szGUIDStr[128];
		sprintf_s(szGUIDStr, "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guidProfile.Data1, guidProfile.Data2, guidProfile.Data3,
			guidProfile.Data4[0], guidProfile.Data4[1], guidProfile.Data4[2], guidProfile.Data4[3],
			guidProfile.Data4[4], guidProfile.Data4[5], guidProfile.Data4[6], guidProfile.Data4[7]);
		profile_element->SetAttribute("GUID", szGUIDStr);
		profile_element->SetAttribute("Name", VIDEO_DECODER_PROFILE_NAME(guidProfile));
		decoder_profiles->LinkEndChild(profile_element);

		vProfiles.push_back(guidProfile);

		D3D11_VIDEO_DECODER_DESC dec_desc;
		dec_desc.Guid = guidProfile;
		for (int idx = 0; idx < sizeof(sample_sizes) / sizeof(sample_sizes[0]); idx++)
		{
			dec_desc.SampleWidth = std::get<0>(sample_sizes[idx]);
			dec_desc.SampleHeight = std::get<1>(sample_sizes[idx]);
			for (int idxFmt = 0; idxFmt < sizeof(g_DXGI_FORMAT_Names) / sizeof(g_DXGI_FORMAT_Names[0]); idxFmt++)
			{
				dec_desc.OutputFormat = (DXGI_FORMAT)idxFmt;

				UINT dec_conf_count = 0;
				if (FAILED(pVideoDevice->GetVideoDecoderConfigCount(&dec_desc, &dec_conf_count)) || dec_conf_count == 0)
					continue;

				TiXmlElement* element_configs = new TiXmlElement("VideoDecoderConfigs");

				element_configs->SetAttribute("SampleWidth", dec_desc.SampleWidth);
				element_configs->SetAttribute("SampleHeight", dec_desc.SampleHeight);
				element_configs->SetAttribute("OutputFormat", DXGI_FORMAT_Name(idxFmt));

				profile_element->LinkEndChild(element_configs);

				D3D11_VIDEO_DECODER_CONFIG dec_config;
				for (UINT idxConfig = 0; idxConfig < dec_conf_count; idxConfig++)
				{
					if (FAILED(pVideoDevice->GetVideoDecoderConfig(&dec_desc, idxConfig, &dec_config)))
						continue;

					TiXmlElement* element_config = new TiXmlElement("Config");
					element_config->SetAttribute("ID", idxConfig);
					element_configs->LinkEndChild(element_config);

					std::ostringstream ss;
					ss << dec_config.guidConfigBitstreamEncryption << '\0';

					TiXmlElement* element_item = new TiXmlElement("guidConfigBitstreamEncryption");
					TiXmlText* pText = new TiXmlText(ss.str().c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("guidConfigMBcontrolEncryption");
					ss.str(""); ss.clear();
					ss << dec_config.guidConfigMBcontrolEncryption << '\0';
					pText = new TiXmlText(ss.str().c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("guidConfigResidDiffEncryption");
					ss.str(""); ss.clear();
					ss << dec_config.guidConfigResidDiffEncryption << '\0';
					pText = new TiXmlText(ss.str().c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("ConfigBitstreamRaw");
					pText = new TiXmlText(std::to_string(dec_config.ConfigBitstreamRaw).c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("ConfigMBcontrolRasterOrder");
					pText = new TiXmlText(std::to_string(dec_config.ConfigMBcontrolRasterOrder).c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("ConfigResidDiffHost");
					pText = new TiXmlText(std::to_string(dec_config.ConfigResidDiffHost).c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("ConfigSpatialResid8");
					pText = new TiXmlText(std::to_string(dec_config.ConfigSpatialResid8).c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("ConfigResid8Subtraction");
					pText = new TiXmlText(std::to_string(dec_config.ConfigResid8Subtraction).c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("ConfigSpatialHost8or9Clipping");
					pText = new TiXmlText(std::to_string(dec_config.ConfigSpatialHost8or9Clipping).c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("ConfigSpatialResidInterleaved");
					pText = new TiXmlText(std::to_string(dec_config.ConfigSpatialResidInterleaved).c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("ConfigIntraResidUnsigned");
					pText = new TiXmlText(std::to_string(dec_config.ConfigIntraResidUnsigned).c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("ConfigResidDiffAccelerator");
					pText = new TiXmlText(std::to_string(dec_config.ConfigResidDiffAccelerator).c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("ConfigHostInverseScan");
					pText = new TiXmlText(std::to_string(dec_config.ConfigHostInverseScan).c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("ConfigSpecificIDCT");
					pText = new TiXmlText(std::to_string(dec_config.ConfigSpecificIDCT).c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("Config4GroupedCoefs");
					pText = new TiXmlText(std::to_string(dec_config.Config4GroupedCoefs).c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("ConfigMinRenderTargetBuffCount");
					pText = new TiXmlText(std::to_string(dec_config.ConfigMinRenderTargetBuffCount).c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);

					element_item = new TiXmlElement("ConfigDecoderSpecific");
					pText = new TiXmlText(std::to_string(dec_config.ConfigDecoderSpecific).c_str());
					element_item->LinkEndChild(pText);
					element_config->LinkEndChild(element_item);
				}
			}
		}

		TiXmlElement* element_cp_caps = new TiXmlElement("ContentProtectionCaps");
		element_cp_caps->SetAttribute("CryptoType", "AES128_CTR");
		profile_element->LinkEndChild(element_cp_caps);

		D3D11_VIDEO_CONTENT_PROTECTION_CAPS cp_caps;
		if (SUCCEEDED(hr = pVideoDevice->GetContentProtectionCaps(&D3D11_CRYPTO_TYPE_AES128_CTR, &guidProfile, &cp_caps)))
		{
			std::stringstream ss;
			TiXmlElement* element_cap = new TiXmlElement("KeyExchangeTypes");
			element_cp_caps->LinkEndChild(element_cap);
			element_cap->SetAttribute("Count", cp_caps.KeyExchangeTypeCount);

			for (UINT idxKey = 0; idxKey < cp_caps.KeyExchangeTypeCount; idxKey++)
			{
				TiXmlElement* element_key = new TiXmlElement("Key");
				element_cap->LinkEndChild(element_key);
				element_key->SetAttribute("ID", idxKey);

				GUID guidKeyExchangeType = GUID_NULL;
				if (SUCCEEDED(hr = pVideoDevice->CheckCryptoKeyExchange(&D3D11_CRYPTO_TYPE_AES128_CTR, &guidProfile, idxKey, &guidKeyExchangeType)))
				{	
					std::stringstream ss;
					ss << guidKeyExchangeType << '\0';
					element_key->SetAttribute("Type", ss.str().c_str());

					TiXmlText* pText = new TiXmlText(guidKeyExchangeType == D3D11_KEY_EXCHANGE_RSAES_OAEP ? "RSAES-OAEP" : "Unknown");
					element_key->LinkEndChild(pText);
				}
				else
				{
					std::stringstream ss;
					ss << "0X" << std::hex << std::setfill('0') << std::setw(8) << hr << '(' << std::dec << hr << ')' << '\0';
					TiXmlText* pText = new TiXmlText(ss.str().c_str());
					element_key->LinkEndChild(pText);
				}
			}

			element_cap = new TiXmlElement("BlockAlignmentSize");
			TiXmlText* pText = new TiXmlText(std::to_string(cp_caps.BlockAlignmentSize).c_str());
			element_cap->LinkEndChild(pText);
			element_cp_caps->LinkEndChild(element_cap);

			element_cap = new TiXmlElement("BlockAlignmentSize");
			ss.str(""); ss.clear();
			ss << std::fixed << std::setprecision(2) << (cp_caps.ProtectedMemorySize / 1024.f / 1024.f) << " MB (" << cp_caps.ProtectedMemorySize << " bytes)" << '\0';
			pText = new TiXmlText(ss.str().c_str());
			element_cap->LinkEndChild(pText);
			element_cp_caps->LinkEndChild(element_cap);

			std::tuple<UINT, std::string, std::string, std::string> caps_names[15] = {
				{ D3D11_CONTENT_PROTECTION_CAPS_SOFTWARE, "CONTENT_PROTECTION_CAPS_SOFTWARE", "The content protection is implemented in software by the driver", "The content protection is NOT implemented in software by the driver"},
				{ D3D11_CONTENT_PROTECTION_CAPS_HARDWARE, "CONTENT_PROTECTION_CAPS_HARDWARE", "The content protection is implemented in hardware by the GPU", "The content protection is NOT implemented in hardware by the GPU"},
				{ D3D11_CONTENT_PROTECTION_CAPS_PROTECTION_ALWAYS_ON, "CONTENT_PROTECTION_CAPS_PROTECTION_ALWAYS_ON", "Content protection is always applied to a protected surface, regardless of whether the application explicitly enables protection", "Content protection may NOT applied to a protected surface"},
				{ D3D11_CONTENT_PROTECTION_CAPS_PARTIAL_DECRYPTION, "CONTENT_PROTECTION_CAPS_PARTIAL_DECRYPTION", "The driver can use partially encrypted buffers", "the entire buffer must be either encrypted or clear" },
				{ D3D11_CONTENT_PROTECTION_CAPS_CONTENT_KEY, "CONTENT_PROTECTION_CAPS_CONTENT_KEY", "The driver can encrypt data using a separate content key that is encrypted using the session key", "The driver can NOT encrypt data using a separate content key that is encrypted using the session key" },
				{ D3D11_CONTENT_PROTECTION_CAPS_FRESHEN_SESSION_KEY, "CONTENT_PROTECTION_CAPS_FRESHEN_SESSION_KEY", "The driver can refresh the session key without renegotiating the key", "The driver can NOT refresh the session key without renegotiating the key" },
				{ D3D11_CONTENT_PROTECTION_CAPS_ENCRYPTED_READ_BACK, "CONTENT_PROTECTION_CAPS_ENCRYPTED_READ_BACK", "The driver can read back encrypted data from a protected surface", "The driver can NOT read back encrypted data from a protected surface." },
				{ D3D11_CONTENT_PROTECTION_CAPS_ENCRYPTED_READ_BACK_KEY, "CONTENT_PROTECTION_CAPS_ENCRYPTED_READ_BACK_KEY", "The driver requires a separate key to read encrypted data from a protected surface", "The driver does NOT require a separate key to read encrypted data from a protected surface" },
				{ D3D11_CONTENT_PROTECTION_CAPS_SEQUENTIAL_CTR_IV, "CONTENT_PROTECTION_CAPS_SEQUENTIAL_CTR_IV", "the application must use a sequential count in the D3D11_AES_CTR_IV structure", "" },
				{ D3D11_CONTENT_PROTECTION_CAPS_ENCRYPT_SLICEDATA_ONLY, "CONTENT_PROTECTION_CAPS_ENCRYPT_SLICEDATA_ONLY", "The driver supports encrypted slice data, but does not support any other encrypted data in the compressed buffer", "The driver does NOT support only encrypted slice data" },
				{ D3D11_CONTENT_PROTECTION_CAPS_DECRYPTION_BLT, "CONTENT_PROTECTION_CAPS_DECRYPTION_BLT", "The driver can copy encrypted data from one resource to another, decrypting the data as part of the process", "The driver can NOT copy encrypted data from one resource to another" },
				{ D3D11_CONTENT_PROTECTION_CAPS_HARDWARE_PROTECT_UNCOMPRESSED, "CONTENT_PROTECTION_CAPS_HARDWARE_PROTECT_UNCOMPRESSED", "The hardware supports the protection of specific resources", "The hardware does NOT support the protection of specific resources" },
				{ D3D11_CONTENT_PROTECTION_CAPS_HARDWARE_PROTECTED_MEMORY_PAGEABLE, "CONTENT_PROTECTION_CAPS_HARDWARE_PROTECTED_MEMORY_PAGEABLE", "Physical pages of a protected resource can be evicted and potentially paged to disk in low memory conditions without losing the contents of the resource when paged back in", "Physical pages of a protected resource can NOT be evicted and potentially paged to disk in low memory conditions without losing the contents of the resource when paged back in" },
				{ D3D11_CONTENT_PROTECTION_CAPS_HARDWARE_TEARDOWN, "CONTENT_PROTECTION_CAPS_HARDWARE_TEARDOWN", "The hardware supports an automatic teardown mechanism that could trigger hardware keys or protected content to become lost in some conditions", "The hardware does NOT support an automatic teardown mechanism that could trigger hardware keys or protected content to become lost in some conditions" },
				{ D3D11_CONTENT_PROTECTION_CAPS_HARDWARE_DRM_COMMUNICATION, "CONTENT_PROTECTION_CAPS_HARDWARE_DRM_COMMUNICATION", "The secure environment is tightly coupled with the GPU and an ID3D11CryptoSession should be used for communication between the user mode DRM component and the secure execution environment", "The secure environment is NOT tightly coupled with the GPU and an ID3D11CryptoSession should be used for communication between the user mode DRM component and the secure execution environment" }
			};

			for (int idxCap = 0; idxCap < sizeof(caps_names) / sizeof(caps_names[0]); idxCap++)
			{
				element_cap = new TiXmlElement(std::get<1>(caps_names[idxCap]).c_str());
				element_cp_caps->LinkEndChild(element_cap);
				element_cap->SetAttribute("Support", (cp_caps.Caps&std::get<0>(caps_names[idxCap]))?"Yes":"No");
				pText = new TiXmlText((cp_caps.Caps&std::get<0>(caps_names[idxCap])) ? std::get<2>(caps_names[idxCap]).c_str() : std::get<3>(caps_names[idxCap]).c_str());
				element_cap->LinkEndChild(pText);
			}
		}
		else
		{
			std::stringstream ss;
			ss << "0X" << std::hex << std::setfill('0') << std::setw(8) << hr << '(' << std::dec << hr << ')' << '\0';
			TiXmlText* pText = new TiXmlText(ss.str().c_str());
			element_cp_caps->LinkEndChild(pText);
		}

		TiXmlElement* element_fmts = new TiXmlElement("SupportFormats");
		profile_element->LinkEndChild(element_fmts);

		for (int idxFmt = 1; idxFmt < sizeof(g_DXGI_FORMAT_Names) / sizeof(g_DXGI_FORMAT_Names[0]); idxFmt++)
		{
			BOOL bSupported = FALSE;
			if (SUCCEEDED(hr = pVideoDevice->CheckVideoDecoderFormat(&guidProfile, (DXGI_FORMAT)idxFmt, &bSupported)) && bSupported)
			{
				TiXmlElement* element_fmt = new TiXmlElement("Format");
				element_fmt->LinkEndChild(new TiXmlText(DXGI_FORMAT_Name(idxFmt)));
				element_fmts->LinkEndChild(element_fmt);
			}
		}
	}

	for (int idxAuthType = D3D11_AUTHENTICATED_CHANNEL_D3D11; idxAuthType <= D3D11_AUTHENTICATED_CHANNEL_DRIVER_HARDWARE; idxAuthType++)
	{
		TiXmlElement* element_auth_type = new TiXmlElement(D3D11_AUTHENTICATED_CHANNEL_TYPE_Name(idxAuthType));
		pVDElement->LinkEndChild(element_auth_type);

		ComPtr<ID3D11AuthenticatedChannel> pAuthenticatedChannel;
		if (SUCCEEDED(hr = pVideoDevice->CreateAuthenticatedChannel((D3D11_AUTHENTICATED_CHANNEL_TYPE)idxAuthType, &pAuthenticatedChannel)))
		{
			TiXmlElement* element_cert = new TiXmlElement("Certificate");
			element_auth_type->LinkEndChild(element_cert);

			UINT cbCertSize = 0;
			if (SUCCEEDED(hr = pAuthenticatedChannel->GetCertificateSize(&cbCertSize)))
			{
				element_cert->SetAttribute("Size", cbCertSize);

				BYTE* pbCertData = new BYTE[cbCertSize];
				if (SUCCEEDED(hr = pAuthenticatedChannel->GetCertificate(cbCertSize, pbCertData)))
				{
					std::ostringstream oss;
					UINT cbLeft = cbCertSize;
					oss << '\n';
					for (UINT idx = 0; idx < (cbCertSize + 15) / 16; idx++)
					{
						UINT cbLeftCol = min(16, cbLeft);
						for (UINT idxCol = 0; idxCol < cbLeftCol; idxCol++)
							oss << std::hex << std::setw(2) << std::setfill('0') << (UINT)pbCertData[idx * 16 + idxCol] << ' ';
						oss << std::setfill(' ') << std::setw(cbLeft >= 16 ? 3 : ((16 - cbLeft) * 3 + 3)) << ' ';
						for (UINT idxCol = 0; idxCol < cbLeftCol; idxCol++, cbLeft--)
							oss << (char)(isprint(pbCertData[idx * 16 + idxCol]) ? pbCertData[idx * 16 + idxCol] : '.');
						oss << '\n';
					}
					oss << std::ends;

					TiXmlText* pText = new TiXmlText(oss.str().c_str());
					element_cert->LinkEndChild(pText);

					pText->SetCDATA(true);

#if 0
					FILE* fp = NULL;
					fopen_s(&fp, "C:\\Users\\Ravin\\Desktop\\authchannel.cer", "wb+");
					if (fp != NULL)
					{
						fwrite(pbCertData, 1, cbCertSize, fp);
						fclose(fp);
					}
#endif
				}
				else
				{
					std::stringstream ss;
					ss << "0X" << std::hex << std::setfill('0') << std::setw(8) << hr << '(' << std::dec << hr << ')' << '\0';
					element_cert->LinkEndChild(new TiXmlText(ss.str().c_str()));
				}
				delete[] pbCertData;
			}
			else
			{
				std::stringstream ss;
				ss << "0X" << std::hex << std::setfill('0') << std::setw(8) << hr << '(' << std::dec << hr << ')' << '\0';
				element_cert->LinkEndChild(new TiXmlText(ss.str().c_str()));
			}

			HANDLE hChannel = INVALID_HANDLE_VALUE;
			pAuthenticatedChannel->GetChannelHandle(&hChannel);
			TiXmlElement* element_channel_handle = new TiXmlElement("ChannelHandle");
			element_auth_type->LinkEndChild(element_channel_handle);
			std::ostringstream oss;
			oss << hChannel << '\0';
			element_channel_handle->LinkEndChild(new TiXmlText(oss.str().c_str()));
		}
		else
		{
			std::stringstream ss;
			ss << "0X" << std::hex << std::setfill('0') << std::setw(8) << hr << '(' << std::dec << hr << ')' << '\0';
			element_auth_type->LinkEndChild(new TiXmlText(ss.str().c_str()));
		}
	}

	TiXmlElement* element_crypto_sessions = new TiXmlElement("CryptoSessions");
	pVDElement->LinkEndChild(element_crypto_sessions);
	element_crypto_sessions->SetAttribute("Crypto_Type", "AES128_CTR");
	element_crypto_sessions->SetAttribute("Exchange_Type", "RSAES-OAEP");

	for (int idxProfile = -1; idxProfile < (int)vProfiles.size(); idxProfile++)
	{
		TiXmlElement* element_crypto_session = new TiXmlElement("CryptoSession");
		element_crypto_sessions->LinkEndChild(element_crypto_session);
		element_crypto_session->SetAttribute("DecoderProfile", idxProfile==-1?"NULL":VIDEO_DECODER_PROFILE_NAME(vProfiles[idxProfile]));

		ComPtr<ID3D11CryptoSession> spCryptoSession;
		hr = pVideoDevice->CreateCryptoSession(&D3D11_CRYPTO_TYPE_AES128_CTR, idxProfile==-1?NULL:&vProfiles[idxProfile], &D3D11_KEY_EXCHANGE_RSAES_OAEP, &spCryptoSession);
		if (SUCCEEDED(hr))
		{
			TiXmlElement* element_cert = new TiXmlElement("Certificate");
			element_crypto_session->LinkEndChild(element_cert);

			UINT cbCertSize = 0;
			if (SUCCEEDED(hr = spCryptoSession->GetCertificateSize(&cbCertSize)))
			{
				element_cert->SetAttribute("Size", cbCertSize);

				BYTE* pbCertData = new BYTE[cbCertSize];
				if (SUCCEEDED(hr = spCryptoSession->GetCertificate(cbCertSize, pbCertData)))
				{
					std::ostringstream oss;
					UINT cbLeft = cbCertSize;
					oss << '\n';
					for (UINT idx = 0; idx < (cbCertSize + 15) / 16; idx++)
					{
						UINT cbLeftCol = min(16, cbLeft);
						for (UINT idxCol = 0; idxCol < cbLeftCol; idxCol++)
							oss << std::hex << std::setw(2) << std::setfill('0') << (UINT)pbCertData[idx * 16 + idxCol] << ' ';
						oss << std::setfill(' ') << std::setw(cbLeft >= 16 ? 3 : ((16 - cbLeft) * 3 + 3)) << ' ';
						for (UINT idxCol = 0; idxCol < cbLeftCol; idxCol++, cbLeft--)
							oss << (char)(isprint(pbCertData[idx * 16 + idxCol]) ? pbCertData[idx * 16 + idxCol] : '.');
						oss << '\n';
					}
					oss << std::ends;

					TiXmlText* pText = new TiXmlText(oss.str().c_str());
					element_cert->LinkEndChild(pText);

					pText->SetCDATA(true);

#if 0
					FILE* fp = NULL;
					fopen_s(&fp, "C:\\Users\\Ravin\\Desktop\\cryptosession.cer", "wb+");
					if (fp != NULL)
					{
						fwrite(pbCertData, 1, cbCertSize, fp);
						fclose(fp);
					}
#endif
				}
				else
				{
					std::stringstream ss;
					ss << "0X" << std::hex << std::setfill('0') << std::setw(8) << hr << '(' << std::dec << hr << ')' << '\0';
					element_cert->LinkEndChild(new TiXmlText(ss.str().c_str()));
				}
				delete[] pbCertData;
			}
			else
			{
				std::stringstream ss;
				ss << "0X" << std::hex << std::setfill('0') << std::setw(8) << hr << '(' << std::dec << hr << ')' << '\0';
				element_cert->LinkEndChild(new TiXmlText(ss.str().c_str()));
			}
		}
		else
		{
			std::stringstream ss;
			ss << "0X" << std::hex << std::setfill('0') << std::setw(8) << hr << '(' << std::dec << hr << ')' << '\0';
			element_crypto_session->LinkEndChild(new TiXmlText(ss.str().c_str()));
		}
	}

	TiXmlElement* element_VP_enums = new TiXmlElement("VideoProcessorEnumerators");
	pVDElement->LinkEndChild(element_VP_enums);

	D3D11_VIDEO_PROCESSOR_CONTENT_DESC desc;
	for (int idxFrameFmt = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE; idxFrameFmt <= D3D11_VIDEO_FRAME_FORMAT_INTERLACED_BOTTOM_FIELD_FIRST; idxFrameFmt++)
	{
		desc.InputFrameFormat = (D3D11_VIDEO_FRAME_FORMAT)idxFrameFmt;

		for (int idxInputVFmt = 0; idxInputVFmt < sizeof(video_fmts) / sizeof(video_fmts[0]); idxInputVFmt++)
		{
			desc.InputFrameRate = std::get<0>(video_fmts[idxInputVFmt]);
			desc.InputWidth = std::get<1>(video_fmts[idxInputVFmt]);
			desc.InputHeight = std::get<2>(video_fmts[idxInputVFmt]);

			for (int idxOutputFmt = 0; idxOutputFmt < sizeof(video_fmts) / sizeof(video_fmts[0]); idxOutputFmt++)
			{
				desc.OutputFrameRate = std::get<0>(video_fmts[idxOutputFmt]);
				desc.OutputWidth = std::get<1>(video_fmts[idxOutputFmt]);
				desc.OutputHeight = std::get<2>(video_fmts[idxOutputFmt]);

				TiXmlElement* prev_element_vp_enum = NULL;
				D3D11_VIDEO_PROCESSOR_CAPS prev_vp_caps;
				memset(&prev_vp_caps, 0xFF, sizeof(prev_vp_caps));
				for (int usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL; usage <= D3D11_VIDEO_USAGE_OPTIMAL_QUALITY; usage++)
				{
					desc.Usage = (D3D11_VIDEO_USAGE)usage;

					ComPtr<ID3D11VideoProcessorEnumerator> spVPEnum;
					if (FAILED(hr = pVideoDevice->CreateVideoProcessorEnumerator(&desc, &spVPEnum)))
						continue;

					TiXmlElement* element_vp_enum = new TiXmlElement("VideoProcessorEnumerator");
					element_vp_enum->SetAttribute("InputFrameFormat", D3D11_VIDEO_FRAME_FORMAT_Name(idxFrameFmt));

					std::ostringstream oss;
					oss << std::fixed << std::setprecision(2) << std::get<0>(video_fmts[idxInputVFmt]).Numerator*1.0f / std::get<0>(video_fmts[idxInputVFmt]).Denominator;
					oss << '(' << std::get<0>(video_fmts[idxInputVFmt]).Numerator << '/' << std::get<0>(video_fmts[idxInputVFmt]).Denominator << ')' << std::ends;
					element_vp_enum->SetAttribute("InputFrameRate", oss.str().c_str());
					element_vp_enum->SetAttribute("InputWidth", std::get<1>(video_fmts[idxInputVFmt]));
					element_vp_enum->SetAttribute("InputHeight", std::get<2>(video_fmts[idxInputVFmt]));

					oss.str("");  oss.clear();
					oss << std::fixed << std::setprecision(2) << std::get<0>(video_fmts[idxOutputFmt]).Numerator*1.0f / std::get<0>(video_fmts[idxOutputFmt]).Denominator;
					oss << '(' << std::get<0>(video_fmts[idxOutputFmt]).Numerator << '/' << std::get<0>(video_fmts[idxOutputFmt]).Denominator << ')' << std::ends;
					element_vp_enum->SetAttribute("OutputFrameRate", oss.str().c_str());
					element_vp_enum->SetAttribute("OutputWidth", std::get<1>(video_fmts[idxOutputFmt]));
					element_vp_enum->SetAttribute("OutputHeight", std::get<2>(video_fmts[idxOutputFmt]));
					
					element_vp_enum->SetAttribute("Usage", D3D11_VIDEO_USAGE_Name(usage));

					D3D11_VIDEO_PROCESSOR_CAPS vp_caps;
					if (SUCCEEDED(hr = spVPEnum->GetVideoProcessorCaps(&vp_caps)))
					{
						if (memcmp(&prev_vp_caps, &vp_caps, sizeof(vp_caps)) == 0)
						{
							if (prev_element_vp_enum != NULL)
							{
								std::string strPrevAttr = prev_element_vp_enum->Attribute("Usage");
								strPrevAttr += ", ";
								strPrevAttr += D3D11_VIDEO_USAGE_Name(usage);
								prev_element_vp_enum->SetAttribute("Usage", strPrevAttr.c_str());
							}
							delete element_vp_enum;
							continue;
						}

						prev_vp_caps = vp_caps;

						std::tuple<UINT, const char*, std::tuple<UINT, const char*>*, UINT> v_vpcaps[] = {
							{ vp_caps.DeviceCaps, "DeviceCaps", device_cap_names, (UINT)_countof(device_cap_names) },
							{ vp_caps.FeatureCaps, "FeatureCaps", feature_cap_names, (UINT)_countof(feature_cap_names) },
							{ vp_caps.FilterCaps, "FilterCaps", filter_cap_names, (UINT)_countof(filter_cap_names) },
							{ vp_caps.InputFormatCaps, "InputFormatCaps", inputformat_cap_names, (UINT)_countof(inputformat_cap_names) },
							{ vp_caps.AutoStreamCaps, "AutoStreamCaps", autostream_cap_names, (UINT)_countof(autostream_cap_names) },
							{ vp_caps.StereoCaps, "StereoCaps", stereo_cap_names, (UINT)_countof(stereo_cap_names) }
						};

						for (int idxCapSet = 0; idxCapSet < _countof(v_vpcaps); idxCapSet++)
						{
							TiXmlElement* element_caps = new TiXmlElement(std::get<1>(v_vpcaps[idxCapSet]));
							oss.str(""); oss.clear(); oss << "0X" << std::hex << std::uppercase << std::get<0>(v_vpcaps[idxCapSet]) << std::ends;
							element_caps->SetAttribute("Value", oss.str().c_str());
							element_vp_enum->LinkEndChild(element_caps);

							oss.str(""); oss.clear();
							for (UINT idxCap = 0; idxCap <= std::get<3>(v_vpcaps[idxCapSet]); idxCap++)
							{
								bool bFirst = true;
								if (std::get<0>(v_vpcaps[idxCapSet])&(std::get<0>((std::get<2>(v_vpcaps[idxCapSet]))[idxCap])))
								{
									if (!bFirst)
										oss << ", ";
									else
										bFirst = false;
									oss << std::get<1>((std::get<2>(v_vpcaps[idxCapSet]))[idxCap]);
								}
							} // for (UINT idxCap = 0; idxCap <= std::get<3>(v_vpcaps[idxCapSet]); idxCap++)
							oss << std::ends;

							element_caps->LinkEndChild(new TiXmlText(oss.str().c_str()));

						} // for (int idxCapSet = 0; idxCapSet < _countof(v_vpcaps); idxCapSet++)

						TiXmlElement* element_RateConversionCapsCount = new TiXmlElement("RateConversionCapsCount");
						element_vp_enum->LinkEndChild(element_RateConversionCapsCount);
						element_RateConversionCapsCount->SetAttribute("Value", vp_caps.RateConversionCapsCount);

						D3D11_VIDEO_PROCESSOR_RATE_CONVERSION_CAPS rateconv_caps;
						for (UINT idxRateConv = 0; idxRateConv < vp_caps.RateConversionCapsCount; idxRateConv++)
						{
							if (FAILED(spVPEnum->GetVideoProcessorRateConversionCaps(idxRateConv, &rateconv_caps)))
								continue;

							static const std::tuple<UINT, const char*> D3D11_VIDEO_PROCESSOR_PROCESSOR_CAP_names[] = {
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_DEINTERLACE_BLEND),
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_DEINTERLACE_BOB),
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_DEINTERLACE_ADAPTIVE),
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_DEINTERLACE_MOTION_COMPENSATION),
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_INVERSE_TELECINE),
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_FRAME_RATE_CONVERSION),
							};

							static const std::tuple<UINT, const char*> D3D11_VIDEO_PROCESSOR_ITELECINE_CAPS_names[] = {
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_ITELECINE_CAPS_32),
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_ITELECINE_CAPS_22),
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_ITELECINE_CAPS_2224),
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_ITELECINE_CAPS_2332),
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_ITELECINE_CAPS_32322),
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_ITELECINE_CAPS_55),
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_ITELECINE_CAPS_64),
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_ITELECINE_CAPS_87),
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_ITELECINE_CAPS_222222222223),
								DECL_TUPLE(D3D11_VIDEO_PROCESSOR_ITELECINE_CAPS_OTHER),
							};

							std::ostringstream oss;

							TiXmlElement* element_rateconversion_cap = new TiXmlElement("RateConversionCap");
							element_RateConversionCapsCount->LinkEndChild(element_rateconversion_cap);

							element_rateconversion_cap->SetAttribute("PastFrames", rateconv_caps.PastFrames);
							element_rateconversion_cap->SetAttribute("FutureFrames", rateconv_caps.FutureFrames);
							oss << "(0X" << std::hex << rateconv_caps.ProcessorCaps << ")(" 
								<< GetBitsString<UINT>(D3D11_VIDEO_PROCESSOR_PROCESSOR_CAP_names, _countof(D3D11_VIDEO_PROCESSOR_PROCESSOR_CAP_names), rateconv_caps.ProcessorCaps) << ")" << std::ends;
							element_rateconversion_cap->SetAttribute("ProcessorCaps", oss.str().c_str());

							oss.str(""); oss.clear();
							oss << "(0X" << std::hex << rateconv_caps.ProcessorCaps << ")("
								<< GetBitsString<UINT>(D3D11_VIDEO_PROCESSOR_ITELECINE_CAPS_names, _countof(D3D11_VIDEO_PROCESSOR_ITELECINE_CAPS_names), rateconv_caps.ITelecineCaps) << ")" << std::ends;
							element_rateconversion_cap->SetAttribute("ITelecineCaps", oss.str().c_str());
							element_rateconversion_cap->SetAttribute("CustomRateCount", rateconv_caps.CustomRateCount);

							D3D11_VIDEO_PROCESSOR_CUSTOM_RATE custom_rate;
							for (UINT idxCustomRC = 0; idxCustomRC < rateconv_caps.CustomRateCount; idxCustomRC++)
							{
								if (FAILED(spVPEnum->GetVideoProcessorCustomRate(idxRateConv, idxCustomRC, &custom_rate)))
									continue;

								TiXmlElement* element_custom_rate = new TiXmlElement("Custom_Rate");
								element_rateconversion_cap->LinkEndChild(element_custom_rate);

								std::ostringstream oss_rate;
								oss_rate << std::fixed << std::setprecision(2) << (float)custom_rate.CustomRate.Numerator / custom_rate.CustomRate.Denominator
									<< "(" << std::dec << custom_rate.CustomRate.Numerator << "/" << custom_rate.CustomRate.Denominator << ")" << std::ends;

								element_custom_rate->SetAttribute("CustomRate", oss_rate.str().c_str());
								element_custom_rate->SetAttribute("OutputFrames", custom_rate.OutputFrames);
								element_custom_rate->SetAttribute("InputInterlaced", custom_rate.InputInterlaced?"Yes":"No");
								element_custom_rate->SetAttribute("InputFramesOrFields", custom_rate.InputFramesOrFields);
							}
						}

						TiXmlElement* element_MaxInputStreams = new TiXmlElement("MaxInputStreams");
						element_vp_enum->LinkEndChild(element_MaxInputStreams);
						element_MaxInputStreams->SetAttribute("Value", vp_caps.MaxInputStreams);

						TiXmlElement* element_MaxStreamStates = new TiXmlElement("MaxStreamStates");
						element_vp_enum->LinkEndChild(element_MaxStreamStates);
						element_MaxStreamStates->SetAttribute("Value", vp_caps.MaxStreamStates);
					}
					else
					{
						memset(&prev_vp_caps, 0xFF, sizeof(prev_vp_caps));
						std::stringstream ss;
						ss << "0X" << std::hex << std::setfill('0') << std::setw(8) << hr << '(' << std::dec << hr << ')' << '\0';
						element_vp_enum->LinkEndChild(new TiXmlText(ss.str().c_str()));
					}

					TiXmlElement* element_filter_ranges = new TiXmlElement("FilterRanges");
					element_filter_ranges->SetAttribute("FilterCount", D3D11_VIDEO_PROCESSOR_FILTER_STEREO_ADJUSTMENT - D3D11_VIDEO_PROCESSOR_FILTER_BRIGHTNESS + 1);
					element_vp_enum->LinkEndChild(element_filter_ranges);

					D3D11_VIDEO_PROCESSOR_FILTER_RANGE filter_range;
					for (UINT idxFilter = D3D11_VIDEO_PROCESSOR_FILTER_BRIGHTNESS; idxFilter <= D3D11_VIDEO_PROCESSOR_FILTER_STEREO_ADJUSTMENT; idxFilter++)
					{
						TiXmlElement* element_filter_range = new TiXmlElement(D3D11_VIDEO_PROCESSOR_FILTER_Name(idxFilter));
						element_filter_ranges->LinkEndChild(element_filter_range);

						if (SUCCEEDED(hr = spVPEnum->GetVideoProcessorFilterRange((D3D11_VIDEO_PROCESSOR_FILTER)idxFilter, &filter_range)))
						{
							element_filter_range->SetAttribute("Minimum", filter_range.Minimum);
							element_filter_range->SetAttribute("Maximum", filter_range.Maximum);
							element_filter_range->SetAttribute("Default", filter_range.Default);
							std::ostringstream oss_mul;
							oss_mul << std::fixed << filter_range.Multiplier << std::ends;
							element_filter_range->SetAttribute("Multiplier", oss_mul.str().c_str());
						}
						else
						{
							std::stringstream ss;
							ss << "0X" << std::hex << std::setfill('0') << std::setw(8) << hr << '(' << std::dec << hr << ')' << '\0';
							element_filter_range->LinkEndChild(new TiXmlText(ss.str().c_str()));
						}
					}

					element_VP_enums->LinkEndChild(element_vp_enum);
					prev_element_vp_enum = element_vp_enum;
				} // for (int usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL; usage <= D3D11_VIDEO_USAGE_OPTIMAL_QUALITY; usage++)
			} // for (int idxOutputFmt = 0; idxOutputFmt < sizeof(video_fmts) / sizeof(video_fmts[0]); idxOutputFmt++)
		} // for (int idxInputVFmt = 0; idxInputVFmt < sizeof(video_fmts) / sizeof(video_fmts[0]); idxInputVFmt++)
	}

	return S_OK;
}
