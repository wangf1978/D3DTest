#pragma once

#include <Windows.h>
#include <stdint.h>

#define AES_BLOCKLEN				(16)
#define AES_KEYSIZE_128				(16)
#define OMAC1_SIZE					(16)

namespace CryptUtil
{
	using OMAC1 = uint8_t[16];
	using AESKEY128 = uint8_t[AES_KEYSIZE_128];

	/*! Generate OMAC1 signature using AES128 
		@param AesKey Session key
		@param pb data
		@param cb Size of the data
		@param pTag Receive the OMAC-1
	*/
	HRESULT ComputeOMAC1(AESKEY128 AesKey, PUCHAR pb, DWORD cb, OMAC1& Tag);

	/*! Validate X509 certification buffer
		@param pbCertificate The X.509 certification data buffer
		@param cbCertificate the data size of X.509 certification 
		@param pConfirmPublicKey the public key to be used for comparing with the root certification 
		@param cbConfirmPublicKey the data size of public key */
	BOOL	ValidateX509Certificate(
		uint8_t * pbCertificate, 
		size_t cbCertificate, 
		const uint8_t* pConfirmPublicKey=nullptr, 
		size_t cbConfirmPublicKey=0);

	/*! Generate the random */
	HRESULT GenerateRandom(LPBYTE pbBuffer, const DWORD dwBufferLen);

	/*!	@brief Validate the certification and get the public key from it */
	HRESULT ValidateAndGetPublicKey(
		const LPBYTE pbCert, 
		const DWORD dwCertLen, 
		BCRYPT_KEY_HANDLE &hPublicKey, 
		const uint8_t* pConfirmPublicKey = nullptr,
		size_t cbConfirmPublicKey = 0);

	HRESULT ValidateCertChainUpToTrustedRoot(
		const PCCERT_CONTEXT pLeafCertContext, 
		const HCERTSTORE hCertStore, 
		const uint8_t* pConfirmPublicKey = nullptr, 
		size_t cbConfirmPublicKey = 0);

	/*!	@brief RSAES-OAEP encryption by Public key */
	HRESULT EncryptRSAESOAEP(
		const LPBYTE pbData,
		const DWORD dwBufferLen,
		const DWORD dwCryptLen,
		const BCRYPT_KEY_HANDLE &hPublicKey);

	/*!	@brief RSAES-OAEP encryption with public key from the cert buffer */
	HRESULT EncryptRSAESOAEPWithPublicKey(
		const LPBYTE pbCert,
		const DWORD dwCertLen,
		const LPBYTE pbData,
		const DWORD dwBufferLen,
		const DWORD dwCryptLen,
		const uint8_t* pConfirmPublicKey = nullptr,
		size_t cbConfirmPublicKey = 0);
}
