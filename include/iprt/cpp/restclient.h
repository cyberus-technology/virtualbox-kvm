/** @file
 * IPRT - C++ Representational State Transfer (REST) Client Classes.
 */

/*
 * Copyright (C) 2008-2023 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_cpp_restclient_h
#define IPRT_INCLUDED_cpp_restclient_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/http.h>
#include <iprt/cpp/restbase.h>
#include <iprt/cpp/reststringmap.h>


/** @defgroup grp_rt_cpp_restclient     C++ Representational State Transfer (REST) Client Classes.
 * @ingroup grp_rt_cpp
 * @{
 */

/**
 * Specialization of RTCRestBinary for use with body parameters in a client.
 *
 * This enables registering data callbacks for provinding data to upload.
 */
class RT_DECL_CLASS RTCRestBinaryParameter : public RTCRestBinary
{
public:
    /** Default constructor. */
    RTCRestBinaryParameter() RT_NOEXCEPT;

    /** Safe copy assignment method. */
    virtual int assignCopy(RTCRestBinaryParameter const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method.
      * @note Resets callbacks and ASSUMES that @a a_cbData is the content length. */
    virtual int assignCopy(RTCRestBinary const &a_rThat) RT_NOEXCEPT RT_OVERRIDE;
    /** Safe copy assignment method.
     * @note Resets callbacks and ASSUMES that @a a_cbData is the content length. */
    virtual int assignCopy(void const *a_pvData, size_t a_cbData) RT_NOEXCEPT RT_OVERRIDE;

    /**
     * Use the specified data buffer directly.
     * @note Resets callbacks and ASSUMES that @a a_cbData is the content length. */
    virtual int assignReadOnly(void const *a_pvData, size_t a_cbData) RT_NOEXCEPT RT_OVERRIDE;
    /**
     * Use the specified data buffer directly.
     * @note This will assert and work like assignReadOnly. */
    virtual int assignWriteable(void *a_pvBuf, size_t a_cbBuf) RT_NOEXCEPT RT_OVERRIDE;

    /** Make a clone of this object. */
    inline RTCRestBinaryParameter *clone() const RT_NOEXCEPT { return (RTCRestBinaryParameter *)baseClone(); }

    /* Overridden methods: */
    virtual RTCRestObjectBase *baseClone() const RT_NOEXCEPT RT_OVERRIDE;
    virtual int resetToDefault() RT_NOEXCEPT RT_OVERRIDE;
    virtual const char *typeName(void) const RT_NOEXCEPT RT_OVERRIDE;

    /** Factory method. */
    static DECLCALLBACK(RTCRestObjectBase *) createInstance(void) RT_NOEXCEPT;

    /**
     * Retrieves the callback data.
     */
    inline void *getCallbackData() const RT_NOEXCEPT  { return m_pvCallbackData; }

    /**
     * Sets the content-type for an upload.
     *
     * @returns VINF_SUCCESS or VERR_NO_STR_MEMORY.
     * @param   a_pszContentType    The content type to set.
     *                              If NULL, no content type is set.
     */
    int setContentType(const char *a_pszContentType) RT_NOEXCEPT;

    /**
     * Gets the content type that was set.
     */
    inline RTCString const &getContentType() const RT_NOEXCEPT { return m_strContentType; }

    /**
     * Gets the content-length value (UINT64_MAX if not available).
     */
    inline uint64_t getContentLength() const RT_NOEXCEPT { return m_cbContentLength; }

    /**
     * Callback for producing bytes to upload.
     *
     * @returns IPRT status code.
     * @param   a_pThis         The related string object.
     * @param   a_pvDst         Where to put the bytes.
     * @param   a_cbDst         Max number of bytes to produce.
     * @param   a_offContent    The byte offset corresponding to the start of @a a_pvDst.
     * @param   a_pcbActual     Where to return the number of bytes actually produced.
     *
     * @remarks Use getCallbackData to get the user data.
     *
     * @note    The @a a_offContent parameter does not imply random access or anthing
     *          like that, it is just a convenience provided by the caller.  The value
     *          is the sum of the previously returned @a *pcbActual values.
     */
    typedef DECLCALLBACKTYPE(int, FNPRODUCER,(RTCRestBinaryParameter *a_pThis, void *a_pvDst, size_t a_cbDst,
                                              uint64_t a_offContent, size_t *a_pcbActual)) /*RT_NOEXCEPT*/;
    /** Pointer to a byte producer callback. */
    typedef FNPRODUCER *PFNPRODUCER;

    /**
     * Sets the producer callback.
     *
     * @param   a_pfnProducer       The callback function for producing data.
     * @param   a_pvCallbackData    Data the can be retrieved from the callback
     *                              using getCallbackData().
     * @param   a_cbContentLength   The amount of data that will be uploaded and
     *                              to be set as the value of the content-length
     *                              header field.  Pass UINT64_MAX if not known.
     *
     * @note    This will drop any buffer previously registered using setUploadData().
     */
    void setProducerCallback(PFNPRODUCER a_pfnProducer, void *a_pvCallbackData = NULL, uint64_t a_cbContentLength = UINT64_MAX) RT_NOEXCEPT;

    /**
     * Preprares transmission via the @a a_hHttp client instance.
     *
     * @returns IPRT status code.
     * @param   a_hHttp             The HTTP client instance.
     * @internal
     */
    virtual int xmitPrepare(RTHTTP a_hHttp) const RT_NOEXCEPT;

    /**
     * For completing and/or undoing setup from xmitPrepare.
     *
     * @param   a_hHttp             The HTTP client instance.
     * @internal
     */
    virtual void xmitComplete(RTHTTP a_hHttp) const RT_NOEXCEPT;

protected:
    /** Number of bytes corresponding to content-length.
     * UINT64_MAX if not known.  Used both for unploads and downloads. */
    uint64_t    m_cbContentLength;
    /** The content type if set (upload only). */
    RTCString   m_strContentType;
    /** Pointer to user-registered producer callback function (upload only). */
    PFNPRODUCER m_pfnProducer;
    /** User argument for both callbacks (both). */
    void       *m_pvCallbackData;

    /** @copydoc FNRTHTTPUPLOADCALLBACK */
    static DECLCALLBACK(int) xmitHttpCallback(RTHTTP hHttp, void *pvBuf, size_t cbBuf, uint64_t offContent,
                                              size_t *pcbActual, void *pvUser) RT_NOEXCEPT;

private:
    /* No copy constructor or copy assignment: */
    RTCRestBinaryParameter(RTCRestBinaryParameter const &a_rThat);
    RTCRestBinaryParameter &operator=(RTCRestBinaryParameter const &a_rThat);
};


/**
 * Specialization of RTCRestBinary for use with responses in a client.
 *
 * This enables registering data callbacks for consuming downloaded data.
 */
class RT_DECL_CLASS RTCRestBinaryResponse : public RTCRestBinary
{
public:
    /** Default constructor. */
    RTCRestBinaryResponse() RT_NOEXCEPT;

