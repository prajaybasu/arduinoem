/*
 Name:		ArduinoEM.ino
 Created:	18-Mar-16 21:16:40
 Author:	Prajay
*/

// the setup function runs once when you press reset or power the board
#include <ArduinoJson.h>
#include <Si7021.h>
#include <TSL2561.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <Si7021.h>
#include <SFE_CC3000.h>
#include <SFE_CC3000_Client.h>
//Static variables for pins - gets replaced on compliation
// Replace them with your own, if using different connections
#define CC3000_INT      2    // Needs to be an interrupt pin (D2/D3)
#define CC3000_EN       49   // Can be any digital pin
#define CC3000_CS       53   // Preferred is pin 10 on Uno
//A0-11 on Mega (54 + AX = analog pin number)
//assuming this project uses Mega because other board dont have many ADC pins, might hook it up to a pro mini with an I2C ADC as well !
#define MQ2_AO          54 //A0
#define MQ3_AO          55  
#define MQ4_AO          56
#define MQ5_AO          57
#define MQ6_AO          58
#define MQ7_AO          59
#define MQ8_AO          60
#define MQ9_AO          61
#define MQ135_AO        62
#define ML8511_AO       63
#define SHARP_AO        64   // GP2Y1010AU0F Pin 5
#define SHARP_LED       3     //GP2Y1010AU0F Pin 3 (PWM)
#define DSM501A_DO_PM1  4     //Pin 2 (PM1 - not PM10)
#define DSM501A_DO_PM25 5     //Pin 4 (PM2.5)
#define SENSE_3V3       65   // A11 -Analog pin, to get more precise readings from 3V3 analog sensors
//Other hard-coded values from datasheets/websites
#define SHARP_DELAY    280    //280 us (microseconds) from GP2Y1010AU0F datasheet
#define SHARP_DELAY1   40     //40 us (microseconds) from GP2Y1010AU0F datasheet\
#define BAUD_BLUETOOTH 250000 // the module can go max upto 1382400, arduino safely upto 200-220k
#define BAUD_USB       250000 // at 115200 you can send 512 bytes at 225.0 Hz, which is good enough since we also need to plug all of that in to graphs and stuff
//Variables which get loaded from eeprom on startup
const char table_name[] = "WeatherDataItem"; // storing these in EEPROM is not viable
const char server[] = "";  // too long, might try concat on load, but that'll defeat its purpose
char ssid[] = "";
char password[] = "";
char buffer[400];
// Sensor variables
float mq2, mq3, mq4, mq5, mq6, mq7, mq8, mq9, mq135, hum, temp, lum, uvb;
//
SFE_CC3000 wifi = SFE_CC3000(CC3000_INT, CC3000_EN, CC3000_CS);
SFE_CC3000_Client client = SFE_CC3000_Client(wifi);
SI7021 si7021;
Adafruit_BMP085 bmp;
TSL2561 tsl(TSL2561_ADDR_FLOAT);


