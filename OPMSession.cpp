#include "stdafx.h"
#include <initguid.h>
#include "OPMSession.h"

COPMSession::COPMSession(IOPMVideoOutput* pOPMVideoOutput, HRESULT* phr)
	: m_spOPMVideoOutput(pOPMVideoOutput)
	, m_ulStatusSequence(0)
	, m_ulCommandSequence(0)
{
	HRESULT hr = S_OK;
	if (pOPMVideoOutput == nullptr)
	{
		if (phr != nullptr)
			*phr = E_POINTER;
		return;
	}

	OPM_RANDOM_NUMBER random;   // Random number from driver.
	ZeroMemory(&random, sizeof(random));

	BYTE *pbCertificate = NULL; // Pointer to a buffer to hold the certificate.
	ULONG cbCertificate = 0;    // Size of the certificate in bytes.

	// Get the driver's certificate chain + random number
	hr = pOPMVideoOutput->StartInitialization(&random, &pbCertificate, &cbCertificate);

	if (FAILED(hr))
	{
		LOG_ERROR(L"[OPM] Failed to start OPM video output initialization {hr: 0X%08X(%s)}.\n", hr, OPM_RESULT_NAME(hr));
		goto done;
	}

	// Parameter generation for initialization
	OPM_ENCRYPTED_INITIALIZATION_PARAMETERS initParam;
	ZeroMemory(&initParam, sizeof(initParam));
	// initParam[ 0..15]	128bit random Number (Driver generated: StartInitialization () value returned)
	// initParam[16..31]	128bit AES session
	// initParam[32..35]	Sequence number (OPM status)
	// initParam[36..39]	Sequence number (OPM command)
	LPBYTE pInitParam = (LPBYTE)&initParam;
	memcpy(pInitParam, &random, sizeof(random));
	pInitParam += sizeof(random);

	hr = CryptUtil::GenerateRandom(pInitParam, AES_KEYSIZE_128 + sizeof(ULONG) + sizeof(ULONG));
	if (FAILED(hr))
	{
		LOG_ERROR(L"[OPM] CryptUtil::GenerateRandom() failed. hr: 0x%08x\n", hr);
		goto done;
	}

	// Save session key to use for encryption during status request
	// initParam will be encrypted, so you can copy the text separately.
	memcpy(m_keySession.abRandomNumber, pInitParam, sizeof(m_keySession.abRandomNumber));
	pInitParam += sizeof(m_keySession.abRandomNumber);

	// OPM save Status request Sequence number
	// Use it to check the output destination
	m_ulStatusSequence = *((ULONG*)pInitParam);
	pInitParam += sizeof(ULONG);

	// OPM Command execution Sequence number
	m_ulCommandSequence = *((ULONG*)pInitParam);
	pInitParam += sizeof(ULONG);

	// Encrypted with RSAES-OAEP using the driver's public key
	hr = CryptUtil::EncryptRSAESOAEPWithPublicKey(
		pbCertificate,								// Certificate chain
		cbCertificate,								// Certificate Chain Size
		(LPBYTE)&initParam,							// Encrypted buffer
		(DWORD)sizeof(initParam),					// Encryption Target buffer size
		(DWORD)(pInitParam - (LPBYTE)&initParam),	// Data length to be encrypted
		NULL,
		0
	);
	if (FAILED(hr))
	{
		LOG_ERROR(L"[OPM] Failed to encrypt with RSAES-OAEP using the driver's public key. hr = 0x%08x\n", hr);
		goto done;
	}

	hr = pOPMVideoOutput->FinishInitialization(&initParam);
	if (FAILED(hr))
	{
		wprintf(L"[OPM] pOPMVideoOutput->FinishInitialization() failed. {hr: 0x%08x(%s)}\n", hr, OPM_RESULT_NAME(hr));
		goto done;
	}

done:
	if (phr)
		*phr = hr;
}


COPMSession::~COPMSession()
{
	m_spOPMVideoOutput = nullptr;
}

