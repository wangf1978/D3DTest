#include "stdafx.h"
#include "CryptUtil.h"
#include <new>

extern int g_verbose_level;

namespace CryptUtil
{
	// Helper functions for some bitwise operations.
	inline void XOR(
		BYTE *lpbLHS,
		const BYTE *lpbRHS,
		DWORD cbSize = AES_BLOCKLEN
	)
	{
		for (DWORD i = 0; i < cbSize; i++)
		{
			lpbLHS[i] ^= lpbRHS[i];
		}
	}

	inline void LShift(const BYTE *lpbOpd, BYTE *lpbRes)
	{
		for (DWORD i = 0; i < AES_BLOCKLEN; i++)
		{
			lpbRes[i] = lpbOpd[i] << 1;
			if (i < AES_BLOCKLEN - 1)
			{
				lpbRes[i] |= ((unsigned char)lpbOpd[i + 1]) >> 7;
			}
		}
	}

	HRESULT ComputeOMAC1(AESKEY128 AesKey, PUCHAR pb, DWORD cb, OMAC1& Tag)
	{
		HRESULT hr = S_OK;
		BCRYPT_ALG_HANDLE hAlg = NULL;
		BCRYPT_KEY_HANDLE hKey = NULL;
		DWORD cbKeyObject = 0;
		DWORD cbData = 0;
		PBYTE pbKeyObject = NULL;

		struct
		{
			BCRYPT_KEY_DATA_BLOB_HEADER Header;
			UCHAR Key[AES_KEYSIZE_128];
		} KeyBlob;

		KeyBlob.Header.dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
		KeyBlob.Header.dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
		KeyBlob.Header.cbKeyData = AES_KEYSIZE_128;
		CopyMemory(KeyBlob.Key, AesKey, sizeof(KeyBlob.Key));

		BYTE rgbLU[OMAC1_SIZE];
		BYTE rgbLU_1[OMAC1_SIZE];
		BYTE rBuffer[OMAC1_SIZE];

		hr = BCryptOpenAlgorithmProvider(
			&hAlg,
			BCRYPT_AES_ALGORITHM,
			MS_PRIMITIVE_PROVIDER,
			0
		);

		//  Get the size needed for the key data
		if (S_OK == hr)
		{
			hr = BCryptGetProperty(
				hAlg,
				BCRYPT_OBJECT_LENGTH,
				(PBYTE)&cbKeyObject,
				sizeof(DWORD),
				&cbData,
				0
			);
		}

		//  Allocate the key data object
		if (S_OK == hr)
		{
			pbKeyObject = new (std::nothrow) BYTE[cbKeyObject];
			if (NULL == pbKeyObject)
			{
				hr = E_OUTOFMEMORY;
			}
		}

		//  Set to CBC chain mode
		if (S_OK == hr)
		{
			hr = BCryptSetProperty(
				hAlg,
				BCRYPT_CHAINING_MODE,
				(PBYTE)BCRYPT_CHAIN_MODE_CBC,
				sizeof(BCRYPT_CHAIN_MODE_CBC),
				0
			);
		}

		//  Set the key
		if (S_OK == hr)
		{
			hr = BCryptImportKey(hAlg, NULL, BCRYPT_KEY_DATA_BLOB, &hKey,
				pbKeyObject, cbKeyObject, (PUCHAR)&KeyBlob, sizeof(KeyBlob), 0);
		}

		//  Encrypt 0s
		if (S_OK == hr)
		{
			DWORD cbBuffer = sizeof(rBuffer);
			ZeroMemory(rBuffer, sizeof(rBuffer));

			hr = BCryptEncrypt(hKey, rBuffer, cbBuffer, NULL, NULL, 0,
				rBuffer, sizeof(rBuffer), &cbBuffer, 0);
		}

		//  Compute OMAC1 parameters
		if (S_OK == hr)
		{
			const BYTE bLU_ComputationConstant = 0x87;
			LPBYTE pbL = rBuffer;

			LShift(pbL, rgbLU);
			if (pbL[0] & 0x80)
			{
				rgbLU[OMAC1_SIZE - 1] ^= bLU_ComputationConstant;
			}
			LShift(rgbLU, rgbLU_1);
			if (rgbLU[0] & 0x80)
			{
				rgbLU_1[OMAC1_SIZE - 1] ^= bLU_ComputationConstant;
			}
		}

		//  Generate the hash. 
		if (S_OK == hr)
		{
			// Redo the key to restart the CBC.

			BCryptDestroyKey(hKey);
			hKey = NULL;

			hr = BCryptImportKey(hAlg, NULL, BCRYPT_KEY_DATA_BLOB, &hKey,
				pbKeyObject, cbKeyObject, (PUCHAR)&KeyBlob, sizeof(KeyBlob), 0);
		}

		if (S_OK == hr)
		{
			PUCHAR pbDataInCur = pb;
			cbData = cb;
			do
			{
				DWORD cbBuffer = 0;

				if (cbData > OMAC1_SIZE)
				{
					CopyMemory(rBuffer, pbDataInCur, OMAC1_SIZE);

					hr = BCryptEncrypt(hKey, rBuffer, sizeof(rBuffer), NULL,
						NULL, 0, rBuffer, sizeof(rBuffer), &cbBuffer, 0);

					pbDataInCur += OMAC1_SIZE;
					cbData -= OMAC1_SIZE;
				}
				else
				{
					if (cbData == OMAC1_SIZE)
					{
						CopyMemory(rBuffer, pbDataInCur, OMAC1_SIZE);
						XOR(rBuffer, rgbLU);
					}
					else
					{
						ZeroMemory(rBuffer, OMAC1_SIZE);
						CopyMemory(rBuffer, pbDataInCur, cbData);
						rBuffer[cbData] = 0x80;

						XOR(rBuffer, rgbLU_1);
					}

					hr = BCryptEncrypt(hKey, rBuffer, sizeof(rBuffer), NULL, NULL,
						0, (PUCHAR)Tag, OMAC1_SIZE, &cbBuffer, 0);

					cbData = 0;
				}

			} while (S_OK == hr && cbData > 0);
		}

		//  Clean up
		if (hKey)
		{
			BCryptDestroyKey(hKey);
		}
		if (hAlg)
		{
			BCryptCloseAlgorithmProvider(hAlg, 0);
		}
		delete[] pbKeyObject;
		return hr;
	}

