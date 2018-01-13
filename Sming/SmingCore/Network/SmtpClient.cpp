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

#include "SmtpClient.h"
#include "../../Services/WebHelpers/base64.h"
#include "../Data/StreamTransformer.h"

#define ADVANCE           { buffer++; len--; }
#define ADVANCE_AND_BREAK { ADVANCE; break; }
#define ADVANCE_UNTIL_EOL do { \
		if(*(buffer - 1) == '\r' && *buffer == '\n') { ADVANCE_AND_BREAK; } \
		ADVANCE; \
} while(len > 0);

#define ADVANCE_UNTIL_EOL_OR_BREAK { ADVANCE_UNTIL_EOL; if(*(buffer-1) != '\n') { break;} }

#define RETURN_ON_ERROR(SUCCESS_CODE) if(codeValue != SUCCESS_CODE) { memcpy(message, line, std::min(lineLength, SMTP_ERROR_LENGTH)); message[SMTP_ERROR_LENGTH]='\0'; return 0; }

// [MailMessage]

void MailMessage::setHeader(const String& name, const String& value)
{
	headers[name] = value;
}

HttpHeaders& MailMessage::getHeaders()
{
	if(!headers.contains("From")) {
		headers["From"] = from;
	}
	if(!headers.contains("To")) {
		headers["To"] = to;
	}
	if(!headers.contains("Cc")) {
		headers["Cc"] = cc;
	}
	headers["Subject"] = subject;

	return headers;
}

bool MailMessage::setBody(const String& body, MimeType mime /* = MIME_TEXT */)
{
	MemoryDataStream *memory = new MemoryDataStream();
	int written = memory->write((uint8_t *)body.c_str(), body.length());
	if(written < body.length()) {
		debug_e("MailMessage::setBody: Unable to store the complete body");
	}

	return setBody(memory, mime);
}

bool MailMessage::setBody(ReadWriteStream* stream, MimeType mime /* = MIME_TEXT */)
{
	if(this->stream != nullptr) {
		debug_e("MailMessage::setBody: Discarding already set stream!");
		delete this->stream;
		this->stream = nullptr;
	}

	this->stream = stream;
	headers["Content-Type"] = ContentType::toString(mime);

	return this;
}

HttpPartResult MailMessage::multipartProducer()
{
	if(attachments.count()) {
		HttpPartResult result = attachments[0];
		attachments.remove(0);
		return result;
	}

	HttpPartResult result;

	return result;
}

bool MailMessage::addAttachment(FileStream* stream) {
	if(stream == NULL) {
		return false;
	}

	String filename = stream->fileName();
	String mime = ContentType::fromFullFileName(filename);

	return addAttachment(stream, mime, filename);
}

bool MailMessage::addAttachment(ReadWriteStream* stream, MimeType mime, const String& filename /* = "" */)
{
	return addAttachment(stream, ContentType::toString(mime), filename);
}

bool MailMessage::addAttachment(ReadWriteStream* stream, const String& mime, const String& filename /* = "" */)
{
	HttpPartResult attachment;
	attachment.stream = new StreamTransformer(stream, base64StreamTransformer, NETWORK_SEND_BUFFER_SIZE + 10);
	attachment.headers = new HttpHeaders();
	(*attachment.headers)["Content-Type"] = mime;
	(*attachment.headers)["Content-Transfer-Encoding"] = "base64";
	(*attachment.headers)["Content-Disposition"] = "attachment";
	if(filename.length()) {
		(*attachment.headers)["Content-Disposition"] += "; filename=\""+ filename +"\"";
	}

	if(attachments.count() == 0) {
		// We are just starting to add attachments
		HttpMultipartStream* mStream = new HttpMultipartStream(HttpPartProducerDelegate(&MailMessage::multipartProducer, this));
		HttpPartResult text;
		text.headers = new HttpHeaders();
		(*text.headers)["Content-Type"] = headers["Content-Type"];
		text.stream = this->stream;

		attachments.addElement(text);
		headers["Content-Type"] = String("multipart/mixed; boundary=") + mStream->getBoundary();
		this->stream = mStream;
	}

	attachments.addElement(attachment);

	return true;
}

void MailMessage::onSent(MailMessageSentCallback callback)
{
	this->callback = callback;
}

// [SmtpClient]

SmtpClient::SmtpClient(bool autoDestroy /* =false */): TcpClient(autoDestroy), outgoingMail(nullptr)
{
}