    /** Safe copy assignment method. */
    virtual int assignCopy(RTCRestBinaryResponse const &a_rThat) RT_NOEXCEPT;
    /** Safe copy assignment method. */
    virtual int assignCopy(RTCRestBinary const &a_rThat) RT_NOEXCEPT RT_OVERRIDE;
    /** Safe copy assignment method.
     * @note This will assert and fail as it makes no sense for a download.  */
    virtual int assignCopy(void const *a_pvData, size_t a_cbData) RT_NOEXCEPT RT_OVERRIDE;

    /**
     * Use the specified data buffer directly.
     * @note This will assert and fail as it makes no sense for a download.
     */
    virtual int assignReadOnly(void const *a_pvData, size_t a_cbData) RT_NOEXCEPT RT_OVERRIDE;
    /**
     * Use the specified data buffer directly.
     * @note This will drop any previously registered producer callback and user data.
     */
    virtual int assignWriteable(void *a_pvBuf, size_t a_cbBuf) RT_NOEXCEPT RT_OVERRIDE;

    /** Make a clone of this object. */
    inline RTCRestBinaryResponse *clone() const RT_NOEXCEPT { return (RTCRestBinaryResponse *)baseClone(); }

    /* Overridden methods: */
    virtual RTCRestObjectBase *baseClone() const RT_NOEXCEPT RT_OVERRIDE;
    virtual int resetToDefault() RT_NOEXCEPT RT_OVERRIDE;
    virtual const char *typeName(void) const RT_NOEXCEPT RT_OVERRIDE;