	BOOL ValidateX509Certificate(uint8_t* pbCertificate, size_t cbCertificate, const uint8_t* pConfirmPublicKey, size_t cbConfirmPublicKey)
	{
		HRESULT hr = E_FAIL;
		DWORD dwError = 0;
		PCCERT_CONTEXT  pCert = NULL;

		if (cbConfirmPublicKey > UINT32_MAX)
			return FALSE;

		CRYPT_DATA_BLOB dataBlob = { (DWORD)cbCertificate, pbCertificate };
		HCERTSTORE hCertStore = CertOpenStore(
			CERT_STORE_PROV_PKCS7,					// store provider type
			PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,// encoding type
			NULL,									// A handle to a cryptographic provider
			CERT_STORE_READONLY_FLAG,				// Open Read-only
			(LPCVOID)&dataBlob						// Certificate
		);

		do
		{
			if ((pCert = CertEnumCertificatesInStore(hCertStore, pCert)) == nullptr)
				break;

			PCCERT_CONTEXT pcIssuer = NULL;
			PCCERT_CONTEXT pcSubject = CertDuplicateCertificateContext(pCert);
			for (; NULL != pcSubject;)
			{
				DWORD dwFlags = 0;
				BOOL bret = TRUE;
				hr = S_OK;

				pcIssuer = CertGetIssuerCertificateFromStore(hCertStore, pcSubject, nullptr, &dwFlags);

				if (pcIssuer == nullptr)
				{
					dwError = GetLastError();
					if (CRYPT_E_SELF_SIGNED != dwError)
					{
						hr = E_FAIL;
						break;
					}
					else
					{
						if ((bret = CertCompareCertificateName(X509_ASN_ENCODING, &pcSubject->pCertInfo->Subject, &pcSubject->pCertInfo->Issuer)) == FALSE)
						{
							hr = E_FAIL;
							break;
						}
					}
				}

				HCRYPTPROV hprov = NULL;
				bret = CryptAcquireContext(&hprov, nullptr, nullptr, PROV_RSA_FULL, CRYPT_SILENT | CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET);

				if (FALSE == bret)
				{
					dwError = GetLastError();
					if (NTE_EXISTS == dwError)
						bret = CryptAcquireContext(&hprov, nullptr, nullptr, PROV_RSA_FULL, CRYPT_SILENT | CRYPT_VERIFYCONTEXT);
					if (FALSE == bret)
					{
						hr = E_FAIL;
						break;
					}
				}

				if ((bret = CryptVerifyCertificateSignatureEx(hprov, X509_ASN_ENCODING,
					CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT, (void*)pcSubject,
					CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT, (void*)(pcIssuer == nullptr ? pcSubject : pcIssuer),
					0, nullptr)) == FALSE)
				{
					hr = E_FAIL;
					break;
				}

				if (nullptr == pcIssuer && pConfirmPublicKey != nullptr && cbConfirmPublicKey > 0)
				{
					CERT_PUBLIC_KEY_INFO msCert;
					msCert.Algorithm = pcSubject->pCertInfo->SubjectPublicKeyInfo.Algorithm;
					msCert.PublicKey.cbData = (DWORD)cbConfirmPublicKey;
					msCert.PublicKey.pbData = (BYTE*)pConfirmPublicKey;
					msCert.PublicKey.cUnusedBits = pcSubject->pCertInfo->SubjectPublicKeyInfo.PublicKey.cUnusedBits;

					if (FALSE == CertComparePublicKeyInfo(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, &pcSubject->pCertInfo->SubjectPublicKeyInfo, &msCert))
					{
						hr = E_FAIL;
						break;
					}
				}

				bret = CryptReleaseContext(hprov, 0);
				hprov = NULL;

				CertFreeCertificateContext(pcSubject);
				pcSubject = pcIssuer;
				pcIssuer = NULL;
			}

			if (pcIssuer != NULL)
				CertFreeCertificateContext(pcIssuer);

			if (pcSubject != NULL)
				CertFreeCertificateContext(pcSubject);

			if (FAILED(hr))
			{
				CertFreeCertificateContext(pCert);
				break;
			}

		} while (pCert != nullptr);

		CertCloseStore(hCertStore, 0);

		if (g_verbose_level > 0)
		{
			if (FAILED(hr))
				wprintf(L"Failed to verify X509 certification.\n");
			else
				wprintf(L"Successfully verify X509 certification.\n");
		}

		return FAILED(hr) ? false : true;
	}