SmtpClient::~SmtpClient()
{
	// TODO: clear all pointers...
	delete stream;
	delete outgoingMail;
	stream = nullptr;
	outgoingMail = nullptr;
	do {
		MailMessage *mail = mailQ.dequeue();
		if(mail == nullptr) {
			break;
		}
		delete mail;
	}
	while(1);
}

bool SmtpClient::connect(const URL& url)
{
	if (getConnectionState() != eTCS_Ready) {
		close();
	}

	this->url = url;
	if(!this->url.Port) {
		this->url.Port = 25;
		if(this->url.Protocol == SMTP_OVER_SSL_PROTOCOL) {
			this->url.Protocol = 465;
		}
	}

	return TcpClient::connect(url.Host, url.Port, (url.Protocol == SMTP_OVER_SSL_PROTOCOL));
}

void SmtpClient::onConnectError(SmtpClientErrorCallback callback)
{
	connectErrorCallback = callback;
}

bool SmtpClient::send(const String&	from, const String&	to,
					  const String& subject, const String& body)
{
	MailMessage *mail = new MailMessage();

	mail->to = to;
	mail->from = from;
	mail->subject = subject;
	mail->setBody(body);

	return send(mail);
}

bool SmtpClient::send(MailMessage* mail)
{
	if(!mailQ.enqueue(mail)) {
		// the mail queue is full
		delete mail;
		return false;
	}

	return true;
}

void SmtpClient::onReadyToSendData(TcpConnectionEvent sourceEvent)
{
	do {
		if(state == eSMTP_Disconnect) {
			close();
			return;
		}

		if(state == eSMTP_StartTLS) {
			sendString("STARTTLS\n\n");
			state = eSMTP_Banner;
			break;
		}

		if(state == eSMTP_SendAuth) {

			if(authMethods.count()) {
				// TODO: Simplify the code in that block...
				Vector<String> preferredOrder;
				if(useSsl) {
					preferredOrder.addElement("PLAIN");
					preferredOrder.addElement("CRAM-MD5");
				}
				else {
					preferredOrder.addElement("CRAM-MD5");
					preferredOrder.addElement("PLAIN");
				}

				for(int i=0; i< preferredOrder.count(); i++) {
					if(authMethods.contains(preferredOrder[i])) {
						if(preferredOrder[i] == "PLAIN") {
							// base64('\0' + username + '\0' + password)
							int tokenLength = url.User.length() + url.Password.length() + 2;
							uint8_t token[tokenLength] = {0};
							memcpy((token+1), url.User.c_str(), url.User.length()); // copy user
							memcpy((token + 2 + url.User.length()), url.Password.c_str(), url.Password.length()); // copy password
							int hashLength = tokenLength * 4;
							char hash[hashLength] = {0};
							base64_encode(tokenLength, token, hashLength, hash);
							sendString("AUTH PLAIN "+String(hash)+"\r\n");

							state = eSMTP_SendingAuth;
							break;
						}
						else if( preferredOrder[i] == "CRAM-MD5"){
							// otherwise we can try the slow cram-md5 authentication...
							sendString("AUTH CRAM-MD5\r\n");
							state = eSMTP_RequestingAuthChallenge;
							break;
						}
					}
				}
			} /* authMethods.count */

			if(state == eSMTP_SendAuth) {
				state = eSMTP_Ready;
			}

			break;
		}

		if(state == eSMTP_SendAuthResponse) {
			// Calculate the CRAM-MD5 response
			//     base64.b64encode("user " +hmac.new(password, base64.b64decode(challenge), hashlib.md5).hexdigest())
			uint8_t digest[MD5_SIZE] = {0};
			hmac_md5((const uint8_t*)authChallenge.c_str(), authChallenge.length(),
					 (const uint8_t*)url.Password.c_str(), url.Password.length(),
					 digest);

			char hexdigest[MD5_SIZE*2+1] = {0};
			char *c = hexdigest;
			for (int i = 0; i < MD5_SIZE; i++) {
					sprintf(c, "%02x", digest[i]);
					c += 2;
			}
			*c = '\0';

			String token = url.User + " " + hexdigest;
			int hashLength = token.length() * 4;
			char hash[hashLength];
			base64_encode(token.length(), (const unsigned char *)token.c_str(), hashLength, hash);
			sendString(String(hash)+"\r\n");
			state = eSMTP_SendingAuth;
			break;
		}

		if(state < eSMTP_Ready) {
			break;
		}

		if(state == eSMTP_Ready) {
			delete outgoingMail;

			debugf("Queue size: %d", mailQ.count());

			outgoingMail = mailQ.dequeue();
			if(!outgoingMail) {
				break;
			}

			state = eSMTP_Start;
		}

		if(state >= eSMTP_Start && state < eSMTP_Sent) {
			if(state >= eSMTP_Start && state <= eSMTP_SendData) {
				if(!sendMailStart(outgoingMail)) {
					break;
				}
			}

			if(state == eSMTP_SendHeader) {
				if(stream != nullptr && !stream->isFinished()) {
					break;
				}

				sendMailHeaders(outgoingMail);

				state = eSMTP_SendingHeaders;
			}

			if(state == eSMTP_SendingHeaders) {
				if(stream != nullptr && !stream->isFinished()) {
					break;
				}

				state = eSMTP_StartBody;
			}

			if(state == eSMTP_StartBody) {
				sendMailBody(outgoingMail);
			}

			if(state == eSMTP_SendingBody) {
				if(stream != nullptr && !stream->isFinished()) {
					break;
				}

				// send the final dot
				state = eSMTP_Sent;
				delete stream;
				stream = nullptr;

				sendString("\r\n.\r\n");

				continue;
			}
		}
	}
	while(0);

	TcpClient::onReadyToSendData(sourceEvent);
}

