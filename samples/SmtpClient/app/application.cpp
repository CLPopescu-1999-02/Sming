#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include "SmingCore/Network/SmtpClient.h"

SmtpClient* client = null;

int onServerError(SmtpClient& client, int code, char* status)
{
	debugf("Status: %s", status);

	return 0; // return non-zero value to abort the connection
}

int onMailSent(SmtpClient& client, int code, char* status)
{
	// get the sent mail message
	MailMessage* mail = client.getCurrentMessage();

	// The status line contains the unique ID that was given to this email
	debugf("Status: %s", status);

	// And if there are no more pending emails then you can disconnect from the server
	if(!client.countPending()) {
		client.quit();
	}

	return 0;
}

void onConnected()
{
#ifdef ENABLE_SSL
	client->addSslOptions(SSL_SERVER_VERIFY_LATER);
#endif

	client->onServerError(onServerError);
	client->connect(URL("smtp://user:password@host:port"));

	MailMessage* mail = new MailMessage();
	mail->from = "admin@sming.com";
	mail->to = "slav@attachix.com";
	mail->subject = "Greetings from Sming";
	mail->setBody("Hello.\r\n.\r\n"
		      	  "This is test email from Sming <https://github.com/SmingHub/Sming>"
		      	  "It contains attachment, Ümlauts, кирилица + etc");

	FileStream* file= new FileStream("image.png");
	mail->addAttachment(file);

	client->onMessageSent(onMailSent);
	client->send(mail);
}

void init()
{
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true); // Allow debug print to serial
	Serial.println("Sming. Let's send an email notification!");

	client = new SmtpClient();

	spiffs_mount();
}
