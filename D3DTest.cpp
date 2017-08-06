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

int main()
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
									flags, levels, ARRAYSIZE(levels), D3D11_SDK_VERSION, &spD3D11Device, &FeatureLevel,nullptr)))
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

#if 0
	TiXmlDocument* xmlDoc = new TiXmlDocument();

	TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "utf-8", "");

	xmlDoc->LinkEndChild(decl);

	TiXmlElement* root = new TiXmlElement("Test");

	wchar_t* szTest = L"ÖÐ¹ú";
	char szUTF8[128];
	int ccUTF8 = WideCharToMultiByte(CP_UTF8, 0, szTest, 2, szUTF8, 128, NULL, NULL);
	if (ccUTF8 > 0)
		szUTF8[ccUTF8] = 0;

	root->SetAttribute("ID", szUTF8);

	xmlDoc->LinkEndChild(root);

	xmlDoc->SaveFile("C:\\Users\\Wangfei\\Desktop\\test.xml");

	delete xmlDoc;

	xmlDoc = new TiXmlDocument();

	xmlDoc->LoadFile("C:\\Users\\Wangfei\\Desktop\\test.xml");

	xmlDoc->Clear();

	xmlDoc->LoadFile("C:\\Users\\Wangfei\\Desktop\\test.xml");

	TiXmlPrinter printer;
	printer.SetIndent("    ");

	xmlDoc->Accept(&printer);
	
	const char* szXml = printer.CStr();

	delete xmlDoc;
#endif

	CD3DReport D3DReport;
	D3DReport.Generate();
	
	printf(D3DReport.GetReport());

	D3DReport.SaveToFile("C:\\Users\\Ravin\\Desktop\\D3DReport.xml");

    return 0;
}

