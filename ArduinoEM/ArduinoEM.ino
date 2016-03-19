/*
 Name:		ArduinoEM.ino
 Created:	18-Mar-16 21:16:40
 Author:	Prajay
*/
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
#define TIMEOUT        30000 // Wifi/http timeout
#define ERR_HEAP_SIZE  400 // (n)bytes = 400 errors per 5 minutes(or whatever upload frequency)
//Variables which get loaded from eeprom on startup
// First address of EEPROM to write to.
// marks the end of the data we wrote to EEPROM
const char table_name[] = "WeatherDataItem"; // storing these in EEPROM is not viable
const char server[] = "arduinoem.azurewebsites.net";  // too long, might try concat on load, but that'll defeat its purpose
const int START_ADDRESS = 0;
const byte EEPROM_END_MARK = 0;
//#define      WLAN_SEC_UNSEC (0)
//#define      WLAN_SEC_WEP  (1)
//#define      WLAN_SEC_WPA (2)
//#define      WLAN_SEC_WPA2  (3)
char buffer[400];
int nextEEPROMaddress;
// Sensor variables
int count = 0; // sampling count
float mq2_min = 0, mq3_min = 0, mq4_min = 0, mq5_min = 0, mq6_min = 0, mq7_min = 0, mq8_min = 0, mq9_min = 0, mq135_min = 0, humidity_min = 0, temperature_min = 0, lux_min = 0, uvb_min = 0, pressure_min = 0, dust_min = 0, dust1_min = 0, dust25_min = 0;
float mq2_max = 0, mq3_max = 0, mq4_max = 0, mq5_max = 0, mq6_max = 0, mq7_max = 0, mq8_max = 0, mq9_max = 0, mq135_max = 0, humidity_max = 0, temperature_max = 0, lux_max = 0, uvb_max = 0, pressure_max = 0, dust_max = 0, dust1_max = 0, dust25_max = 0;
float mq2_avg = 0, mq3_avg = 0, mq4_avg = 0, mq5_avg = 0, mq6_avg = 0, mq7_avg = 0, mq8_avg = 0, mq9_avg = 0, mq135_avg = 0, humidity_avg = 0, temperature_avg = 0, lux_avg = 0, uvb_avg = 0, pressure_avg = 0, dust_avg = 0, dust1_avg = 0, dust25_avg = 0;
char err[ERR_HEAP_SIZE] = ""; // store last n errors until every wifi request (per 5 mins default), initialize with null terminator
							  // A = Wifi EEPROM add fail
							  // E = EEPROM fail
							  // W = Wifi
							  // C = Credential/Connection Info
							  // S = BMP180
							  // J = jsonobject send to azure fail
							  // U = USB data send fail
							  // B = BT data send fail
							  //1 MQ2
							  //2 MQ3
							  //3 MQ4
							  //4 MQ5
							  //6 MQ7
							  //7 MQ8
							  //8 MQ9
							  //9 MQ135
							  //D DSM501A
							  //G Sharp GP2y... dust sensor
SFE_CC3000 wifi = SFE_CC3000(CC3000_INT, CC3000_EN, CC3000_CS);
SFE_CC3000_Client client = SFE_CC3000_Client(wifi);
SI7021 si7021;
Adafruit_BMP085 bmp;
TSL2561 tsl(TSL2561_ADDR_FLOAT);
StaticJsonBuffer<5000> jsonBuffer;
JsonObject& outputObject = jsonBuffer.createObject();
float readTemperature()
{
	return si7021.readTemp();
}
float readHumidity()
{
	return si7021.readHumidity();
}
float readPressure()
{
	return bmp.readPressure();
}
float readLux()
{
	return tsl.calculateLux(tsl.getLuminosity(TSL2561_FULLSPECTRUM), tsl.getLuminosity(TSL2561_INFRARED));
}
bool initwifi()
{
	// Initialize CC3000 (configure SPI communications)
	if (!wifi.init())
	{
		strcat(err, "W");
		return false;
	}
	char *ssid = readEEPROMString(START_ADDRESS, 0);
	char *password = readEEPROMString(START_ADDRESS, 1);
	char *sec = readEEPROMString(START_ADDRESS, 2);
	if (!wifi.connect(ssid, sec[0], password, TIMEOUT))
	{
		strcat(err, "C");
		return false;
	}
	return true;
}

