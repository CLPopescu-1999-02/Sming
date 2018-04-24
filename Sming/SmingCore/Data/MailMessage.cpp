/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/SmingHub/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 *
 * @author Slavey Karadzhov <slaff@attachix.com>
 *
 ****/

#include "MailMessage.h"

#include "../Data/Stream/Base64OutputStream.h"
#include "../Data/Stream/QuotedPrintableOutputStream.h"

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
	attachment.stream = new Base64OutputStream(stream);
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
		(*text.headers)["Content-Transfer-Encoding"] = "quoted-printable";
		text.stream = new QuotedPrintableOutputStream(this->stream);

		attachments.addElement(text);
		headers["Content-Type"] = String("multipart/mixed; boundary=") + mStream->getBoundary();
		this->stream = mStream;
	}

	attachments.addElement(attachment);

	return true;
}