bool initwifi()
{
	//TODO : restore wifi creds from  EEPROM
	// Initialize CC3000 (configure SPI communications)
	if (wifi.init()) {
		Serial.println("CC3000 initialization complete");
	}
	else {
		Serial.println("Something went wrong during CC3000 init!");
	}

}
int freeRam()
{
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

bool initI2C()
{
	si7021.begin();
	//check if bmp180 is initialized
	if (!bmp.begin() || tsl.begin())
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
bool sendDataUSB(char _err[], float _mq2_min, float _mq2_max, float _mq2_avg, float _mq3_min, float _mq3_max, float _mq3_avg, float _mq4_min, float _mq4_max, float _mq4_avg, float _mq5_min, float _mq5_max,
	float _mq5_avg, float _mq6_min, float _mq6_max, float _mq6_avg, float _mq7_min, float _mq7_max, float _mq7_avg, float _mq8_min, float _mq8_max, float _mq8_avg, float _mq9_min, float _mq9_max, float _mq9_avg,
	float _mq135_min, float _mq135_max, float _mq135_avg, float _temp_min, float _temp_max, float _temp_avg, float _hum_min, float _hum_max, float _hum_avg, float _lum_min, float _lum_max, float _lum_avg,
	float _uvb_min, float _uvb_max, float _uvb_avg)
{
	StaticJsonBuffer<JSON_OBJECT_SIZE(42)> jsonBuffer;
	JsonObject& object = jsonBuffer.createObject();
	object["mq2_min"] = _mq2_min; object["mq2_avg"] = _mq2_avg; object["mq2_max"] = _mq2_max;
	object["mq3_min"] = _mq3_min; object["mq3_avg"] = _mq3_avg; object["mq3_max"] = _mq3_max;
	object["mq4_min"] = _mq4_min; object["mq4_avg"] = _mq4_avg; object["mq4_max"] = _mq4_max;
	object["mq5_min"] = _mq5_min; object["mq5_avg"] = _mq5_avg; object["mq5_max"] = _mq5_max;
	object["mq6_min"] = _mq6_min; object["mq6_avg"] = _mq6_avg; object["mq6_max"] = _mq6_max;
	object["mq7_min"] = _mq7_min; object["mq7_avg"] = _mq7_avg; object["mq7_max"] = _mq7_max;
	object["mq8_min"] = _mq8_min; object["mq8_avg"] = _mq8_avg; object["mq8_max"] = _mq8_max;
	object["mq9_min"] = _mq9_min; object["mq9_avg"] = _mq9_avg; object["mq9_max"] = _mq9_max;
	object["mq135_min"] = _mq135_min; object["mq135_avg"] = _mq135_avg; object["mq135_max"] = _mq135_max;
	object["temp_min"] = _temp_min; object["temp_avg"] = _temp_avg; object["temp_max"] = _temp_max;
	object["hum_min"] = _hum_min; object["hum_avg"] = _hum_avg; object["hum_max"] = _hum_max;
	object["lum_min"] = _lum_min; object["lum_avg"] = _lum_avg; object["lum_max"] = _lum_max;
	object["uvb_min"] = _uvb_min; object["uvb_avg"] = _uvb_avg; object["uvb_max"] = _uvb_max;
	object["err"] = _err;
	object.printTo(Serial);
}
//This function takes 39 +1 values from the sensor and then creates a json object, which is sent to the UWP app to 
bool sendDataBT(char _err[], float _mq2_min, float _mq2_max, float _mq2_avg, float _mq3_min, float _mq3_max, float _mq3_avg, float _mq4_min, float _mq4_max, float _mq4_avg, float _mq5_min, float _mq5_max,
	float _mq5_avg, float _mq6_min, float _mq6_max, float _mq6_avg, float _mq7_min, float _mq7_max, float _mq7_avg, float _mq8_min, float _mq8_max, float _mq8_avg, float _mq9_min, float _mq9_max, float _mq9_avg,
	float _mq135_min, float _mq135_max, float _mq135_avg, float _temp_min, float _temp_max, float _temp_avg, float _hum_min, float _hum_max, float _hum_avg, float _lum_min, float _lum_max, float _lum_avg,
	float _uvb_min, float _uvb_max, float _uvb_avg)
{
	StaticJsonBuffer<JSON_OBJECT_SIZE(42)> jsonBuffer;
	JsonObject& object = jsonBuffer.createObject();
	object["mq2_min"] = _mq2_min; object["mq2_avg"] = _mq2_avg; object["mq2_max"] = _mq2_max;
	object["mq3_min"] = _mq3_min; object["mq3_avg"] = _mq3_avg; object["mq3_max"] = _mq3_max;
	object["mq4_min"] = _mq4_min; object["mq4_avg"] = _mq4_avg; object["mq4_max"] = _mq4_max;
	object["mq5_min"] = _mq5_min; object["mq5_avg"] = _mq5_avg; object["mq5_max"] = _mq5_max;
	object["mq6_min"] = _mq6_min; object["mq6_avg"] = _mq6_avg; object["mq6_max"] = _mq6_max;
	object["mq7_min"] = _mq7_min; object["mq7_avg"] = _mq7_avg; object["mq7_max"] = _mq7_max;
	object["mq8_min"] = _mq8_min; object["mq8_avg"] = _mq8_avg; object["mq8_max"] = _mq8_max;
	object["mq9_min"] = _mq9_min; object["mq9_avg"] = _mq9_avg; object["mq9_max"] = _mq9_max;
	object["mq135_min"] = _mq135_min; object["mq135_avg"] = _mq135_avg; object["mq135_max"] = _mq135_max;
	object["temp_min"] = _temp_min; object["temp_avg"] = _temp_avg; object["temp_max"] = _temp_max;
	object["hum_min"] = _hum_min; object["hum_avg"] = _hum_avg; object["hum_max"] = _hum_max;
	object["lum_min"] = _lum_min; object["lum_avg"] = _lum_avg; object["lum_max"] = _lum_max;
	object["uvb_min"] = _uvb_min; object["uvb_avg"] = _uvb_avg; object["uvb_max"] = _uvb_max;
	object["err"] = _err;
	object.printTo(Serial1);
}
bool sendDataWifi(char _err[], float _mq2_min, float _mq2_max, float _mq2_avg, float _mq3_min, float _mq3_max, float _mq3_avg, float _mq4_min, float _mq4_max, float _mq4_avg, float _mq5_min, float _mq5_max,
	float _mq5_avg, float _mq6_min, float _mq6_max, float _mq6_avg, float _mq7_min, float _mq7_max, float _mq7_avg, float _mq8_min, float _mq8_max, float _mq8_avg, float _mq9_min, float _mq9_max, float _mq9_avg,
	float _mq135_min, float _mq135_max, float _mq135_avg, float _temp_min, float _temp_max, float _temp_avg, float _hum_min, float _hum_max, float _hum_avg, float _lum_min, float _lum_max, float _lum_avg,
	float _uvb_min, float _uvb_max, float _uvb_avg)
{
	StaticJsonBuffer<JSON_OBJECT_SIZE(42)> jsonBuffer;
	JsonObject& object = jsonBuffer.createObject();
	object["mq2_min"] = _mq2_min; object["mq2_avg"] = _mq2_avg; object["mq2_max"] = _mq2_max;
	object["mq3_min"] = _mq3_min; object["mq3_avg"] = _mq3_avg; object["mq3_max"] = _mq3_max;
	object["mq4_min"] = _mq4_min; object["mq4_avg"] = _mq4_avg; object["mq4_max"] = _mq4_max;
	object["mq5_min"] = _mq5_min; object["mq5_avg"] = _mq5_avg; object["mq5_max"] = _mq5_max;
	object["mq6_min"] = _mq6_min; object["mq6_avg"] = _mq6_avg; object["mq6_max"] = _mq6_max;
	object["mq7_min"] = _mq7_min; object["mq7_avg"] = _mq7_avg; object["mq7_max"] = _mq7_max;
	object["mq8_min"] = _mq8_min; object["mq8_avg"] = _mq8_avg; object["mq8_max"] = _mq8_max;
	object["mq9_min"] = _mq9_min; object["mq9_avg"] = _mq9_avg; object["mq9_max"] = _mq9_max;
	object["mq135_min"] = _mq135_min; object["mq135_avg"] = _mq135_avg; object["mq135_max"] = _mq135_max;
	object["temp_min"] = _temp_min; object["temp_avg"] = _temp_avg; object["temp_max"] = _temp_max;
	object["hum_min"] = _hum_min; object["hum_avg"] = _hum_avg; object["hum_max"] = _hum_max;
	object["lum_min"] = _lum_min; object["lum_avg"] = _lum_avg; object["lum_max"] = _lum_max;
	object["uvb_min"] = _uvb_min; object["uvb_avg"] = _uvb_avg; object["uvb_max"] = _uvb_max;
	object["err"] = _err;
	if (client.connect(server, 80))
	{
		sprintf(buffer, "POST /tables/%s HTTP/1.1", table_name);
		client.println(buffer);
		sprintf(buffer, "Host: %s", server);
		client.println(buffer);
		client.println("ZUMO-API-VERSION: 2.0.0"); // Azure Easy Tables header
		client.println("Content-Type: application/json"); // JSON content type
		object.printTo(buffer, sizeof(buffer));    // POST body
		client.print("Content-Length: ");  // Content length
		client.println(strlen(buffer));
		// End of headers
		client.println();
		// Request body
		client.println(buffer);
		return 1;
	}
	else
	{
		return 0;
	}
}
void setup()
{
	Serial.begin(115200);
	Serial1.begin(115200);
	pinMode(SHARP_LED, OUTPUT);
	pinMode(DSM501A_DO_PM1, INPUT);
	pinMode(DSM501A_DO_PM25, INPUT);
	initwifi();
	initI2C(); // initialize I2C sensors
}
void loop()
{

}
