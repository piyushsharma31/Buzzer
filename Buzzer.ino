#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266httpUpdate.h>
#include <EepromUtil.h>
#include <ESPConfig.h>
#include <ESP8266Controller.h>
#include "Buzzer.h"

WiFiUDP Udp;
int POWER_LED_PIN = 2;

// maximum bytes for LEDController = 3 LED x 201 bytes = 603 bytes. UDP_TX_PACKET_MAX_SIZE max size of UDP packet
byte packetBuffer[255 * 3];
byte replyBuffer[255 * 3];
short replyBufferSize = 0;

ESPConfig configuration(/*controller name*/	"Buzzer", /*location*/ "Unknown", /*firmware version*/ "buzz.200913.bin", /*router SSID*/ "onion", /*router SSID key*/ "242374666");

// initialize Buzzer
Buzzer buzzer(/*controller name*/ "Buzzer", /*pin*/ 0, /*No. of capabilities*/ 3, configuration.sizeOfEEPROM());

void setup() {

	//delay(1000);
	DEBUG_PRINTLN("ESPController::setup");
	Serial.begin(115200);//,SERIAL_8N1,SERIAL_TX_ONLY);

	configuration.init(POWER_LED_PIN);
	buzzer.loadCapabilities();

	Udp.begin(port);
	DEBUG_PRINTLN("ESPController::setup end");
}

void loop() {

	int packetSize = Udp.parsePacket();

	if (packetSize) {

		DEBUG_PRINTLN();
		DEBUG_PRINT("Received packet of size "); DEBUG_PRINT(packetSize); DEBUG_PRINT(" from "); DEBUG_PRINT(Udp.remoteIP()); DEBUG_PRINT(", port "); DEBUG_PRINTLN(Udp.remotePort());

		// read the packet into packetBuffer
		Udp.read(packetBuffer, packetSize);
		packetBuffer[packetSize] = 0;

		// initialize the replyBuffer
		memcpy(replyBuffer, packetBuffer, 3);

		_udp_packet udpPacket;

		// prepare the UDP header from buffer
		udpPacket._size = packetBuffer[1] << 8 | packetBuffer[0];
		udpPacket._command = packetBuffer[2];
		udpPacket._payload = (char*)packetBuffer + 3;

		if (udpPacket._command == DEVICE_COMMAND_DISCOVER) {
			DEBUG_PRINTLN("command = DEVICE_COMMAND_DISCOVER");

			replyBufferSize = 3 + configuration.discover(replyBuffer+3);

		} else if (udpPacket._command == DEVICE_COMMAND_SET_CONFIGURATION) {
			DEBUG_PRINTLN("command = DEVICE_COMMAND_SET_CONFIGURATION");

			replyBufferSize = 3 + configuration.set(replyBuffer+3, (byte*)udpPacket._payload);

		} else if (udpPacket._command == DEVICE_COMMAND_GET_CONTROLLER) {
			DEBUG_PRINTLN("command = DEVICE_COMMAND_GET_CONTROLLER");

			byte _pin = udpPacket._payload[0];
			if (_pin == buzzer.pin) {

				memcpy(replyBuffer + 3, buzzer.toByteArray(), buzzer.sizeOfUDPPayload());
				replyBufferSize = 3 + buzzer.sizeOfUDPPayload();

			}

		} else if (udpPacket._command == DEVICE_COMMAND_SET_CONTROLLER) {
			DEBUG_PRINTLN("command = DEVICE_COMMAND_SET_CONTROLLER");

			byte _pin = udpPacket._payload[0];

			// (OVERRIDE) send 3 bytes (size, command) as reply to client
			replyBufferSize = 3;

			if (_pin == buzzer.pin) {

				if (buzzer.fromByteArray((byte*)udpPacket._payload)) {
					//eepromUpdatePending = true;//moved to fromByteArray()
					DEBUG_PRINTLN("eepromUpdatePending = true");
				}
			}

		} else if (udpPacket._command == DEVICE_COMMAND_GETALL_CONTROLLER) {
			DEBUG_PRINTLN("command = DEVICE_COMMAND_GETALL_CONTROLLER");

			memcpy(replyBuffer + 3, buzzer.toByteArray(), buzzer.sizeOfUDPPayload());

			replyBufferSize = 3 + buzzer.sizeOfUDPPayload();

		} else if (udpPacket._command == DEVICE_COMMAND_SETALL_CONTROLLER) {
			DEBUG_PRINTLN("command = DEVICE_COMMAND_SETALL_CONTROLLER");

			int i = 0;

			// update the LED variables with new values
			for (int count = 0; count < 3; count++) {

				if (udpPacket._payload[i] == buzzer.pin) {

					if (buzzer.fromByteArray((byte*)udpPacket._payload + i)) {
						buzzer.saveCapabilities();
					}

					i = i + buzzer.sizeOfEEPROM();

				}
			}

			// (OVERRIDE) send 3 bytes (size, command) as reply to client
			replyBufferSize = 3;

		}

		// update the size of replyBuffer in packet bytes
		replyBuffer[0] = lowByte(replyBufferSize);
		replyBuffer[1] = highByte(replyBufferSize);

		// send a reply, to the IP address and port that sent us the packet we received
		Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
		Udp.write(replyBuffer, replyBufferSize);
		Udp.endPacket();
	}

	buzzer.loop();

	yield();

}