    /** Factory method. */
    static DECLCALLBACK(RTCRestObjectBase *) createInstance(void) RT_NOEXCEPT;

    /**
     * Retrieves the callback data.
     */
    inline void *getCallbackData() const RT_NOEXCEPT { return m_pvCallbackData; }

    /**
     * Sets the max size to download to memory.
     *
     * This also indicates the intention to download to a memory buffer, so it
     * will drop any previously registered consumer callback and its user data.
     *
     * @param   a_cbMaxDownload Maximum number of bytes to download to memory.
     *                          If 0, a default is selected (currently 32MiB for
     *                          32-bit hosts and 128MiB for 64-bit).
     */
    void setMaxDownloadSize(size_t a_cbMaxDownload) RT_NOEXCEPT;

    /**
     * Gets the content-length value (UINT64_MAX if not available).
     */
    inline uint64_t getContentLength() const RT_NOEXCEPT { return m_cbContentLength; }

    /**
     * Callback for consuming downloaded bytes.
     *
     * @returns IPRT status code.
     * @param   a_pThis         The related string object.
     * @param   a_pvSrc         Buffer containing the bytes.
     * @param   a_cbSrc         The number of bytes in the buffer.
     * @param   a_uHttpStatus   The HTTP status code.
     * @param   a_offContent    The byte offset corresponding to the start of @a a_pvSrc.
     * @param   a_cbContent     The content length field value, UINT64_MAX if not available.
     *
     * @remarks Use getCallbackData to get the user data.
     *
     * @note    The @a a_offContent parameter does not imply random access or anthing
     *          like that, it is just a convenience provided by the caller.  The value
     *          is the sum of the previous @a a_cbSrc values.
     */
    typedef DECLCALLBACKTYPE(int, FNCONSUMER,(RTCRestBinaryResponse *a_pThis, const void *a_pvSrc, size_t a_cbSrc,
                                              uint32_t a_uHttpStatus, uint64_t a_offContent, uint64_t a_cbContent)) /*RT_NOEXCEPT*/;
    /** Pointer to a byte consumer callback. */
    typedef FNCONSUMER *PFNCONSUMER;

    /**
     * Sets the consumer callback.
     *
     * @param   a_pfnConsumer       The callback function for consuming downloaded data.
     *                              NULL if data should be stored in m_pbData (the default).
     * @param   a_pvCallbackData    Data the can be retrieved from the callback
     *                              using getCallbackData().
     */
    void setConsumerCallback(PFNCONSUMER a_pfnConsumer, void *a_pvCallbackData = NULL) RT_NOEXCEPT;

    /**
     * Preprares for receiving via the @a a_hHttp client instance.
     *
     * @returns IPRT status code.
     * @param   a_hHttp             The HTTP client instance.
     * @param   a_fCallbackFlags    The HTTP callback flags (status code spec).
     * @internal
     */
    virtual int receivePrepare(RTHTTP a_hHttp, uint32_t a_fCallbackFlags) RT_NOEXCEPT;

    /**
     * For completing and/or undoing setup from receivePrepare.
     *
     * @param   a_hHttp             The HTTP client instance.
     * @internal
     */
    virtual void receiveComplete(RTHTTP a_hHttp) RT_NOEXCEPT;

protected:
    /** Number of bytes corresponding to content-length.
     * UINT64_MAX if not known.  Used both for unploads and downloads. */
    uint64_t    m_cbContentLength;
    /** Number of bytes downloaded thus far. */
    uint64_t    m_cbDownloaded;
    /** Pointer to user-registered consumer callback function (download only). */
    PFNCONSUMER m_pfnConsumer;
    /** User argument for both callbacks (both). */
    void       *m_pvCallbackData;
    /** Maximum data to download to memory (download only). */
    size_t      m_cbMaxDownload;