HRESULT COPMSession::_StatusRequest(GUID guidInformation, OPM_REQUESTED_INFORMATION& StatusOutput, uint8_t* pParams, size_t cbParams)
{
	HRESULT hr = S_OK;
	OPM_OMAC rgbSignature = { 0 };
	OPM_GET_INFO_PARAMETERS StatusInput;

	//--------------------------------------------------------------------
	// Prepare the status request structure.
	//--------------------------------------------------------------------

	ZeroMemory(&StatusInput, sizeof(StatusInput));
	ZeroMemory(&StatusOutput, sizeof(StatusOutput));

	if (pParams != NULL && cbParams > 0)
	{
		if (cbParams > sizeof(StatusInput.abParameters))
			return E_INVALIDARG;

		memcpy(StatusInput.abParameters, pParams, cbParams);
		StatusInput.cbParametersSize = (ULONG)cbParams;
	}

	hr = BCryptGenRandom(
		NULL,
		(BYTE*)&(StatusInput.rnRandomNumber),
		OPM_128_BIT_RANDOM_NUMBER_SIZE,
		BCRYPT_USE_SYSTEM_PREFERRED_RNG
	);

	if (FAILED(hr))
	{
		LOG_WARNING(L"[OPM] Failed to generate random buffer {hr: 0X%08X}.\n", hr);
		goto done;
	}

	StatusInput.guidInformation = guidInformation;			// Request GUID.
	StatusInput.ulSequenceNumber = m_ulStatusSequence;		// Sequence number.

	//  Sign the request structure, not including the OMAC field.
	hr = CryptUtil::ComputeOMAC1(
		m_keySession.abRandomNumber,                        // Session key.
		(BYTE*)&StatusInput + OPM_OMAC_SIZE,                // Data
		sizeof(OPM_GET_INFO_PARAMETERS) - OPM_OMAC_SIZE,    // Size
		StatusInput.omac.abOMAC                             // Receives the OMAC
	);

	if (FAILED(hr))
	{
		LOG_WARNING(L"[OPM] Failed to compute OMAC-1 {hr: 0X%08X}.\n", hr);
		goto done;
	}

	//  Send the status request.
	hr = m_spOPMVideoOutput->GetInformation(&StatusInput, &StatusOutput);

	if (FAILED(hr))
	{
		LOG_WARNING(L"[OPM] Failed to call GetInformation with OPM video output interface {hr: 0X%08X(%s)}.\n", hr, OPM_RESULT_NAME(hr));
		goto done;
	}

	//--------------------------------------------------------------------
	// Verify the signature.
	//--------------------------------------------------------------------

	// Calculate our own signature.
	hr = CryptUtil::ComputeOMAC1(
		m_keySession.abRandomNumber,
		(BYTE*)&StatusOutput + OPM_OMAC_SIZE,
		sizeof(OPM_REQUESTED_INFORMATION) - OPM_OMAC_SIZE,
		rgbSignature.abOMAC
	);

	if (FAILED(hr))
	{
		LOG_WARNING(L"[OPM] Failed to calculate the OMAC-1 value based on the Status output from OPM {hr: 0X%08X}.\n", hr);
		goto done;
	}

	if (memcmp(StatusOutput.omac.abOMAC, rgbSignature.abOMAC, OPM_OMAC_SIZE))
	{
		// The signature does not match.
		LOG_WARNING(L"[OPM] The status output of OPM failed to verify the OMAC-1 signature.\n");
		hr = E_FAIL;
		goto done;
	}

	// Update the sequence number.
	m_ulStatusSequence++;

	// Verify rnRandomNumber between input status and output status
	if (StatusOutput.cbRequestedInformationSize < sizeof(OPM_RANDOM_NUMBER))
	{
		LOG_WARNING(L"[OPM] The size (%lu bytes) of requested information should be not less than %zu.\n", StatusOutput.cbRequestedInformationSize, sizeof(OPM_RANDOM_NUMBER));
		hr = E_UNEXPECTED;
		goto done;
	}

	//  Verify the random number.
	if (0 != memcmp(StatusOutput.abRequestedInformation, (BYTE*)&StatusInput.rnRandomNumber, sizeof(OPM_RANDOM_NUMBER)))
	{
		LOG_WARNING(L"[OPM] The input random number is not equal to the output random number.\n");
		hr = E_UNEXPECTED;
		goto done;
	}

done:
	return hr;
}