bool SmtpClient::sendMailStart(MailMessage* mail)
{
	if(options & SMTP_OPT_PIPELINE) {
		if(state < eSMTP_SendingMail) {
			sendString("MAIL FROM:"+mail->from+"\r\n");
			sendString("RCPT TO:"+mail->to+"\r\n");
			sendString("DATA\r\n");

			state = eSMTP_SendingMail;
		}

		return true;
	}

	switch(state) {
		case eSMTP_Start: {
			sendString("MAIL FROM:"+mail->from+"\r\n");
			state = eSMTP_SendingMail;
			break;
		}

		case eSMTP_SendRcpt: {
			sendString("RCPT TO:"+mail->to+"\r\n");
			state = eSMTP_SendingRcpt;
			break;
		}

		case eSMTP_SendData: {
			sendString("DATA\r\n");
			state = eSMTP_SendingData;
			return true;
		}
	}

	return false;
}

void SmtpClient::sendMailHeaders(MailMessage* mail)
{
	mail->getHeaders();
	for(int i=0; i< mail->headers.count(); i++) {
		String key = mail->headers.keyAt(i);
		String value = mail->headers.valueAt(i);
		sendString(key+": "+ value + "\r\n");
	}
	sendString("\r\n");
}

bool SmtpClient::sendMailBody(MailMessage* mail)
{
	if(state == eSMTP_StartBody) {
		state = eSMTP_SendingBody;

		if(mail->stream == nullptr) {
			return true;
		}

		delete stream;
		stream = mail->stream; // avoid intermediate buffers
		mail->stream = nullptr;
		return false;
	}

	if(stream == nullptr) {
		// we are done for now
		return true;
	}

	if(mail->stream == nullptr && !stream->isFinished()) {
		return false;
	}

	return true;
}

void SmtpClient::quit()
{
	sendString("QUIT\r\n");
	state = eSMTP_Quitting;
}


err_t SmtpClient::onReceive(pbuf *buf)
{
	if (buf == nullptr) {
		return TcpClient::onReceive(buf);
	}

	pbuf *cur = buf;
	int parsedBytes = 0;
	while (cur != nullptr && cur->len > 0) {
		parsedBytes += smtpParse((char*) cur->payload, cur->len);
		cur = cur->next;
	}

	if (parsedBytes != buf->tot_len) {
		debug_e("Got error: %s:%s", code, message);
		if(state < eSMTP_Ready && connectErrorCallback) {
			connectErrorCallback(*this, codeValue, message);
		}

		if(outgoingMail!=nullptr && outgoingMail->callback) {
			outgoingMail->callback(*outgoingMail, codeValue, message);
		}

		TcpClient::onReceive(nullptr);

		return ERR_ABRT;
	}

	TcpClient::onReceive(buf);

	return ERR_OK;
}

