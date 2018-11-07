// D3DTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "tinyxml\tinyxml.h"
#include <Windows.h>
#include "D3DReport.h"
#include <wrl/client.h>
#include <tuple>

using namespace Microsoft::WRL;

#define VDEC_PROFILE_NAME(x)	\
	(x == D3D11_DECODER_PROFILE_MPEG2_MOCOMP?"PROFILE_MPEG2_MOCOMP":(\
	(x == D3D11_DECODER_PROFILE_MPEG2_IDCT?"PROFILE_MPEG2_IDCT":(\
	(x == D3D11_DECODER_PROFILE_MPEG2_VLD?"PROFILE_MPEG2_VLD":(\
	(x == D3D11_DECODER_PROFILE_MPEG1_VLD?"PROFILE_MPEG1_VLD":(\
	(x == D3D11_DECODER_PROFILE_MPEG2and1_VLD?"PROFILE_MPEG2and1_VLD":(\
	(x == D3D11_DECODER_PROFILE_H264_MOCOMP_NOFGT?"PROFILE_H264_MOCOMP_NOFGT":(\
	(x == D3D11_DECODER_PROFILE_H264_MOCOMP_FGT?"PROFILE_H264_MOCOMP_FGT":(\
	(x == D3D11_DECODER_PROFILE_H264_IDCT_NOFGT?"PROFILE_H264_IDCT_NOFGT":(\
	(x == D3D11_DECODER_PROFILE_H264_IDCT_FGT?"PROFILE_H264_IDCT_FGT":(\
	(x == D3D11_DECODER_PROFILE_H264_VLD_NOFGT?"PROFILE_H264_VLD_NOFGT":(\
	(x == D3D11_DECODER_PROFILE_H264_VLD_FGT?"PROFILE_H264_VLD_FGT":(\
	(x == D3D11_DECODER_PROFILE_H264_VLD_WITHFMOASO_NOFGT?"PROFILE_H264_VLD_WITHFMOASO_NOFGT":(\
	(x == D3D11_DECODER_PROFILE_H264_VLD_STEREO_PROGRESSIVE_NOFGT?"PROFILE_H264_VLD_STEREO_PROGRESSIVE_NOFGT":(\
	(x == D3D11_DECODER_PROFILE_H264_VLD_STEREO_NOFGT?"PROFILE_H264_VLD_STEREO_NOFGT":(\
	(x == D3D11_DECODER_PROFILE_H264_VLD_MULTIVIEW_NOFGT?"PROFILE_H264_VLD_MULTIVIEW_NOFGT":(\
	(x == D3D11_DECODER_PROFILE_WMV8_POSTPROC?"PROFILE_WMV8_POSTPROC":(\
	(x == D3D11_DECODER_PROFILE_WMV8_MOCOMP?"PROFILE_WMV8_MOCOMP":(\
	(x == D3D11_DECODER_PROFILE_WMV9_POSTPROC?"PROFILE_WMV9_POSTPROC":(\
	(x == D3D11_DECODER_PROFILE_WMV9_MOCOMP?"PROFILE_WMV9_MOCOMP":(\
	(x == D3D11_DECODER_PROFILE_WMV9_IDCT?"PROFILE_WMV9_IDCT":(\
	(x == D3D11_DECODER_PROFILE_VC1_POSTPROC?"PROFILE_VC1_POSTPROC":(\
	(x == D3D11_DECODER_PROFILE_VC1_MOCOMP?"PROFILE_VC1_MOCOMP":(\
	(x == D3D11_DECODER_PROFILE_VC1_IDCT?"PROFILE_VC1_IDCT":(\
	(x == D3D11_DECODER_PROFILE_VC1_VLD?"PROFILE_VC1_VLD":(\
	(x == D3D11_DECODER_PROFILE_VC1_D2010?"PROFILE_VC1_D2010":(\
	(x == D3D11_DECODER_PROFILE_MPEG4PT2_VLD_SIMPLE?"PROFILE_MPEG4PT2_VLD_SIMPLE":(\
	(x == D3D11_DECODER_PROFILE_MPEG4PT2_VLD_ADVSIMPLE_NOGMC?"PROFILE_MPEG4PT2_VLD_ADVSIMPLE_NOGMC":(\
	(x == D3D11_DECODER_PROFILE_MPEG4PT2_VLD_ADVSIMPLE_GMC?"PROFILE_MPEG4PT2_VLD_ADVSIMPLE_GMC":(\
	(x == D3D11_DECODER_PROFILE_HEVC_VLD_MAIN?"PROFILE_HEVC_VLD_MAIN":(\
	(x == D3D11_DECODER_PROFILE_HEVC_VLD_MAIN10?"PROFILE_HEVC_VLD_MAIN10":(\
	(x == D3D11_DECODER_PROFILE_VP9_VLD_PROFILE0?"PROFILE_VP9_VLD_PROFILE0":(\
	(x == D3D11_DECODER_PROFILE_VP9_VLD_10BIT_PROFILE2?"PROFILE_VP9_VLD_10BIT_PROFILE2":(\
	(x == D3D11_DECODER_PROFILE_VP8_VLD?"PROFILE_VP8_VLD":"Unknown")))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))

void PrintBuf(TCHAR* buf, int buf_size, int nLeadingSpace)
{
	TCHAR szTmp[128];
	int nLeftChars = buf_size;
	int nLines = nLeftChars / 16 + (nLeftChars % 16 == 0 ? 0 : 1);

	if (nLeadingSpace > buf_size)
		return;

	for (int i = 0; i < nLeadingSpace; i++)
		szTmp[i] = _T(' ');

	for (int i = 0; i < nLines; i++)
	{
		int nWritten = nLeadingSpace;
		nWritten = _stprintf_s(szTmp, _countof(szTmp), _T("%26s"), _T(" "));
		for (int j = 0; j < (nLeftChars < 16 ? nLeftChars : 16); j++)
			nWritten += _stprintf_s(szTmp + nWritten, _countof(szTmp) - nWritten, _T("%02X "), buf[i * 16 + j]);

		nWritten += _stprintf_s(szTmp + nWritten, _countof(szTmp) - nWritten, _T("- "));

		for (int j = 0; j < (nLeftChars < 16 ? nLeftChars : 16); j++)
			nWritten += _stprintf_s(szTmp + nWritten, _countof(szTmp) - nWritten, _T("%c"),
				_istprint(buf[i * 16 + j]) ? buf[i * 16 + j] : _T('.'));

		nWritten += _stprintf_s(szTmp + nWritten, _countof(szTmp) - nWritten, _T("\n"));

		if (nLeftChars < 16)
			nLeftChars = 0;
		else
			nLeftChars -= 16;

		_tprintf(szTmp);
	}
	_tprintf(_T("\n"));
}

using FLAG_NAME_PAIR = const std::tuple<UINT, const TCHAR*>;

static FLAG_NAME_PAIR Device_state_flag_names[] = {
	DECL_TUPLE_T(DISPLAY_DEVICE_ATTACHED_TO_DESKTOP),
	DECL_TUPLE_T(DISPLAY_DEVICE_MULTI_DRIVER),
	DECL_TUPLE_T(DISPLAY_DEVICE_PRIMARY_DEVICE),
	DECL_TUPLE_T(DISPLAY_DEVICE_MIRRORING_DRIVER),
	DECL_TUPLE_T(DISPLAY_DEVICE_VGA_COMPATIBLE),
	DECL_TUPLE_T(DISPLAY_DEVICE_REMOVABLE),
	DECL_TUPLE_T(DISPLAY_DEVICE_ACC_DRIVER),
	DECL_TUPLE_T(DISPLAY_DEVICE_RDPUDD),
	DECL_TUPLE_T(DISPLAY_DEVICE_DISCONNECT),
	DECL_TUPLE_T(DISPLAY_DEVICE_REMOTE),
	DECL_TUPLE_T(DISPLAY_DEVICE_MODESPRUNED),
	DECL_TUPLE_T(DISPLAY_DEVICE_TS_COMPATIBLE),
	DECL_TUPLE_T(DISPLAY_DEVICE_UNSAFE_MODES_ON),
};

void GetFlagsDesc(UINT nFlags, FLAG_NAME_PAIR* flag_names, size_t flag_count, TCHAR* szDesc, int ccDesc)
{
	bool bFirst = true;
	int ccWritten = 0;
	memset(szDesc, 0, ccDesc);
	for (size_t i = 0; i < flag_count; i++)
	{
		if (nFlags&(std::get<0>(flag_names[i])))
		{
			ccWritten += _stprintf_s(szDesc + ccWritten, ccDesc - ccWritten, _T("%s%s"), !bFirst ? _T(" | ") : _T(""), std::get<1>(flag_names[i]));
			bFirst = false;
		}
	}
}

int ListDisplayDevices()
{
	DWORD iDevNum = 0;
	TCHAR szTemp[1024];
	do
	{
		DISPLAY_DEVICE disp_dev;
		memset(&disp_dev, 0, sizeof(disp_dev));
		disp_dev.cb = sizeof(disp_dev);
		if (!EnumDisplayDevices(NULL, iDevNum, &disp_dev, 0))
		{
			break;
		}

		_tprintf(_T("Display Device#%d:\n"), iDevNum);

		_tprintf(_T("    %20s: %s\n"), _T("Adaptor DeviceName"), disp_dev.DeviceName);
		_tprintf(_T("    %20s: %s\n"), _T("Adaptor DeviceString"), disp_dev.DeviceString);

		GetFlagsDesc(disp_dev.StateFlags, Device_state_flag_names, _countof(Device_state_flag_names), szTemp, _countof(szTemp));
		_tprintf(_T("    %20s: 0X%X, %s\n"), _T("Adaptor StateFlags"), disp_dev.StateFlags, szTemp);
	
		_tprintf(_T("    %20s: \n"), _T("Adaptor DeviceID"));
		PrintBuf(disp_dev.DeviceID, _countof(disp_dev.DeviceID), 26);

		_tprintf(_T("    %20s: \n"), _T("Adaptor DeviceKey"));
		PrintBuf(disp_dev.DeviceKey, _countof(disp_dev.DeviceKey), 26);

		TCHAR szDeviceName[32];
		_tcscpy_s(szDeviceName, _countof(szDeviceName), disp_dev.DeviceName);
		if (EnumDisplayDevices(szDeviceName, 0, &disp_dev, 0))
		{
			_tprintf(_T("    %20s: %s\n"), _T("Monitor DeviceName"), disp_dev.DeviceName);
			_tprintf(_T("    %20s: %s\n"), _T("Monitor DeviceString"), disp_dev.DeviceString);

			GetFlagsDesc(disp_dev.StateFlags, Device_state_flag_names, _countof(Device_state_flag_names), szTemp, _countof(szTemp));
			_tprintf(_T("    %20s: 0X%X, %s\n"), _T("Monitor StateFlags"), disp_dev.StateFlags, szTemp);

			_tprintf(_T("    %20s: \n"), _T("Monitor DeviceID"));
			PrintBuf(disp_dev.DeviceID, _countof(disp_dev.DeviceID), 26);

			_tprintf(_T("    %20s: \n"), _T("Monitor DeviceKey"));
			PrintBuf(disp_dev.DeviceKey, _countof(disp_dev.DeviceKey), 26);
		}

		iDevNum++;
	} while (true);

	return 0;
}

void PrintUsage()
{
	printf("Usage:\n");
	printf("    D3D11Report [options] command [ arg ...]\n");

	printf("    Commands:\n");
	printf("        help      show more detailed help message for each categories\n");
	printf("        devices   show the display devices and their information\n");
	printf("        report    show the collected D3D report\n");
}

int _tmain(int argc, const TCHAR* argv[])
{
	// TODO...
	// As for options, command arguments parsing, it will be implemented later

	if (argc < 2)
	{
		PrintUsage();
		return 0;
	}

	if (_tcsicmp(argv[1], _T("devices")) == 0)
	{
		ListDisplayDevices();
	}
	else if (_tcsicmp(argv[1], _T("report")) == 0)
	{

		HRESULT hr = S_OK;
		ComPtr<ID3D11Device> spD3D11Device;
		D3D_FEATURE_LEVEL FeatureLevel;
		static const D3D_FEATURE_LEVEL levels[] =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_10_0,
		};

		UINT flags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT | D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;

		if (SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			flags, levels, ARRAYSIZE(levels), D3D11_SDK_VERSION, &spD3D11Device, &FeatureLevel, nullptr)))
		{
			ComPtr<ID3D11VideoDevice> spVideoDevice;
			if (SUCCEEDED(spD3D11Device.As(&spVideoDevice)))
			{
				UINT nProfileCount = spVideoDevice->GetVideoDecoderProfileCount();
				for (UINT idxProfile = 0; idxProfile < spVideoDevice->GetVideoDecoderProfileCount(); idxProfile++)
				{
					GUID guidProfile = GUID_NULL;
					if (SUCCEEDED(spVideoDevice->GetVideoDecoderProfile(idxProfile, &guidProfile)))
					{
						D3D11_VIDEO_CONTENT_PROTECTION_CAPS cp_caps;
						if (FAILED(hr = spVideoDevice->GetContentProtectionCaps(&D3D11_CRYPTO_TYPE_AES128_CTR, &guidProfile, &cp_caps)))
							printf("Failed to get content protection caps for profile %s, hr: 0X%X\n", VDEC_PROFILE_NAME(guidProfile), hr);
						else
							printf("Get content protection caps for profile %s successfully\n", VDEC_PROFILE_NAME(guidProfile));
					}
				}
			}
		}

		CD3DReport D3DReport;
		D3DReport.Generate();

		if (argc < 3)
			printf(D3DReport.GetReport());
		else
		{
#ifdef _UNICODE
			char szReportPath[MAX_PATH];
			if (WideCharToMultiByte(CP_UTF8, 0, argv[2], -1, szReportPath, sizeof(szReportPath), NULL, NULL) <= 0)
			{
				_tprintf(_T("Failed to recognize the file path \"%s\" to store the D3D report."), argv[2]);
				return -1;
			}
			D3DReport.SaveToFile(szReportPath);
#else
			D3DReport.SaveToFile(argv[2]);
#endif
		}
	}

    return 0;
}