HRESULT COPMSession::CheckOPMSessionStatus(ULONG ulOPMSessionStatus)
{
	if (ulOPMSessionStatus != OPM_STATUS_NORMAL)
	{
		// Abnormal status
		wchar_t szStatusDesc[1024];
		GetFlagsDescW(ulOPMSessionStatus, OPM_STATUS_flag_names, _countof(OPM_STATUS_flag_names), szStatusDesc, _countof(szStatusDesc), L"OPM_STATUS_NORMAL");
		LOG_WARNING(L"[OPM] OPM session status: %s, it is abnormal.\n", szStatusDesc);
		return E_FAIL;
	}

	return S_OK;
}

HRESULT COPMSession::GetCurrentHDCPSRMVersion(ULONG& HDCPSRMVer, ULONG* pulStatusFlags)
{
	HRESULT hr = S_OK;

	ULONG ulStatusFlags = 0;
	OPM_REQUESTED_INFORMATION StatusOutput;
	hr = _StatusRequest(OPM_GET_CURRENT_HDCP_SRM_VERSION, StatusOutput);
	if (FAILED(hr))
		return hr;

	OPM_STANDARD_INFORMATION StatusInfo;
	ZeroMemory(&StatusInfo, sizeof(StatusInfo));

	ULONG cbLen = (sizeof(OPM_STANDARD_INFORMATION), StatusOutput.cbRequestedInformationSize);

	if (cbLen != 0)
	{
		// Copy the response into the array.
		CopyMemory((BYTE*)&StatusInfo, StatusOutput.abRequestedInformation, cbLen);
	}

	AMP_SAFEASSIGN(pulStatusFlags, StatusInfo.ulStatusFlags);
	HDCPSRMVer = StatusInfo.ulInformation;

	// Verify the status of the OPM session.
	hr = CheckOPMSessionStatus(StatusInfo.ulStatusFlags);

	return hr;
}

HRESULT COPMSession::GetConnectedHDCPDeviceInformation(ULONG& ulHDCPFlags, OPM_HDCP_KEY_SELECTION_VECTOR& HDCP_Key_Selection_Vector, ULONG* pulStatusFlags)
{
	HRESULT hr = S_OK;

	ULONG ulStatusFlags = 0;
	OPM_REQUESTED_INFORMATION StatusOutput;
	hr = _StatusRequest(OPM_GET_CONNECTED_HDCP_DEVICE_INFORMATION, StatusOutput);
	if (FAILED(hr))
		return hr;

	OPM_CONNECTED_HDCP_DEVICE_INFORMATION StatusInfo;
	ZeroMemory(&StatusInfo, sizeof(StatusInfo));

	ULONG cbLen = (sizeof(OPM_CONNECTED_HDCP_DEVICE_INFORMATION), StatusOutput.cbRequestedInformationSize);

	if (cbLen != 0)
	{
		// Copy the response into the array.
		CopyMemory((BYTE*)&StatusInfo, StatusOutput.abRequestedInformation, cbLen);
	}

	AMP_SAFEASSIGN(pulStatusFlags, StatusInfo.ulStatusFlags);
	ulHDCPFlags = StatusInfo.ulHDCPFlags;
	memcpy(&HDCP_Key_Selection_Vector.abKeySelectionVector, StatusInfo.ksvB.abKeySelectionVector, sizeof(StatusInfo.ksvB.abKeySelectionVector));

	// Verify the status of the OPM session.
	hr = CheckOPMSessionStatus(StatusInfo.ulStatusFlags);

	return hr;
}

HRESULT COPMSession::GetACPAndCGMSASignaling(OPM_ACP_AND_CGMSA_SIGNALING& signaling)
{
	HRESULT hr = S_OK;

	ULONG ulStatusFlags = 0;
	OPM_REQUESTED_INFORMATION StatusOutput;
	hr = _StatusRequest(OPM_GET_ACP_AND_CGMSA_SIGNALING, StatusOutput);
	if (FAILED(hr))
		return hr;

	ZeroMemory(&signaling, sizeof(signaling));

	if (sizeof(OPM_CONNECTED_HDCP_DEVICE_INFORMATION) > StatusOutput.cbRequestedInformationSize)
		return E_UNEXPECTED;

	// Copy the response into the array.
	CopyMemory((BYTE*)&signaling, StatusOutput.abRequestedInformation, sizeof(OPM_CONNECTED_HDCP_DEVICE_INFORMATION));

	// Verify the status of the OPM session.
	hr = CheckOPMSessionStatus(signaling.ulStatusFlags);

	return hr;
}