	HRESULT CryptUtil::GenerateRandom(LPBYTE pbBuffer, const DWORD dwBufferLen)
	{
		if (NULL == pbBuffer)
		{
			wprintf(L"[CryptUtil] NULL pointer.\n");
			return E_POINTER;
		}

		if (0 == dwBufferLen)
		{
			wprintf(L"[CryptUtil] Invalid BufferLength. length = %d\n", dwBufferLen);
			return E_INVALIDARG;
		}

		//! Get Cryptographic Provider
		BOOL bRet = FALSE;
		HCRYPTPROV hCryptProv = NULL;
		bRet = CryptAcquireContext(
			&hCryptProv,   // Receives a handle to the CSP.
			NULL,          // Use the default key container.
			NULL,          // Use the default CSP.
			PROV_RSA_AES,  // Use the AES provider (public-key algorithm).
			CRYPT_SILENT | CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET // not display any user interface / Creates a new key container
		);

		if (FALSE == bRet)
		{
			DWORD dwError = GetLastError();
			// If the default key container already exists (error code is NTE_EXISTS),
			// CRYPT_NEWKEYSET set to No flag (no new keys) and try again.
			if (dwError == NTE_EXISTS)
			{
				wprintf(L"[CryptUtil] CryptAcquireContext(new key set) failed( a key exists already).\n");

				bRet = CryptAcquireContext(
					&hCryptProv,	// Receives a handle to the CSP.
					NULL,			// Use the default key container.
					NULL,			// Use the default CSP.
					PROV_RSA_AES,	// Use the AES provider (public-key algorithm).
					CRYPT_SILENT | CRYPT_VERIFYCONTEXT // not display any user interface
				);
				if (FALSE == bRet)
				{
					dwError = GetLastError();
					HRESULT hr = HRESULT_FROM_WIN32(dwError);
					if (SUCCEEDED(hr))
					{
						hr = E_FAIL;
					}
					wprintf(L"[CryptUtil] CryptAcquireContext() failed. GetLastError() = 0x%08x\n", dwError);

					return hr;
				}
			}
			else
			{
				// Guest User cannot get handle
				HRESULT hr = HRESULT_FROM_WIN32(dwError);
				if (SUCCEEDED(hr))
				{
					hr = E_FAIL;
				}
				wprintf(L"[CryptUtil] CryptAcquireContext(new key set) failed. GetLastError() = 0x%08x\n", dwError);

				return hr;
			}
		}

		//! Generate the random
		HRESULT hr = S_OK;
		bRet = CryptGenRandom(
			hCryptProv,	// handle of a CSP.
			dwBufferLen,// Number of bytes of random data to be generated
			pbBuffer	// Buffer to receive the returned data
		);
		if (FALSE == bRet)
		{
			DWORD dwError = GetLastError();
			wprintf(L"[CryptUtil] CryptGenRandom() failed. GetLastError() = 0x%08x\n", dwError);
			hr = HRESULT_FROM_WIN32(dwError);
			if (SUCCEEDED(hr))
			{
				hr = E_FAIL;
			}
		}

		//! Release crypt context
		bRet = CryptReleaseContext(hCryptProv, 0);
		if (FALSE == bRet)
		{
			DWORD dwError = GetLastError();
			wprintf(L"[CryptUtil] CryptReleaseContext() failed. GetLastError() = 0x%08x\n", dwError);
			hr = HRESULT_FROM_WIN32(dwError);
			if (SUCCEEDED(hr))
			{
				hr = E_FAIL;
			}
		}

		return hr;
	}

