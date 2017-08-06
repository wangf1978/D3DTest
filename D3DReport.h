#pragma once

#include <d3d11.h>
#include <d3d10.h>
#include <d3d10_1.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <vector>
#include "tinyxml\tinyxml.h"
#include <tuple>
#include <iostream>
#include <sstream>
#include <stdio.h>

#define DECL_TUPLE(x)	{x, #x}

template<typename T>
inline std::string GetBitsString(const std::tuple<T, const char*>* bit_names, size_t bits_count, T bits_value)
{
	bool bFirst = true;
	std::ostringstream oss;
	for (int idxBit = 0; idxBit < bits_count; idxBit++)
	{
		if (std::get<0>(bit_names[idxBit])&bits_value)
		{
			oss << (bFirst ? "" : ", ") << std::get<1>(bit_names[idxBit]);
			if (bFirst)
				bFirst = false;
		}
	}
	oss << std::ends;
	return oss.str();
}

class CD3DReport
{
public:
	CD3DReport();
	virtual ~CD3DReport();

public:
	/*!	@brief Generate the report */
	HRESULT			Generate();
	/*!	@brief return a UTF-8 XML string for whole D3D report*/
	const char*		GetReport();
	/**/
	bool			SaveToFile(const char* str_file_name);

private:
	TiXmlPrinter	m_xmlPrinter;
	TiXmlDocument	m_xmlDoc;

private:
	HRESULT			GenerateAdaptorReport(IDXGIAdapter* pAdater, TiXmlElement* pAdaptorElement);
	HRESULT			GenerateOutputReport(IDXGIOutput* pOutput, TiXmlElement* pOutputElement);
	HRESULT			GenerateVideoDeviceReport(ID3D11VideoDevice* pVideoDevice, TiXmlElement* pVDElement);
};

