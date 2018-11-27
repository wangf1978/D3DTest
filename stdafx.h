// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <memory.h>
#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <tuple>
#include <algorithm>
#include <vector>
#include <assert.h>
#include <tchar.h>
#include <stdint.h>
#include <inttypes.h>

extern int g_verbose_level;

#define VERBOSE_LOG_OUTPUT(fmtmsg, ...)	{\
	if (g_verbose_level > 0)\
		_tprintf(fmtmsg, ##__VA_ARGS__);\
}

#define LOG_OUTPUT(fmtmsg, ...)		_tprintf(fmtmsg, ##__VA_ARGS__)

#define LOG_DEBUG		VERBOSE_LOG_OUTPUT
#define LOG_INFO		VERBOSE_LOG_OUTPUT
#define LOG_WARNING		VERBOSE_LOG_OUTPUT
#define LOG_ERROR		LOG_OUTPUT

#ifdef _DEBUG
#define AMP_Assert( expr ) if( !(expr) ) { \
	_tprintf(_T("[AMPSDK] ** Assertion failed: \"%s\" at %s:%d %s()\n"), _T(#expr), _T(__FILE__), __LINE__, _T(__FUNCTION__) );\
	int* x = 0; *x = 0; \
	}

#define AMP_Verify( expr ) if( !(expr) ) { \
	_tprintf( _T("[AMPSDK] ** Verification failed: \"%s\" at %s:%d %s()\n"), _T(#expr), _T(__FILE__), __LINE__, _T(__FUNCTION__) );\
	int* x = 0; *x = 0; \
	}
#else
#define AMP_Assert( expr ) if( !(expr) ) { \
	_tprintf(_T("[AMPSDK] ** Assertion failed: \"%s\" at %s:%d %s()\n"), _T(#expr), _T(__FILE__), __LINE__, _T(__FUNCTION__) );\
	}

#define AMP_Verify( expr ) if( !(expr) ) { \
	_tprintf( _T("[AMPSDK] ** Verification failed: \"%s\" at %s:%d %s()\n"), _T(#expr), _T(__FILE__), __LINE__, _T(__FUNCTION__) );\
	}
#endif

#ifdef _WIN32
#define CODE_NOP1(p)						p
#else
#define CODE_NOP1(p)						(void)0
#endif

#define AMP_ABS(A)							((A) < 0 ? (-(A)) : (A))
#define AMP_ABS_MINUS(A, B)					((A)>=(B)?((A)-(B)):((B)-(A)))
#define AMP_MIN(A, B)						((A) <= (B)?(A):(B))
#define AMP_MAX(A, B)						((A) >= (B)?(A):(B))

#define AMP_SAFERELEASE(p)					if(p){p->Release();p = NULL;}CODE_NOP1(p)
#define AMP_SAFEASSIGN(p, v)				if(p){*(p) = (v);}CODE_NOP1(p)
#define AMP_SAFEASSIGN1(p, v)				if(p){*(p) = (v); if(v)v->ProcAddRef();}CODE_NOP1(p)

#define AMP_SAFEDEL(p)						if(p){delete p;p = NULL;}CODE_NOP1(p)
#define AMP_SAFEDELA(p)						if(p){delete [] p;p = NULL;}CODE_NOP1(p)

#ifndef UNREFERENCED_PARAMETER
#ifdef _WIN32
#define UNREFERENCED_PARAMETER(P)          (P)
#else
#define UNREFERENCED_PARAMETER(P)		   (void)0
#endif
#endif

#ifndef DBG_UNREFERENCED_PARAMETER
#ifdef _WIN32
#define DBG_UNREFERENCED_PARAMETER(P)      (P)
#else
#define DBG_UNREFERENCED_PARAMETER(P)      (void)0
#endif
#endif

#ifndef DBG_UNREFERENCED_LOCAL_VARIABLE
#ifdef _WIN32
#define DBG_UNREFERENCED_LOCAL_VARIABLE(V) (V)
#else
#define DBG_UNREFERENCED_LOCAL_VARIABLE(V) (void)0
#endif
#endif

#define RET_CODE_SUCCESS					0
#define RET_CODE_ERROR					   -1
#define RET_CODE_ERROR_NOTIMPL			   -2
#define RET_CODE_ERROR_FILE_NOT_EXIST	   -3
#define RET_CODE_ERROR_PERMISSION		   -6
#define RET_CODE_INVALID_PARAMETER		   -7
#define RET_CODE_OUTOFMEMORY			   -8

#define RET_CODE_NEEDMOREINPUT			   -1000
#define RET_CODE_NEEDBYTEALIGN			   -1001

#define RET_CODE_HEADER_LOST			   -2000			// Header information can't be retrieved.
#define RET_CODE_BUFFER_TOO_SMALL		   -2001			// Can't retrieve all information field of struct from the memory block
#define RET_CODE_BUFFER_NOT_COMPATIBLE	   -2002			// The loaded buffer is not compatible with spec.
#define RET_CODE_BUFFER_NOT_FOUND		   -2003
#define RET_CODE_ERROR_CRC				   -2004

#define RET_CODE_BOX_TOO_SMALL			   -2100			// ISO 14496-12 box size is too small, and can't unpack the information according to spec
#define RET_CODE_BOX_INCOMPATIBLE		   -2101			// the current stream is incompatible with ISO 14496-12

#define RET_CODE_CONTINUE					256
#define RET_CODE_UOP_COMPLETED				257
#define RET_CODE_ALREADY_EXIST				500
#define RET_CODE_NOTHING_TODO				501
#define RET_CODE_REQUIRE_MORE_MEM			502
#define RET_CODE_DELAY_APPLY				503
#define RET_CODE_PSR_OVERWRITE				504
#define RET_CODE_CONTINUE_NAVICMD			505

#define _MACRO_W(x)							L ## x

#define DECL_TUPLE_A(x)						{x, #x}
#define DECL_TUPLE_W(x)						{x, _MACRO_W(#x)}

#ifdef _UNICODE
#define DECL_TUPLE(x)						DECL_TUPLE_W(##x)
#else
#define DECL_TUPLE(x)						DECL_TUPLE_A(##x)
#endif

#define DECL_ENUM_ITEMW(x, l)				((x) == l? _MACRO_W(#l):
#define DECL_ENUM_LAST_ITEMW(x, l, v)		((x) == l? _MACRO_W(#l):v

#define DECL_ENUM_ITEMA(x, l)				((x) == l? #l:
#define DECL_ENUM_LAST_ITEMA(x, l, v)		((x) == l? #l:v

enum FLAG_VALUE
{
	FLAG_UNSET = 0,
	FLAG_SET = 1,
	FLAG_UNKNOWN = 2,
};

using FLAG_NAME_PAIRW = const std::tuple<int, const wchar_t*>;
using FLAG_NAME_PAIRA = const std::tuple<int, const char*>;

inline static void GetFlagsDescW(int nFlags, FLAG_NAME_PAIRW* flag_names, size_t flag_count, wchar_t* szDesc, int ccDesc, const wchar_t* szZeroFlagStr=L"")
{
	bool bFirst = true;
	int ccWritten = 0;
	memset(szDesc, 0, ccDesc);
	for (size_t i = 0; i < flag_count; i++)
	{
		if (nFlags&(std::get<0>(flag_names[i])))
		{
			ccWritten += swprintf_s(szDesc + ccWritten, ccDesc - ccWritten, L"%s%s", !bFirst ? L" | " : L"", std::get<1>(flag_names[i]));
			bFirst = false;
		}
	}

	if (szDesc[0] == L'\0')
		wcscpy_s(szDesc, ccDesc, szZeroFlagStr);
}

inline static void GetFlagsDescA(int nFlags, FLAG_NAME_PAIRA* flag_names, size_t flag_count, char* szDesc, int ccDesc, const char* szZeroFlagStr = "")
{
	bool bFirst = true;
	int ccWritten = 0;
	memset(szDesc, 0, ccDesc);
	for (size_t i = 0; i < flag_count; i++)
	{
		if (nFlags&(std::get<0>(flag_names[i])))
		{
			ccWritten += sprintf_s(szDesc + ccWritten, ccDesc - ccWritten, "%s%s", !bFirst ? " | " : "", std::get<1>(flag_names[i]));
			bFirst = false;
		}
	}

	if (szDesc[0] == L'\0')
		strcpy_s(szDesc, ccDesc, szZeroFlagStr);
}

#ifdef _UNICODE
#define FLAG_NAME_PAIR						FLAG_NAME_PAIRW
#define GetFlagsDesc						GetFlagsDescW
#else
#define FLAG_NAME_PAIR						FLAG_NAME_PAIRA
#define GetFlagsDesc						GetFlagsDescA
#endif

// TODO: reference additional headers your program requires here