	HRESULT ValidateAndGetPublicKey(const LPBYTE pbCert, const DWORD dwCertLen, BCRYPT_KEY_HANDLE &hPublicKey, const uint8_t* pConfirmPublicKey, size_t cbConfirmPublicKey)
	{
		if (NULL == pbCert)
		{
			wprintf(L"NULL pointer.\n");
			return E_POINTER;
		}

		if (0 == dwCertLen)
		{
			wprintf(L"Invalid cert length. length = %d\n", dwCertLen);
			return E_INVALIDARG;
		}

		CRYPT_DATA_BLOB dataBlob = {
			dwCertLen,
			pbCert };

		HCERTSTORE hCertStore = NULL;
		hCertStore = CertOpenStore(
			CERT_STORE_PROV_PKCS7,					// store provider type
			PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,// encoding type
			NULL,									// A handle to a cryptographic provider
			CERT_STORE_READONLY_FLAG,				// Open Read-only
			(LPCVOID)&dataBlob						// Certificate
		);
		if (!hCertStore)
		{
			wprintf(L"CertOpenStore() failed. GetLastError() = 0x%08x\n", GetLastError());
			return E_FAIL;
		}

		PCCERT_CONTEXT  pCert = NULL;
		PCCERT_CONTEXT  pCertNext = NULL;
		HRESULT hr = S_OK;
		BOOL bRet = FALSE;
		DWORD dwError = ERROR_SUCCESS;
		for (;;)
		{
			pCertNext = CertEnumCertificatesInStore(
				hCertStore,
				pCertNext
			);

			if (NULL == pCertNext)
			{
				break;
			}

			// If you hold the previous certificate, the release will always succeed, so you don't need to check the return value.
			if (pCert)
			{
				CertFreeCertificateContext(pCert);
			}

			// If you pass the certificate to the second argument of CertEnumCertificatesInStore, it is opened and it is duplicated.
			pCert = CertDuplicateCertificateContext(pCertNext);

			//! Determine if the certificate chain from the acquired certificate goes to the root certificate with the MS public key
			hr = CryptUtil::ValidateCertChainUpToTrustedRoot(
				pCert,
				hCertStore,
				pConfirmPublicKey,
				cbConfirmPublicKey
			);

			if (FAILED(hr))
			{
				wprintf(L"validateCertChainUpToTrustedRoot() failed. hr = 0x%08x\n", hr);

				//! Driver Certificate Release
				// CertFreeCertificateContext always returns success
				CertFreeCertificateContext(pCertNext);
				CertFreeCertificateContext(pCert);
				bRet = CertCloseStore(hCertStore, 0);
				if (FALSE == bRet)
				{
					dwError = GetLastError();
					wprintf(L"CertCloseStore() failed. GetLastError() = 0x%08x\n", dwError);
				}
				return hr;
			}
		}

		// Exit certificate store when certificate acquisition fails
		if (NULL == pCert)
		{
			dwError = GetLastError();
			wprintf(L"CertEnumCertificatesInStore() failed. GetLastError() = 0x%08x\n", dwError);
			bRet = CertCloseStore(hCertStore, 0);
			if (FALSE == bRet)
			{
				wprintf(L"CertCloseStore() failed. GetLastError() = 0x%08x\n", GetLastError());
			}
			hr = HRESULT_FROM_WIN32(dwError);
			if (SUCCEEDED(hr))
			{
				hr = E_FAIL;
			}
			return hr;
		}

		//! Load the public key of the driver certificate
		BCRYPT_KEY_HANDLE hbcKey = NULL;
		bRet = CryptImportPublicKeyInfoEx2(
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,	// The certificate encoding type that was used to encrypt the subject.
			&(pCert->pCertInfo->SubjectPublicKeyInfo),	// The address of a CERT_PUBLIC_KEY_INFO structure that contains the public key information to import into the provider.
			0,											// A set of flags that modify the behavior of this function
			NULL,										// This parameter is reserved for future use
			&hPublicKey									// The address of a BCRYPT_KEY_HANDLE variable that receives the handle of the imported key.
		);
		if (FALSE == bRet)
		{
			dwError = GetLastError();
			wprintf(L"CryptImportPublicKeyInfoEx2() failed. GetLastError() = 0x%08x\n", dwError);
			hr = HRESULT_FROM_WIN32(dwError);
			if (SUCCEEDED(hr))
			{
				hr = E_FAIL;
			}
			// The rest is only for the liberation process continue as this
		}

		//! Driver Certificate Release
		// CertFreeCertificateContext always returns success
		CertFreeCertificateContext(pCert);
		pCert = NULL;

		//! Certificate store close
		bRet = CertCloseStore(hCertStore, 0);
		hCertStore = NULL;
		if (FALSE == bRet)
		{
			dwError = GetLastError();
			wprintf(L"CertCloseStore() failed. GetLastError() = 0x%08x\n", dwError);
			hr = HRESULT_FROM_WIN32(dwError);
			if (SUCCEEDED(hr))
			{
				hr = E_FAIL;
			}
		}

		return hr;
	}