    /** @copydoc FNRTHTTPDOWNLOADCALLBACK */
    static DECLCALLBACK(int) receiveHttpCallback(RTHTTP hHttp, void const *pvBuf, size_t cbBuf, uint32_t uHttpStatus,
                                                 uint64_t offContent, uint64_t cbContent, void *pvUser) RT_NOEXCEPT;

private:
    /* No copy constructor or copy assignment: */
    RTCRestBinaryResponse(RTCRestBinaryResponse const &a_rThat);
    RTCRestBinaryResponse &operator=(RTCRestBinaryResponse const &a_rThat);
};


/**
 * Base class for REST client requests.
 *
 * This encapsulates parameters and helps transform them into a HTTP request.
 *
 * Parameters can be transfered in a number of places:
 *      - Path part of the URL.
 *      - Query part of the URL.
 *      - HTTP header fields.
 *      - FORM body.
 *      - JSON body.
 *      - XML body.
 *      - ...
 *
 * They can be require or optional.  The latter may have default values.  In
 * swagger 3 they can also be nullable, which means the null-indicator cannot
 * be used for tracking optional parameters.
 */
class RT_DECL_CLASS RTCRestClientRequestBase
{
public:
    RTCRestClientRequestBase() RT_NOEXCEPT;
    virtual ~RTCRestClientRequestBase();
    RTCRestClientRequestBase(RTCRestClientRequestBase const &a_rThat) RT_NOEXCEPT;
    RTCRestClientRequestBase &operator=(RTCRestClientRequestBase const &a_rThat) RT_NOEXCEPT;

    /**
     * Reset all members to default values.
     * @returns IPRT status code.
     */
    virtual int resetToDefault() RT_NOEXCEPT = 0;

    /**
     * Getter for the operation name.  Provided by the generated
     * subclasses so that base class code may use it for more
     * informative logs.
     */
    virtual const char *getOperationName() const RT_NOEXCEPT = 0;

    /**
     * Prepares the HTTP handle for transmitting this request.
     *
     * @returns IPRT status code.
     * @param   a_pStrPath  Where to set path parameters.  Will be appended to the base path.
     * @param   a_pStrQuery Where to set query parameters.
     * @param   a_hHttp     Where to set header parameters and such.
     * @param   a_pStrBody  Where to set body parameters.
     */
    virtual int xmitPrepare(RTCString *a_pStrPath, RTCString *a_pStrQuery, RTHTTP a_hHttp, RTCString *a_pStrBody) const RT_NOEXCEPT = 0;

    /**
     * Always called after the request has been transmitted.
     *
     * @param   a_rcStatus  Negative numbers are IPRT errors, positive are HTTP status codes.
     * @param   a_hHttp     The HTTP handle the request was performed on.
     */
    virtual void xmitComplete(int a_rcStatus, RTHTTP a_hHttp) const RT_NOEXCEPT = 0;

    /**
     * Checks if there are were any assignment errors.
     */
    inline bool hasAssignmentErrors() const RT_NOEXCEPT { return m_fErrorSet != 0; }

protected:
    /** Set of fields that have been explicitly assigned a value. */
    uint64_t m_fIsSet;
    /** Set of fields where value assigning failed. */
    uint64_t m_fErrorSet;

    /** Path parameter descriptor. */
    typedef struct
    {
        const char                 *pszName;    /**< The name string to replace (including {}). */
        size_t                      cchName;    /**< Length of pszName. */
        uint32_t                    fFlags;     /**< The toString flags. */
        uint8_t                     iBitNo;     /**< The parameter bit number. */
    } PATHPARAMDESC;

    /** Path parameter state. */
    typedef struct
    {
        RTCRestObjectBase const    *pObj;       /**< Pointer to the parameter object. */
        size_t                      offName;    /**< Maintained by worker. */
    } PATHPARAMSTATE;