HRESULT COPMSession::GetConnectorType(ULONG& ulConnectorType, ULONG* pulStatusFlags)
{
	HRESULT hr = S_OK;

	ULONG ulStatusFlags = 0;
	OPM_REQUESTED_INFORMATION StatusOutput;
	hr = _StatusRequest(OPM_GET_CONNECTOR_TYPE, StatusOutput);
	if (FAILED(hr))
		return hr;

	OPM_STANDARD_INFORMATION StatusInfo;
	ZeroMemory(&StatusInfo, sizeof(StatusInfo));

	ULONG cbLen = (sizeof(OPM_STANDARD_INFORMATION), StatusOutput.cbRequestedInformationSize);

	if (cbLen != 0)
	{
		// Copy the response into the array.
		CopyMemory((BYTE*)&StatusInfo, StatusOutput.abRequestedInformation, cbLen);
	}

	AMP_SAFEASSIGN(pulStatusFlags, StatusInfo.ulStatusFlags);
	ulConnectorType = StatusInfo.ulInformation;

	// Verify the status of the OPM session.
	hr = CheckOPMSessionStatus(StatusInfo.ulStatusFlags);

	return hr;
}

HRESULT COPMSession::GetSupportedProtectionTypes(ULONG& ulProtectionTypes, ULONG* pulStatusFlags)
{
	HRESULT hr = S_OK;

	ULONG ulStatusFlags = 0;
	OPM_REQUESTED_INFORMATION StatusOutput;
	hr = _StatusRequest(OPM_GET_SUPPORTED_PROTECTION_TYPES, StatusOutput);
	if (FAILED(hr))
		return hr;

	OPM_STANDARD_INFORMATION StatusInfo;
	ZeroMemory(&StatusInfo, sizeof(StatusInfo));

	ULONG cbLen = (sizeof(OPM_STANDARD_INFORMATION), StatusOutput.cbRequestedInformationSize);

	if (cbLen != 0)
	{
		// Copy the response into the array.
		CopyMemory((BYTE*)&StatusInfo, StatusOutput.abRequestedInformation, cbLen);
	}

	AMP_SAFEASSIGN(pulStatusFlags, StatusInfo.ulStatusFlags);
	ulProtectionTypes = StatusInfo.ulInformation;

	// Verify the status of the OPM session.
	hr = CheckOPMSessionStatus(StatusInfo.ulStatusFlags);

	return hr;
}

HRESULT COPMSession::GetVirtualProtectionLevel(ULONG protection_type, ULONG& protection_level, ULONG* pulStatusFlags)
{
	HRESULT hr = S_OK;

	ULONG ulStatusFlags = 0;
	OPM_REQUESTED_INFORMATION StatusOutput;
	hr = _StatusRequest(OPM_GET_VIRTUAL_PROTECTION_LEVEL, StatusOutput, (uint8_t*)&protection_type, sizeof(protection_type));
	if (FAILED(hr))
		return hr;

	OPM_STANDARD_INFORMATION StatusInfo;
	ZeroMemory(&StatusInfo, sizeof(StatusInfo));

	ULONG cbLen = (sizeof(OPM_STANDARD_INFORMATION), StatusOutput.cbRequestedInformationSize);

	if (cbLen != 0)
	{
		// Copy the response into the array.
		CopyMemory((BYTE*)&StatusInfo, StatusOutput.abRequestedInformation, cbLen);
	}

	AMP_SAFEASSIGN(pulStatusFlags, StatusInfo.ulStatusFlags);
	protection_level = StatusInfo.ulInformation;

	// Verify the status of the OPM session.
	hr = CheckOPMSessionStatus(StatusInfo.ulStatusFlags);

	return hr;
}