	HRESULT ValidateCertChainUpToTrustedRoot(const PCCERT_CONTEXT pLeafCertContext, const HCERTSTORE hCertStore, const uint8_t* pConfirmPublicKey, size_t cbConfirmPublicKey)
	{
		if ((NULL == pLeafCertContext) || (NULL == hCertStore))
		{
			wprintf(L"Parameter is NULL.\n");
			return E_POINTER;
		}

		// Duplicate the certificate context for the leaf certificate.
		PCCERT_CONTEXT pcSubject = CertDuplicateCertificateContext(pLeafCertContext);

		//! Recursively locate the certificate issuer certificate and loop it until it is no longer found
		HRESULT hr = S_OK;

		for (; NULL != pcSubject;)
		{
			DWORD   dwError = 0;
			BOOL    bret = TRUE;

			//! Find the issuer of the certificate you are currently looking at.
			PCCERT_CONTEXT  pcIssuer = NULL;
			DWORD           dwFlags = 0;
			pcIssuer = CertGetIssuerCertificateFromStore(
				hCertStore,	// Handle of a certificate store
				pcSubject,	// Pointer to a CERT_CONTEXT structure that contains the subject information
				NULL,		// (optional)Pointer to a CERT_CONTEXT structure that contains the issuer information. 
				&dwFlags	// flags enable verification checks on the returned certificate.
			);
			if (NULL == pcIssuer)
			{
				dwError = GetLastError();
				hr = HRESULT_FROM_WIN32(dwError);
				if (SUCCEEDED(hr))
				{
					hr = E_FAIL;
				}

				if (g_verbose_level > 0)
				{
					if (CRYPT_E_SELF_SIGNED == dwError)
					{
						// Self-signed
						wprintf(L"CertGetIssuerCertificateFromStore() failed. GetLastError() = 0x%08x(CRYPT_E_SELF_SIGNED)\n", dwError);
					}
					else
					{
						// Non-self-signed error code
						wprintf(L"CertGetIssuerCertificateFromStore() failed. GetLastError() = 0x%08x\n", dwError);
					}
				}
			}

			/* If the issuer is not found,
			 * The error code is crypt_e_self_signed
			 * If the Subject and Issuer of the certificate match
			 * Withdraw the error as a root certificate */
			if (FAILED(hr) && CRYPT_E_SELF_SIGNED == dwError)
			{
				hr = S_OK;
				bret = CertCompareCertificateName(
					X509_ASN_ENCODING,				// Specifies the encoding type used.
					&pcSubject->pCertInfo->Subject,	// Pointer to a CERT_NAME_BLOB for the first name in the comparison.
					&pcSubject->pCertInfo->Issuer	// Pointer to a CERT_NAME_BLOB for the second name in the comparison.
				);
				if (FALSE == bret)
				{
					dwError = ::GetLastError();
					hr = ::HRESULT_FROM_WIN32(dwError);
					if (SUCCEEDED(hr))
					{
						hr = E_FAIL;
					}
					wprintf(L"CertCompareCertificateName() failed. GetLastError() = 0x%08x\n", dwError);
				}
			}

			//! Get Cryptographic Provider
			HCRYPTPROV hprov = NULL;
			if (SUCCEEDED(hr))
			{
				bret = CryptAcquireContext(
					&hprov,							// Receives a handle to the CSP.
					NULL,							// Use the default key container.
					NULL,							// Use the default CSP.
					PROV_RSA_FULL,					// Use the RSA provider
					CRYPT_SILENT | CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET	// not display any user interface / Creates a new key container
				);
				if (FALSE == bret)
				{
					dwError = GetLastError();
					hr = HRESULT_FROM_WIN32(dwError);
					if (SUCCEEDED(hr))
					{
						hr = E_FAIL;
					}
					if (NTE_EXISTS != dwError)
					{
						wprintf(L"CryptAcquireContext() failed. GetLastError() = 0x%08x\n", dwError);
					}
				}
			}

			// If the default key container already exists (error code is nte_exists), Set the CRYPT_NEWKEYSET flag without a new key and try again.
			if (FAILED(hr) && NTE_EXISTS == dwError)
			{
				hr = S_OK;
				bret = CryptAcquireContext(
					&hprov,			// Receives a handle to the CSP.
					NULL,			// Use the default key container.
					NULL,			// Use the default CSP.
					PROV_RSA_FULL,	// Use the RSA provider
					CRYPT_SILENT | CRYPT_VERIFYCONTEXT 	// not display any user interface 
				);

				if (FALSE == bret)
				{
					dwError = GetLastError();
					hr = HRESULT_FROM_WIN32(dwError);
					if (SUCCEEDED(hr))
					{
						hr = E_FAIL;
					}
					wprintf(L"CryptAcquireContext() failed. GetLastError() = 0x%08x\n", dwError);
				}
			}

			/* Verify the signature of the certificate using the issuer's public key.
			* If you have a root certificate, designate yourself as the issuer. */
			if (SUCCEEDED(hr))
			{
				if (NULL != pcIssuer)
				{// Not self-signed
					bret = CryptVerifyCertificateSignatureEx(
						hprov,								// handle to the CSP.
						X509_ASN_ENCODING,					// The certificate encoding type that was used to encrypt the subject.
						CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT,// The subject type.
						(void*)pcSubject,					// Certificate currently being validated
						CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT,// The issuer type.
						(void*)pcIssuer,					// Signer of the certificate currently being validated
						0,									// reserved
						NULL								// reserved
					);
				}
				else
				{// self-signed
					bret = CryptVerifyCertificateSignatureEx(
						hprov,								// handle to the CSP.
						X509_ASN_ENCODING,					// The certificate encoding type that was used to encrypt the subject.
						CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT,// The subject type.
						(void*)pcSubject,					// Certificate currently being validated
						CRYPT_VERIFY_CERT_SIGN_SUBJECT_CERT,// The issuer type.
						(void*)pcSubject,					// Certificate currently being validated (pass the certificate itself because it is self-signed)
						0,								    // reserved
						NULL								// reserved
					);
				}
				if (FALSE == bret)
				{
					dwError = GetLastError();
					hr = HRESULT_FROM_WIN32(dwError);
					if (SUCCEEDED(hr))
					{
						hr = E_FAIL;
					}
					wprintf(L"CryptVerifyCertificateSignatureEx() failed. GetLastError() = 0x%08x\n", dwError);
				}
			}

			//! Check the MS Public key if you are looking at the root certificate
			if (SUCCEEDED(hr))
			{
				if (NULL == pcIssuer && cbConfirmPublicKey > 0 && pConfirmPublicKey != nullptr)
				{
					CERT_PUBLIC_KEY_INFO msCert;
					msCert.Algorithm = pcSubject->pCertInfo->SubjectPublicKeyInfo.Algorithm;
					msCert.PublicKey.cbData = (DWORD)cbConfirmPublicKey;
					msCert.PublicKey.pbData = (BYTE*)pConfirmPublicKey;
					msCert.PublicKey.cUnusedBits = pcSubject->pCertInfo->SubjectPublicKeyInfo.PublicKey.cUnusedBits;
					if (FALSE == CertComparePublicKeyInfo(
						X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,	// Specifies the encoding type used.
						&pcSubject->pCertInfo->SubjectPublicKeyInfo,// Pointer to the CERT_PUBLIC_KEY_INFO for the first public key in the comparison.
						&msCert))									// Pointer to the CERT_PUBLIC_KEY_INFO for the second public key in the comparison.
					{
						// Public Key inconsistency
						dwError = GetLastError();
						hr = HRESULT_FROM_WIN32(dwError);
						if (SUCCEEDED(hr))
						{
							hr = E_FAIL;
						}

						LOG_WARNING(L"Public key and trusted public key mismatch.\n");
					}
				}
			}

			//! Release a temporary Crypt provider
			bret = CryptReleaseContext(hprov, 0);
			hprov = NULL;
			if (FALSE == bret)
			{
				dwError = GetLastError();
				hr = HRESULT_FROM_WIN32(dwError);
				if (SUCCEEDED(hr))
				{
					hr = E_FAIL;
				}
				wprintf(L"CryptReleaseContext() failed. GetLastError() = 0x%08x\n", dwError);
			}

			//! Release the certificate context that was inspected
			CertFreeCertificateContext(pcSubject);

			//! Make the certificate context that I found this time the next inspection target
			// Issuer's context is used as a Subject, so don't release it.
			pcSubject = pcIssuer;

			// Exit the loop if the error was in the middle
			if (FAILED(hr))
			{
				if (pcIssuer)
				{
					CertFreeCertificateContext(pcIssuer);
					pcIssuer = NULL;
				}
				break;
			}
		}

		return hr;
	}