    /**
     * Do path parameters.
     *
     * @returns IPRT status code
     * @param   a_pStrPath          The destination path.
     * @param   a_pszPathTemplate   The path template string.
     * @param   a_cchPathTemplate   The length of the path template string.
     * @param   a_paPathParams      The path parameter descriptors (static).
     * @param   a_paPathParamStates The path parameter objects and states.
     * @param   a_cPathParams       Number of path parameters.
     */
    int doPathParameters(RTCString *a_pStrPath, const char *a_pszPathTemplate, size_t a_cchPathTemplate,
                         PATHPARAMDESC const *a_paPathParams, PATHPARAMSTATE *a_paPathParamStates, size_t a_cPathParams) const RT_NOEXCEPT;

    /** Query parameter descriptor. */
    typedef struct
    {
        const char *pszName;            /**< The parameter name. */
        uint32_t    fFlags;             /**< The toString flags. */
        bool        fRequired;          /**< Required or not. */
        uint8_t     iBitNo;             /**< The parameter bit number. */
    } QUERYPARAMDESC;

    /**
     * Do query parameters.
     *
     * @returns IPRT status code
     * @param   a_pStrQuery         The destination string.
     * @param   a_paQueryParams     The query parameter descriptors.
     * @param   a_papQueryParamObjs The query parameter objects, parallel to @a a_paQueryParams.
     * @param   a_cQueryParams      Number of query parameters.
     */
    int doQueryParameters(RTCString *a_pStrQuery, QUERYPARAMDESC const *a_paQueryParams,
                          RTCRestObjectBase const **a_papQueryParamObjs, size_t a_cQueryParams) const RT_NOEXCEPT;

    /** Header parameter descriptor. */
    typedef struct
    {
        const char *pszName;            /**< The parameter name. */
        uint32_t    fFlags;             /**< The toString flags. */
        bool        fRequired;          /**< Required or not. */
        uint8_t     iBitNo;             /**< The parameter bit number. */
        bool        fMapCollection;     /**< Collect headers starting with pszName into a map. */
    } HEADERPARAMDESC;

    /**
     * Do header parameters.
     *
     * @returns IPRT status code
     * @param   a_hHttp                 Where to set header parameters.
     * @param   a_paHeaderParams        The header parameter descriptors.
     * @param   a_papHeaderParamObjs    The header parameter objects, parallel to @a a_paHeaderParams.
     * @param   a_cHeaderParams         Number of header parameters.
     */
    int doHeaderParameters(RTHTTP a_hHttp, HEADERPARAMDESC const *a_paHeaderParams,
                           RTCRestObjectBase const **a_papHeaderParamObjs, size_t a_cHeaderParams) const RT_NOEXCEPT;
};


/**
 * Base class for REST client responses.
 */
class RT_DECL_CLASS RTCRestClientResponseBase
{
public:
    /** Default constructor. */
    RTCRestClientResponseBase() RT_NOEXCEPT;
    /** Destructor. */
    virtual ~RTCRestClientResponseBase();
    /** Copy constructor. */
    RTCRestClientResponseBase(RTCRestClientResponseBase const &a_rThat);
    /** Copy assignment operator. */
    RTCRestClientResponseBase &operator=(RTCRestClientResponseBase const &a_rThat);

    /**
     * Resets the object state.
     */
    virtual void reset(void) RT_NOEXCEPT;

    /**
     * Getter for the operation name.  Provided by the generated
     * subclasses so that base class code may use it for more
     * informative logs.
     */
    virtual const char *getOperationName() const RT_NOEXCEPT = 0;

    /**
     * Prepares the HTTP handle for receiving the response.
     *
     * This may install callbacks and such like.
     *
     * When overridden, the parent class must always be called.
     *
     * @returns IPRT status code.
     * @param   a_hHttp     The HTTP handle to prepare for receiving.
     */
    virtual int receivePrepare(RTHTTP a_hHttp) RT_NOEXCEPT;

    /**
     * Called when the HTTP request has been completely received.
     *
     * @param   a_rcStatus  Negative numbers are IPRT errors, positive are HTTP status codes.
     * @param   a_hHttp     The HTTP handle the request was performed on.
     *                      This can be NIL_RTHTTP should something fail early, in
     *                      which case it is possible receivePrepare() wasn't called.
     *
     * @note    Called before consumeBody() but after consumeHeader().
     */
    virtual void receiveComplete(int a_rcStatus, RTHTTP a_hHttp) RT_NOEXCEPT;

