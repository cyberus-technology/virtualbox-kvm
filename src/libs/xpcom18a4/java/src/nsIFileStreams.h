/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM nsIFileStreams.idl
 */

#ifndef __gen_nsIFileStreams_h__
#define __gen_nsIFileStreams_h__


#ifndef __gen_nsIInputStream_h__
#include "nsIInputStream.h"
#endif

#ifndef __gen_nsIOutputStream_h__
#include "nsIOutputStream.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class nsIFile; /* forward declaration */


/* starting interface:    nsIFileInputStream */
#define NS_IFILEINPUTSTREAM_IID_STR "e3d56a20-c7ec-11d3-8cda-0060b0fc14a3"

#define NS_IFILEINPUTSTREAM_IID \
  {0xe3d56a20, 0xc7ec, 0x11d3, \
    { 0x8c, 0xda, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3 }}

/**
 * An input stream that allows you to read from a file.
 */
class NS_NO_VTABLE nsIFileInputStream : public nsIInputStream {
 public: 

  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IFILEINPUTSTREAM_IID)

  /**
     * @param file          file to read from (must QI to nsILocalFile)
     * @param ioFlags       file open flags listed in prio.h
     * @param perm          file mode bits listed in prio.h
     * @param behaviorFlags flags specifying various behaviors of the class
     *        (see enumerations in the class)
     */
  /* void init (in nsIFile file, in long ioFlags, in long perm, in long behaviorFlags); */
  NS_IMETHOD Init(nsIFile *file, PRInt32 ioFlags, PRInt32 perm, PRInt32 behaviorFlags) = 0;

  /**
     * If this is set, the file will be deleted by the time the stream is
     * closed.  It may be removed before the stream is closed if it is possible
     * to delete it and still read from it.
     *
     * If OPEN_ON_READ is defined, and the file was recreated after the first
     * delete, the file will be deleted again when it is closed again.
     */
  enum { DELETE_ON_CLOSE = 2 };

  /**
     * If this is set, the file will close automatically when the end of the
     * file is reached.
     */
  enum { CLOSE_ON_EOF = 4 };

  /**
     * If this is set, the file will be reopened whenever Seek(0) occurs.  If
     * the file is already open and the seek occurs, it will happen naturally.
     * (The file will only be reopened if it is closed for some reason.)
     */
  enum { REOPEN_ON_REWIND = 8 };

};

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIFILEINPUTSTREAM \
  NS_IMETHOD Init(nsIFile *file, PRInt32 ioFlags, PRInt32 perm, PRInt32 behaviorFlags); \

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIFILEINPUTSTREAM(_to) \
  NS_IMETHOD Init(nsIFile *file, PRInt32 ioFlags, PRInt32 perm, PRInt32 behaviorFlags) { return _to Init(file, ioFlags, perm, behaviorFlags); } \

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIFILEINPUTSTREAM(_to) \
  NS_IMETHOD Init(nsIFile *file, PRInt32 ioFlags, PRInt32 perm, PRInt32 behaviorFlags) { return !_to ? NS_ERROR_NULL_POINTER : _to->Init(file, ioFlags, perm, behaviorFlags); } \

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsFileInputStream : public nsIFileInputStream
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFILEINPUTSTREAM

  nsFileInputStream();

private:
  ~nsFileInputStream();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsFileInputStream, nsIFileInputStream)

nsFileInputStream::nsFileInputStream()
{
  /* member initializers and constructor code */
}

nsFileInputStream::~nsFileInputStream()
{
  /* destructor code */
}

/* void init (in nsIFile file, in long ioFlags, in long perm, in long behaviorFlags); */
NS_IMETHODIMP nsFileInputStream::Init(nsIFile *file, PRInt32 ioFlags, PRInt32 perm, PRInt32 behaviorFlags)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    nsIFileOutputStream */
#define NS_IFILEOUTPUTSTREAM_IID_STR "e6f68040-c7ec-11d3-8cda-0060b0fc14a3"

#define NS_IFILEOUTPUTSTREAM_IID \
  {0xe6f68040, 0xc7ec, 0x11d3, \
    { 0x8c, 0xda, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3 }}

/**
 * An output stream that lets you stream to a file.
 */
class NS_NO_VTABLE nsIFileOutputStream : public nsIOutputStream {
 public: 

  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IFILEOUTPUTSTREAM_IID)

  /**
     * @param file          - file to write to (must QI to nsILocalFile)
     * @param ioFlags       - file open flags listed in prio.h
     * @param perm          - file mode bits listed in prio.h
     * @param behaviorFlags flags specifying various behaviors of the class
     *        (currently none supported)
     */
  /* void init (in nsIFile file, in long ioFlags, in long perm, in long behaviorFlags); */
  NS_IMETHOD Init(nsIFile *file, PRInt32 ioFlags, PRInt32 perm, PRInt32 behaviorFlags) = 0;

};

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_NSIFILEOUTPUTSTREAM \
  NS_IMETHOD Init(nsIFile *file, PRInt32 ioFlags, PRInt32 perm, PRInt32 behaviorFlags); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_NSIFILEOUTPUTSTREAM(_to) \
  NS_IMETHOD Init(nsIFile *file, PRInt32 ioFlags, PRInt32 perm, PRInt32 behaviorFlags) { return _to Init(file, ioFlags, perm, behaviorFlags); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_NSIFILEOUTPUTSTREAM(_to) \
  NS_IMETHOD Init(nsIFile *file, PRInt32 ioFlags, PRInt32 perm, PRInt32 behaviorFlags) { return !_to ? NS_ERROR_NULL_POINTER : _to->Init(file, ioFlags, perm, behaviorFlags); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class nsFileOutputStream : public nsIFileOutputStream
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFILEOUTPUTSTREAM

  nsFileOutputStream();

private:
  ~nsFileOutputStream();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsFileOutputStream, nsIFileOutputStream)

nsFileOutputStream::nsFileOutputStream()
{
  /* member initializers and constructor code */
}

nsFileOutputStream::~nsFileOutputStream()
{
  /* destructor code */
}

/* void init (in nsIFile file, in long ioFlags, in long perm, in long behaviorFlags); */
NS_IMETHODIMP nsFileOutputStream::Init(nsIFile *file, PRInt32 ioFlags, PRInt32 perm, PRInt32 behaviorFlags)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_nsIFileStreams_h__ */
