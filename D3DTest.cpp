// D3DTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "tinyxml\tinyxml.h"
#include <Windows.h>
#include "D3DReport.h"
#include <wrl/client.h>
#include <tuple>
#include <d3d9.h>
#include <initguid.h>
#include <opmapi.h>
#include <wmcodecdsp.h>
#include "OPMSession.h"

#define AES_KEYSIZE		(16)

int g_verbose_level = 0;

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

static FLAG_NAME_PAIR Device_state_flag_names[] = {
	DECL_TUPLE(DISPLAY_DEVICE_ATTACHED_TO_DESKTOP),
	DECL_TUPLE(DISPLAY_DEVICE_MULTI_DRIVER),
	DECL_TUPLE(DISPLAY_DEVICE_PRIMARY_DEVICE),
	DECL_TUPLE(DISPLAY_DEVICE_MIRRORING_DRIVER),
	DECL_TUPLE(DISPLAY_DEVICE_VGA_COMPATIBLE),
	DECL_TUPLE(DISPLAY_DEVICE_REMOVABLE),
	DECL_TUPLE(DISPLAY_DEVICE_ACC_DRIVER),
	DECL_TUPLE(DISPLAY_DEVICE_RDPUDD),
	DECL_TUPLE(DISPLAY_DEVICE_DISCONNECT),
	DECL_TUPLE(DISPLAY_DEVICE_REMOTE),
	DECL_TUPLE(DISPLAY_DEVICE_MODESPRUNED),
	DECL_TUPLE(DISPLAY_DEVICE_TS_COMPATIBLE),
	DECL_TUPLE(DISPLAY_DEVICE_UNSAFE_MODES_ON),
};

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

void CopyAndAdvancePtr(BYTE*& pDest, const BYTE* pSrc, DWORD cb)
{
	memcpy(pDest, pSrc, cb);
	pDest += cb;
}