	HRESULT EncryptRSAESOAEP(const LPBYTE pbData, const DWORD dwBufferLen, const DWORD dwCryptLen, const BCRYPT_KEY_HANDLE &hPublicKey)
	{
		if (NULL == pbData)
		{
			wprintf(L"NULL pointer.\n");
			return E_POINTER;
		}

		if (0 == dwCryptLen)
		{
			wprintf(L"Invalid CryptLength. length = %d\n", dwCryptLen);
			return E_INVALIDARG;
		}

		if (dwCryptLen > dwBufferLen)
		{
			wprintf(L"CryptLength greater than BufferLength. CryptLength = %d, BufferLength = %d\n", dwCryptLen, dwBufferLen);
			return E_INVALIDARG;
		}
		/*! RSAES-OAEP encryption
		* RSAES-OAEP Parameters
		* Hash
		*  SHA-512
		* MGF
		*  MGF1 (Defined on page 48 in PKCS #1 v2.1: RSA Cryptography Standard)
		*  L is always the empty string.
		*/
		BCRYPT_OAEP_PADDING_INFO padInfo = {
			BCRYPT_SHA512_ALGORITHM,	// cryptographic algorithm to use to create the padding.
			NULL,						// The address of a buffer that contains the data to use to create the padding.
			0							// Contains the number of bytes in the buffer to use to create the padding.
		};

		HRESULT hr = S_OK;
		ULONG result = 0;

		hr = HRESULT_FROM_NT(
			BCryptEncrypt(
				hPublicKey,			// cryptographic keys
				pbData,				// PlainText Storage buffer
				dwCryptLen,			// PlainText buffer length
				(LPVOID)&padInfo,	// padding information
				NULL,				// Initialization vectors
				0,					// Initialization vector Size
				pbData,				// Encrypted statement Storage buffer
				dwBufferLen,		// Cipher text buffer length
				&result,			// The Output cipher buffer length
				BCRYPT_PAD_OAEP		// Additional flags
			)
		);
		if (FAILED(hr))
		{
			wprintf(L"BCryptEncrypt failed. hr = 0x%08x\n", hr);
		}
		return hr;
	}

