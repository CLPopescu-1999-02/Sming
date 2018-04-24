/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/SmingHub/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 *
 * @author Slavey Karadzhov <slaff@attachix.com>
 *
 ****/

/** @defgroup   smtpclient SMTP client
 *  @brief      Provides SMTP/S client
 *  @ingroup    tcpclient
 *  @{
 */

#ifndef _SMING_CORE_DATA_MESSAGE_H_
#define _SMING_CORE_DATA_MESSAGE_H_

#include "../../Wiring/WString.h"
#include "../../Wiring/WVector.h"
#include "../DataSourceStream.h"
#include "../Network/WebConstants.h"

// TODO: Move the simpleConcurrentQueue to a better place
#include "../Network/Http/HttpCommon.h"
#include "Data/Stream/MultipartStream.h"

class SmtpClient;

class MailMessage
{
	friend class SmtpClient;
public:

	String to;
	String from;
	String subject;
	String cc;

	void setHeader(const String& name, const String& value);

	HttpHeaders& getHeaders();

	/**
	 * @brief Sets the body of the email
	 * @param String body
	 * @param MimeType mime
	 */
	bool setBody(const String& body, MimeType mime = MIME_TEXT);

	/**
	 * @brief Sets the body of the email
	 * @param Stream& stream
	 * @param MimeType mime
	 */
	bool setBody(ReadWriteStream* stream, MimeType mime = MIME_TEXT);

	/**
	 * @brief Adds attachment to the email
	 */
	bool addAttachment(FileStream* stream);

	/**
	 * @brief Adds attachment to the email
	 */
	bool addAttachment(ReadWriteStream* stream, MimeType mime, const String& filename = "");

	/**
	 * @brief Adds attachment to the email
	 */
	bool addAttachment(ReadWriteStream* stream, const String& mime, const String& filename = "");

	/**
	 * @brief Get the generated data stream
	 */
	ReadWriteStream* getData();

private:
	ReadWriteStream* stream = nullptr;
	HttpHeaders headers;
	Vector<HttpPartResult> attachments;

private:
	/**
	 * @brief Takes care to fetch the correct streams for a message
	 * @note The magic where all streams and attachments are packed together is happening here
	 */
	HttpPartResult multipartProducer();
};

/** @} */
#endif /* _SMING_CORE_DATA_MESSAGE_H_ */