void ShowOPMVideoOutputStatus(IOPMVideoOutput* pOPMVideoOutput, int nIndent= 0)
{
	HRESULT hr = S_OK;
	if (pOPMVideoOutput == nullptr)
		return;

	WCHAR szIndent[1024];
	memset(szIndent, 0, sizeof(szIndent));
	for (size_t i = 0; i < nIndent; i++)
		szIndent[i] = L' ';

	ULONG ulStatusFlag = 0;
	COPMSession OPMSession(pOPMVideoOutput, &hr);

	if (FAILED(hr))
	{
		wprintf(L"Failed to initialize a OPM session {hr: 0X%08X(%s)}.\n", hr, OPM_RESULT_NAME(hr));
		return;
	}

	ULONG ConnectorType = 0;
	hr = OPMSession.GetConnectorType(ConnectorType, &ulStatusFlag);
	
	if (SUCCEEDED(hr))
	{
		if (ConnectorType&OPM_COPP_COMPATIBLE_CONNECTOR_TYPE_INTERNAL)
		{
			wprintf(L"%sConnector Type: Internal connector\n", szIndent);
			wprintf(L"%s                (*)The connection between the graphics adapter and the display device is permanent and not accessible to the user\n", szIndent);
		}

		switch (ConnectorType&OPM_BUS_IMPLEMENTATION_MODIFIER_MASK)
		{
		case OPM_CONNECTOR_TYPE_VGA:
			wprintf(L"%sConnector Type: Video Graphics Array (VGA) connector\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_SVIDEO:
			wprintf(L"%sConnector Type: S-Video connector\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_COMPOSITE_VIDEO:
			wprintf(L"%sConnector Type: Composite video connector\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_COMPONENT_VIDEO:
			wprintf(L"%sConnector Type: Component video connector\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_DVI:
			wprintf(L"%sConnector Type: Digital video interface (DVI) connector\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_HDMI:
			wprintf(L"%sConnector Type: High-definition multimedia interface (HDMI) connector\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_LVDS:
			wprintf(L"%sConnector Type: Low voltage differential signaling (LVDS) connector or MIPI Digital Serial Interface (DSI)\n", szIndent);
			wprintf(L"%s                A connector using the LVDS or MIPI Digital Serial Interface (DSI) interface to connect internally to a display device. The connection between the graphics adapter and the display device is permanent, non-removable, and not accessible to the user. Applications should not enable High-Bandwidth Digital Content Protection (HDCP) for this connector\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_D_JPN:
			wprintf(L"%sConnector Type: Japanese D connector\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_SDI:
			wprintf(L"%sConnector Type: SDI (serial digital image) connector\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_DISPLAYPORT_EXTERNAL:
			wprintf(L"%sConnector Type: A display port that connects externally to a display device\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_DISPLAYPORT_EMBEDDED:
			wprintf(L"%sConnector Type: An embedded display port that connects internally to a display device. Also known as an integrated display port.\n", szIndent);
			wprintf(L"%s                (*)Applications should not enable High-Bandwidth Digital Content Protection(HDCP) for embedded display ports\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_UDI_EXTERNAL:
			wprintf(L"%sConnector Type: A Unified Display Interface (UDI) that connects externally to a display device\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_UDI_EMBEDDED:
			wprintf(L"%sConnector Type: An embedded UDI that connects internally to a display device\n", szIndent);
			wprintf(L"%s    (*) Also known as an integrated UDI\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_RESERVED:
			wprintf(L"%sConnector Type: Reserved for future use\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_MIRACAST:
			wprintf(L"%sConnector Type: A Miracast wireless connector\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_TRANSPORT_AGNOSTIC_DIGITAL_MODE_A:
			wprintf(L"%sConnector Type: Transport Agnostic digital(Mode A) connector\n", szIndent);
			break;
		case OPM_CONNECTOR_TYPE_TRANSPORT_AGNOSTIC_DIGITAL_MODE_B:
			wprintf(L"%sConnector Type: Transport Agnostic digital(Mode B) connector\n", szIndent);
			break;
		default:
			wprintf(L"%sConnector Type: Unknown (0X%08X).\n", szIndent, ConnectorType);
		}
	}
	else
		wprintf(L"%sConnector Type: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	ULONG ulProtectionTypes = 0;
	hr = OPMSession.GetSupportedProtectionTypes(ulProtectionTypes, &ulStatusFlag);

	if (SUCCEEDED(hr))
	{
		WCHAR szProtectionTypeDesc[1024];
		GetFlagsDesc(ulProtectionTypes,
			OPM_PROTECTION_TYPE_flag_names, _countof(OPM_PROTECTION_TYPE_flag_names),
			szProtectionTypeDesc, _countof(szProtectionTypeDesc),
			L"OPM_PROTECTION_TYPE_NONE");
		wprintf(L"%sSupported Protection Types: %s\n", szIndent, szProtectionTypeDesc);
	}
	else
		wprintf(L"%sSupported Protection Types: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	OPM_ACP_AND_CGMSA_SIGNALING signal_info;
	hr = OPMSession.GetACPAndCGMSASignaling(signal_info);
	if (SUCCEEDED(hr))
	{
		WCHAR szTVProtectionStds[1024];
		GetFlagsDescW(signal_info.ulAvailableTVProtectionStandards,
			TV_PROTECTION_STD_flag_names, _countof(TV_PROTECTION_STD_flag_names),
			szTVProtectionStds, _countof(szTVProtectionStds),
			L"OPM_PROTECTION_STANDARD_NONE");
		wprintf(L"%sACP and CGMSA Signaling: \n", szIndent);
		wprintf(L"%s                       | AvailableTVProtectionStandards: %s\n", szIndent, szTVProtectionStds);
		GetFlagsDescW(signal_info.ulActiveTVProtectionStandard,
			TV_PROTECTION_STD_flag_names, _countof(TV_PROTECTION_STD_flag_names),
			szTVProtectionStds, _countof(szTVProtectionStds),
			L"OPM_PROTECTION_STANDARD_NONE");
		wprintf(L"%s                       | ActiveTVProtectionStandard: %s\n", szIndent, szTVProtectionStds);
		wprintf(L"%s                       | AspectRatioValidMask1: 0X%X\n", szIndent, signal_info.ulAspectRatioValidMask1);
		wprintf(L"%s                       | AspectRatioData1: 0X%X\n", szIndent, signal_info.ulAspectRatioData1);
		wprintf(L"%s                       | AspectRatioValidMask2: 0X%X\n", szIndent, signal_info.ulAspectRatioValidMask2);
		wprintf(L"%s                       | AspectRatioData2: 0X%X\n", szIndent, signal_info.ulAspectRatioData2);
		wprintf(L"%s                       | AspectRatioValidMask3: 0X%X\n", szIndent, signal_info.ulAspectRatioValidMask3);
		wprintf(L"%s                       | AspectRatioData3: 0X%X\n", szIndent, signal_info.ulAspectRatioData3);
	}
	else
		wprintf(L"%sACP and CGMSA Signaling: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	ULONG ulHDCPFlags = 0;
	OPM_HDCP_KEY_SELECTION_VECTOR ksv;
	hr = OPMSession.GetConnectedHDCPDeviceInformation(ulHDCPFlags, ksv);
	if (SUCCEEDED(hr))
	{
		wprintf(L"%sConnected HDCP Device Information: \n", szIndent);
		wprintf(L"%s                                 | HDCPFlags: %s \n", szIndent, 
			ulHDCPFlags == OPM_HDCP_FLAG_NONE?L"OPM_HDCP_FLAG_NONE":
			ulHDCPFlags == OPM_HDCP_FLAG_REPEATER?L"OPM_HDCP_FLAG_REPEATER":L"Unknown");
		wprintf(L"%s                                 | KSV: %02X %02X %02X %02X %02X\n", szIndent, 
			ksv.abKeySelectionVector[0], ksv.abKeySelectionVector[1], ksv.abKeySelectionVector[2],
			ksv.abKeySelectionVector[3], ksv.abKeySelectionVector[4]);
	}
	else
		wprintf(L"%sConnected HDCP Device Information: N/A {hr: 0X%X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	ULONG ulHDCPSRMVersion = 0;
	hr = OPMSession.GetCurrentHDCPSRMVersion(ulHDCPSRMVersion, &ulStatusFlag);
	if (SUCCEEDED(hr))
		wprintf(L"%sHDCP SRM Version: %d(0X%X)\n", szIndent, ulHDCPSRMVersion, ulHDCPSRMVersion);
	else
		wprintf(L"%sHDCP SRM Version: N/A {hr: 0X%X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	ULONG ulBusType = 0;
	hr = OPMSession.GetAdaptorBusType(ulBusType, &ulStatusFlag);

	if (SUCCEEDED(hr))
	{
		WCHAR szBusTypeDesc[1024];
		GetFlagsDescW(ulBusType, OPM_BUS_TYPE_flag_names, _countof(OPM_BUS_TYPE_flag_names), szBusTypeDesc, _countof(szBusTypeDesc), L"OPM_BUS_TYPE_OTHER");

		wprintf(L"%sAdaptor Bus Type: %s\n", szIndent, szBusTypeDesc);
	}
	else
		wprintf(L"%sAdaptor Bus Type: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	UINT64 ullOutputID = 0;
	hr = OPMSession.GetOutputID(ullOutputID, &ulStatusFlag);
	
	if (SUCCEEDED(hr))
		wprintf(L"%sOutput ID: %llu (0X%llX)\n", szIndent, ullOutputID, ullOutputID);
	else
		wprintf(L"%sOutput ID: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	OPM_OUTPUT_HARDWARE_PROTECTION hardware_protection;
	hr = OPMSession.GetOutputHardwareProtectionSupport(hardware_protection, &ulStatusFlag);

	if (SUCCEEDED(hr))
		wprintf(L"%sOutput Hardware Protection Support: %s\n", szIndent, hardware_protection == OPM_OUTPUT_HARDWARE_PROTECTION_SUPPORTED ? L"Yes" : L"No");
	else
		wprintf(L"%sOutput Hardware Protection Support: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	DWORD codecMerit = 0;
	hr = OPMSession.GetCodecInfo(__uuidof(CMSH264DecoderMFT), codecMerit);
	if (SUCCEEDED(hr))
		wprintf(L"%sMSH264 MFT Merit: %lu\n", szIndent, codecMerit);
	else
		wprintf(L"%sMSH264 MFT Merit: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	OPM_ACTUAL_OUTPUT_FORMAT output_format;
	hr = OPMSession.GetActualOutputFormat(output_format);
	if (SUCCEEDED(hr))
	{
		wprintf(L"%sActual Output Format: \n", szIndent);
		wprintf(L"%s                    | DisplayWidth: %lu\n", szIndent, output_format.ulDisplayWidth);
		wprintf(L"%s                    | DisplayHeight: %lu\n", szIndent, output_format.ulDisplayHeight);
		wprintf(L"%s                    | Display interlace mode: %s\n", szIndent, SampleFormat_NameW(output_format.dsfSampleInterleaveFormat));
		wprintf(L"%s                    | D3D Format: %s\n", szIndent, D3DFormat_NameW(output_format.d3dFormat));
		wprintf(L"%s                    | Refresh Rate: %f\n", szIndent, output_format.ulFrequencyNumerator*1.0f/output_format.ulFrequencyDenominator);
	}
	else
		wprintf(L"%sActual Output Format: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	ULONG ulDVICharacteristics = 0;
	if ((ConnectorType&OPM_BUS_IMPLEMENTATION_MODIFIER_MASK) == OPM_CONNECTOR_TYPE_DVI)
	{
		hr = OPMSession.GetDVICharacteristics(ulDVICharacteristics, &ulStatusFlag);
		if (SUCCEEDED(hr))
			wprintf(L"%sDVI Characteristics: %s\n", szIndent,
				ulDVICharacteristics == OPM_DVI_CHARACTERISTIC_1_0 ? L"OPM_DVI_CHARACTERISTIC_1_0" :
				ulDVICharacteristics == OPM_DVI_CHARACTERISTIC_1_1_OR_ABOVE ? L"OPM_DVI_CHARACTERISTIC_1_1_OR_ABOVE" : L"Unknown");
		else
			wprintf(L"%sDVI Characteristics: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));
	}

	ULONG protection_level = 0;
	hr = OPMSession.GetVirtualProtectionLevel(OPM_PROTECTION_TYPE_ACP, protection_level, &ulStatusFlag);
	if (SUCCEEDED(hr))
		wprintf(L"%sVirtual Protection Level for ACP: %s\n", szIndent, OPM_ACP_LEVEL_NAMEW(protection_level));
	else
		wprintf(L"%sVirtual Protection Level for ACP: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	protection_level = 0;
	hr = OPMSession.GetVirtualProtectionLevel(OPM_PROTECTION_TYPE_CGMSA, protection_level, &ulStatusFlag);
	if (SUCCEEDED(hr))
		wprintf(L"%sVirtual Protection Level for CGMS-A: %s\n", szIndent, OPM_CGMSA_LEVEL_NAMEW(protection_level));
	else
		wprintf(L"%sVirtual Protection Level for CGMS-A: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	protection_level = 0;
	hr = OPMSession.GetVirtualProtectionLevel(OPM_PROTECTION_TYPE_HDCP, protection_level, &ulStatusFlag);
	if (SUCCEEDED(hr))
		wprintf(L"%sVirtual Protection Level for HDCP: %s\n", szIndent, OPM_HDCP_LEVEL_NAMEW(protection_level));
	else
		wprintf(L"%sVirtual Protection Level for HDCP: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	protection_level = 0;
	hr = OPMSession.GetVirtualProtectionLevel(OPM_PROTECTION_TYPE_DPCP, protection_level, &ulStatusFlag);
	if (SUCCEEDED(hr))
		wprintf(L"%sVirtual Protection Level for DPCP: %s\n", szIndent, OPM_DPCP_LEVEL_NAMEW(protection_level));
	else
		wprintf(L"%sVirtual Protection Level for DPCP: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	hr = OPMSession.GetActualProtectionLevel(OPM_PROTECTION_TYPE_ACP, protection_level, &ulStatusFlag);
	if (SUCCEEDED(hr))
		wprintf(L"%sActual Protection Level for ACP: %s\n", szIndent, OPM_ACP_LEVEL_NAMEW(protection_level));
	else
		wprintf(L"%sActual Protection Level for ACP: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	protection_level = 0;
	hr = OPMSession.GetActualProtectionLevel(OPM_PROTECTION_TYPE_CGMSA, protection_level, &ulStatusFlag);
	if (SUCCEEDED(hr))
		wprintf(L"%sActual Protection Level for CGMS-A: %s\n", szIndent, OPM_CGMSA_LEVEL_NAMEW(protection_level));
	else
		wprintf(L"%sActual Protection Level for CGMS-A: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	protection_level = 0;
	hr = OPMSession.GetActualProtectionLevel(OPM_PROTECTION_TYPE_HDCP, protection_level, &ulStatusFlag);
	if (SUCCEEDED(hr))
		wprintf(L"%sActual Protection Level for HDCP: %s\n", szIndent, OPM_HDCP_LEVEL_NAMEW(protection_level));
	else
		wprintf(L"%sActual Protection Level for HDCP: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));

	protection_level = 0;
	hr = OPMSession.GetActualProtectionLevel(OPM_PROTECTION_TYPE_DPCP, protection_level, &ulStatusFlag);
	if (SUCCEEDED(hr))
		wprintf(L"%sActual Protection Level for DPCP: %s\n", szIndent, OPM_DPCP_LEVEL_NAMEW(protection_level));
	else
		wprintf(L"%sActual Protection Level for DPCP: N/A {hr: 0X%08X(%s)}\n", szIndent, hr, OPM_RESULT_NAME(hr));
}

void ShowMonitorOPMStatus(HMONITOR hMonitor, int nIndent=0)
{
	HRESULT hr = S_OK;
	ULONG ulNumVideoOutputs;
	IOPMVideoOutput **ppOPMVideoOutput = nullptr;
	hr = OPMGetVideoOutputsFromHMONITOR(hMonitor, OPM_VOS_OPM_SEMANTICS, &ulNumVideoOutputs, &ppOPMVideoOutput);

	if (FAILED(hr))
	{
		wprintf(L"Failed to get OPM video outputs {hr: 0X%X}.\n", hr);
		return;
	}

	for (UINT i = 0; i < ulNumVideoOutputs; i++)
	{
		if (ppOPMVideoOutput[i] == nullptr)
			continue;

		WCHAR szFmtStr[256];
		swprintf_s(szFmtStr, _countof(szFmtStr), L"%%%dsOPMVideoOutput#%%d:\n", nIndent);

		wprintf(szFmtStr, L"", i);
		ShowOPMVideoOutputStatus(ppOPMVideoOutput[i], nIndent + 4);
	}
}

HRESULT EnableHDCP(HMONITOR hMonitor, int nIndent = 0)
{
	HRESULT hr = S_OK;
	ULONG ulNumVideoOutputs;
	IOPMVideoOutput **ppOPMVideoOutput = nullptr;
	hr = OPMGetVideoOutputsFromHMONITOR(hMonitor, OPM_VOS_OPM_SEMANTICS, &ulNumVideoOutputs, &ppOPMVideoOutput);

	WCHAR szIndent[1024];
	memset(szIndent, 0, sizeof(szIndent));
	for (size_t i = 0; i < nIndent; i++)
		szIndent[i] = L' ';

	if (FAILED(hr))
	{
		wprintf(L"Failed to get OPM video outputs {hr: 0X%X}.\n", hr);
		return hr;
	}

	for (UINT i = 0; i < ulNumVideoOutputs; i++)
	{
		if (ppOPMVideoOutput[i] == nullptr)
			continue;

		ULONG ulStatusFlag = 0;
		ULONG ulProtectionTypes = 0;
		COPMSession OPMSession(ppOPMVideoOutput[i], &hr);

		hr = OPMSession.GetSupportedProtectionTypes(ulProtectionTypes);

		if (ulProtectionTypes&OPM_PROTECTION_TYPE_HDCP)
		{
			ULONG ulSetHDCPSRMVersion = 0;
			hr = OPMSession.SetHDCPSRM(nullptr, 0, &ulSetHDCPSRMVersion);
			if (SUCCEEDED(hr))
			{
				wprintf(L"%sSet HDCP SRM: Success.\n", szIndent);

				hr = OPMSession.SetProtectionLevel(OPM_PROTECTION_TYPE_HDCP, OPM_HDCP_ON);
				if (SUCCEEDED(hr))
					wprintf(L"%sSet Protection Level(OPM_PROTECTION_TYPE_HDCP, OPM_HDCP_ON): Success.\n", szIndent);
				else
					wprintf(L"%sSet Protection Level(OPM_PROTECTION_TYPE_HDCP, OPM_HDCP_ON): Failed {hr: 0X%08X(%s)}.\n", szIndent, hr, OPM_RESULT_NAME(hr));
			}
			else
				wprintf(L"%sSet HDCP SRM: Failed {hr: 0X%08X}.\n", szIndent, hr);
		}

		if (FAILED(hr))
			break;
	}

	return hr;
}

void ListMonitorOPMStatus()
{
	HRESULT hr = S_OK;
	// Generate the report for IDXGIAdaptor
	ComPtr<IDXGIFactory> spFactory;
	if (SUCCEEDED(CreateDXGIFactory(IID_IDXGIFactory, (void**)&spFactory)))
	{
		UINT idxAdaptor = 0;
		ComPtr<IDXGIAdapter> spAdaptor;
		while (SUCCEEDED(spFactory->EnumAdapters(idxAdaptor, &spAdaptor)))
		{
			UINT idxOutput = 0;
			ComPtr<IDXGIOutput> spOutput;
			while (SUCCEEDED(spAdaptor->EnumOutputs(idxOutput, &spOutput)))
			{
				DXGI_OUTPUT_DESC output_desc;
				if (SUCCEEDED(spOutput->GetDesc(&output_desc)))
				{
					wprintf(L"\nOutput device name: %s\n", output_desc.DeviceName);
					ShowMonitorOPMStatus(output_desc.Monitor, 4);

					int nTries = 0;
					do
					{
						if (SUCCEEDED(EnableHDCP(output_desc.Monitor, 4)))
						{
							ShowMonitorOPMStatus(output_desc.Monitor, 4);
						}

						nTries++;
						::Sleep(100);

					} while (nTries < 3);
				}

				idxOutput++;
			}

			idxAdaptor++;
		}
	}
	else
	{
		_tprintf(_T("[D3DReport] Failed to create DXGI Factory.\n"));
	}
}

void PrintUsage()
{
	printf("Usage:\n");
	printf("    D3D11Report [options] command [ arg ...]\n");

	printf("    Commands:\n");
	printf("        help      show more detailed help message for each categories\n");
	printf("        devices   show the display devices and their information\n");
	printf("        report    show the collected D3D report\n");
	printf("        opm       show the opm status of monitors\n");
	printf("    Options:\n");
	printf("        --verbose show more logs\n");
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

	if (argc >= 3)
	{
		for (int i = 2; i < argc; i++) {
			if (_tcsncmp(argv[i], _T("--verbose"), 9) == 0)
			{
				g_verbose_level = 1;
			}
		}
	}

	if (_tcsicmp(argv[1], _T("devices")) == 0)
	{
		ListDisplayDevices();
	}
	else if (_tcsicmp(argv[1], _T("opm")) == 0)
	{
		ListMonitorOPMStatus();
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

