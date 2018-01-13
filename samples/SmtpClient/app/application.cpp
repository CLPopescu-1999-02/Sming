#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include "SmingCore/Network/SmtpClient.h"

SmtpClient* client = null;


void onConnectError(const SmtpClient& client, int code, char *status)
{
	debugf("Status: %s", status);
}

void onMailSent(const MailMessage& message, int code, char* status)
{
	debugf("Status: %s", status);
}

void onConnected() 
{
#ifdef ENABLE_SSL
	client->addSslOptions(SSL_SERVER_VERIFY_LATER);
#endif

	client->onConnectError(onConnectError);
	client->connect(URL("smtp://user:password@host:port"));

	MailMessage* mail = new MailMessage();
	mail->from = "admin@sming.com";
	mail->to = "slav@attachix.com";
	mail->subject = "Smtp Subject";
	mail->setBody("Test Data");

	FileStream* file= new FileStream("data.txt");
	mail->addAttachment(file);

	mail->onSent(onMailSent);

	client->send(mail);
}

void init()
{
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true); // Allow debug print to serial
	Serial.println("Sming. Let's send an email notification!");

	client = new SmtpClient();

	// TODO:: finish the sample once the SmtpClient is ready for prime time.
}