HRESULT COPMSession::GetActualProtectionLevel(ULONG protection_type, ULONG& protection_level, ULONG* pulStatusFlags)
{
	HRESULT hr = S_OK;

	ULONG ulStatusFlags = 0;
	OPM_REQUESTED_INFORMATION StatusOutput;
	hr = _StatusRequest(OPM_GET_ACTUAL_PROTECTION_LEVEL, StatusOutput, (uint8_t*)&protection_type, sizeof(protection_type));
	if (FAILED(hr))
		return hr;

	OPM_STANDARD_INFORMATION StatusInfo;
	ZeroMemory(&StatusInfo, sizeof(StatusInfo));

	ULONG cbLen = (sizeof(OPM_STANDARD_INFORMATION), StatusOutput.cbRequestedInformationSize);

	if (cbLen != 0)
	{
		// Copy the response into the array.
		CopyMemory((BYTE*)&StatusInfo, StatusOutput.abRequestedInformation, cbLen);
	}

	AMP_SAFEASSIGN(pulStatusFlags, StatusInfo.ulStatusFlags);
	protection_level = StatusInfo.ulInformation;

	// Verify the status of the OPM session.
	hr = CheckOPMSessionStatus(StatusInfo.ulStatusFlags);

	return hr;
}

HRESULT COPMSession::GetActualOutputFormat(OPM_ACTUAL_OUTPUT_FORMAT& output_format)
{
	HRESULT hr = S_OK;

	ULONG ulStatusFlags = 0;
	OPM_REQUESTED_INFORMATION StatusOutput;
	hr = _StatusRequest(OPM_GET_ACTUAL_OUTPUT_FORMAT, StatusOutput);
	if (FAILED(hr))
		return hr;

	ZeroMemory(&output_format, sizeof(output_format));

	if (sizeof(OPM_ACTUAL_OUTPUT_FORMAT) > StatusOutput.cbRequestedInformationSize)
		return E_UNEXPECTED;

	// Copy the response into the array.
	CopyMemory((BYTE*)&output_format, StatusOutput.abRequestedInformation, sizeof(OPM_ACTUAL_OUTPUT_FORMAT));

	// Verify the status of the OPM session.
	hr = CheckOPMSessionStatus(output_format.ulStatusFlags);

	return hr;
}

HRESULT COPMSession::GetAdaptorBusType(ULONG& ulBusType, ULONG* pulStatusFlags)
{
	HRESULT hr = S_OK;

	ULONG ulStatusFlags = 0;
	OPM_REQUESTED_INFORMATION StatusOutput;
	hr = _StatusRequest(OPM_GET_ADAPTER_BUS_TYPE, StatusOutput);
	if (FAILED(hr))
		return hr;

	OPM_STANDARD_INFORMATION StatusInfo;
	ZeroMemory(&StatusInfo, sizeof(StatusInfo));

	ULONG cbLen = (sizeof(OPM_STANDARD_INFORMATION), StatusOutput.cbRequestedInformationSize);

	if (cbLen != 0)
	{
		// Copy the response into the array.
		CopyMemory((BYTE*)&StatusInfo, StatusOutput.abRequestedInformation, cbLen);
	}

	AMP_SAFEASSIGN(pulStatusFlags, StatusInfo.ulStatusFlags);
	ulBusType = StatusInfo.ulInformation;

	// Verify the status of the OPM session.
	hr = CheckOPMSessionStatus(StatusInfo.ulStatusFlags);

	return hr;
}

HRESULT COPMSession::GetOutputID(UINT64& ullOutputID, ULONG* pulStatusFlags)
{
	HRESULT hr = S_OK;

	ULONG ulStatusFlags = 0;
	OPM_REQUESTED_INFORMATION StatusOutput;
	hr = _StatusRequest(OPM_GET_OUTPUT_ID, StatusOutput);
	if (FAILED(hr))
		return hr;

	OPM_OUTPUT_ID_DATA StatusInfo;
	ZeroMemory(&StatusInfo, sizeof(StatusInfo));

	ULONG cbLen = (sizeof(OPM_OUTPUT_ID_DATA), StatusOutput.cbRequestedInformationSize);

	if (cbLen != 0)
	{
		// Copy the response into the array.
		CopyMemory((BYTE*)&StatusInfo, StatusOutput.abRequestedInformation, cbLen);
	}

	AMP_SAFEASSIGN(pulStatusFlags, StatusInfo.ulStatusFlags);
	ullOutputID = StatusInfo.OutputId;

	// Verify the status of the OPM session.
	hr = CheckOPMSessionStatus(StatusInfo.ulStatusFlags);

	return hr;
}

