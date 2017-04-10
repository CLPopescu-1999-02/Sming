/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/anakod/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 ****/

#ifndef _SMING_CORE_NETWORK_WEBCONSTANTS_H_
#define _SMING_CORE_NETWORK_WEBCONSTANTS_H_

#define CONTENT_TYPE_HTML = "text/html";
#define CONTENT_TYPE_TEXT = "text/plain";
#define CONTENT_TYPE_JS = "text/javascript";
#define CONTENT_TYPE_CSS = "text/css";
#define CONTENT_TYPE_XML = "text/xml";

	// Images
#define CONTENT_TYPE_JPEG = "image/jpeg";
#define CONTENT_TYPE_GIF = "image/gif";
#define CONTENT_TYPE_PNG = "image/png";
#define CONTENT_TYPE_SVG = "image/svg+xml";
#define CONTENT_TYPE_ICO = "image/x-icon";

#define CONTENT_TYPE_GZIP = "application/x-gzip";
#define CONTENT_TYPE_ZIP = "application/zip";
#define CONTENT_TYPE_JSON = "application/json";



#define MIME_TYPE_MAP(XX)                  \
  /* Type | extension start | Mime type */ \
							               \
  /* Texts */				               \
  XX(HTML, "htm", "text/html")             \
  XX(TEXT, "txt", "text/plain")            \
  XX(JS, "js", "text/javascript")          \
  XX(CSS, "css", "text/css")               \
  XX(XML, "xml", "text/xml")               \
  XX(JSON, "json", "application/json")     \
  	  	  	  	  	  	  	               \
  /* Images */                             \
  XX(JPEG, "jpg", "image/jpeg")            \
  XX(GIF, "git", "image/gif")              \
  XX(PNG, "png", "image/png")              \
  XX(SVG, "svg", "image/svg+xml")          \
  XX(ICO, "ico", "image/x-icon")           \
                                           \
  /* Archives */                           \
  XX(GZIP, "gzip", "application/x-gzip")   \
  XX(ZIP, "zip", "application/zip")        \
  	  	  	  	  	  	  	  	           \
  /* Binary and Form*/                     \
  XX(BINARY, "", "application/octet-stream")   \
  XX(FORM_URL_ENCODED, "", "application/x-www-form-urlencoded") \
  XX(FORM_MULTIPART, "", "multipart/form-data") \

enum MimeType
{
#define XX(name, extensionStart, mime) MIME_##name,
	MIME_TYPE_MAP(XX)
#undef XX
};

namespace ContentType
{
	static const char* fromFileExtension(const String extension)
	{
		String ext = extension;
		ext.toLowerCase();

		#define XX(name, extensionStart, mime) if(ext.startsWith(extensionStart)) {  return #mime; }
		  MIME_TYPE_MAP(XX)
		#undef XX

		// Unknown
		return "<unknown>";
	}

	static const char *toString(enum MimeType m)
	{
		#define XX(name, extensionStart, mime) if(MIME_##name == m) {  return #mime; }
		  MIME_TYPE_MAP(XX)
		#undef XX

		// Unknown
		return "<unknown>";
	}

	static const char* fromFullFileName(const String fileName)
	{
		int p = fileName.lastIndexOf('.');
		if (p != -1)
		{
			String ext = fileName.substring(p + 1);
			const char *mime = ContentType::fromFileExtension(ext);
			return mime;
		}

		return NULL;
	}
};

namespace RequestMethod
{
	static const char* GET = "GET";
	static const char* HEAD = "HEAD";
	static const char* POST = "POST";
	static const char* PUT = "PUT";
	static const char* DELETE = "DELETE";
};

namespace HttpStatusCode
{
	static const char* OK = "200 OK";
	static const char* SwitchingProtocols = "101 Switching Protocols";
	static const char* Found = "302 Found";

	static const char* BadRequest = "400 Bad Request";
	static const char* NotFound = "404 Not Found";
	static const char* Forbidden = "403 Forbidden";
	static const char* Unauthorized = "401 Unauthorized";

	static const char* NotImplemented = "501 Not Implemented";
};

#endif /* _SMING_CORE_NETWORK_WEBCONSTANTS_H_ */