bool initI2C()
{
	si7021.begin(); // this library doesnt have proper check mechanism for NOW that returns
	si7021.setHumidityRes(12); // set sensor resolution
	tsl.begin();
	tsl.setGain(TSL2561_GAIN_16X);
	tsl.setTiming(TSL2561_INTEGRATIONTIME_101MS)
		//check if bmp180 is initialized, tsl.begin always returns true
		if (!bmp.begin())
		{
			strcat(err, "S");
			return false;
		}
		else
		{
			return true;
		}
}
//writes null terminated strings
bool writeWifiEEPROM(char _ssid[], char _password[], char _security[])
{
	nextEEPROMaddress = 0; // Cause addtoEEPROM function to overwrite data using update function
	if (addToEEPROM(_ssid) == 1 && addToEEPROM(_password) == 1 && addToEEPROM(_security) == 1)
	{
		EEPROM.update(nextEEPROMaddress++, EEPROM_END_MARK);
		return true;
	}
	else
	{
		strcat(err, "E");
		return false;
	}

}
boolean addToEEPROM(char *text) {
	if (nextEEPROMaddress + strlen(text) + 1 > EEPROM.length()) {
		//  Serial.print(F("ERROR: string would overflow EEPROM length of "));
		//  Serial.print(EEPROM.length());
		//Serial.println(F(" bytes."));
		strcat(err, "A");
		return false;
	}
	do {
		EEPROM.update(nextEEPROMaddress++, (byte)*text);
	} while (*text++ != '\0');
	//  Serial.println(F("Written to EEPROM."));
	return true;
}
char *readEEPROMString(int baseAddress, int stringNumber) {
	int start;   // EEPROM address of the first byte of the string to return.
	int length;  // length (bytes) of the string to return, less the terminating null.
	char ch;
	int nextAddress;  // next address to read from EEPROM.
	char *result;     // points to the dynamically-allocated result to return.
	int i;
	nextAddress = START_ADDRESS;
	for (i = 0; i < stringNumber; ++i) {
		// If the first byte is an end mark, we've run out of strings too early.
		ch = (char)EEPROM.read(nextAddress++);
		if (ch == (char)EEPROM_END_MARK || nextAddress >= EEPROM.length()) {
			return (char *)0;  // not enough strings are in EEPROM.
		}

		// Read through the string's terminating null (0).
		while (ch != '\0' && nextAddress < EEPROM.length()) {
			ch = EEPROM.read(nextAddress++);
		}
	}
	// We're now at the start of what should be our string.
	start = nextAddress;
	// If the first byte is an end mark, we've run out of strings too early.
	ch = (char)EEPROM.read(nextAddress++);
	if (ch == (char)EEPROM_END_MARK) {
		return (char *)0;  // not enough strings are in EEPROM.
	}
	// Count to the end of this string.
	length = 0;
	while (ch != '\0' && nextAddress < EEPROM.length()) {
		++length;
		ch = EEPROM.read(nextAddress++);
	}
	// Allocate space for the string, then copy it.
	result = new char[length + 1];
	nextAddress = start;
	for (i = 0; i < length; ++i) {
		result[i] = (char)EEPROM.read(nextAddress++);
	}
	result[i] = '\0';
	return result;

}