    /**
     * Callback that consumes HTTP body data from the server.
     *
     * @param   a_pchData  Body data.
     * @param   a_cbData   Amount of body data.
     *
     * @note    Called after consumeHeader().
     */
    virtual void consumeBody(const char *a_pchData, size_t a_cbData) RT_NOEXCEPT;

    /**
     * Called after status, headers and body all have been presented.
     */
    virtual void receiveFinal() RT_NOEXCEPT;

    /**
     * Getter for m_rcStatus.
     * @returns Negative numbers are IPRT errors, positive are HTTP status codes.
     */
    inline int getStatus() const RT_NOEXCEPT { return m_rcStatus; }

    /**
     * Getter for m_rcHttp.
     * @returns HTTP status code or VERR_NOT_AVAILABLE.
     */
    inline int getHttpStatus() const RT_NOEXCEPT { return m_rcHttp; }

    /**
     * Getter for m_pErrInfo.
     */
    inline PCRTERRINFO getErrInfo(void) const RT_NOEXCEPT { return m_pErrInfo; }

    /**
     * Getter for m_strContentType.
     */
    inline RTCString const &getContentType(void) const RT_NOEXCEPT { return m_strContentType; }


protected:
    /** Negative numbers are IPRT errors, positive are HTTP status codes. */
    int         m_rcStatus;
    /** The HTTP status code, VERR_NOT_AVAILABLE if not set. */
    int         m_rcHttp;
    /** Error information. */
    PRTERRINFO  m_pErrInfo;
    /** The value of the Content-Type header field. */
    RTCString   m_strContentType;

    PRTERRINFO  getErrInfoInternal(void) RT_NOEXCEPT;
    void        deleteErrInfo(void) RT_NOEXCEPT;
    void        copyErrInfo(PCRTERRINFO pErrInfo) RT_NOEXCEPT;

    /**
     * Reports an error (or warning if a_rc non-negative).
     *
     * @returns a_rc
     * @param   a_rc        The status code to report and return.  The first
     *                      error status is assigned to m_rcStatus, subsequent
     *                      ones as well as informational statuses are not
     *                      recorded by m_rcStatus.
     * @param   a_pszFormat The message format string.
     * @param   ...         Message arguments.
     */
    int addError(int a_rc, const char *a_pszFormat, ...) RT_NOEXCEPT;

    /**
     * Deserializes a header field value.
     *
     * @returns IPRT status code.
     * @param   a_pObj              The object to deserialize into.
     * @param   a_pchValue          Pointer to the value (not zero terminated).
     *                              Not necessarily valid UTF-8!
     * @param   a_cchValue          The value length.
     * @param   a_fFlags            Flags to pass to fromString().
     * @param   a_pszErrorTag       The error tag (field name).
     */
    int deserializeHeader(RTCRestObjectBase *a_pObj, const char *a_pchValue, size_t a_cchValue,
                          uint32_t a_fFlags, const char *a_pszErrorTag) RT_NOEXCEPT;

    /**
     * Deserializes a header field value.
     *
     * @returns IPRT status code.
     * @param   a_pMap              The string map object to deserialize into.
     * @param   a_pchField          Pointer to the map field name.  (Caller dropped the prefix.)
     *                              Not necessarily valid UTF-8!
     * @param   a_cchField          Length of field name.
     * @param   a_pchValue          Pointer to the value (not zero terminated).
     *                              Not necessarily valid UTF-8!
     * @param   a_cchValue          The value length.
     * @param   a_fFlags            Flags to pass to fromString().
     * @param   a_pszErrorTag       The error tag (field name).
     */
    int deserializeHeaderIntoMap(RTCRestStringMapBase *a_pMap, const char *a_pchField, size_t a_cchField,
                                 const char *a_pchValue, size_t a_cchValue, uint32_t a_fFlags, const char *a_pszErrorTag) RT_NOEXCEPT;

    /**
     * Helper that does the deserializing of the response body
     * via deserializeBodyFromJsonCursor().
     *
     * @param   a_pchData       The body blob.
     * @param   a_cbData        The size of the body blob.
     * @param   a_pszBodyName   The name of the body parameter.
     */
    void deserializeBody(const char *a_pchData, size_t a_cbData, const char *a_pszBodyName) RT_NOEXCEPT;

