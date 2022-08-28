#ifndef Buzzer_h
#define Buzzer_h

// store last time EEPROM was updated with Buzzer values
unsigned long lastMillisUpdateEEPROM = 0;
unsigned long currentMillis = 0;

class Buzzer : public ESP8266Controller {

public:

	// store Buzzer current state; can be different from pinState when blink & fade both are ON
	short state = LOW;

	// pause time in seconds when buzzer is set to play continuously
	short pauseTime = 0;

	// play time in seconds
	short playTime = 0;

	// store current time
	unsigned long currentMillis = millis();
	// last time played
	unsigned long playPreviousMillis = 0;
	// last time play was paused
	unsigned long pausePreviousMillis = 0;

	// time when first entered loop() function for this object
	unsigned long startTime = 0;

	// EEPROM address where this color is stored
	//int rgb_address = 0;

public:
	Buzzer(char* nam, uint8_t _pin, uint8_t capCount, int start_address):ESP8266Controller(nam, _pin, capCount, start_address)	{
		DEBUG_PRINTLN("Buzzer::Buzzer");

		//pin = _pin;//called in ESP8266Controller()
		//strcpy(controllerName, nam);//called in ESP8266Controller()

		pinMode(pin, OUTPUT);
		digitalWrite(pin, LOW);

		// On and Off Buzzer
		strcpy(capabilities[0]._name, "ON/OFF");
		capabilities[0]._value_min = 0;
		capabilities[0]._value_max = 1;
		capabilities[0]._value  = 0;

		// Toggle the Buzzer (on/off) at an interval (pauseTime)
		strcpy(capabilities[1]._name, "PAUSE TIME");
		capabilities[1]._value_min = 0;
		capabilities[1]._value_max = 60000;
		capabilities[1]._value  = 0;

		// Buzzer play duration (playTime)
		strcpy(capabilities[2]._name, "PLAY TIME");
		capabilities[2]._value_min = 0;
		capabilities[2]._value_max = 60000;
		capabilities[2]._value  = 0;
	}

public:
	virtual void loop();

};

void Buzzer::loop() {
	// check to see if it's time to change the state of the Buzzer
	currentMillis = millis();

	if (currentMillis - lastEepromUpdate > 15000) {
		DEBUG_PRINTLN();

		lastEepromUpdate = millis();
		DEBUG_PRINT("[MAIN] Free heap: ");
		Serial.println(ESP.getFreeHeap(), DEC);

		// save object every one minute if required
		if(eepromUpdatePending == true) {

			saveCapabilities();
			eepromUpdatePending = false;
			DEBUG_PRINTLN("saveCapabilities eepromUpdatePending");
		}

	}

	if (capabilities[1]._value  == 0) {
		// pauseTime==0 seconds

		if (capabilities[0]._value  == 0 && pinState == HIGH ) {
			// Buzzer is currently ON & OnOff==Off

			// Buzzer should be ALWAYS OFF in these conditions
			pinState = LOW;
			digitalWrite(pin, pinState);

			DEBUG_PRINT(currentMillis / 1000); DEBUG_PRINT(" Buzzer "); toString();

		} else if(capabilities[0]._value  == 1 && pinState == HIGH ) {
			// Buzzer is currently ON & OnOff==On

			if(currentMillis - playPreviousMillis >= capabilities[2]._value) {

				// Put off the Buzzer after play time
				pinState = LOW;
				digitalWrite(pin, pinState);
				capabilities[0]._value = 0;

				DEBUG_PRINT(currentMillis / 1000); DEBUG_PRINT(" Buzzer "); toString();

			}

		} else if(capabilities[0]._value  == 1 && pinState == LOW ) {
			// Buzzer is currently OFF & OnOff==On

			// Buzzer should be ALWAYS ON in these conditions
			pinState = HIGH;
			digitalWrite(pin, pinState);
			playPreviousMillis = currentMillis;

			DEBUG_PRINT(currentMillis / 1000); DEBUG_PRINT(" Buzzer "); toString();

		}

	} else if (capabilities[1]._value  > 0 && capabilities[0]._value  == 1) {
		// blinking==ON & OnOff==On

		if ( currentMillis - pausePreviousMillis >= capabilities[1]._value) {
			// pause delay over

			if (pinState == LOW) {
				// Buzzer should turn on
				pinState = HIGH;
				digitalWrite(pin, pinState);

				// Remember last play time
				playPreviousMillis = currentMillis;

				DEBUG_PRINT(currentMillis / 1000); DEBUG_PRINT(" Buzzer "); toString();
			}

		}

		if ( currentMillis - playPreviousMillis >= capabilities[2]._value) {
			// play time over

			if (pinState == HIGH) {
				// Buzzer should turn off
				pinState = LOW;
				digitalWrite(pin, pinState);

				// Remember last pause time
				pausePreviousMillis = currentMillis;

				DEBUG_PRINT(currentMillis / 1000); DEBUG_PRINT(" Buzzer "); toString();
			}

		}

	} else if (capabilities[1]._value  > 0 && capabilities[0]._value  == 0) {
		// blinking==ON & OnOff==Off

		if(pinState==HIGH) {
			// Buzzer should be ALWAYS OFF in these conditions
			pinState = LOW;
			digitalWrite(pin, pinState);

			DEBUG_PRINT(currentMillis / 1000); DEBUG_PRINT(" Buzzer "); toString();
		}

	}
}

#endif