bool sendDataUSB(char _err[], float _mq2_min, float _mq2_max, float _mq2_avg, float _mq3_min, float _mq3_max, float _mq3_avg, float _mq4_min, float _mq4_max, float _mq4_avg, float _mq5_min, float _mq5_max,
	float _mq5_avg, float _mq6_min, float _mq6_max, float _mq6_avg, float _mq7_min, float _mq7_max, float _mq7_avg, float _mq8_min, float _mq8_max, float _mq8_avg, float _mq9_min, float _mq9_max, float _mq9_avg,
	float _mq135_min, float _mq135_max, float _mq135_avg, float _temperature_min, float _temperature_max, float _temperature_avg, float _humidity_min, float _humidity_max, float _humidity_avg, float _lux_min, float _lux_max, float _lux_avg,
	float _uvb_min, float _uvb_max, float _uvb_avg, float _pressure_min, float _pressure_max, float _pressure_avg, float _dust_min, float _dust_avg, float _dust_max, float _dust25_min, float _dust25_avg, float _dust25_max, float _dust1_min, float _dust1_avg, float _dust1_max)
{
	outputObject["mq2_min"] = _mq2_min; outputObject["mq2_avg"] = _mq2_avg; outputObject["mq2_max"] = _mq2_max;
	outputObject["mq3_min"] = _mq3_min; outputObject["mq3_avg"] = _mq3_avg; outputObject["mq3_max"] = _mq3_max;
	outputObject["mq4_min"] = _mq4_min; outputObject["mq4_avg"] = _mq4_avg; outputObject["mq4_max"] = _mq4_max;
	outputObject["mq5_min"] = _mq5_min; outputObject["mq5_avg"] = _mq5_avg; outputObject["mq5_max"] = _mq5_max;
	outputObject["mq6_min"] = _mq6_min; outputObject["mq6_avg"] = _mq6_avg; outputObject["mq6_max"] = _mq6_max;
	outputObject["mq7_min"] = _mq7_min; outputObject["mq7_avg"] = _mq7_avg; outputObject["mq7_max"] = _mq7_max;
	outputObject["mq8_min"] = _mq8_min; outputObject["mq8_avg"] = _mq8_avg; outputObject["mq8_max"] = _mq8_max;
	outputObject["mq9_min"] = _mq9_min; outputObject["mq9_avg"] = _mq9_avg; outputObject["mq9_max"] = _mq9_max;
	outputObject["mq135_min"] = _mq135_min; outputObject["mq135_avg"] = _mq135_avg; outputObject["mq135_max"] = _mq135_max;
	outputObject["temperature_min"] = _temperature_min; outputObject["temperature_avg"] = _temperature_avg; outputObject["temperature_max"] = _temperature_max;
	outputObject["humidity_min"] = _humidity_min; outputObject["humidity_avg"] = _humidity_avg; outputObject["humidity_max"] = _humidity_max;
	outputObject["lux_min"] = _lux_min; outputObject["lux_avg"] = _lux_avg; outputObject["lux_max"] = _lux_max;
	outputObject["uvb_min"] = _uvb_min; outputObject["uvb_avg"] = _uvb_avg; outputObject["uvb_max"] = _uvb_max;
	outputObject["err"] = _err; outputObject["pressure_min"] = _pressure_min; outputObject["pressure_max"] = _pressure_max; outputObject["pressure_avg"] = _pressure_avg;
	outputObject["dust_min"] = _dust_min; outputObject["dust_max"] = _dust_max; outputObject["dust_avg"] = _dust_avg;
	outputObject["dust1_min"] = _dust1_min; outputObject["dust1_max"] = _dust1_max; outputObject["dust1_avg"] = _dust1_avg;
	outputObject["dust25_min"] = _dust25_min; outputObject["dust25_max"] = _dust25_max; outputObject["dust25_avg"] = _dust25_avg;
	if (!outputObject.printTo(Serial))
	{
		strcat(err, "U");
		return false;
	}
	else
	{
		return true;
	}
}
//This function takes 39 +1 values from the sensor and then creates a json object, which is sent to the UWP app to 
bool sendDataBT(char _err[], float _mq2_min, float _mq2_max, float _mq2_avg, float _mq3_min, float _mq3_max, float _mq3_avg, float _mq4_min, float _mq4_max, float _mq4_avg, float _mq5_min, float _mq5_max,
	float _mq5_avg, float _mq6_min, float _mq6_max, float _mq6_avg, float _mq7_min, float _mq7_max, float _mq7_avg, float _mq8_min, float _mq8_max, float _mq8_avg, float _mq9_min, float _mq9_max, float _mq9_avg,
	float _mq135_min, float _mq135_max, float _mq135_avg, float _temperature_min, float _temperature_max, float _temperature_avg, float _humidity_min, float _humidity_max, float _humidity_avg, float _lux_min, float _lux_max, float _lux_avg,
	float _uvb_min, float _uvb_max, float _uvb_avg, float _pressure_min, float _pressure_max, float _pressure_avg, float _dust_min, float _dust_avg, float _dust_max, float _dust25_min, float _dust25_avg, float _dust25_max, float _dust1_min, float _dust1_avg, float _dust1_max)
{
	outputObject["mq2_min"] = _mq2_min; outputObject["mq2_avg"] = _mq2_avg; outputObject["mq2_max"] = _mq2_max;
	outputObject["mq3_min"] = _mq3_min; outputObject["mq3_avg"] = _mq3_avg; outputObject["mq3_max"] = _mq3_max;
	outputObject["mq4_min"] = _mq4_min; outputObject["mq4_avg"] = _mq4_avg; outputObject["mq4_max"] = _mq4_max;
	outputObject["mq5_min"] = _mq5_min; outputObject["mq5_avg"] = _mq5_avg; outputObject["mq5_max"] = _mq5_max;
	outputObject["mq6_min"] = _mq6_min; outputObject["mq6_avg"] = _mq6_avg; outputObject["mq6_max"] = _mq6_max;
	outputObject["mq7_min"] = _mq7_min; outputObject["mq7_avg"] = _mq7_avg; outputObject["mq7_max"] = _mq7_max;
	outputObject["mq8_min"] = _mq8_min; outputObject["mq8_avg"] = _mq8_avg; outputObject["mq8_max"] = _mq8_max;
	outputObject["mq9_min"] = _mq9_min; outputObject["mq9_avg"] = _mq9_avg; outputObject["mq9_max"] = _mq9_max;
	outputObject["mq135_min"] = _mq135_min; outputObject["mq135_avg"] = _mq135_avg; outputObject["mq135_max"] = _mq135_max;
	outputObject["temperature_min"] = _temperature_min; outputObject["temperature_avg"] = _temperature_avg; outputObject["temperature_max"] = _temperature_max;
	outputObject["humidity_min"] = _humidity_min; outputObject["humidity_avg"] = _humidity_avg; outputObject["humidity_max"] = _humidity_max;
	outputObject["lux_min"] = _lux_min; outputObject["lux_avg"] = _lux_avg; outputObject["lux_max"] = _lux_max;
	outputObject["uvb_min"] = _uvb_min; outputObject["uvb_avg"] = _uvb_avg; outputObject["uvb_max"] = _uvb_max;
	outputObject["err"] = _err; outputObject["pressure_min"] = _pressure_min; outputObject["pressure_max"] = _pressure_max; outputObject["pressure_avg"] = _pressure_avg;
	outputObject["dust_min"] = _dust_min; outputObject["dust_max"] = _dust_max; outputObject["dust_avg"] = _dust_avg;
	outputObject["dust1_min"] = _dust1_min; outputObject["dust1_max"] = _dust1_max; outputObject["dust1_avg"] = _dust1_avg;
	outputObject["dust25_min"] = _dust25_min; outputObject["dust25_max"] = _dust25_max; outputObject["dust25_avg"] = _dust25_avg;

	if (!outputObject.printTo(Serial1))
	{
		strcat(err, "B");
		return false;
	}
	else
	{
		return true;
	}
}
bool sendDataWifi(char _err[], float _mq2_min, float _mq2_max, float _mq2_avg, float _mq3_min, float _mq3_max, float _mq3_avg, float _mq4_min, float _mq4_max, float _mq4_avg, float _mq5_min, float _mq5_max,
	float _mq5_avg, float _mq6_min, float _mq6_max, float _mq6_avg, float _mq7_min, float _mq7_max, float _mq7_avg, float _mq8_min, float _mq8_max, float _mq8_avg, float _mq9_min, float _mq9_max, float _mq9_avg,
	float _mq135_min, float _mq135_max, float _mq135_avg, float _temperature_min, float _temperature_max, float _temperature_avg, float _humidity_min, float _humidity_max, float _humidity_avg, float _lux_min, float _lux_max, float _lux_avg,
	float _uvb_min, float _uvb_max, float _uvb_avg, float _pressure_min, float _pressure_max, float _pressure_avg, float _dust_min, float _dust_avg, float _dust_max, float _dust25_min, float _dust25_avg, float _dust25_max, float _dust1_min, float _dust1_avg, float _dust1_max)
{
	outputObject["mq2_min"] = _mq2_min; outputObject["mq2_avg"] = _mq2_avg; outputObject["mq2_max"] = _mq2_max;
	outputObject["mq3_min"] = _mq3_min; outputObject["mq3_avg"] = _mq3_avg; outputObject["mq3_max"] = _mq3_max;
	outputObject["mq4_min"] = _mq4_min; outputObject["mq4_avg"] = _mq4_avg; outputObject["mq4_max"] = _mq4_max;
	outputObject["mq5_min"] = _mq5_min; outputObject["mq5_avg"] = _mq5_avg; outputObject["mq5_max"] = _mq5_max;
	outputObject["mq6_min"] = _mq6_min; outputObject["mq6_avg"] = _mq6_avg; outputObject["mq6_max"] = _mq6_max;
	outputObject["mq7_min"] = _mq7_min; outputObject["mq7_avg"] = _mq7_avg; outputObject["mq7_max"] = _mq7_max;
	outputObject["mq8_min"] = _mq8_min; outputObject["mq8_avg"] = _mq8_avg; outputObject["mq8_max"] = _mq8_max;
	outputObject["mq9_min"] = _mq9_min; outputObject["mq9_avg"] = _mq9_avg; outputObject["mq9_max"] = _mq9_max;
	outputObject["mq135_min"] = _mq135_min; outputObject["mq135_avg"] = _mq135_avg; outputObject["mq135_max"] = _mq135_max;
	outputObject["temperature_min"] = _temperature_min; outputObject["temperature_avg"] = _temperature_avg; outputObject["temperature_max"] = _temperature_max;
	outputObject["humidity_min"] = _humidity_min; outputObject["humidity_avg"] = _humidity_avg; outputObject["humidity_max"] = _humidity_max;
	outputObject["lux_min"] = _lux_min; outputObject["lux_avg"] = _lux_avg; outputObject["lux_max"] = _lux_max;
	outputObject["uvb_min"] = _uvb_min; outputObject["uvb_avg"] = _uvb_avg; outputObject["uvb_max"] = _uvb_max;
	outputObject["err"] = _err; outputObject["pressure_min"] = _pressure_min; outputObject["pressure_max"] = _pressure_max; outputObject["pressure_avg"] = _pressure_avg;
	outputObject["dust_min"] = _dust_min; outputObject["dust_max"] = _dust_max; outputObject["dust_avg"] = _dust_avg;
	outputObject["dust1_min"] = _dust1_min; outputObject["dust1_max"] = _dust1_max; outputObject["dust1_avg"] = _dust1_avg;
	outputObject["dust25_min"] = _dust25_min; outputObject["dust25_max"] = _dust25_max; outputObject["dust25_avg"] = _dust25_avg;
	if (client.connect(server, 80))
	{
		sprintf(buffer, "POST /tables/%s HTTP/1.1", table_name);
		client.println(buffer);
		sprintf(buffer, "Host: %s", server);
		client.println(buffer);
		client.println("ZUMO-API-VERSION: 2.0.0"); // Azure Mobile Services header,not required if your app is configuired not to check version
		client.println("Content-Type: application/json"); // JSON content type
		outputObject.printTo(buffer, sizeof(buffer));    // POST body
		client.print("Content-Length: ");  // Content length
		client.println(strlen(buffer));
		// End of headers
		client.println();
		// Request body
		client.println(buffer);
		err[0] = '\0';
		return true;
	}
	else
	{

		err;
		strcat(err, "J");
		return false;
	}
}

void setup()
{
	nextEEPROMaddress = START_ADDRESS;
	Serial.begin(115200);
	Serial1.begin(115200); // Apparently COM ports on windows only support upto 115200 baud as per device manager
	pinMode(SHARP_LED, OUTPUT);
	pinMode(DSM501A_DO_PM1, INPUT);
	pinMode(DSM501A_DO_PM25, INPUT);
	initwifi();
	initI2C(); // initialize I2C sensors
}
void loop()
{

}

