/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/anakod/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 *
 * SmtpClient
 *
 * Author: 2018 - Slavey Karadzhov <slav@attachix.com>
 *
 ****/

/** @defgroup   smtpclient SMTP client
 *  @brief      Provides SMTP/S client
 *  @ingroup    tcpclient
 *  @{
 */

#pragma once

#include "TcpClient.h"
#include "URL.h"
#include "../../Wiring/WString.h"
#include "../../Wiring/WVector.h"
#include "../../Wiring/FIFO.h"
#include "../DataSourceStream.h"
#include "WebConstants.h"

// TODO: Move the simpleConcurrentQueue to a better place
#include "Http/HttpCommon.h"
#include "Http/Stream/HttpMultipartStream.h"

#undef min
#undef max
#include <functional>

#define SMTP_PROTOCOL "smtp"
#define SMTP_OVER_SSL_PROTOCOL "smtps"

/* Max number of attachments per mail */
#define SMTP_MAX_ATTACHMENTS 5
/* Maximum waiting emails in the mail queue */
#define SMTP_QUEUE_SIZE 5

/* Buffer size used to read the error messages */
#define SMTP_ERROR_LENGTH 40

/**
 * SMTP response codes
 */
#define SMTP_CODE_SERVICE_READY 220
#define SMTP_CODE_BYE 			221
#define SMTP_CODE_AUTH_OK 		235
#define SMTP_CODE_REQUEST_OK    250
#define SMTP_CODE_AUTH_CHALLENGE 334
#define SMTP_CODE_START_DATA    354

#define SMTP_OPT_PIPELINE bit(0)
#define SMTP_OPT_STARTTLS bit(1)
#define SMTP_OPT_AUTH_PLAIN bit(2)
#define SMTP_OPT_AUTH_LOGIN bit(3)
#define SMTP_OPT_AUTH_CRAM_MD5 bit(4)

enum SmtpState
{
	eSMTP_Banner = 0,
	eSMTP_Hello,
	eSMTP_StartTLS,
	eSMTP_SendAuth,
	eSMTP_SendingAuthLogin,
	eSMTP_RequestingAuthChallenge,
	eSMTP_SendAuthResponse,
	eSMTP_SendingAuth,
	eSMTP_Ready,
	eSMTP_Start,
	eSMTP_SendMail,
	eSMTP_SendingMail,
	eSMTP_SendRcpt,
	eSMTP_SendingRcpt,
	eSMTP_SendData,
	eSMTP_SendingData,
	eSMTP_SendHeader,
	eSMTP_SendingHeaders,
	eSMTP_StartBody,
	eSMTP_SendingBody,
	eSMTP_Sent,
	eSMTP_Quitting,
	eSMTP_Disconnect
};

class SmtpClient;
class MailMessage;

typedef HashMap<String, String> HttpHeaders;

typedef std::function<void(SmtpClient& client, int code, char* status)> SmtpClientErrorCallback;
typedef std::function<void(MailMessage& message, int code, char* status)> MailMessageSentCallback;


typedef struct {
	Stream* stream;
	MimeType mime = MIME_TEXT;
} MailAttachment;

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
	 * @brief callback that will be called once the message is sent
	 */
	void onSent(MailMessageSentCallback callback);

	/**
	 * @brief Get the generated data stream
	 */
	ReadWriteStream* getData();
private:
	ReadWriteStream* stream = nullptr;
	HttpHeaders headers;
	MailMessageSentCallback callback = nullptr;
	Vector<HttpPartResult> attachments;

private:
	/**
	 * @brief Takes care to fetch the correct streams for a message
	 * @note The magic where all streams and attachments are packed together is happening here
	 */
	HttpPartResult multipartProducer();
};

class SmtpClient : protected TcpClient
{
public:
	SmtpClient(bool autoDestroy=false);
	virtual ~SmtpClient();

	/**
	 * @brief Connects to remote URL
	 * @param URL - provides the protocol, remote server, port and user credentials
	 * 				allowed protocols:
	 * 					- smtp  - clear text SMTP
	 * 					- smtps - SMTP over SSL connection
	 */
	bool connect(const URL& url);

	void onConnectError(SmtpClientErrorCallback callback);

	/**
	 * @brief Send a single message to the SMTP server
	 */
	bool send(const String&	from, const String&	to, const String& subject, const String& body);

	bool send(MailMessage* message);

	void quit();

	using TcpClient::setTimeOut;

#ifdef ENABLE_SSL
	using TcpClient::addSslOptions;
	using TcpClient::setSslFingerprint;
	using TcpClient::pinCertificate;
	using TcpClient::setSslClientKeyCert;
	using TcpClient::freeSslClientKeyCert;
	using TcpClient::getSsl;
#endif

protected:
	virtual err_t onReceive(pbuf *buf);
	virtual void onReadyToSendData(TcpConnectionEvent sourceEvent);

	bool sendMailStart(MailMessage* mail);
	void sendMailHeaders(MailMessage* mail);
	bool sendMailBody(MailMessage* mail);

private:
	URL url;
	Vector<String> authMethods;
	SimpleConcurrentQueue<MailMessage*,SMTP_QUEUE_SIZE> mailQ;
	char code[4] = {0};
	int codeValue = 0;
	String authChallenge;
	char message[SMTP_ERROR_LENGTH] = {0};
	bool isLastLine = false;
	uint8_t codeLength = 0;
	int options = 0;
	MailMessage *outgoingMail = nullptr;
	SmtpState state = eSMTP_Banner;

	SmtpClientErrorCallback connectErrorCallback = nullptr;


private:
	/**
	 * @brief Simple and naive SMTP parser with a state machine
	 */
	int smtpParse(char* data, size_t len);
};