    /**
     * Called by deserializeBody to do the actual body deserialization.
     *
     * @param   a_rCursor       The JSON cursor.
     */
    virtual void deserializeBodyFromJsonCursor(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT;

    /**
     * Primary json cursor for parsing bodies.
     */
    class PrimaryJsonCursorForBody : public RTCRestJsonPrimaryCursor
    {
    public:
        RTCRestClientResponseBase *m_pThat; /**< Pointer to response object. */
        PrimaryJsonCursorForBody(RTJSONVAL hValue, const char *pszName, RTCRestClientResponseBase *a_pThat) RT_NOEXCEPT;
        virtual int addError(RTCRestJsonCursor const &a_rCursor, int a_rc, const char *a_pszFormat, ...) RT_NOEXCEPT RT_OVERRIDE;
        virtual int unknownField(RTCRestJsonCursor const &a_rCursor) RT_NOEXCEPT RT_OVERRIDE;
    };


    /**
     * Consumes a header.
     *
     * Child classes can override this to pick up their header fields, but must
     * always call the parent class.
     *
     * @returns IPRT status code.
     * @param   a_uMatchWord    Match word constructed by RTHTTP_MAKE_HDR_MATCH_WORD
     * @param   a_pchField      The field name (not zero terminated).
     *                          Not necessarily valid UTF-8!
     * @param   a_cchField      The length of the field.
     * @param   a_pchValue      The field value (not zero terminated).
     * @param   a_cchValue      The length of the value.
     */
    virtual int consumeHeader(uint32_t a_uMatchWord, const char *a_pchField, size_t a_cchField,
                              const char *a_pchValue, size_t a_cchValue) RT_NOEXCEPT;

private:
    /** Callback for use with RTHttpSetHeaderCallback. */
    static DECLCALLBACK(int) receiveHttpHeaderCallback(RTHTTP hHttp, uint32_t uMatchWord, const char *pchField, size_t cchField,
                                                       const char *pchValue, size_t cchValue, void *pvUser) RT_NOEXCEPT;
};


/**
 * Base class for REST client responses.
 */
class RT_DECL_CLASS RTCRestClientApiBase
{
public:
    RTCRestClientApiBase() RT_NOEXCEPT;
    virtual ~RTCRestClientApiBase();

    /** @name Host and Base path (URL) handling.
     * @{ */
    /**
     * Gets the server URL.
     */
    const char *getServerUrl(void) const RT_NOEXCEPT;

    /**
     * Sets the whole server URL.
     * @returns IPRT status code.
     * @param   a_pszUrl        The new server URL.  NULL/empty to reset to default.
     */
    int setServerUrl(const char *a_pszUrl) RT_NOEXCEPT;

    /**
     * Sets the scheme part of the the server URL.
     * @returns IPRT status code.
     * @param   a_pszScheme     The new scheme.  Does not accept NULL or empty string.
     */
    int setServerScheme(const char *a_pszScheme) RT_NOEXCEPT;

    /**
     * Sets the authority (hostname + port) part of the the server URL.
     * @returns IPRT status code.
     * @param   a_pszAuthority  The new authority.  Does not accept NULL or empty string.
     */
    int setServerAuthority(const char *a_pszAuthority) RT_NOEXCEPT;

    /**
     * Sets the base path part of the the server URL.
     * @returns IPRT status code.
     * @param   a_pszBasePath   The new base path.  Does not accept NULL or empty string.
     */
    int setServerBasePath(const char *a_pszBasePath) RT_NOEXCEPT;

    /**
     * Gets the default server URL as specified in the specs.
     * @returns Server URL.
     */
    virtual const char *getDefaultServerUrl() const RT_NOEXCEPT = 0;

    /**
     * Gets the default server base path as specified in the specs.
     * @returns Host string (start of URL).
     */
    virtual const char *getDefaultServerBasePath() const RT_NOEXCEPT = 0;
    /** @} */