HRESULT COPMSession::GetDVICharacteristics(ULONG& ulCharacteristics, ULONG* pulStatusFlags)
{
	HRESULT hr = S_OK;

	ULONG ulStatusFlags = 0;
	OPM_REQUESTED_INFORMATION StatusOutput;
	hr = _StatusRequest(OPM_GET_DVI_CHARACTERISTICS, StatusOutput);
	if (FAILED(hr))
		return hr;

	OPM_STANDARD_INFORMATION StatusInfo;
	ZeroMemory(&StatusInfo, sizeof(StatusInfo));

	ULONG cbLen = (sizeof(OPM_STANDARD_INFORMATION), StatusOutput.cbRequestedInformationSize);

	if (cbLen != 0)
	{
		// Copy the response into the array.
		CopyMemory((BYTE*)&StatusInfo, StatusOutput.abRequestedInformation, cbLen);
	}

	AMP_SAFEASSIGN(pulStatusFlags, StatusInfo.ulStatusFlags);
	ulCharacteristics = StatusInfo.ulInformation;

	// Verify the status of the OPM session.
	hr = CheckOPMSessionStatus(StatusInfo.ulStatusFlags);

	return hr;
}

HRESULT COPMSession::GetCodecInfo(CLSID clsidCodecMFT, DWORD& Merit)
{
	HRESULT hr = S_OK;

	ULONG ulStatusFlags = 0;
	OPM_REQUESTED_INFORMATION StatusOutput;
	OPM_GET_CODEC_INFO_PARAMETERS codec_info_params;
	codec_info_params.cbVerifier = sizeof(CLSID);
	memset(codec_info_params.Verifier, 0, sizeof(codec_info_params.Verifier));
	*((CLSID*)codec_info_params.Verifier) = clsidCodecMFT;

	hr = _StatusRequest(OPM_GET_CODEC_INFO, StatusOutput, (uint8_t*)&codec_info_params, sizeof(codec_info_params));
	if (FAILED(hr))
		return hr;

	OPM_GET_CODEC_INFO_INFORMATION StatusInfo;
	ZeroMemory(&StatusInfo, sizeof(StatusInfo));

	ULONG cbLen = (sizeof(OPM_GET_CODEC_INFO_INFORMATION), StatusOutput.cbRequestedInformationSize);

	if (cbLen != 0)
	{
		// Copy the response into the array.
		CopyMemory((BYTE*)&StatusInfo, StatusOutput.abRequestedInformation, cbLen);
	}

	Merit =	 StatusInfo.Merit;

	return hr; 
}

HRESULT COPMSession::GetOutputHardwareProtectionSupport(OPM_OUTPUT_HARDWARE_PROTECTION& eSupportState, ULONG* pulStatusFlags)
{
	HRESULT hr = S_OK;

	ULONG ulStatusFlags = 0;
	OPM_REQUESTED_INFORMATION StatusOutput;
	hr = _StatusRequest(OPM_GET_OUTPUT_HARDWARE_PROTECTION_SUPPORT, StatusOutput);
	if (FAILED(hr))
		return hr;

	OPM_STANDARD_INFORMATION StatusInfo;
	ZeroMemory(&StatusInfo, sizeof(StatusInfo));

	ULONG cbLen = (sizeof(OPM_STANDARD_INFORMATION), StatusOutput.cbRequestedInformationSize);

	if (cbLen != 0)
	{
		// Copy the response into the array.
		CopyMemory((BYTE*)&StatusInfo, StatusOutput.abRequestedInformation, cbLen);
	}

	AMP_SAFEASSIGN(pulStatusFlags, StatusInfo.ulStatusFlags);
	eSupportState = (OPM_OUTPUT_HARDWARE_PROTECTION)StatusInfo.ulInformation;

	// Verify the status of the OPM session.
	hr = CheckOPMSessionStatus(StatusInfo.ulStatusFlags);

	return hr;
}

