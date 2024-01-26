/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM py_test_component.idl
 */

#ifndef __gen_py_test_component_h__
#define __gen_py_test_component_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

#ifndef __gen_nsIVariant_h__
#include "nsIVariant.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif

/* starting interface:    nsIPythonTestInterface */
#define NS_IPYTHONTESTINTERFACE_IID_STR "1ecaed4f-e4d5-4ee7-abf0-7d72ae1441d7"

#define NS_IPYTHONTESTINTERFACE_IID \
  {0x1ecaed4f, 0xe4d5, 0x4ee7, \
    { 0xab, 0xf0, 0x7d, 0x72, 0xae, 0x14, 0x41, 0xd7 }}

class NS_NO_VTABLE nsIPythonTestInterface : public nsISupports {
 public: 

  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IPYTHONTESTINTERFACE_IID)

  enum { One = 1 };

  enum { Two = 2 };

  enum { MinusOne = -1 };

  enum { BigLong = 2147483647 };

  enum { BiggerLong = 4294967295 };

  enum { BigULong = 4294967295U };

  /* attribute boolean boolean_value; */
  NS_IMETHOD GetBoolean_value(PRBool *aBoolean_value) = 0;
  NS_IMETHOD SetBoolean_value(PRBool aBoolean_value) = 0;

  /* attribute octet octet_value; */
  NS_IMETHOD GetOctet_value(PRUint8 *aOctet_value) = 0;
  NS_IMETHOD SetOctet_value(PRUint8 aOctet_value) = 0;

  /* attribute short short_value; */
  NS_IMETHOD GetShort_value(PRInt16 *aShort_value) = 0;
  NS_IMETHOD SetShort_value(PRInt16 aShort_value) = 0;

  /* attribute unsigned short ushort_value; */
  NS_IMETHOD GetUshort_value(PRUint16 *aUshort_value) = 0;
  NS_IMETHOD SetUshort_value(PRUint16 aUshort_value) = 0;

  /* attribute long long_value; */
  NS_IMETHOD GetLong_value(PRInt32 *aLong_value) = 0;
  NS_IMETHOD SetLong_value(PRInt32 aLong_value) = 0;

  /* attribute unsigned long ulong_value; */
  NS_IMETHOD GetUlong_value(PRUint32 *aUlong_value) = 0;
  NS_IMETHOD SetUlong_value(PRUint32 aUlong_value) = 0;

  /* attribute long long long_long_value; */
  NS_IMETHOD GetLong_long_value(PRInt64 *aLong_long_value) = 0;
  NS_IMETHOD SetLong_long_value(PRInt64 aLong_long_value) = 0;

  /* attribute unsigned long long ulong_long_value; */
  NS_IMETHOD GetUlong_long_value(PRUint64 *aUlong_long_value) = 0;
  NS_IMETHOD SetUlong_long_value(PRUint64 aUlong_long_value) = 0;

  /* attribute float float_value; */
  NS_IMETHOD GetFloat_value(float *aFloat_value) = 0;
  NS_IMETHOD SetFloat_value(float aFloat_value) = 0;

  /* attribute double double_value; */
  NS_IMETHOD GetDouble_value(double *aDouble_value) = 0;
  NS_IMETHOD SetDouble_value(double aDouble_value) = 0;

  /* attribute char char_value; */
  NS_IMETHOD GetChar_value(char *aChar_value) = 0;
  NS_IMETHOD SetChar_value(char aChar_value) = 0;

  /* attribute wchar wchar_value; */
  NS_IMETHOD GetWchar_value(PRUnichar *aWchar_value) = 0;
  NS_IMETHOD SetWchar_value(PRUnichar aWchar_value) = 0;

  /* attribute string string_value; */
  NS_IMETHOD GetString_value(char * *aString_value) = 0;
  NS_IMETHOD SetString_value(const char * aString_value) = 0;

  /* attribute wstring wstring_value; */
  NS_IMETHOD GetWstring_value(PRUnichar * *aWstring_value) = 0;
  NS_IMETHOD SetWstring_value(const PRUnichar * aWstring_value) = 0;

  /* attribute AString astring_value; */
  NS_IMETHOD GetAstring_value(nsAString & aAstring_value) = 0;
  NS_IMETHOD SetAstring_value(const nsAString & aAstring_value) = 0;

  /* attribute ACString acstring_value; */
  NS_IMETHOD GetAcstring_value(nsACString & aAcstring_value) = 0;
  NS_IMETHOD SetAcstring_value(const nsACString & aAcstring_value) = 0;

  /* attribute AUTF8String utf8string_value; */
  NS_IMETHOD GetUtf8string_value(nsACString & aUtf8string_value) = 0;
  NS_IMETHOD SetUtf8string_value(const nsACString & aUtf8string_value) = 0;

  /* attribute nsIIDRef iid_value; */
  NS_IMETHOD GetIid_value(nsIID & *aIid_value) = 0;
  NS_IMETHOD SetIid_value(const nsIID & aIid_value) = 0;

  /* attribute nsIPythonTestInterface interface_value; */
  NS_IMETHOD GetInterface_value(nsIPythonTestInterface * *aInterface_value) = 0;
  NS_IMETHOD SetInterface_value(nsIPythonTestInterface * aInterface_value) = 0;

  /* attribute nsISupports isupports_value; */
  NS_IMETHOD GetIsupports_value(nsISupports * *aIsupports_value) = 0;
  NS_IMETHOD SetIsupports_value(nsISupports * aIsupports_value) = 0;

  /* boolean do_boolean (in boolean p1, inout boolean p2, out boolean p3); */
  NS_IMETHOD Do_boolean(PRBool p1, PRBool *p2, PRBool *p3, PRBool *_retval) = 0;

  /* octet do_octet (in octet p1, inout octet p2, out octet p3); */
  NS_IMETHOD Do_octet(PRUint8 p1, PRUint8 *p2, PRUint8 *p3, PRUint8 *_retval) = 0;

  /* short do_short (in short p1, inout short p2, out short p3); */
  NS_IMETHOD Do_short(PRInt16 p1, PRInt16 *p2, PRInt16 *p3, PRInt16 *_retval) = 0;

  /* unsigned short do_unsigned_short (in unsigned short p1, inout unsigned short p2, out unsigned short p3); */
  NS_IMETHOD Do_unsigned_short(PRUint16 p1, PRUint16 *p2, PRUint16 *p3, PRUint16 *_retval) = 0;

  /* long do_long (in long p1, inout long p2, out long p3); */
  NS_IMETHOD Do_long(PRInt32 p1, PRInt32 *p2, PRInt32 *p3, PRInt32 *_retval) = 0;

  /* unsigned long do_unsigned_long (in unsigned long p1, inout unsigned long p2, out unsigned long p3); */
  NS_IMETHOD Do_unsigned_long(PRUint32 p1, PRUint32 *p2, PRUint32 *p3, PRUint32 *_retval) = 0;

  /* long long do_long_long (in long long p1, inout long long p2, out long long p3); */
  NS_IMETHOD Do_long_long(PRInt64 p1, PRInt64 *p2, PRInt64 *p3, PRInt64 *_retval) = 0;

  /* unsigned long long do_unsigned_long_long (in unsigned long long p1, inout unsigned long long p2, out unsigned long long p3); */
  NS_IMETHOD Do_unsigned_long_long(PRUint64 p1, PRUint64 *p2, PRUint64 *p3, PRUint64 *_retval) = 0;

  /* float do_float (in float p1, inout float p2, out float p3); */
  NS_IMETHOD Do_float(float p1, float *p2, float *p3, float *_retval) = 0;

  /* double do_double (in double p1, inout double p2, out double p3); */
  NS_IMETHOD Do_double(double p1, double *p2, double *p3, double *_retval) = 0;

  /* char do_char (in char p1, inout char p2, out char p3); */
  NS_IMETHOD Do_char(char p1, char *p2, char *p3, char *_retval) = 0;

  /* wchar do_wchar (in wchar p1, inout wchar p2, out wchar p3); */
  NS_IMETHOD Do_wchar(PRUnichar p1, PRUnichar *p2, PRUnichar *p3, PRUnichar *_retval) = 0;

  /* string do_string (in string p1, inout string p2, out string p3); */
  NS_IMETHOD Do_string(const char *p1, char **p2, char **p3, char **_retval) = 0;

  /* wstring do_wstring (in wstring p1, inout wstring p2, out wstring p3); */
  NS_IMETHOD Do_wstring(const PRUnichar *p1, PRUnichar **p2, PRUnichar **p3, PRUnichar **_retval) = 0;

  /* nsIIDRef do_nsIIDRef (in nsIIDRef p1, inout nsIIDRef p2, out nsIIDRef p3); */
  NS_IMETHOD Do_nsIIDRef(const nsIID & p1, nsIID & *p2, nsIID & *p3, nsIID & *_retval) = 0;

  /* nsIPythonTestInterface do_nsIPythonTestInterface (in nsIPythonTestInterface p1, inout nsIPythonTestInterface p2, out nsIPythonTestInterface p3); */
  NS_IMETHOD Do_nsIPythonTestInterface(nsIPythonTestInterface *p1, nsIPythonTestInterface **p2, nsIPythonTestInterface **p3, nsIPythonTestInterface **_retval) = 0;

  /* nsISupports do_nsISupports (in nsISupports p1, inout nsISupports p2, out nsISupports p3); */
  NS_IMETHOD Do_nsISupports(nsISupports *p1, nsISupports **p2, nsISupports **p3, nsISupports **_retval) = 0;

  /* void do_nsISupportsIs (in nsIIDRef iid, [iid_is (iid), retval] out nsQIResult result); */
  NS_IMETHOD Do_nsISupportsIs(const nsIID & iid, void * *result) = 0;

};

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIPYTHONTESTINTERFACE \
  NS_IMETHOD GetBoolean_value(PRBool *aBoolean_value); \
  NS_IMETHOD SetBoolean_value(PRBool aBoolean_value); \
  NS_IMETHOD GetOctet_value(PRUint8 *aOctet_value); \
  NS_IMETHOD SetOctet_value(PRUint8 aOctet_value); \
  NS_IMETHOD GetShort_value(PRInt16 *aShort_value); \
  NS_IMETHOD SetShort_value(PRInt16 aShort_value); \
  NS_IMETHOD GetUshort_value(PRUint16 *aUshort_value); \
  NS_IMETHOD SetUshort_value(PRUint16 aUshort_value); \
  NS_IMETHOD GetLong_value(PRInt32 *aLong_value); \
  NS_IMETHOD SetLong_value(PRInt32 aLong_value); \
  NS_IMETHOD GetUlong_value(PRUint32 *aUlong_value); \
  NS_IMETHOD SetUlong_value(PRUint32 aUlong_value); \
  NS_IMETHOD GetLong_long_value(PRInt64 *aLong_long_value); \
  NS_IMETHOD SetLong_long_value(PRInt64 aLong_long_value); \
  NS_IMETHOD GetUlong_long_value(PRUint64 *aUlong_long_value); \
  NS_IMETHOD SetUlong_long_value(PRUint64 aUlong_long_value); \
  NS_IMETHOD GetFloat_value(float *aFloat_value); \
  NS_IMETHOD SetFloat_value(float aFloat_value); \
  NS_IMETHOD GetDouble_value(double *aDouble_value); \
  NS_IMETHOD SetDouble_value(double aDouble_value); \
  NS_IMETHOD GetChar_value(char *aChar_value); \
  NS_IMETHOD SetChar_value(char aChar_value); \
  NS_IMETHOD GetWchar_value(PRUnichar *aWchar_value); \
  NS_IMETHOD SetWchar_value(PRUnichar aWchar_value); \
  NS_IMETHOD GetString_value(char * *aString_value); \
  NS_IMETHOD SetString_value(const char * aString_value); \
  NS_IMETHOD GetWstring_value(PRUnichar * *aWstring_value); \
  NS_IMETHOD SetWstring_value(const PRUnichar * aWstring_value); \
  NS_IMETHOD GetAstring_value(nsAString & aAstring_value); \
  NS_IMETHOD SetAstring_value(const nsAString & aAstring_value); \
  NS_IMETHOD GetAcstring_value(nsACString & aAcstring_value); \
  NS_IMETHOD SetAcstring_value(const nsACString & aAcstring_value); \
  NS_IMETHOD GetUtf8string_value(nsACString & aUtf8string_value); \
  NS_IMETHOD SetUtf8string_value(const nsACString & aUtf8string_value); \
  NS_IMETHOD GetIid_value(nsIID & *aIid_value); \
  NS_IMETHOD SetIid_value(const nsIID & aIid_value); \
  NS_IMETHOD GetInterface_value(nsIPythonTestInterface * *aInterface_value); \
  NS_IMETHOD SetInterface_value(nsIPythonTestInterface * aInterface_value); \
  NS_IMETHOD GetIsupports_value(nsISupports * *aIsupports_value); \
  NS_IMETHOD SetIsupports_value(nsISupports * aIsupports_value); \
  NS_IMETHOD Do_boolean(PRBool p1, PRBool *p2, PRBool *p3, PRBool *_retval); \
  NS_IMETHOD Do_octet(PRUint8 p1, PRUint8 *p2, PRUint8 *p3, PRUint8 *_retval); \
  NS_IMETHOD Do_short(PRInt16 p1, PRInt16 *p2, PRInt16 *p3, PRInt16 *_retval); \
  NS_IMETHOD Do_unsigned_short(PRUint16 p1, PRUint16 *p2, PRUint16 *p3, PRUint16 *_retval); \
  NS_IMETHOD Do_long(PRInt32 p1, PRInt32 *p2, PRInt32 *p3, PRInt32 *_retval); \
  NS_IMETHOD Do_unsigned_long(PRUint32 p1, PRUint32 *p2, PRUint32 *p3, PRUint32 *_retval); \
  NS_IMETHOD Do_long_long(PRInt64 p1, PRInt64 *p2, PRInt64 *p3, PRInt64 *_retval); \
  NS_IMETHOD Do_unsigned_long_long(PRUint64 p1, PRUint64 *p2, PRUint64 *p3, PRUint64 *_retval); \
  NS_IMETHOD Do_float(float p1, float *p2, float *p3, float *_retval); \
  NS_IMETHOD Do_double(double p1, double *p2, double *p3, double *_retval); \
  NS_IMETHOD Do_char(char p1, char *p2, char *p3, char *_retval); \
  NS_IMETHOD Do_wchar(PRUnichar p1, PRUnichar *p2, PRUnichar *p3, PRUnichar *_retval); \
  NS_IMETHOD Do_string(const char *p1, char **p2, char **p3, char **_retval); \
  NS_IMETHOD Do_wstring(const PRUnichar *p1, PRUnichar **p2, PRUnichar **p3, PRUnichar **_retval); \
  NS_IMETHOD Do_nsIIDRef(const nsIID & p1, nsIID & *p2, nsIID & *p3, nsIID & *_retval); \
  NS_IMETHOD Do_nsIPythonTestInterface(nsIPythonTestInterface *p1, nsIPythonTestInterface **p2, nsIPythonTestInterface **p3, nsIPythonTestInterface **_retval); \
  NS_IMETHOD Do_nsISupports(nsISupports *p1, nsISupports **p2, nsISupports **p3, nsISupports **_retval); \
  NS_IMETHOD Do_nsISupportsIs(const nsIID & iid, void * *result); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIPYTHONTESTINTERFACE(_to) \
  NS_IMETHOD GetBoolean_value(PRBool *aBoolean_value) { return _to GetBoolean_value(aBoolean_value); } \
  NS_IMETHOD SetBoolean_value(PRBool aBoolean_value) { return _to SetBoolean_value(aBoolean_value); } \
  NS_IMETHOD GetOctet_value(PRUint8 *aOctet_value) { return _to GetOctet_value(aOctet_value); } \
  NS_IMETHOD SetOctet_value(PRUint8 aOctet_value) { return _to SetOctet_value(aOctet_value); } \
  NS_IMETHOD GetShort_value(PRInt16 *aShort_value) { return _to GetShort_value(aShort_value); } \
  NS_IMETHOD SetShort_value(PRInt16 aShort_value) { return _to SetShort_value(aShort_value); } \
  NS_IMETHOD GetUshort_value(PRUint16 *aUshort_value) { return _to GetUshort_value(aUshort_value); } \
  NS_IMETHOD SetUshort_value(PRUint16 aUshort_value) { return _to SetUshort_value(aUshort_value); } \
  NS_IMETHOD GetLong_value(PRInt32 *aLong_value) { return _to GetLong_value(aLong_value); } \
  NS_IMETHOD SetLong_value(PRInt32 aLong_value) { return _to SetLong_value(aLong_value); } \
  NS_IMETHOD GetUlong_value(PRUint32 *aUlong_value) { return _to GetUlong_value(aUlong_value); } \
  NS_IMETHOD SetUlong_value(PRUint32 aUlong_value) { return _to SetUlong_value(aUlong_value); } \
  NS_IMETHOD GetLong_long_value(PRInt64 *aLong_long_value) { return _to GetLong_long_value(aLong_long_value); } \
  NS_IMETHOD SetLong_long_value(PRInt64 aLong_long_value) { return _to SetLong_long_value(aLong_long_value); } \
  NS_IMETHOD GetUlong_long_value(PRUint64 *aUlong_long_value) { return _to GetUlong_long_value(aUlong_long_value); } \
  NS_IMETHOD SetUlong_long_value(PRUint64 aUlong_long_value) { return _to SetUlong_long_value(aUlong_long_value); } \
  NS_IMETHOD GetFloat_value(float *aFloat_value) { return _to GetFloat_value(aFloat_value); } \
  NS_IMETHOD SetFloat_value(float aFloat_value) { return _to SetFloat_value(aFloat_value); } \
  NS_IMETHOD GetDouble_value(double *aDouble_value) { return _to GetDouble_value(aDouble_value); } \
  NS_IMETHOD SetDouble_value(double aDouble_value) { return _to SetDouble_value(aDouble_value); } \
  NS_IMETHOD GetChar_value(char *aChar_value) { return _to GetChar_value(aChar_value); } \
  NS_IMETHOD SetChar_value(char aChar_value) { return _to SetChar_value(aChar_value); } \
  NS_IMETHOD GetWchar_value(PRUnichar *aWchar_value) { return _to GetWchar_value(aWchar_value); } \
  NS_IMETHOD SetWchar_value(PRUnichar aWchar_value) { return _to SetWchar_value(aWchar_value); } \
  NS_IMETHOD GetString_value(char * *aString_value) { return _to GetString_value(aString_value); } \
  NS_IMETHOD SetString_value(const char * aString_value) { return _to SetString_value(aString_value); } \
  NS_IMETHOD GetWstring_value(PRUnichar * *aWstring_value) { return _to GetWstring_value(aWstring_value); } \
  NS_IMETHOD SetWstring_value(const PRUnichar * aWstring_value) { return _to SetWstring_value(aWstring_value); } \
  NS_IMETHOD GetAstring_value(nsAString & aAstring_value) { return _to GetAstring_value(aAstring_value); } \
  NS_IMETHOD SetAstring_value(const nsAString & aAstring_value) { return _to SetAstring_value(aAstring_value); } \
  NS_IMETHOD GetAcstring_value(nsACString & aAcstring_value) { return _to GetAcstring_value(aAcstring_value); } \
  NS_IMETHOD SetAcstring_value(const nsACString & aAcstring_value) { return _to SetAcstring_value(aAcstring_value); } \
  NS_IMETHOD GetUtf8string_value(nsACString & aUtf8string_value) { return _to GetUtf8string_value(aUtf8string_value); } \
  NS_IMETHOD SetUtf8string_value(const nsACString & aUtf8string_value) { return _to SetUtf8string_value(aUtf8string_value); } \
  NS_IMETHOD GetIid_value(nsIID & *aIid_value) { return _to GetIid_value(aIid_value); } \
  NS_IMETHOD SetIid_value(const nsIID & aIid_value) { return _to SetIid_value(aIid_value); } \
  NS_IMETHOD GetInterface_value(nsIPythonTestInterface * *aInterface_value) { return _to GetInterface_value(aInterface_value); } \
  NS_IMETHOD SetInterface_value(nsIPythonTestInterface * aInterface_value) { return _to SetInterface_value(aInterface_value); } \
  NS_IMETHOD GetIsupports_value(nsISupports * *aIsupports_value) { return _to GetIsupports_value(aIsupports_value); } \
  NS_IMETHOD SetIsupports_value(nsISupports * aIsupports_value) { return _to SetIsupports_value(aIsupports_value); } \
  NS_IMETHOD Do_boolean(PRBool p1, PRBool *p2, PRBool *p3, PRBool *_retval) { return _to Do_boolean(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_octet(PRUint8 p1, PRUint8 *p2, PRUint8 *p3, PRUint8 *_retval) { return _to Do_octet(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_short(PRInt16 p1, PRInt16 *p2, PRInt16 *p3, PRInt16 *_retval) { return _to Do_short(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_unsigned_short(PRUint16 p1, PRUint16 *p2, PRUint16 *p3, PRUint16 *_retval) { return _to Do_unsigned_short(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_long(PRInt32 p1, PRInt32 *p2, PRInt32 *p3, PRInt32 *_retval) { return _to Do_long(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_unsigned_long(PRUint32 p1, PRUint32 *p2, PRUint32 *p3, PRUint32 *_retval) { return _to Do_unsigned_long(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_long_long(PRInt64 p1, PRInt64 *p2, PRInt64 *p3, PRInt64 *_retval) { return _to Do_long_long(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_unsigned_long_long(PRUint64 p1, PRUint64 *p2, PRUint64 *p3, PRUint64 *_retval) { return _to Do_unsigned_long_long(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_float(float p1, float *p2, float *p3, float *_retval) { return _to Do_float(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_double(double p1, double *p2, double *p3, double *_retval) { return _to Do_double(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_char(char p1, char *p2, char *p3, char *_retval) { return _to Do_char(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_wchar(PRUnichar p1, PRUnichar *p2, PRUnichar *p3, PRUnichar *_retval) { return _to Do_wchar(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_string(const char *p1, char **p2, char **p3, char **_retval) { return _to Do_string(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_wstring(const PRUnichar *p1, PRUnichar **p2, PRUnichar **p3, PRUnichar **_retval) { return _to Do_wstring(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_nsIIDRef(const nsIID & p1, nsIID & *p2, nsIID & *p3, nsIID & *_retval) { return _to Do_nsIIDRef(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_nsIPythonTestInterface(nsIPythonTestInterface *p1, nsIPythonTestInterface **p2, nsIPythonTestInterface **p3, nsIPythonTestInterface **_retval) { return _to Do_nsIPythonTestInterface(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_nsISupports(nsISupports *p1, nsISupports **p2, nsISupports **p3, nsISupports **_retval) { return _to Do_nsISupports(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_nsISupportsIs(const nsIID & iid, void * *result) { return _to Do_nsISupportsIs(iid, result); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIPYTHONTESTINTERFACE(_to) \
  NS_IMETHOD GetBoolean_value(PRBool *aBoolean_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetBoolean_value(aBoolean_value); } \
  NS_IMETHOD SetBoolean_value(PRBool aBoolean_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetBoolean_value(aBoolean_value); } \
  NS_IMETHOD GetOctet_value(PRUint8 *aOctet_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetOctet_value(aOctet_value); } \
  NS_IMETHOD SetOctet_value(PRUint8 aOctet_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetOctet_value(aOctet_value); } \
  NS_IMETHOD GetShort_value(PRInt16 *aShort_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetShort_value(aShort_value); } \
  NS_IMETHOD SetShort_value(PRInt16 aShort_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetShort_value(aShort_value); } \
  NS_IMETHOD GetUshort_value(PRUint16 *aUshort_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetUshort_value(aUshort_value); } \
  NS_IMETHOD SetUshort_value(PRUint16 aUshort_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetUshort_value(aUshort_value); } \
  NS_IMETHOD GetLong_value(PRInt32 *aLong_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetLong_value(aLong_value); } \
  NS_IMETHOD SetLong_value(PRInt32 aLong_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetLong_value(aLong_value); } \
  NS_IMETHOD GetUlong_value(PRUint32 *aUlong_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetUlong_value(aUlong_value); } \
  NS_IMETHOD SetUlong_value(PRUint32 aUlong_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetUlong_value(aUlong_value); } \
  NS_IMETHOD GetLong_long_value(PRInt64 *aLong_long_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetLong_long_value(aLong_long_value); } \
  NS_IMETHOD SetLong_long_value(PRInt64 aLong_long_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetLong_long_value(aLong_long_value); } \
  NS_IMETHOD GetUlong_long_value(PRUint64 *aUlong_long_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetUlong_long_value(aUlong_long_value); } \
  NS_IMETHOD SetUlong_long_value(PRUint64 aUlong_long_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetUlong_long_value(aUlong_long_value); } \
  NS_IMETHOD GetFloat_value(float *aFloat_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetFloat_value(aFloat_value); } \
  NS_IMETHOD SetFloat_value(float aFloat_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetFloat_value(aFloat_value); } \
  NS_IMETHOD GetDouble_value(double *aDouble_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDouble_value(aDouble_value); } \
  NS_IMETHOD SetDouble_value(double aDouble_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetDouble_value(aDouble_value); } \
  NS_IMETHOD GetChar_value(char *aChar_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetChar_value(aChar_value); } \
  NS_IMETHOD SetChar_value(char aChar_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetChar_value(aChar_value); } \
  NS_IMETHOD GetWchar_value(PRUnichar *aWchar_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetWchar_value(aWchar_value); } \
  NS_IMETHOD SetWchar_value(PRUnichar aWchar_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetWchar_value(aWchar_value); } \
  NS_IMETHOD GetString_value(char * *aString_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetString_value(aString_value); } \
  NS_IMETHOD SetString_value(const char * aString_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetString_value(aString_value); } \
  NS_IMETHOD GetWstring_value(PRUnichar * *aWstring_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetWstring_value(aWstring_value); } \
  NS_IMETHOD SetWstring_value(const PRUnichar * aWstring_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetWstring_value(aWstring_value); } \
  NS_IMETHOD GetAstring_value(nsAString & aAstring_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAstring_value(aAstring_value); } \
  NS_IMETHOD SetAstring_value(const nsAString & aAstring_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetAstring_value(aAstring_value); } \
  NS_IMETHOD GetAcstring_value(nsACString & aAcstring_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAcstring_value(aAcstring_value); } \
  NS_IMETHOD SetAcstring_value(const nsACString & aAcstring_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetAcstring_value(aAcstring_value); } \
  NS_IMETHOD GetUtf8string_value(nsACString & aUtf8string_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetUtf8string_value(aUtf8string_value); } \
  NS_IMETHOD SetUtf8string_value(const nsACString & aUtf8string_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetUtf8string_value(aUtf8string_value); } \
  NS_IMETHOD GetIid_value(nsIID & *aIid_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetIid_value(aIid_value); } \
  NS_IMETHOD SetIid_value(const nsIID & aIid_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetIid_value(aIid_value); } \
  NS_IMETHOD GetInterface_value(nsIPythonTestInterface * *aInterface_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetInterface_value(aInterface_value); } \
  NS_IMETHOD SetInterface_value(nsIPythonTestInterface * aInterface_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetInterface_value(aInterface_value); } \
  NS_IMETHOD GetIsupports_value(nsISupports * *aIsupports_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetIsupports_value(aIsupports_value); } \
  NS_IMETHOD SetIsupports_value(nsISupports * aIsupports_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetIsupports_value(aIsupports_value); } \
  NS_IMETHOD Do_boolean(PRBool p1, PRBool *p2, PRBool *p3, PRBool *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_boolean(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_octet(PRUint8 p1, PRUint8 *p2, PRUint8 *p3, PRUint8 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_octet(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_short(PRInt16 p1, PRInt16 *p2, PRInt16 *p3, PRInt16 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_short(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_unsigned_short(PRUint16 p1, PRUint16 *p2, PRUint16 *p3, PRUint16 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_unsigned_short(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_long(PRInt32 p1, PRInt32 *p2, PRInt32 *p3, PRInt32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_long(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_unsigned_long(PRUint32 p1, PRUint32 *p2, PRUint32 *p3, PRUint32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_unsigned_long(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_long_long(PRInt64 p1, PRInt64 *p2, PRInt64 *p3, PRInt64 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_long_long(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_unsigned_long_long(PRUint64 p1, PRUint64 *p2, PRUint64 *p3, PRUint64 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_unsigned_long_long(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_float(float p1, float *p2, float *p3, float *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_float(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_double(double p1, double *p2, double *p3, double *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_double(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_char(char p1, char *p2, char *p3, char *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_char(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_wchar(PRUnichar p1, PRUnichar *p2, PRUnichar *p3, PRUnichar *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_wchar(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_string(const char *p1, char **p2, char **p3, char **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_string(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_wstring(const PRUnichar *p1, PRUnichar **p2, PRUnichar **p3, PRUnichar **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_wstring(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_nsIIDRef(const nsIID & p1, nsIID & *p2, nsIID & *p3, nsIID & *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_nsIIDRef(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_nsIPythonTestInterface(nsIPythonTestInterface *p1, nsIPythonTestInterface **p2, nsIPythonTestInterface **p3, nsIPythonTestInterface **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_nsIPythonTestInterface(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_nsISupports(nsISupports *p1, nsISupports **p2, nsISupports **p3, nsISupports **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_nsISupports(p1, p2, p3, _retval); } \
  NS_IMETHOD Do_nsISupportsIs(const nsIID & iid, void * *result) { return !_to ? NS_ERROR_NULL_POINTER : _to->Do_nsISupportsIs(iid, result); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsPythonTestInterface : public nsIPythonTestInterface
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPYTHONTESTINTERFACE

  nsPythonTestInterface();

private:
  ~nsPythonTestInterface();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsPythonTestInterface, nsIPythonTestInterface)

nsPythonTestInterface::nsPythonTestInterface()
{
  /* member initializers and constructor code */
}

nsPythonTestInterface::~nsPythonTestInterface()
{
  /* destructor code */
}

/* attribute boolean boolean_value; */
NS_IMETHODIMP nsPythonTestInterface::GetBoolean_value(PRBool *aBoolean_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetBoolean_value(PRBool aBoolean_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute octet octet_value; */
NS_IMETHODIMP nsPythonTestInterface::GetOctet_value(PRUint8 *aOctet_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetOctet_value(PRUint8 aOctet_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute short short_value; */
NS_IMETHODIMP nsPythonTestInterface::GetShort_value(PRInt16 *aShort_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetShort_value(PRInt16 aShort_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute unsigned short ushort_value; */
NS_IMETHODIMP nsPythonTestInterface::GetUshort_value(PRUint16 *aUshort_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetUshort_value(PRUint16 aUshort_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute long long_value; */
NS_IMETHODIMP nsPythonTestInterface::GetLong_value(PRInt32 *aLong_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetLong_value(PRInt32 aLong_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute unsigned long ulong_value; */
NS_IMETHODIMP nsPythonTestInterface::GetUlong_value(PRUint32 *aUlong_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetUlong_value(PRUint32 aUlong_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute long long long_long_value; */
NS_IMETHODIMP nsPythonTestInterface::GetLong_long_value(PRInt64 *aLong_long_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetLong_long_value(PRInt64 aLong_long_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute unsigned long long ulong_long_value; */
NS_IMETHODIMP nsPythonTestInterface::GetUlong_long_value(PRUint64 *aUlong_long_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetUlong_long_value(PRUint64 aUlong_long_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute float float_value; */
NS_IMETHODIMP nsPythonTestInterface::GetFloat_value(float *aFloat_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetFloat_value(float aFloat_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute double double_value; */
NS_IMETHODIMP nsPythonTestInterface::GetDouble_value(double *aDouble_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetDouble_value(double aDouble_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute char char_value; */
NS_IMETHODIMP nsPythonTestInterface::GetChar_value(char *aChar_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetChar_value(char aChar_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute wchar wchar_value; */
NS_IMETHODIMP nsPythonTestInterface::GetWchar_value(PRUnichar *aWchar_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetWchar_value(PRUnichar aWchar_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute string string_value; */
NS_IMETHODIMP nsPythonTestInterface::GetString_value(char * *aString_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetString_value(const char * aString_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute wstring wstring_value; */
NS_IMETHODIMP nsPythonTestInterface::GetWstring_value(PRUnichar * *aWstring_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetWstring_value(const PRUnichar * aWstring_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute AString astring_value; */
NS_IMETHODIMP nsPythonTestInterface::GetAstring_value(nsAString & aAstring_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetAstring_value(const nsAString & aAstring_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute ACString acstring_value; */
NS_IMETHODIMP nsPythonTestInterface::GetAcstring_value(nsACString & aAcstring_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetAcstring_value(const nsACString & aAcstring_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute AUTF8String utf8string_value; */
NS_IMETHODIMP nsPythonTestInterface::GetUtf8string_value(nsACString & aUtf8string_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetUtf8string_value(const nsACString & aUtf8string_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsIIDRef iid_value; */
NS_IMETHODIMP nsPythonTestInterface::GetIid_value(nsIID & *aIid_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetIid_value(const nsIID & aIid_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsIPythonTestInterface interface_value; */
NS_IMETHODIMP nsPythonTestInterface::GetInterface_value(nsIPythonTestInterface * *aInterface_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetInterface_value(nsIPythonTestInterface * aInterface_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute nsISupports isupports_value; */
NS_IMETHODIMP nsPythonTestInterface::GetIsupports_value(nsISupports * *aIsupports_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterface::SetIsupports_value(nsISupports * aIsupports_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean do_boolean (in boolean p1, inout boolean p2, out boolean p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_boolean(PRBool p1, PRBool *p2, PRBool *p3, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* octet do_octet (in octet p1, inout octet p2, out octet p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_octet(PRUint8 p1, PRUint8 *p2, PRUint8 *p3, PRUint8 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* short do_short (in short p1, inout short p2, out short p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_short(PRInt16 p1, PRInt16 *p2, PRInt16 *p3, PRInt16 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* unsigned short do_unsigned_short (in unsigned short p1, inout unsigned short p2, out unsigned short p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_unsigned_short(PRUint16 p1, PRUint16 *p2, PRUint16 *p3, PRUint16 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* long do_long (in long p1, inout long p2, out long p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_long(PRInt32 p1, PRInt32 *p2, PRInt32 *p3, PRInt32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* unsigned long do_unsigned_long (in unsigned long p1, inout unsigned long p2, out unsigned long p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_unsigned_long(PRUint32 p1, PRUint32 *p2, PRUint32 *p3, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* long long do_long_long (in long long p1, inout long long p2, out long long p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_long_long(PRInt64 p1, PRInt64 *p2, PRInt64 *p3, PRInt64 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* unsigned long long do_unsigned_long_long (in unsigned long long p1, inout unsigned long long p2, out unsigned long long p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_unsigned_long_long(PRUint64 p1, PRUint64 *p2, PRUint64 *p3, PRUint64 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* float do_float (in float p1, inout float p2, out float p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_float(float p1, float *p2, float *p3, float *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* double do_double (in double p1, inout double p2, out double p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_double(double p1, double *p2, double *p3, double *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* char do_char (in char p1, inout char p2, out char p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_char(char p1, char *p2, char *p3, char *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* wchar do_wchar (in wchar p1, inout wchar p2, out wchar p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_wchar(PRUnichar p1, PRUnichar *p2, PRUnichar *p3, PRUnichar *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* string do_string (in string p1, inout string p2, out string p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_string(const char *p1, char **p2, char **p3, char **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* wstring do_wstring (in wstring p1, inout wstring p2, out wstring p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_wstring(const PRUnichar *p1, PRUnichar **p2, PRUnichar **p3, PRUnichar **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIIDRef do_nsIIDRef (in nsIIDRef p1, inout nsIIDRef p2, out nsIIDRef p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_nsIIDRef(const nsIID & p1, nsIID & *p2, nsIID & *p3, nsIID & *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIPythonTestInterface do_nsIPythonTestInterface (in nsIPythonTestInterface p1, inout nsIPythonTestInterface p2, out nsIPythonTestInterface p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_nsIPythonTestInterface(nsIPythonTestInterface *p1, nsIPythonTestInterface **p2, nsIPythonTestInterface **p3, nsIPythonTestInterface **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsISupports do_nsISupports (in nsISupports p1, inout nsISupports p2, out nsISupports p3); */
NS_IMETHODIMP nsPythonTestInterface::Do_nsISupports(nsISupports *p1, nsISupports **p2, nsISupports **p3, nsISupports **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void do_nsISupportsIs (in nsIIDRef iid, [iid_is (iid), retval] out nsQIResult result); */
NS_IMETHODIMP nsPythonTestInterface::Do_nsISupportsIs(const nsIID & iid, void * *result)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    nsIPythonTestInterfaceExtra */
#define NS_IPYTHONTESTINTERFACEEXTRA_IID_STR "b38d1538-fe92-42c3-831f-285242edeea4"

#define NS_IPYTHONTESTINTERFACEEXTRA_IID \
  {0xb38d1538, 0xfe92, 0x42c3, \
    { 0x83, 0x1f, 0x28, 0x52, 0x42, 0xed, 0xee, 0xa4 }}

class NS_NO_VTABLE nsIPythonTestInterfaceExtra : public nsIPythonTestInterface {
 public: 

  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IPYTHONTESTINTERFACEEXTRA_IID)

  /* void MultiplyEachItemInIntegerArray (in PRInt32 val, in PRUint32 count, [array, size_is (count)] inout PRInt32 valueArray); */
  NS_IMETHOD MultiplyEachItemInIntegerArray(PRInt32 val, PRUint32 count, PRInt32 **valueArray) = 0;

  /* void MultiplyEachItemInIntegerArrayAndAppend (in PRInt32 val, inout PRUint32 count, [array, size_is (count)] inout PRInt32 valueArray); */
  NS_IMETHOD MultiplyEachItemInIntegerArrayAndAppend(PRInt32 val, PRUint32 *count, PRInt32 **valueArray) = 0;

  /* void CompareStringArrays ([array, size_is (count)] in string arr1, [array, size_is (count)] in string arr2, in unsigned long count, [retval] out short result); */
  NS_IMETHOD CompareStringArrays(const char **arr1, const char **arr2, PRUint32 count, PRInt16 *result) = 0;

  /* void DoubleStringArray (inout PRUint32 count, [array, size_is (count)] inout string valueArray); */
  NS_IMETHOD DoubleStringArray(PRUint32 *count, char ***valueArray) = 0;

  /* void ReverseStringArray (in PRUint32 count, [array, size_is (count)] inout string valueArray); */
  NS_IMETHOD ReverseStringArray(PRUint32 count, char ***valueArray) = 0;

  /* void DoubleString (inout PRUint32 count, [size_is (count)] inout string str); */
  NS_IMETHOD DoubleString(PRUint32 *count, char **str) = 0;

  /* void DoubleString2 (in PRUint32 in_count, [size_is (in_count)] in string in_str, out PRUint32 out_count, [size_is (out_count)] out string out_str); */
  NS_IMETHOD DoubleString2(PRUint32 in_count, const char *in_str, PRUint32 *out_count, char **out_str) = 0;

  /* void DoubleString3 (in PRUint32 in_count, [size_is (in_count)] in string in_str, out PRUint32 out_count, [size_is (out_count), retval] out string out_str); */
  NS_IMETHOD DoubleString3(PRUint32 in_count, const char *in_str, PRUint32 *out_count, char **out_str) = 0;

  /* void DoubleString4 ([size_is (count)] in string in_str, inout PRUint32 count, [size_is (count)] out string out_str); */
  NS_IMETHOD DoubleString4(const char *in_str, PRUint32 *count, char **out_str) = 0;

  /* void UpString (in PRUint32 count, [size_is (count)] inout string str); */
  NS_IMETHOD UpString(PRUint32 count, char **str) = 0;

  /* void UpString2 (in PRUint32 count, [size_is (count)] in string in_str, [size_is (count)] out string out_str); */
  NS_IMETHOD UpString2(PRUint32 count, const char *in_str, char **out_str) = 0;

  /* void CopyUTF8String (in AUTF8String in_str, out AUTF8String out_str); */
  NS_IMETHOD CopyUTF8String(const nsACString & in_str, nsACString & out_str) = 0;

  /* void CopyUTF8String2 (in AUTF8String in_str, out AUTF8String out_str); */
  NS_IMETHOD CopyUTF8String2(const nsACString & in_str, nsACString & out_str) = 0;

  /* void GetFixedString (in PRUint32 count, [size_is (count)] out string out_str); */
  NS_IMETHOD GetFixedString(PRUint32 count, char **out_str) = 0;

  /* void DoubleWideString (inout PRUint32 count, [size_is (count)] inout wstring str); */
  NS_IMETHOD DoubleWideString(PRUint32 *count, PRUnichar **str) = 0;

  /* void DoubleWideString2 (in PRUint32 in_count, [size_is (in_count)] in wstring in_str, out PRUint32 out_count, [size_is (out_count)] out wstring out_str); */
  NS_IMETHOD DoubleWideString2(PRUint32 in_count, const PRUnichar *in_str, PRUint32 *out_count, PRUnichar **out_str) = 0;

  /* void DoubleWideString3 (in PRUint32 in_count, [size_is (in_count)] in wstring in_str, out PRUint32 out_count, [size_is (out_count), retval] out wstring out_str); */
  NS_IMETHOD DoubleWideString3(PRUint32 in_count, const PRUnichar *in_str, PRUint32 *out_count, PRUnichar **out_str) = 0;

  /* void DoubleWideString4 ([size_is (count)] in wstring in_str, inout PRUint32 count, [size_is (count)] out wstring out_str); */
  NS_IMETHOD DoubleWideString4(const PRUnichar *in_str, PRUint32 *count, PRUnichar **out_str) = 0;

  /* void UpWideString (in PRUint32 count, [size_is (count)] inout wstring str); */
  NS_IMETHOD UpWideString(PRUint32 count, PRUnichar **str) = 0;

  /* void UpWideString2 (in PRUint32 count, [size_is (count)] in wstring in_str, [size_is (count)] out wstring out_str); */
  NS_IMETHOD UpWideString2(PRUint32 count, const PRUnichar *in_str, PRUnichar **out_str) = 0;

  /* void GetFixedWideString (in PRUint32 count, [size_is (count)] out string out_str); */
  NS_IMETHOD GetFixedWideString(PRUint32 count, char **out_str) = 0;

  /* void GetStrings (out PRUint32 count, [array, size_is (count), retval] out string str); */
  NS_IMETHOD GetStrings(PRUint32 *count, char ***str) = 0;

  /* void UpOctetArray (inout PRUint32 count, [array, size_is (count)] inout PRUint8 data); */
  NS_IMETHOD UpOctetArray(PRUint32 *count, PRUint8 **data) = 0;

  /* void UpOctetArray2 (inout PRUint32 count, [array, size_is (count)] inout PRUint8 data); */
  NS_IMETHOD UpOctetArray2(PRUint32 *count, PRUint8 **data) = 0;

  /* void CheckInterfaceArray (in PRUint32 count, [array, size_is (count)] in nsISupports data, [retval] out PRBool all_non_null); */
  NS_IMETHOD CheckInterfaceArray(PRUint32 count, nsISupports **data, PRBool *all_non_null) = 0;

  /* void CopyInterfaceArray (in PRUint32 count, [array, size_is (count)] in nsISupports data, [array, size_is (out_count)] out nsISupports out_data, out PRUint32 out_count); */
  NS_IMETHOD CopyInterfaceArray(PRUint32 count, nsISupports **data, nsISupports ***out_data, PRUint32 *out_count) = 0;

  /* void GetInterfaceArray (out PRUint32 count, [array, size_is (count)] out nsISupports data); */
  NS_IMETHOD GetInterfaceArray(PRUint32 *count, nsISupports ***data) = 0;

  /* void ExtendInterfaceArray (inout PRUint32 count, [array, size_is (count)] inout nsISupports data); */
  NS_IMETHOD ExtendInterfaceArray(PRUint32 *count, nsISupports ***data) = 0;

  /* void CheckIIDArray (in PRUint32 count, [array, size_is (count)] in nsIIDRef data, [retval] out PRBool all_mine); */
  NS_IMETHOD CheckIIDArray(PRUint32 count, const nsIID & *data, PRBool *all_mine) = 0;

  /* void GetIIDArray (out PRUint32 count, [array, size_is (count)] out nsIIDRef data); */
  NS_IMETHOD GetIIDArray(PRUint32 *count, nsIID & **data) = 0;

  /* void ExtendIIDArray (inout PRUint32 count, [array, size_is (count)] inout nsIIDRef data); */
  NS_IMETHOD ExtendIIDArray(PRUint32 *count, nsIID & **data) = 0;

  /* void SumArrays (in PRUint32 count, [array, size_is (count)] in PRInt32 array1, [array, size_is (count)] in PRInt32 array2, [retval] out PRInt32 result); */
  NS_IMETHOD SumArrays(PRUint32 count, PRInt32 *array1, PRInt32 *array2, PRInt32 *result) = 0;

  /* void GetArrays (out PRUint32 count, [array, size_is (count)] out PRInt32 array1, [array, size_is (count)] out PRInt32 array2); */
  NS_IMETHOD GetArrays(PRUint32 *count, PRInt32 **array1, PRInt32 **array2) = 0;

  /* void GetFixedArray (in PRUint32 count, [array, size_is (count)] out PRInt32 array1); */
  NS_IMETHOD GetFixedArray(PRUint32 count, PRInt32 **array1) = 0;

  /* void CopyArray (in PRUint32 count, [array, size_is (count)] in PRInt32 array1, [array, size_is (count)] out PRInt32 array2); */
  NS_IMETHOD CopyArray(PRUint32 count, PRInt32 *array1, PRInt32 **array2) = 0;

  /* void CopyAndDoubleArray (inout PRUint32 count, [array, size_is (count)] in PRInt32 array1, [array, size_is (count)] out PRInt32 array2); */
  NS_IMETHOD CopyAndDoubleArray(PRUint32 *count, PRInt32 *array1, PRInt32 **array2) = 0;

  /* void AppendArray (inout PRUint32 count, [array, size_is (count)] in PRInt32 array1, [array, size_is (count)] inout PRInt32 array2); */
  NS_IMETHOD AppendArray(PRUint32 *count, PRInt32 *array1, PRInt32 **array2) = 0;

  /* void AppendVariant (in nsIVariant variant, inout nsIVariant result); */
  NS_IMETHOD AppendVariant(nsIVariant *variant, nsIVariant **result) = 0;

  /* nsIVariant CopyVariant (in nsIVariant variant); */
  NS_IMETHOD CopyVariant(nsIVariant *variant, nsIVariant **_retval) = 0;

  /* nsIVariant SumVariants (in PRUint32 incount, [array, size_is (incount)] in nsIVariant variants); */
  NS_IMETHOD SumVariants(PRUint32 incount, nsIVariant **variants, nsIVariant **_retval) = 0;

};

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIPYTHONTESTINTERFACEEXTRA \
  NS_IMETHOD MultiplyEachItemInIntegerArray(PRInt32 val, PRUint32 count, PRInt32 **valueArray); \
  NS_IMETHOD MultiplyEachItemInIntegerArrayAndAppend(PRInt32 val, PRUint32 *count, PRInt32 **valueArray); \
  NS_IMETHOD CompareStringArrays(const char **arr1, const char **arr2, PRUint32 count, PRInt16 *result); \
  NS_IMETHOD DoubleStringArray(PRUint32 *count, char ***valueArray); \
  NS_IMETHOD ReverseStringArray(PRUint32 count, char ***valueArray); \
  NS_IMETHOD DoubleString(PRUint32 *count, char **str); \
  NS_IMETHOD DoubleString2(PRUint32 in_count, const char *in_str, PRUint32 *out_count, char **out_str); \
  NS_IMETHOD DoubleString3(PRUint32 in_count, const char *in_str, PRUint32 *out_count, char **out_str); \
  NS_IMETHOD DoubleString4(const char *in_str, PRUint32 *count, char **out_str); \
  NS_IMETHOD UpString(PRUint32 count, char **str); \
  NS_IMETHOD UpString2(PRUint32 count, const char *in_str, char **out_str); \
  NS_IMETHOD CopyUTF8String(const nsACString & in_str, nsACString & out_str); \
  NS_IMETHOD CopyUTF8String2(const nsACString & in_str, nsACString & out_str); \
  NS_IMETHOD GetFixedString(PRUint32 count, char **out_str); \
  NS_IMETHOD DoubleWideString(PRUint32 *count, PRUnichar **str); \
  NS_IMETHOD DoubleWideString2(PRUint32 in_count, const PRUnichar *in_str, PRUint32 *out_count, PRUnichar **out_str); \
  NS_IMETHOD DoubleWideString3(PRUint32 in_count, const PRUnichar *in_str, PRUint32 *out_count, PRUnichar **out_str); \
  NS_IMETHOD DoubleWideString4(const PRUnichar *in_str, PRUint32 *count, PRUnichar **out_str); \
  NS_IMETHOD UpWideString(PRUint32 count, PRUnichar **str); \
  NS_IMETHOD UpWideString2(PRUint32 count, const PRUnichar *in_str, PRUnichar **out_str); \
  NS_IMETHOD GetFixedWideString(PRUint32 count, char **out_str); \
  NS_IMETHOD GetStrings(PRUint32 *count, char ***str); \
  NS_IMETHOD UpOctetArray(PRUint32 *count, PRUint8 **data); \
  NS_IMETHOD UpOctetArray2(PRUint32 *count, PRUint8 **data); \
  NS_IMETHOD CheckInterfaceArray(PRUint32 count, nsISupports **data, PRBool *all_non_null); \
  NS_IMETHOD CopyInterfaceArray(PRUint32 count, nsISupports **data, nsISupports ***out_data, PRUint32 *out_count); \
  NS_IMETHOD GetInterfaceArray(PRUint32 *count, nsISupports ***data); \
  NS_IMETHOD ExtendInterfaceArray(PRUint32 *count, nsISupports ***data); \
  NS_IMETHOD CheckIIDArray(PRUint32 count, const nsIID & *data, PRBool *all_mine); \
  NS_IMETHOD GetIIDArray(PRUint32 *count, nsIID & **data); \
  NS_IMETHOD ExtendIIDArray(PRUint32 *count, nsIID & **data); \
  NS_IMETHOD SumArrays(PRUint32 count, PRInt32 *array1, PRInt32 *array2, PRInt32 *result); \
  NS_IMETHOD GetArrays(PRUint32 *count, PRInt32 **array1, PRInt32 **array2); \
  NS_IMETHOD GetFixedArray(PRUint32 count, PRInt32 **array1); \
  NS_IMETHOD CopyArray(PRUint32 count, PRInt32 *array1, PRInt32 **array2); \
  NS_IMETHOD CopyAndDoubleArray(PRUint32 *count, PRInt32 *array1, PRInt32 **array2); \
  NS_IMETHOD AppendArray(PRUint32 *count, PRInt32 *array1, PRInt32 **array2); \
  NS_IMETHOD AppendVariant(nsIVariant *variant, nsIVariant **result); \
  NS_IMETHOD CopyVariant(nsIVariant *variant, nsIVariant **_retval); \
  NS_IMETHOD SumVariants(PRUint32 incount, nsIVariant **variants, nsIVariant **_retval); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIPYTHONTESTINTERFACEEXTRA(_to) \
  NS_IMETHOD MultiplyEachItemInIntegerArray(PRInt32 val, PRUint32 count, PRInt32 **valueArray) { return _to MultiplyEachItemInIntegerArray(val, count, valueArray); } \
  NS_IMETHOD MultiplyEachItemInIntegerArrayAndAppend(PRInt32 val, PRUint32 *count, PRInt32 **valueArray) { return _to MultiplyEachItemInIntegerArrayAndAppend(val, count, valueArray); } \
  NS_IMETHOD CompareStringArrays(const char **arr1, const char **arr2, PRUint32 count, PRInt16 *result) { return _to CompareStringArrays(arr1, arr2, count, result); } \
  NS_IMETHOD DoubleStringArray(PRUint32 *count, char ***valueArray) { return _to DoubleStringArray(count, valueArray); } \
  NS_IMETHOD ReverseStringArray(PRUint32 count, char ***valueArray) { return _to ReverseStringArray(count, valueArray); } \
  NS_IMETHOD DoubleString(PRUint32 *count, char **str) { return _to DoubleString(count, str); } \
  NS_IMETHOD DoubleString2(PRUint32 in_count, const char *in_str, PRUint32 *out_count, char **out_str) { return _to DoubleString2(in_count, in_str, out_count, out_str); } \
  NS_IMETHOD DoubleString3(PRUint32 in_count, const char *in_str, PRUint32 *out_count, char **out_str) { return _to DoubleString3(in_count, in_str, out_count, out_str); } \
  NS_IMETHOD DoubleString4(const char *in_str, PRUint32 *count, char **out_str) { return _to DoubleString4(in_str, count, out_str); } \
  NS_IMETHOD UpString(PRUint32 count, char **str) { return _to UpString(count, str); } \
  NS_IMETHOD UpString2(PRUint32 count, const char *in_str, char **out_str) { return _to UpString2(count, in_str, out_str); } \
  NS_IMETHOD CopyUTF8String(const nsACString & in_str, nsACString & out_str) { return _to CopyUTF8String(in_str, out_str); } \
  NS_IMETHOD CopyUTF8String2(const nsACString & in_str, nsACString & out_str) { return _to CopyUTF8String2(in_str, out_str); } \
  NS_IMETHOD GetFixedString(PRUint32 count, char **out_str) { return _to GetFixedString(count, out_str); } \
  NS_IMETHOD DoubleWideString(PRUint32 *count, PRUnichar **str) { return _to DoubleWideString(count, str); } \
  NS_IMETHOD DoubleWideString2(PRUint32 in_count, const PRUnichar *in_str, PRUint32 *out_count, PRUnichar **out_str) { return _to DoubleWideString2(in_count, in_str, out_count, out_str); } \
  NS_IMETHOD DoubleWideString3(PRUint32 in_count, const PRUnichar *in_str, PRUint32 *out_count, PRUnichar **out_str) { return _to DoubleWideString3(in_count, in_str, out_count, out_str); } \
  NS_IMETHOD DoubleWideString4(const PRUnichar *in_str, PRUint32 *count, PRUnichar **out_str) { return _to DoubleWideString4(in_str, count, out_str); } \
  NS_IMETHOD UpWideString(PRUint32 count, PRUnichar **str) { return _to UpWideString(count, str); } \
  NS_IMETHOD UpWideString2(PRUint32 count, const PRUnichar *in_str, PRUnichar **out_str) { return _to UpWideString2(count, in_str, out_str); } \
  NS_IMETHOD GetFixedWideString(PRUint32 count, char **out_str) { return _to GetFixedWideString(count, out_str); } \
  NS_IMETHOD GetStrings(PRUint32 *count, char ***str) { return _to GetStrings(count, str); } \
  NS_IMETHOD UpOctetArray(PRUint32 *count, PRUint8 **data) { return _to UpOctetArray(count, data); } \
  NS_IMETHOD UpOctetArray2(PRUint32 *count, PRUint8 **data) { return _to UpOctetArray2(count, data); } \
  NS_IMETHOD CheckInterfaceArray(PRUint32 count, nsISupports **data, PRBool *all_non_null) { return _to CheckInterfaceArray(count, data, all_non_null); } \
  NS_IMETHOD CopyInterfaceArray(PRUint32 count, nsISupports **data, nsISupports ***out_data, PRUint32 *out_count) { return _to CopyInterfaceArray(count, data, out_data, out_count); } \
  NS_IMETHOD GetInterfaceArray(PRUint32 *count, nsISupports ***data) { return _to GetInterfaceArray(count, data); } \
  NS_IMETHOD ExtendInterfaceArray(PRUint32 *count, nsISupports ***data) { return _to ExtendInterfaceArray(count, data); } \
  NS_IMETHOD CheckIIDArray(PRUint32 count, const nsIID & *data, PRBool *all_mine) { return _to CheckIIDArray(count, data, all_mine); } \
  NS_IMETHOD GetIIDArray(PRUint32 *count, nsIID & **data) { return _to GetIIDArray(count, data); } \
  NS_IMETHOD ExtendIIDArray(PRUint32 *count, nsIID & **data) { return _to ExtendIIDArray(count, data); } \
  NS_IMETHOD SumArrays(PRUint32 count, PRInt32 *array1, PRInt32 *array2, PRInt32 *result) { return _to SumArrays(count, array1, array2, result); } \
  NS_IMETHOD GetArrays(PRUint32 *count, PRInt32 **array1, PRInt32 **array2) { return _to GetArrays(count, array1, array2); } \
  NS_IMETHOD GetFixedArray(PRUint32 count, PRInt32 **array1) { return _to GetFixedArray(count, array1); } \
  NS_IMETHOD CopyArray(PRUint32 count, PRInt32 *array1, PRInt32 **array2) { return _to CopyArray(count, array1, array2); } \
  NS_IMETHOD CopyAndDoubleArray(PRUint32 *count, PRInt32 *array1, PRInt32 **array2) { return _to CopyAndDoubleArray(count, array1, array2); } \
  NS_IMETHOD AppendArray(PRUint32 *count, PRInt32 *array1, PRInt32 **array2) { return _to AppendArray(count, array1, array2); } \
  NS_IMETHOD AppendVariant(nsIVariant *variant, nsIVariant **result) { return _to AppendVariant(variant, result); } \
  NS_IMETHOD CopyVariant(nsIVariant *variant, nsIVariant **_retval) { return _to CopyVariant(variant, _retval); } \
  NS_IMETHOD SumVariants(PRUint32 incount, nsIVariant **variants, nsIVariant **_retval) { return _to SumVariants(incount, variants, _retval); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIPYTHONTESTINTERFACEEXTRA(_to) \
  NS_IMETHOD MultiplyEachItemInIntegerArray(PRInt32 val, PRUint32 count, PRInt32 **valueArray) { return !_to ? NS_ERROR_NULL_POINTER : _to->MultiplyEachItemInIntegerArray(val, count, valueArray); } \
  NS_IMETHOD MultiplyEachItemInIntegerArrayAndAppend(PRInt32 val, PRUint32 *count, PRInt32 **valueArray) { return !_to ? NS_ERROR_NULL_POINTER : _to->MultiplyEachItemInIntegerArrayAndAppend(val, count, valueArray); } \
  NS_IMETHOD CompareStringArrays(const char **arr1, const char **arr2, PRUint32 count, PRInt16 *result) { return !_to ? NS_ERROR_NULL_POINTER : _to->CompareStringArrays(arr1, arr2, count, result); } \
  NS_IMETHOD DoubleStringArray(PRUint32 *count, char ***valueArray) { return !_to ? NS_ERROR_NULL_POINTER : _to->DoubleStringArray(count, valueArray); } \
  NS_IMETHOD ReverseStringArray(PRUint32 count, char ***valueArray) { return !_to ? NS_ERROR_NULL_POINTER : _to->ReverseStringArray(count, valueArray); } \
  NS_IMETHOD DoubleString(PRUint32 *count, char **str) { return !_to ? NS_ERROR_NULL_POINTER : _to->DoubleString(count, str); } \
  NS_IMETHOD DoubleString2(PRUint32 in_count, const char *in_str, PRUint32 *out_count, char **out_str) { return !_to ? NS_ERROR_NULL_POINTER : _to->DoubleString2(in_count, in_str, out_count, out_str); } \
  NS_IMETHOD DoubleString3(PRUint32 in_count, const char *in_str, PRUint32 *out_count, char **out_str) { return !_to ? NS_ERROR_NULL_POINTER : _to->DoubleString3(in_count, in_str, out_count, out_str); } \
  NS_IMETHOD DoubleString4(const char *in_str, PRUint32 *count, char **out_str) { return !_to ? NS_ERROR_NULL_POINTER : _to->DoubleString4(in_str, count, out_str); } \
  NS_IMETHOD UpString(PRUint32 count, char **str) { return !_to ? NS_ERROR_NULL_POINTER : _to->UpString(count, str); } \
  NS_IMETHOD UpString2(PRUint32 count, const char *in_str, char **out_str) { return !_to ? NS_ERROR_NULL_POINTER : _to->UpString2(count, in_str, out_str); } \
  NS_IMETHOD CopyUTF8String(const nsACString & in_str, nsACString & out_str) { return !_to ? NS_ERROR_NULL_POINTER : _to->CopyUTF8String(in_str, out_str); } \
  NS_IMETHOD CopyUTF8String2(const nsACString & in_str, nsACString & out_str) { return !_to ? NS_ERROR_NULL_POINTER : _to->CopyUTF8String2(in_str, out_str); } \
  NS_IMETHOD GetFixedString(PRUint32 count, char **out_str) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetFixedString(count, out_str); } \
  NS_IMETHOD DoubleWideString(PRUint32 *count, PRUnichar **str) { return !_to ? NS_ERROR_NULL_POINTER : _to->DoubleWideString(count, str); } \
  NS_IMETHOD DoubleWideString2(PRUint32 in_count, const PRUnichar *in_str, PRUint32 *out_count, PRUnichar **out_str) { return !_to ? NS_ERROR_NULL_POINTER : _to->DoubleWideString2(in_count, in_str, out_count, out_str); } \
  NS_IMETHOD DoubleWideString3(PRUint32 in_count, const PRUnichar *in_str, PRUint32 *out_count, PRUnichar **out_str) { return !_to ? NS_ERROR_NULL_POINTER : _to->DoubleWideString3(in_count, in_str, out_count, out_str); } \
  NS_IMETHOD DoubleWideString4(const PRUnichar *in_str, PRUint32 *count, PRUnichar **out_str) { return !_to ? NS_ERROR_NULL_POINTER : _to->DoubleWideString4(in_str, count, out_str); } \
  NS_IMETHOD UpWideString(PRUint32 count, PRUnichar **str) { return !_to ? NS_ERROR_NULL_POINTER : _to->UpWideString(count, str); } \
  NS_IMETHOD UpWideString2(PRUint32 count, const PRUnichar *in_str, PRUnichar **out_str) { return !_to ? NS_ERROR_NULL_POINTER : _to->UpWideString2(count, in_str, out_str); } \
  NS_IMETHOD GetFixedWideString(PRUint32 count, char **out_str) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetFixedWideString(count, out_str); } \
  NS_IMETHOD GetStrings(PRUint32 *count, char ***str) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetStrings(count, str); } \
  NS_IMETHOD UpOctetArray(PRUint32 *count, PRUint8 **data) { return !_to ? NS_ERROR_NULL_POINTER : _to->UpOctetArray(count, data); } \
  NS_IMETHOD UpOctetArray2(PRUint32 *count, PRUint8 **data) { return !_to ? NS_ERROR_NULL_POINTER : _to->UpOctetArray2(count, data); } \
  NS_IMETHOD CheckInterfaceArray(PRUint32 count, nsISupports **data, PRBool *all_non_null) { return !_to ? NS_ERROR_NULL_POINTER : _to->CheckInterfaceArray(count, data, all_non_null); } \
  NS_IMETHOD CopyInterfaceArray(PRUint32 count, nsISupports **data, nsISupports ***out_data, PRUint32 *out_count) { return !_to ? NS_ERROR_NULL_POINTER : _to->CopyInterfaceArray(count, data, out_data, out_count); } \
  NS_IMETHOD GetInterfaceArray(PRUint32 *count, nsISupports ***data) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetInterfaceArray(count, data); } \
  NS_IMETHOD ExtendInterfaceArray(PRUint32 *count, nsISupports ***data) { return !_to ? NS_ERROR_NULL_POINTER : _to->ExtendInterfaceArray(count, data); } \
  NS_IMETHOD CheckIIDArray(PRUint32 count, const nsIID & *data, PRBool *all_mine) { return !_to ? NS_ERROR_NULL_POINTER : _to->CheckIIDArray(count, data, all_mine); } \
  NS_IMETHOD GetIIDArray(PRUint32 *count, nsIID & **data) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetIIDArray(count, data); } \
  NS_IMETHOD ExtendIIDArray(PRUint32 *count, nsIID & **data) { return !_to ? NS_ERROR_NULL_POINTER : _to->ExtendIIDArray(count, data); } \
  NS_IMETHOD SumArrays(PRUint32 count, PRInt32 *array1, PRInt32 *array2, PRInt32 *result) { return !_to ? NS_ERROR_NULL_POINTER : _to->SumArrays(count, array1, array2, result); } \
  NS_IMETHOD GetArrays(PRUint32 *count, PRInt32 **array1, PRInt32 **array2) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetArrays(count, array1, array2); } \
  NS_IMETHOD GetFixedArray(PRUint32 count, PRInt32 **array1) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetFixedArray(count, array1); } \
  NS_IMETHOD CopyArray(PRUint32 count, PRInt32 *array1, PRInt32 **array2) { return !_to ? NS_ERROR_NULL_POINTER : _to->CopyArray(count, array1, array2); } \
  NS_IMETHOD CopyAndDoubleArray(PRUint32 *count, PRInt32 *array1, PRInt32 **array2) { return !_to ? NS_ERROR_NULL_POINTER : _to->CopyAndDoubleArray(count, array1, array2); } \
  NS_IMETHOD AppendArray(PRUint32 *count, PRInt32 *array1, PRInt32 **array2) { return !_to ? NS_ERROR_NULL_POINTER : _to->AppendArray(count, array1, array2); } \
  NS_IMETHOD AppendVariant(nsIVariant *variant, nsIVariant **result) { return !_to ? NS_ERROR_NULL_POINTER : _to->AppendVariant(variant, result); } \
  NS_IMETHOD CopyVariant(nsIVariant *variant, nsIVariant **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->CopyVariant(variant, _retval); } \
  NS_IMETHOD SumVariants(PRUint32 incount, nsIVariant **variants, nsIVariant **_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->SumVariants(incount, variants, _retval); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsPythonTestInterfaceExtra : public nsIPythonTestInterfaceExtra
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPYTHONTESTINTERFACEEXTRA

  nsPythonTestInterfaceExtra();

private:
  ~nsPythonTestInterfaceExtra();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsPythonTestInterfaceExtra, nsIPythonTestInterfaceExtra)

nsPythonTestInterfaceExtra::nsPythonTestInterfaceExtra()
{
  /* member initializers and constructor code */
}

nsPythonTestInterfaceExtra::~nsPythonTestInterfaceExtra()
{
  /* destructor code */
}

/* void MultiplyEachItemInIntegerArray (in PRInt32 val, in PRUint32 count, [array, size_is (count)] inout PRInt32 valueArray); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::MultiplyEachItemInIntegerArray(PRInt32 val, PRUint32 count, PRInt32 **valueArray)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void MultiplyEachItemInIntegerArrayAndAppend (in PRInt32 val, inout PRUint32 count, [array, size_is (count)] inout PRInt32 valueArray); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::MultiplyEachItemInIntegerArrayAndAppend(PRInt32 val, PRUint32 *count, PRInt32 **valueArray)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void CompareStringArrays ([array, size_is (count)] in string arr1, [array, size_is (count)] in string arr2, in unsigned long count, [retval] out short result); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::CompareStringArrays(const char **arr1, const char **arr2, PRUint32 count, PRInt16 *result)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void DoubleStringArray (inout PRUint32 count, [array, size_is (count)] inout string valueArray); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::DoubleStringArray(PRUint32 *count, char ***valueArray)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void ReverseStringArray (in PRUint32 count, [array, size_is (count)] inout string valueArray); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::ReverseStringArray(PRUint32 count, char ***valueArray)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void DoubleString (inout PRUint32 count, [size_is (count)] inout string str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::DoubleString(PRUint32 *count, char **str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void DoubleString2 (in PRUint32 in_count, [size_is (in_count)] in string in_str, out PRUint32 out_count, [size_is (out_count)] out string out_str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::DoubleString2(PRUint32 in_count, const char *in_str, PRUint32 *out_count, char **out_str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void DoubleString3 (in PRUint32 in_count, [size_is (in_count)] in string in_str, out PRUint32 out_count, [size_is (out_count), retval] out string out_str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::DoubleString3(PRUint32 in_count, const char *in_str, PRUint32 *out_count, char **out_str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void DoubleString4 ([size_is (count)] in string in_str, inout PRUint32 count, [size_is (count)] out string out_str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::DoubleString4(const char *in_str, PRUint32 *count, char **out_str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void UpString (in PRUint32 count, [size_is (count)] inout string str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::UpString(PRUint32 count, char **str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void UpString2 (in PRUint32 count, [size_is (count)] in string in_str, [size_is (count)] out string out_str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::UpString2(PRUint32 count, const char *in_str, char **out_str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void CopyUTF8String (in AUTF8String in_str, out AUTF8String out_str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::CopyUTF8String(const nsACString & in_str, nsACString & out_str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void CopyUTF8String2 (in AUTF8String in_str, out AUTF8String out_str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::CopyUTF8String2(const nsACString & in_str, nsACString & out_str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void GetFixedString (in PRUint32 count, [size_is (count)] out string out_str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::GetFixedString(PRUint32 count, char **out_str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void DoubleWideString (inout PRUint32 count, [size_is (count)] inout wstring str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::DoubleWideString(PRUint32 *count, PRUnichar **str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void DoubleWideString2 (in PRUint32 in_count, [size_is (in_count)] in wstring in_str, out PRUint32 out_count, [size_is (out_count)] out wstring out_str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::DoubleWideString2(PRUint32 in_count, const PRUnichar *in_str, PRUint32 *out_count, PRUnichar **out_str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void DoubleWideString3 (in PRUint32 in_count, [size_is (in_count)] in wstring in_str, out PRUint32 out_count, [size_is (out_count), retval] out wstring out_str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::DoubleWideString3(PRUint32 in_count, const PRUnichar *in_str, PRUint32 *out_count, PRUnichar **out_str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void DoubleWideString4 ([size_is (count)] in wstring in_str, inout PRUint32 count, [size_is (count)] out wstring out_str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::DoubleWideString4(const PRUnichar *in_str, PRUint32 *count, PRUnichar **out_str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void UpWideString (in PRUint32 count, [size_is (count)] inout wstring str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::UpWideString(PRUint32 count, PRUnichar **str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void UpWideString2 (in PRUint32 count, [size_is (count)] in wstring in_str, [size_is (count)] out wstring out_str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::UpWideString2(PRUint32 count, const PRUnichar *in_str, PRUnichar **out_str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void GetFixedWideString (in PRUint32 count, [size_is (count)] out string out_str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::GetFixedWideString(PRUint32 count, char **out_str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void GetStrings (out PRUint32 count, [array, size_is (count), retval] out string str); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::GetStrings(PRUint32 *count, char ***str)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void UpOctetArray (inout PRUint32 count, [array, size_is (count)] inout PRUint8 data); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::UpOctetArray(PRUint32 *count, PRUint8 **data)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void UpOctetArray2 (inout PRUint32 count, [array, size_is (count)] inout PRUint8 data); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::UpOctetArray2(PRUint32 *count, PRUint8 **data)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void CheckInterfaceArray (in PRUint32 count, [array, size_is (count)] in nsISupports data, [retval] out PRBool all_non_null); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::CheckInterfaceArray(PRUint32 count, nsISupports **data, PRBool *all_non_null)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void CopyInterfaceArray (in PRUint32 count, [array, size_is (count)] in nsISupports data, [array, size_is (out_count)] out nsISupports out_data, out PRUint32 out_count); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::CopyInterfaceArray(PRUint32 count, nsISupports **data, nsISupports ***out_data, PRUint32 *out_count)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void GetInterfaceArray (out PRUint32 count, [array, size_is (count)] out nsISupports data); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::GetInterfaceArray(PRUint32 *count, nsISupports ***data)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void ExtendInterfaceArray (inout PRUint32 count, [array, size_is (count)] inout nsISupports data); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::ExtendInterfaceArray(PRUint32 *count, nsISupports ***data)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void CheckIIDArray (in PRUint32 count, [array, size_is (count)] in nsIIDRef data, [retval] out PRBool all_mine); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::CheckIIDArray(PRUint32 count, const nsIID & *data, PRBool *all_mine)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void GetIIDArray (out PRUint32 count, [array, size_is (count)] out nsIIDRef data); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::GetIIDArray(PRUint32 *count, nsIID & **data)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void ExtendIIDArray (inout PRUint32 count, [array, size_is (count)] inout nsIIDRef data); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::ExtendIIDArray(PRUint32 *count, nsIID & **data)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void SumArrays (in PRUint32 count, [array, size_is (count)] in PRInt32 array1, [array, size_is (count)] in PRInt32 array2, [retval] out PRInt32 result); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::SumArrays(PRUint32 count, PRInt32 *array1, PRInt32 *array2, PRInt32 *result)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void GetArrays (out PRUint32 count, [array, size_is (count)] out PRInt32 array1, [array, size_is (count)] out PRInt32 array2); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::GetArrays(PRUint32 *count, PRInt32 **array1, PRInt32 **array2)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void GetFixedArray (in PRUint32 count, [array, size_is (count)] out PRInt32 array1); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::GetFixedArray(PRUint32 count, PRInt32 **array1)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void CopyArray (in PRUint32 count, [array, size_is (count)] in PRInt32 array1, [array, size_is (count)] out PRInt32 array2); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::CopyArray(PRUint32 count, PRInt32 *array1, PRInt32 **array2)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void CopyAndDoubleArray (inout PRUint32 count, [array, size_is (count)] in PRInt32 array1, [array, size_is (count)] out PRInt32 array2); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::CopyAndDoubleArray(PRUint32 *count, PRInt32 *array1, PRInt32 **array2)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void AppendArray (inout PRUint32 count, [array, size_is (count)] in PRInt32 array1, [array, size_is (count)] inout PRInt32 array2); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::AppendArray(PRUint32 *count, PRInt32 *array1, PRInt32 **array2)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void AppendVariant (in nsIVariant variant, inout nsIVariant result); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::AppendVariant(nsIVariant *variant, nsIVariant **result)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIVariant CopyVariant (in nsIVariant variant); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::CopyVariant(nsIVariant *variant, nsIVariant **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIVariant SumVariants (in PRUint32 incount, [array, size_is (incount)] in nsIVariant variants); */
NS_IMETHODIMP nsPythonTestInterfaceExtra::SumVariants(PRUint32 incount, nsIVariant **variants, nsIVariant **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    nsIPythonTestInterfaceDOMStrings */
#define NS_IPYTHONTESTINTERFACEDOMSTRINGS_IID_STR "657ae651-a973-4818-8c06-f4b948b3d758"

#define NS_IPYTHONTESTINTERFACEDOMSTRINGS_IID \
  {0x657ae651, 0xa973, 0x4818, \
    { 0x8c, 0x06, 0xf4, 0xb9, 0x48, 0xb3, 0xd7, 0x58 }}

class NS_NO_VTABLE nsIPythonTestInterfaceDOMStrings : public nsIPythonTestInterfaceExtra {
 public: 

  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IPYTHONTESTINTERFACEDOMSTRINGS_IID)

  /* DOMString GetDOMStringResult (in PRInt32 length); */
  NS_IMETHOD GetDOMStringResult(PRInt32 length, nsAString & _retval) = 0;

  /* void GetDOMStringOut (in PRInt32 length, [retval] out DOMString s); */
  NS_IMETHOD GetDOMStringOut(PRInt32 length, nsAString & s) = 0;

  /* PRUint32 GetDOMStringLength (in DOMString s); */
  NS_IMETHOD GetDOMStringLength(const nsAString & s, PRUint32 *_retval) = 0;

  /* PRUint32 GetDOMStringRefLength (in DOMStringRef s); */
  NS_IMETHOD GetDOMStringRefLength(const nsAString & s, PRUint32 *_retval) = 0;

  /* PRUint32 GetDOMStringPtrLength (in DOMStringPtr s); */
  NS_IMETHOD GetDOMStringPtrLength(const nsAString * s, PRUint32 *_retval) = 0;

  /* void ConcatDOMStrings (in DOMString s1, in DOMString s2, out DOMString ret); */
  NS_IMETHOD ConcatDOMStrings(const nsAString & s1, const nsAString & s2, nsAString & ret) = 0;

  /* attribute DOMString domstring_value; */
  NS_IMETHOD GetDomstring_value(nsAString & aDomstring_value) = 0;
  NS_IMETHOD SetDomstring_value(const nsAString & aDomstring_value) = 0;

  /* readonly attribute DOMString domstring_value_ro; */
  NS_IMETHOD GetDomstring_value_ro(nsAString & aDomstring_value_ro) = 0;

};

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIPYTHONTESTINTERFACEDOMSTRINGS \
  NS_IMETHOD GetDOMStringResult(PRInt32 length, nsAString & _retval); \
  NS_IMETHOD GetDOMStringOut(PRInt32 length, nsAString & s); \
  NS_IMETHOD GetDOMStringLength(const nsAString & s, PRUint32 *_retval); \
  NS_IMETHOD GetDOMStringRefLength(const nsAString & s, PRUint32 *_retval); \
  NS_IMETHOD GetDOMStringPtrLength(const nsAString * s, PRUint32 *_retval); \
  NS_IMETHOD ConcatDOMStrings(const nsAString & s1, const nsAString & s2, nsAString & ret); \
  NS_IMETHOD GetDomstring_value(nsAString & aDomstring_value); \
  NS_IMETHOD SetDomstring_value(const nsAString & aDomstring_value); \
  NS_IMETHOD GetDomstring_value_ro(nsAString & aDomstring_value_ro); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIPYTHONTESTINTERFACEDOMSTRINGS(_to) \
  NS_IMETHOD GetDOMStringResult(PRInt32 length, nsAString & _retval) { return _to GetDOMStringResult(length, _retval); } \
  NS_IMETHOD GetDOMStringOut(PRInt32 length, nsAString & s) { return _to GetDOMStringOut(length, s); } \
  NS_IMETHOD GetDOMStringLength(const nsAString & s, PRUint32 *_retval) { return _to GetDOMStringLength(s, _retval); } \
  NS_IMETHOD GetDOMStringRefLength(const nsAString & s, PRUint32 *_retval) { return _to GetDOMStringRefLength(s, _retval); } \
  NS_IMETHOD GetDOMStringPtrLength(const nsAString * s, PRUint32 *_retval) { return _to GetDOMStringPtrLength(s, _retval); } \
  NS_IMETHOD ConcatDOMStrings(const nsAString & s1, const nsAString & s2, nsAString & ret) { return _to ConcatDOMStrings(s1, s2, ret); } \
  NS_IMETHOD GetDomstring_value(nsAString & aDomstring_value) { return _to GetDomstring_value(aDomstring_value); } \
  NS_IMETHOD SetDomstring_value(const nsAString & aDomstring_value) { return _to SetDomstring_value(aDomstring_value); } \
  NS_IMETHOD GetDomstring_value_ro(nsAString & aDomstring_value_ro) { return _to GetDomstring_value_ro(aDomstring_value_ro); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIPYTHONTESTINTERFACEDOMSTRINGS(_to) \
  NS_IMETHOD GetDOMStringResult(PRInt32 length, nsAString & _retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDOMStringResult(length, _retval); } \
  NS_IMETHOD GetDOMStringOut(PRInt32 length, nsAString & s) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDOMStringOut(length, s); } \
  NS_IMETHOD GetDOMStringLength(const nsAString & s, PRUint32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDOMStringLength(s, _retval); } \
  NS_IMETHOD GetDOMStringRefLength(const nsAString & s, PRUint32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDOMStringRefLength(s, _retval); } \
  NS_IMETHOD GetDOMStringPtrLength(const nsAString * s, PRUint32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDOMStringPtrLength(s, _retval); } \
  NS_IMETHOD ConcatDOMStrings(const nsAString & s1, const nsAString & s2, nsAString & ret) { return !_to ? NS_ERROR_NULL_POINTER : _to->ConcatDOMStrings(s1, s2, ret); } \
  NS_IMETHOD GetDomstring_value(nsAString & aDomstring_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDomstring_value(aDomstring_value); } \
  NS_IMETHOD SetDomstring_value(const nsAString & aDomstring_value) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetDomstring_value(aDomstring_value); } \
  NS_IMETHOD GetDomstring_value_ro(nsAString & aDomstring_value_ro) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDomstring_value_ro(aDomstring_value_ro); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsPythonTestInterfaceDOMStrings : public nsIPythonTestInterfaceDOMStrings
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPYTHONTESTINTERFACEDOMSTRINGS

  nsPythonTestInterfaceDOMStrings();

private:
  ~nsPythonTestInterfaceDOMStrings();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsPythonTestInterfaceDOMStrings, nsIPythonTestInterfaceDOMStrings)

nsPythonTestInterfaceDOMStrings::nsPythonTestInterfaceDOMStrings()
{
  /* member initializers and constructor code */
}

nsPythonTestInterfaceDOMStrings::~nsPythonTestInterfaceDOMStrings()
{
  /* destructor code */
}

/* DOMString GetDOMStringResult (in PRInt32 length); */
NS_IMETHODIMP nsPythonTestInterfaceDOMStrings::GetDOMStringResult(PRInt32 length, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void GetDOMStringOut (in PRInt32 length, [retval] out DOMString s); */
NS_IMETHODIMP nsPythonTestInterfaceDOMStrings::GetDOMStringOut(PRInt32 length, nsAString & s)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* PRUint32 GetDOMStringLength (in DOMString s); */
NS_IMETHODIMP nsPythonTestInterfaceDOMStrings::GetDOMStringLength(const nsAString & s, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* PRUint32 GetDOMStringRefLength (in DOMStringRef s); */
NS_IMETHODIMP nsPythonTestInterfaceDOMStrings::GetDOMStringRefLength(const nsAString & s, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* PRUint32 GetDOMStringPtrLength (in DOMStringPtr s); */
NS_IMETHODIMP nsPythonTestInterfaceDOMStrings::GetDOMStringPtrLength(const nsAString * s, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void ConcatDOMStrings (in DOMString s1, in DOMString s2, out DOMString ret); */
NS_IMETHODIMP nsPythonTestInterfaceDOMStrings::ConcatDOMStrings(const nsAString & s1, const nsAString & s2, nsAString & ret)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute DOMString domstring_value; */
NS_IMETHODIMP nsPythonTestInterfaceDOMStrings::GetDomstring_value(nsAString & aDomstring_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsPythonTestInterfaceDOMStrings::SetDomstring_value(const nsAString & aDomstring_value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute DOMString domstring_value_ro; */
NS_IMETHODIMP nsPythonTestInterfaceDOMStrings::GetDomstring_value_ro(nsAString & aDomstring_value_ro)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_py_test_component_h__ */