	// RSAES-OAEP encryption by Public key
	HRESULT EncryptRSAESOAEPWithPublicKey(
		const LPBYTE pbCert,
		const DWORD dwCertLen,
		const LPBYTE pbData,
		const DWORD dwBufferLen,
		const DWORD dwCryptLen,
		const uint8_t* pConfirmPublicKey,
		size_t cbConfirmPublicKey)
	{
		if ((NULL == pbCert) || (NULL == pbData))
		{
			wprintf(L"NULL pointer.\n");
			return E_POINTER;
		}

		if ((0 == dwCertLen) || (0 == dwBufferLen) || (0 == dwCryptLen))
		{
			wprintf(L"Invalid length. cert length = %d, buffer length = %d, crypt length = %d\n",
				dwCertLen, dwBufferLen, dwCryptLen);
			return E_INVALIDARG;
		}

		if (dwCryptLen > dwBufferLen)
		{
			wprintf(L"CryptLength greater than BufferLength. CryptLength = %d, BufferLength = %d",
				dwCryptLen, dwBufferLen);
			return E_INVALIDARG;
		}

		//! Get the public key
		BCRYPT_KEY_HANDLE hPublicKey = NULL;
		HRESULT hr = S_OK;

		hr = CryptUtil::ValidateAndGetPublicKey(
			pbCert,
			dwCertLen,
			hPublicKey,
			pConfirmPublicKey,
			cbConfirmPublicKey);

		if (FAILED(hr))
		{
			wprintf(L"getPublicKey() failed. hr = 0x%08x\n", hr);
			return hr;
		}

		//! Encrypted with RSAES-OAEP
		// Error checking is carried out after key release
		hr = EncryptRSAESOAEP(
			pbData,
			dwBufferLen,
			dwCryptLen,
			hPublicKey
		);

		//! Open Public key
		NTSTATUS status = 0;
		status = BCryptDestroyKey(hPublicKey);
		if (FAILED(hr))
		{
			wprintf(L"doRSAESOAEP() failed. hr = 0x%08x\n", hr);
			return hr;
		}

		// Public key release Success decision
		hr = HRESULT_FROM_NT(status);
		if (FAILED(hr))
		{
			wprintf(L"BCryptDestroyKey() failed. hr = 0x%08x\n", hr);
			return hr;
		}

		return S_OK;
	}
}