HRESULT COPMSession::SetProtectionLevel(ULONG ulProtectionType, ULONG ulProtectionLevel)
{
	HRESULT hr = S_OK;
	//--------------------------------------------------------------------
	// Prepare the command structure.
	//--------------------------------------------------------------------

	// Data specific to the OPM_SET_PROTECTION_LEVEL command.
	OPM_SET_PROTECTION_LEVEL_PARAMETERS CommandInput;

	ZeroMemory(&CommandInput, sizeof(CommandInput));

	CommandInput.ulProtectionType = ulProtectionType;
	CommandInput.ulProtectionLevel = ulProtectionLevel;

	// Common command parameters
	OPM_CONFIGURE_PARAMETERS Command;
	ZeroMemory(&Command, sizeof(Command));

	Command.guidSetting = OPM_SET_PROTECTION_LEVEL;
	Command.ulSequenceNumber = m_ulCommandSequence;
	Command.cbParametersSize = sizeof(OPM_SET_PROTECTION_LEVEL_PARAMETERS);
	CopyMemory(&Command.abParameters[0], (BYTE*)&CommandInput, Command.cbParametersSize);

	//  Sign the command structure, not including the OMAC-1 field.
	hr = CryptUtil::ComputeOMAC1(
		m_keySession.abRandomNumber,
		(BYTE*)&Command + OPM_OMAC_SIZE,
		sizeof(OPM_CONFIGURE_PARAMETERS) - OPM_OMAC_SIZE,
		Command.omac.abOMAC);

	if (FAILED(hr))
	{
		goto done;
	}

	//  Send the command.
	hr = m_spOPMVideoOutput->Configure(&Command, 0, NULL);

	if (FAILED(hr))
	{
		LOG_WARNING(L"[OPM] Failed to configure OPM_SET_PROTECTION_LEVEL {hr: 0X%08X(%s)}.\n", hr, OPM_RESULT_NAME(hr));
		goto done;
	}

	//  Update the sequence number.
	m_ulCommandSequence++;

done:
	return hr;
}

HRESULT COPMSession::SetHDCPSRM(const uint8_t* pSRMData, size_t cbSRMData, ULONG* pulHDCPSRMVer)
{
	HRESULT hr = S_OK;
	//--------------------------------------------------------------------
	// Prepare the command structure.
	//--------------------------------------------------------------------

	// Empty SRM
	const uint8_t pbEmptySRM[] = {
		0x80,0x00,0x00,0x01,0x00,0x00,0x00,0x2b,
		0xd2,0x48,0x9e,0x49,0xd0,0x57,0xae,0x31,
		0x5b,0x1a,0xbc,0xe0,0x0e,0x4f,0x6b,0x92,
		0xa6,0xba,0x03,0x3b,0x98,0xcc,0xed,0x4a,
		0x97,0x8f,0x5d,0xd2,0x27,0x29,0x25,0x19,
		0xa5,0xd5,0xf0,0x5d,0x5e,0x56,0x3d,0x0e
	};

	if (pSRMData == nullptr || cbSRMData == 0)
	{
		pSRMData = pbEmptySRM;
		cbSRMData = sizeof(pbEmptySRM);
	}

	if (cbSRMData < 4 || cbSRMData > ULONG_MAX)
		return E_INVALIDARG;

	// Common command parameters
	OPM_CONFIGURE_PARAMETERS Command;
	ZeroMemory(&Command, sizeof(Command));

	Command.guidSetting = OPM_SET_HDCP_SRM;
	Command.ulSequenceNumber = m_ulCommandSequence;
	Command.cbParametersSize = sizeof(OPM_SET_HDCP_SRM_PARAMETERS);
	OPM_SET_HDCP_SRM_PARAMETERS *pSRMParameter = (OPM_SET_HDCP_SRM_PARAMETERS*)&Command.abParameters[0];

	pSRMParameter->ulSRMVersion = (pSRMData[2] << 8) | pSRMData[3];

	//  Sign the command structure, not including the OMAC-1 field.
	hr = CryptUtil::ComputeOMAC1(
		m_keySession.abRandomNumber,
		(BYTE*)&Command + OPM_OMAC_SIZE,
		sizeof(OPM_CONFIGURE_PARAMETERS) - OPM_OMAC_SIZE,
		Command.omac.abOMAC);

	if (FAILED(hr))
	{
		goto done;
	}

	//  Send the command.
	hr = m_spOPMVideoOutput->Configure(&Command, (ULONG)cbSRMData, pSRMData);

	if (FAILED(hr))
	{
		LOG_WARNING(L"[OPM] Failed to configure OPM_SET_HDCP_SRM {hr: 0X%08X(%s)}.\n", hr, OPM_RESULT_NAME(hr));
		goto done;
	}

	AMP_SAFEASSIGN(pulHDCPSRMVer, (pSRMData[2] << 8) | pSRMData[3]);

	//  Update the sequence number.
	m_ulCommandSequence++;

done:
	return hr;
}