    /**
     * Sets the CA file to use for HTTPS.
     */
    int setCAFile(const char *pcszCAFile) RT_NOEXCEPT;
    /** @overload */
    int setCAFile(const RTCString &strCAFile) RT_NOEXCEPT;

    /** Flags to doCall. */
    enum
    {
        kDoCall_OciReqSignExcludeBody = 1, /**< Exclude the body when doing OCI request signing. */
        kDoCall_RequireBody           = 2  /**< The body is required. */
    };

protected:
    /** Handle to the HTTP connection object. */
    RTHTTP  m_hHttp;
    /** The server URL to use.  If empty use the default. */
    RTCString m_strServerUrl;
    /** The CA file to use.  If empty use the default. */
    RTCString m_strCAFile;

    /* Make non-copyable (RTCNonCopyable causes warnings): */
    RTCRestClientApiBase(RTCRestClientApiBase const &);
    RTCRestClientApiBase *operator=(RTCRestClientApiBase const &);

    /**
     * Re-initializes the HTTP instance.
     *
     * @returns IPRT status code.
     */
    virtual int reinitHttpInstance() RT_NOEXCEPT;

    /**
     * Hook that's called when doCall has fully assembled the request.
     *
     * Can be used for request signing and similar final steps.
     *
     * @returns IPRT status code.
     * @param   a_hHttp         The HTTP client instance.
     * @param   a_rStrFullUrl   The full URL.
     * @param   a_enmHttpMethod The HTTP request method.
     * @param   a_rStrXmitBody  The body text.
     * @param   a_fFlags        kDoCall_XXX.
     */
    virtual int xmitReady(RTHTTP a_hHttp, RTCString const &a_rStrFullUrl, RTHTTPMETHOD a_enmHttpMethod,
                          RTCString const &a_rStrXmitBody, uint32_t a_fFlags) RT_NOEXCEPT;

    /**
     * Implements stuff for making an API call.
     *
     * @returns a_pResponse->getStatus()
     * @param   a_rRequest      Reference to the request object.
     * @param   a_enmHttpMethod The HTTP request method.
     * @param   a_pResponse     Pointer to the response object.
     * @param   a_pszMethod     The method name, for logging purposes.
     * @param   a_fFlags        kDoCall_XXX.
     */
    virtual int doCall(RTCRestClientRequestBase const &a_rRequest, RTHTTPMETHOD a_enmHttpMethod,
                       RTCRestClientResponseBase *a_pResponse, const char *a_pszMethod, uint32_t a_fFlags) RT_NOEXCEPT;

    /**
     * Implements OCI style request signing.
     *
     * @returns IPRT status code.
     * @param   a_hHttp         The HTTP client instance.
     * @param   a_rStrFullUrl   The full URL.
     * @param   a_enmHttpMethod The HTTP request method.
     * @param   a_rStrXmitBody  The body text.
     * @param   a_fFlags        kDoCall_XXX.
     * @param   a_hKey          The key to use for signing.
     * @param   a_rStrKeyId     The key ID.
     *
     * @remarks The signing scheme is covered by a series of drafts RFC, the latest being:
     *                  https://tools.ietf.org/html/draft-cavage-http-signatures-10
     */
    int ociSignRequest(RTHTTP a_hHttp, RTCString const &a_rStrFullUrl, RTHTTPMETHOD a_enmHttpMethod,
                       RTCString const &a_rStrXmitBody, uint32_t a_fFlags, RTCRKEY a_hKey, RTCString const &a_rStrKeyId) RT_NOEXCEPT;

    /**
     * Worker for the server URL modifiers.
     *
     * @returns IPRT status code.
     * @param   a_pszServerUrl  The current server URL (for comparing).
     * @param   a_offDst        The offset of the component in the current server URL.
     * @param   a_cchDst        The current component length.
     * @param   a_pszSrc        The new URL component value.
     * @param   a_cchSrc        The length of the new component.
     */
    int setServerUrlPart(const char *a_pszServerUrl, size_t a_offDst, size_t a_cchDst, const char *a_pszSrc, size_t a_cchSrc) RT_NOEXCEPT;
};

/** @} */

#endif /* !IPRT_INCLUDED_cpp_restclient_h */