int SmtpClient::smtpParse(char* buffer, size_t len)
{
	char *start = buffer;
	while(len) {
	    char currentByte = *buffer;
	    // parse the code...
	    if(codeLength < 3) {
	    	code[codeLength++] = currentByte;
	    	ADVANCE;
	    	continue;
	    }
	    else if (codeLength == 3) {
	    	code[codeLength] = '\0';
	    	if(currentByte != ' ' && currentByte != '-') {
	    		// the code must be followed by space or minus
	    		return 0;
	    	}

	    	char *tmp;
	    	codeValue = strtol(code, &tmp, 10);
	    	isLastLine = (currentByte == ' ');
	    	codeLength++;
	    	ADVANCE;
	    }

	    char *line = buffer;
	    ADVANCE_UNTIL_EOL_OR_BREAK;
	    codeLength = 0;
	    int lineLength = (buffer-line) - 2;

	    switch(state) {
	    case eSMTP_Banner: {
	    	RETURN_ON_ERROR(SMTP_CODE_SERVICE_READY);

	    	if(!useSsl && (options & SMTP_OPT_STARTTLS)) {
	    		useSsl = true;
	    		TcpConnection::staticOnConnected((void *)this, tcp, ERR_OK);
	    	}

	    	sendString("EHLO "+url.Host+"\r\n");
			state = eSMTP_Hello;

			break;
		}

	    case eSMTP_Hello: {
	    	RETURN_ON_ERROR(SMTP_CODE_REQUEST_OK);

			if(strncmp(line, "PIPELINING", lineLength) == 0) {
				// PIPELINING (see: https://tools.ietf.org/html/rfc2920)
				 options |= SMTP_OPT_PIPELINE;
			}
			else if(strncmp(line, "STARTTLS",lineLength) == 0) {
				// STARTTLS (see: https://www.ietf.org/rfc/rfc3207.txt)
				options |= SMTP_OPT_STARTTLS;
			}
			else if(strncmp(line, "AUTH ", 5) == 0) {
				// Process authentication methods
				// Ex: 250-AUTH CRAM-MD5 PLAIN LOGIN
				// See: https://tools.ietf.org/html/rfc4954
				int offset = 0;
				int pos = -1;

				String text(line + 5, lineLength - 5);
				splitString(text,' ', authMethods);
			}

			if(isLastLine) {
				state = eSMTP_Ready;
#if 0
				if(!useSsl && (options & SMTP_OPT_STARTTLS)) {
					state = eSMTP_StartTLS;
				} else
#endif
				if(url.User && authMethods.count()) {
					state = eSMTP_SendAuth;
				}
			}

			break;
		}

	    case eSMTP_RequestingAuthChallenge: {
	    	RETURN_ON_ERROR(SMTP_CODE_AUTH_CHALLENGE);
	    	uint8_t out[lineLength];
	    	int outlen = lineLength;

	    	base64_decode(line, lineLength, out, &outlen);
	    	authChallenge = String((const char*)out, outlen);
	    	state = eSMTP_SendAuthResponse;

	    	break;
	    }

	    case eSMTP_SendingAuth: {
	    	RETURN_ON_ERROR(SMTP_CODE_AUTH_OK);

	    	authMethods.clear();

	    	state = eSMTP_Ready;

	    	break;
	    }

	    case eSMTP_SendingMail: {
	    	RETURN_ON_ERROR(SMTP_CODE_REQUEST_OK);

	    	state = (options & SMTP_OPT_PIPELINE? eSMTP_SendingRcpt: eSMTP_SendRcpt);

	    	break;
		}

	    case eSMTP_SendingRcpt:{
	    	RETURN_ON_ERROR(SMTP_CODE_REQUEST_OK);

			state = (options & SMTP_OPT_PIPELINE? eSMTP_SendingData: eSMTP_SendData);

			break;
		}
	    case eSMTP_SendingData: {
	    	RETURN_ON_ERROR(SMTP_CODE_START_DATA);

	    	state = eSMTP_SendHeader;

	    	break;
	    }
	    case eSMTP_Sent: {
	    	RETURN_ON_ERROR(SMTP_CODE_REQUEST_OK);

	    	state = eSMTP_Ready;

	    	if(outgoingMail!=nullptr && outgoingMail->callback) {
	    		outgoingMail->callback(*outgoingMail, codeValue, message);
	    	}
	    	delete outgoingMail;
	    	outgoingMail=nullptr;

	    	break;
	    }

	    case eSMTP_Quitting: {
	    	RETURN_ON_ERROR(SMTP_CODE_BYE);
	    	close();
	    	state = eSMTP_Disconnect;

	    	break;
	    }

	    default:
	    	memcpy(message, line, std::min(lineLength, SMTP_ERROR_LENGTH));

		} /* switch(state) */
	}

	return (buffer - start);
}