HRESULT COPMSession::SetACPAndCGMSASignaling(OPM_SET_ACP_AND_CGMSA_SIGNALING_PARAMETERS params)
{
	HRESULT hr = S_OK;
	
	// Common command parameters
	OPM_CONFIGURE_PARAMETERS Command;
	ZeroMemory(&Command, sizeof(Command));

	Command.guidSetting = OPM_SET_ACP_AND_CGMSA_SIGNALING;
	Command.ulSequenceNumber = m_ulCommandSequence;
	Command.cbParametersSize = sizeof(OPM_SET_ACP_AND_CGMSA_SIGNALING_PARAMETERS);
	OPM_SET_ACP_AND_CGMSA_SIGNALING_PARAMETERS* pSignaling = (OPM_SET_ACP_AND_CGMSA_SIGNALING_PARAMETERS*)&Command.abParameters[0];

	//--------------------------------------------------------------------
	// Prepare the command structure.
	//--------------------------------------------------------------------
	// Data specific to the OPM_SET_ACP_AND_CGMSA_SIGNALING command.
	CopyMemory(&pSignaling, (BYTE*)&params, Command.cbParametersSize);

	//  Sign the command structure, not including the OMAC-1 field.
	hr = CryptUtil::ComputeOMAC1(
		m_keySession.abRandomNumber,
		(BYTE*)&Command + OPM_OMAC_SIZE,
		sizeof(OPM_CONFIGURE_PARAMETERS) - OPM_OMAC_SIZE,
		Command.omac.abOMAC);

	if (FAILED(hr))
	{
		goto done;
	}

	//  Send the command.
	hr = m_spOPMVideoOutput->Configure(&Command, 0, NULL);

	if (FAILED(hr))
	{
		LOG_WARNING(L"[OPM] Failed to configure OPM_SET_ACP_AND_CGMSA_SIGNALING {hr: 0X%08X(%s)}.\n", hr, OPM_RESULT_NAME(hr));
		goto done;
	}

	//  Update the sequence number.
	m_ulCommandSequence++;

done:
	return hr;
}

HRESULT COPMSession::SetProtectionLevelAccordingToCSSDVD(ULONG ulProtectionType, ULONG ulProtectionLevel)
{
	HRESULT hr = S_OK;
	//--------------------------------------------------------------------
	// Prepare the command structure.
	//--------------------------------------------------------------------

	// Data specific to the OPM_SET_PROTECTION_LEVEL command.
	OPM_SET_PROTECTION_LEVEL_PARAMETERS CommandInput;

	ZeroMemory(&CommandInput, sizeof(CommandInput));

	CommandInput.ulProtectionType = ulProtectionType;
	CommandInput.ulProtectionLevel = ulProtectionLevel;

	// Common command parameters
	OPM_CONFIGURE_PARAMETERS Command;
	ZeroMemory(&Command, sizeof(Command));

	Command.guidSetting = OPM_SET_PROTECTION_LEVEL_ACCORDING_TO_CSS_DVD;
	Command.ulSequenceNumber = m_ulCommandSequence;
	Command.cbParametersSize = sizeof(OPM_SET_PROTECTION_LEVEL_PARAMETERS);
	CopyMemory(&Command.abParameters[0], (BYTE*)&CommandInput, Command.cbParametersSize);

	//  Sign the command structure, not including the OMAC-1 field.
	hr = CryptUtil::ComputeOMAC1(
		m_keySession.abRandomNumber,
		(BYTE*)&Command + OPM_OMAC_SIZE,
		sizeof(OPM_CONFIGURE_PARAMETERS) - OPM_OMAC_SIZE,
		Command.omac.abOMAC);

	if (FAILED(hr))
	{
		goto done;
	}

	//  Send the command.
	hr = m_spOPMVideoOutput->Configure(&Command, 0, NULL);

	if (FAILED(hr))
	{
		LOG_WARNING(L"[OPM] Failed to configure OPM_SET_PROTECTION_LEVEL_ACCORDING_TO_CSS_DVD {hr: 0X%08X(%s)}.\n", hr, OPM_RESULT_NAME(hr));
		goto done;
	}

	//  Update the sequence number.
	m_ulCommandSequence++;

done:
	return hr;
}

