/*
 Name:		ArduinoEM.ino
 Created:	18-Mar-16 21:16:40
 Author:	Prajay
*/
#include <SharpDust.h>
#include <DSM501.h>
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
const int CC3000_INT = 2;      //D2 on Mega2560. CC3000 INTERRUPT pin
const int CC3000_EN = 49;     //D49 on Mega2560. CC3000 ENABLE pin. Can be any Digitial pin.
const int CC3000_CS = 53;		//D53(Hardware SPI Chip Select) on Mega2560. CC3000 Chip Select pin.
const int MQ2_AO = 54;     //A0 on Mega2560. 
const int MQ3_AO = 55;     //A1 on Mega2560. 
const int MQ4_AO = 56;     //A2 on Mega2560. 
const int MQ5_AO = 57;     //A3 on Mega2560. 
const int MQ6_AO = 58;     //A4 on Mega2560. 
const int MQ7_AO = 59;     //A5 on Mega2560. 
const int MQ8_AO = 60;     //A6 on Mega2560. 
const int MQ9_AO = 61;     //A7 on Mega2560. 
const int MQ135_AO = 62;     //A8 on Mega2560. 
const int ML8511_AO = 63;     //A9 on Mega2560. 
const int SHARP_AO = 64;     //A10 on Mega2560. GP2Y1010AU0F Pin 5.
const int SHARP_LED = 3;      //D3 (PWM) on Mega2560. GP2Y1010AU0F Pin 3.
const int DSM501A_DO_PM1 = 4;      //D4 (PWM) on Mega2560. DSM501A Pin 2 (PM1 - not PM10)
const int DSM501A_DO_PM25 = 5;      //D5 (PWM) on Mega2560. DSM501A Pin 4 (PM2.5)
const int SENSE_3V3 = 65;     //A11 on Mega2560. For precise voltage reference for 3V3 sensors.
const int TIMEOUT = 30000;  //Wifi/http timeout
const int ERR_HEAP_SIZE = 256;    //(n)bytes = 256 errors per 5 minutes(or whatever upload frequency), do not go over 512 to avoid CC3000 buffer overflow
const int maxCount = 300;    //Max number of samples
const char table_name[] = "WeatherDataItem"; //
const char server[] = "arduinoem.azurewebsites.net";  // too long, might try concat on load, but that'll defeat its purpose
const int START_ADDRESS = 0;
const byte EEPROM_END_MARK = 0;
int device_id = 1;
boolean USBData = false;                   //Is data over USB enabled 
boolean BTData = false;                    //Is data over Bluetooth enabled
boolean wifiConnected;             //Is CC3000 connected to a wifi AP 
char buffer[1536];                 //HTTP client buffer
int nextEEPROMaddress;
int deviceId = 0;                  //To avoid conflicts when building a newtork of such devices
unsigned long lastMillis_data;
unsigned long lastMillis_sample;
unsigned long lastMillis_send;
int dsmCount = 0;
int count = 0; // Current sampling count
float mq2_min = 1024, mq3_min = 1024, mq4_min = 1024, mq5_min = 1024, mq6_min = 1024, mq7_min = 1024, mq8_min = 1024, mq9_min = 1024, mq135_min = 1024, humidity_min = 1024, temperature_min = 1024, lux_min = 1024, uvb_min = 1024, pressure_min = 1024, dust_min = 1024, dust1_min = 1024, dust25_min = 1024;	// Minimum stored value for current sample period. NOTE  : use non zero initial value, high enough
float mq2_max = 0, mq3_max = 0, mq4_max = 0, mq5_max = 0, mq6_max = 0, mq7_max = 0, mq8_max = 0, mq9_max = 0, mq135_max = 0, humidity_max = 0, temperature_max = 0, lux_max = 0, uvb_max = 0, pressure_max = 0, dust_max = 0, dust1_max = 0, dust25_max = 0;	// Average value once divided by count, since it gets added to sample
float mq2_avg = 0, mq3_avg = 0, mq4_avg = 0, mq5_avg = 0, mq6_avg = 0, mq7_avg = 0, mq8_avg = 0, mq9_avg = 0, mq135_avg = 0, humidity_avg = 0, temperature_avg = 0, lux_avg = 0, uvb_avg = 0, pressure_avg = 0, dust_avg = 0, dust1_avg = 0, dust25_avg = 0;	// Minimum stored value for current sample period
char err[ERR_HEAP_SIZE] = ""; // initialze with \0 for sanity
// store last n errors until every wifi request (per 5 mins default), initialize with null terminator
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

SFE_CC3000 wifi = SFE_CC3000(CC3000_INT, CC3000_EN, CC3000_CS); // Object reference to the wifi module
SFE_CC3000_Client client = SFE_CC3000_Client(wifi); // Object reference to http client
//#define      WLAN_SEC_UNSEC (0)
//#define      WLAN_SEC_WEP  (1)
//#define      WLAN_SEC_WPA (2)
//#define      WLAN_SEC_WPA2  (3)

DSM501 dsm501(DSM501A_DO_PM1, DSM501A_DO_PM25);
SI7021 si7021; // Object Referece to si7021 sensor
Adafruit_BMP085 bmp; // Object Reference to bmp180 sensor
TSL2561 tsl(TSL2561_ADDR_FLOAT); // Object reference to TSL2561 sensor

void sampleData()
{
	count++;
	float uvb = readUVB();
	if (uvb > uvb_max)
	{
		uvb_max = uvb;
	}
	if (uvb < uvb_min)
	{
		uvb_min = uvb;
	}
	uvb_avg += uvb;
	sampleIICSensors();
	sampleGasSensors();
	sampleDustSensors();
}
void sendSampledData()
{
	sendDataWifi(err, mq2_min, mq2_max, (mq2_avg / count), mq3_min, mq3_max, (mq3_avg / count), mq4_min, mq4_max,
		(mq4_avg / count), mq5_min, mq5_max, (mq5_avg / count), mq6_min, mq6_max, (mq6_avg / count),
		mq7_min, mq7_max, (mq7_avg / count), mq8_min, mq8_max, (mq8_avg / count),
		mq9_min, mq9_max, (mq9_avg / count), mq135_min, mq135_max, (mq135_avg / count),
		temperature_min, temperature_max, (temperature_avg / count),
		humidity_min, humidity_max, (humidity_avg / count), lux_min, lux_max, (lux_avg / count),
		uvb_min, uvb_max, (uvb_avg / count), pressure_min, pressure_max, (pressure_avg / count),
		dust_min, dust_max, (dust_avg / count), dust25_min, dust25_max, (dust25_avg / dsmCount),
		dust1_min, dust1_max, (dust1_avg / dsmCount));
	delay(75);
	//reset all variables 
	count = 0;
	mq2_min = 1024; mq3_min = 1024; mq4_min = 1024; mq5_min = 1024; mq6_min = 1024; mq7_min = 1024; mq8_min = 1024; mq9_min = 1024; mq135_min = 1024; humidity_min = 1024; temperature_min = 1024; lux_min = 1024; uvb_min = 1024; pressure_min = 1024; dust_min = 1024; dust1_min = 1024; dust25_min = 1024;
	mq2_max = 0; mq3_max = 0; mq4_max = 0; mq5_max = 0; mq6_max = 0; mq7_max = 0; mq8_max = 0; mq9_max = 0; mq135_max = 0; humidity_max = 0; temperature_max = 0; lux_max = 0; uvb_max = 0; pressure_max = 0; dust_max = 0; dust1_max = 0; dust25_max = 0;
	mq2_avg = 0; mq3_avg = 0; mq4_avg = 0; mq5_avg = 0; mq6_avg = 0; mq7_avg = 0; mq8_avg = 0; mq9_avg = 0; mq135_avg = 0; humidity_avg = 0; temperature_avg = 0; lux_avg = 0; uvb_avg = 0; pressure_avg = 0; dust_avg = 0; dust1_avg = 0; dust25_avg = 0;


}
void sampleDustSensors()
{
	float dust = readDust();
	if (dust == 0 )
	{
		dust = readDust(); 
	}
	if (dust > dust_max || dust_max == 0)
	{
		dust_max = dust;
	}
	if (dust < dust_min || dust_min == 1024)
	{
		dust_min = dust;
	}
	if (count == 1||count % 5 == 0) // sample dsm every 5 normal samples, since minimum time is 30s, also do it on first sample
	{
		dsm501.update();
		float dust1 = dsm501.getParticleWeight(0), dust25 = dsm501.getParticleWeight(1);
		if (dust1 != 0 && dust25 != 0)
		{
			dsmCount++;
			if (dust1 > dust1_max || dust1_max == 0)
			{
				dust1_max = dust1;
			}
			if (dust1 < dust1_min || dust1_min == 1024)
			{
				dust1_min = dust1;
			}
			if (dust25 > dust25_max || dust25_max == 0)
			{
				dust25_max = dust25;
			}
			if (dust25 < dust25_min || dust25_min == 1024)
			{
				dust25_min = dust25;
			}
			dust1_avg += dust1;
			dust25_avg += dust25;
		}
	}
	dust_avg += dust;

}
//Samples I2C sensors
//To be called from another function only
void sampleIICSensors()
{
	float temperature = readTemperature(), pressure = readPressure(), humidity = readHumidity(), lux = readLux();
	if (temperature > temperature_max)
	{
		temperature_max = temperature;
	}
	if (temperature < temperature_min || temperature_min == 1024)
	{
		temperature_min = temperature;
	}
	if (humidity > humidity_max)
	{
		humidity_max = humidity;
	}
	if (humidity < humidity_min)
	{
		humidity_min = humidity;
	}
	if (pressure > pressure_max)
	{
		pressure_max = pressure;
	}
	if (pressure < pressure_min || pressure_min == 1024)
	{
		pressure_min = pressure;
	}
	if (lux > lux_max)
	{
		lux_max = lux;
	}
	if (lux < lux_min)
	{
		lux_min = lux;
	}
	temperature_avg += temperature; pressure_avg += pressure;
	humidity_avg += humidity; lux_avg += lux;
}
//Samples the analog data for the gas sensors.
//To be called from another function only
void sampleGasSensors()
{
	//They are powered by a Switch mode power supply, therefore need avergaign to reduce noise spikes
	float mq2 = averageAnalogRead(MQ2_AO), mq3 = averageAnalogRead(MQ3_AO), mq4 = averageAnalogRead(MQ4_AO), mq5 = averageAnalogRead(MQ6_AO), mq6 = averageAnalogRead(MQ6_AO),
		mq7 = averageAnalogRead(MQ7_AO), mq8 = averageAnalogRead(MQ8_AO), mq9 = averageAnalogRead(MQ9_AO), mq135 = averageAnalogRead(MQ135_AO);

	if (mq2 > mq2_max || mq2_max == 0)
	{
		mq2_max = mq2;
	}
	if (mq2 < mq2_min || mq2_min == 1024)
	{
		mq2_min = mq2;
	}
	if (mq3 > mq3_max || mq3_max == 0)
	{
		mq3_max = mq3;
	}
	if (mq3 < mq3_min || mq3_min == 1024)
	{
		mq3_min = mq3;
	}
	if (mq4 > mq4_max || mq4_max == 0)
	{
		mq4_max = mq4;
	}
	if (mq4 < mq4_min || mq4_min == 1024)
	{
		mq4_min = mq4;
	}
	if (mq5 > mq5_max || mq5_max == 0)
	{
		mq5_max = mq5;
	}
	if (mq5 < mq5_min || mq5_min == 1024)
	{
		mq5_min = mq5;
	}
	if (mq6 > mq6_max || mq6_max == 0)
	{
		mq6_max = mq6;
	}
	if (mq6 < mq6_min || mq6_min == 1024)
	{
		mq6_min = mq6;
	}
	if (mq7 > mq7_max)
	{
		mq7_max = mq7;
	}
	if (mq7 < mq7_min || mq7_min == 1024)
	{
		mq7_min = mq7;
	}
	if (mq8 > mq8_max || mq8_max == 0)
	{
		mq8_max = mq8;
	}
	if (mq8 < mq8_min || mq8_min == 1024)
	{
		mq8_min = mq8;
	}
	if (mq9 > mq9_max || mq9_max == 0)
	{
		mq9_max = mq9;
	}
	if (mq9 < mq9_min || mq9_min == 1024)
	{
		mq9_min = mq9;
	}
	if (mq135 > mq135_max || mq135_max == 0)
	{
		mq135_max = mq135;
	}
	if (mq135 < mq135_min || mq135_min == 1024)
	{
		mq135_min = mq135;
	}
	mq2_avg += mq2; mq3_avg += mq3;
	mq4_avg += mq4; mq5_avg += mq5;
	mq6_avg += mq6; mq7_avg += mq7;
	mq8_avg += mq8; mq9_avg += mq9;
	mq135_avg += mq135;

}

//Queries SI7021 and returns the temperature in �C (degrees Celsius)
float readTemperature()
{
	return si7021.readTemp();
}
//Queries SI7021 and returns the relative humidity in % (percentage)
float readHumidity()
{
	return si7021.readHumidity();
}
//Queries BMP180 and returns the pressure in Pa
float readPressure()
{
	return bmp.readPressure();
}
//Queries TSL2561 and returns the calculated lux
float readLux()
{
	return tsl.calculateLux(tsl.getLuminosity(TSL2561_FULLSPECTRUM), tsl.getLuminosity(TSL2561_INFRARED));
}
//Reads uvb from ML8511 sensor
//returns the value in mW/cm^2
float readUVB()
{
	float uvLevel = averageAnalogRead(ML8511_AO);
	float refLevel = 675.18 ;//averageAnalogRead(SENSE_3V3);
	float outputVoltage = 3.3 / refLevel * uvLevel;
	float uvIntensity = mapfloat(outputVoltage, 0.99, 2.8, 0.0, 15.0);
	return uvIntensity;
}
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
//Reads dust value from Sharp Dust Sensor in//  TODO : unit
float readDust()
{
	return SharpDust.measure();
}
float readDust1()
{
	dsm501.update();
	return dsm501.getParticleWeight(0);
}
float readDust25()
{
	dsm501.update();
	return dsm501.getParticleWeight(1);
}
float averageAnalogRead(int pinToRead)
{
	byte numberOfReadings = 8;
	float runningValue = 0;
	for (int x = 0; x < numberOfReadings; x++)
		runningValue += analogRead(pinToRead);
	runningValue /= numberOfReadings;
	return(runningValue);
}
//Initializes CC3000 and returns false if it fails to initialize or connect to the stored wifi credentials
boolean initWifi()
{
	if (!wifi.init())
	{
		strcat(err, "W");
		return false;
	}
	if (!connectWifi())
	{
		return false;
	}
	return true;
}
//Connects to wifi using credentials stored in EEPROM, returns true if successful
boolean connectWifi()
{
	if (wifiConnected == 1 && !wifi.disconnect())
	{
		strcat(err, "C");
		return false;
	}
	else
	{
		char *ssid = readEEPROMString(START_ADDRESS, 0);
		char *password = readEEPROMString(START_ADDRESS, 1);
		char *sec = readEEPROMString(START_ADDRESS, 2);
		if (!wifi.connect(ssid, (int)(sec[0] - '0'), password, TIMEOUT))
		{
			strcat(err, "C");
			wifiConnected = false;
			return false;
		}
		wifiConnected = true;
		return true;
	}
}
//Initializes the I2C devices, returns false if one of them is not initialized (if detectable)
boolean initI2C()
{
	si7021.begin();
	si7021.setHumidityRes(12);
	tsl.begin();
	tsl.setGain(TSL2561_GAIN_16X);
	tsl.setTiming(TSL2561_INTEGRATIONTIME_101MS);
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
//Writes wifi credentials to EEPROM safely,  returns false if operation failed. Look in credits for source
boolean writeWifiEEPROM(char _ssid[], char _password[], char _security[])
{
	nextEEPROMaddress = 0;
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
//Adds a string to EEPROM, returns false if operation failed. Look in credits for source
boolean addToEEPROM(char *text)
{
	if (nextEEPROMaddress + strlen(text) + 1 > EEPROM.length())
	{
		strcat(err, "A");
		return false;
	}
	do
	{
		EEPROM.update(nextEEPROMaddress++, (byte)*text);
	} while (*text++ != '\0');
	return true;
}
//Returns the string read from EEPROM. Look in credits for source
char *readEEPROMString(int baseAddress, int stringNumber)
{
	int start, length, i, nextAddress;
	char ch;
	char *result;
	nextAddress = START_ADDRESS;
	for (i = 0; i < stringNumber; ++i)
	{
		ch = (char)EEPROM.read(nextAddress++);
		if (ch == (char)EEPROM_END_MARK || nextAddress >= EEPROM.length())
		{
			return (char *)0;
		}
		while (ch != '\0' && nextAddress < EEPROM.length())
		{
			ch = EEPROM.read(nextAddress++);
		}
	}
	start = nextAddress;
	ch = (char)EEPROM.read(nextAddress++);
	if (ch == (char)EEPROM_END_MARK)
	{
		return (char *)0;
	}
	length = 0;
	while (ch != '\0' && nextAddress < EEPROM.length())
	{
		++length;
		ch = EEPROM.read(nextAddress++);
	}
	result = new char[length + 1];
	nextAddress = start;
	for (i = 0; i < length; ++i)
	{
		result[i] = (char)EEPROM.read(nextAddress++);
	}
	result[i] = '\0';
	return result;
}
//Wrapper overload method for preserving sanity
//Sends current sensor(not avg!) data and extremes of stored sampled data from last 5 minutes. 
// Checks if currently any data is being sent to avoid overflows
// Make sure to call the function using a mills timer
boolean sendDataUSB()
{
	if (USBData)
	{
		sendDataUSB(err, mq2_min, mq2_max, analogRead(MQ2_AO), mq3_min, mq3_max, analogRead(MQ3_AO), mq4_min, mq4_max, analogRead(MQ4_AO), mq5_min, mq5_max, analogRead(MQ5_AO), mq6_min, mq6_max, analogRead(MQ6_AO), mq7_min, mq7_max, analogRead(MQ7_AO), mq8_min, mq8_max, analogRead(MQ8_AO), mq9_min, mq9_max, analogRead(MQ9_AO), mq135_min, mq135_avg, analogRead(MQ135_AO), temperature_min, temperature_max, readTemperature(), humidity_min, humidity_max, readHumidity(), lux_min, lux_max, readLux(), uvb_min, uvb_max, readUVB(), pressure_min, pressure_max, readPressure(), dust_min, dust_max, readDust(), dust25_min, dust25_max, readDust25(), dust1_min, dust1_max, readDust1());
	}
}
//Wrapper overload method for preserving sanity
//Sends current sensor(not avg!) data and extremes of stored sampled data from last 5 minutes. 
// Checks if currently any data is being sent to avoid overflows
// Make sure to call the function using a mills timer
boolean sendDataBT()
{
	if (BTData)
	{
		sendDataBT(err, mq2_min, mq2_max, analogRead(MQ2_AO), mq3_min, mq3_max, analogRead(MQ3_AO), mq4_min, mq4_max, analogRead(MQ4_AO), mq5_min, mq5_max, analogRead(MQ5_AO), mq6_min, mq6_max, analogRead(MQ6_AO), mq7_min, mq7_max, analogRead(MQ7_AO), mq8_min, mq8_max, analogRead(MQ8_AO), mq9_min, mq9_max, analogRead(MQ9_AO), mq135_min, mq135_avg, analogRead(MQ135_AO), temperature_min, temperature_max, readTemperature(), humidity_min, humidity_max, readHumidity(), lux_min, lux_max, readLux(), uvb_min, uvb_max, readUVB(), pressure_min, pressure_max, readPressure(), dust_min, dust_max, readDust(), dust25_min, dust25_max, readDust25(), dust1_min, dust1_max, readDust1());
	}
}
//Constructs a json string sends that string over Serial interface, which is connected to TTL-USB converter by default.
//TODO : This will not work for MQ7
//Returns false if operation fails
boolean sendDataUSB(char _err[], float _mq2_min, float _mq2_max, float _mq2_avg, float _mq3_min, float _mq3_max, float _mq3_avg, float _mq4_min, float _mq4_max, float _mq4_avg, float _mq5_min, float _mq5_max,
	float _mq5_avg, float _mq6_min, float _mq6_max, float _mq6_avg, float _mq7_min, float _mq7_max, float _mq7_avg, float _mq8_min, float _mq8_max, float _mq8_avg, float _mq9_min, float _mq9_max, float _mq9_avg,
	float _mq135_min, float _mq135_max, float _mq135_avg, float _temperature_min, float _temperature_max, float _temperature_avg, float _humidity_min, float _humidity_max, float _humidity_avg, float _lux_min, float _lux_max, float _lux_avg,
	float _uvb_min, float _uvb_max, float _uvb_avg, float _pressure_min, float _pressure_max, float _pressure_avg, float _dust_min, float _dust_max, float _dust_avg, float _dust25_min, float _dust25_max, float _dust25_avg, float _dust1_min, float _dust1_max, float _dust1_avg)
{
	StaticJsonBuffer<1024> jsonBuffer;
	JsonObject& outputObject = jsonBuffer.createObject();
	outputObject["mq2_min"] = _mq2_min; outputObject["mq2_avg"] = _mq2_avg; outputObject["mq2_max"] = _mq2_max; outputObject["mq3_min"] = _mq3_min; outputObject["mq3_avg"] = _mq3_avg; outputObject["mq3_max"] = _mq3_max;
	outputObject["mq4_min"] = _mq4_min; outputObject["mq4_avg"] = _mq4_avg; outputObject["mq4_max"] = _mq4_max; outputObject["mq5_min"] = _mq5_min; outputObject["mq5_avg"] = _mq5_avg; outputObject["mq5_max"] = _mq5_max;
	outputObject["mq6_min"] = _mq6_min; outputObject["mq6_avg"] = _mq6_avg; outputObject["mq6_max"] = _mq6_max; outputObject["mq7_min"] = _mq7_min; outputObject["mq7_avg"] = _mq7_avg; outputObject["mq7_max"] = _mq7_max;
	outputObject["mq8_min"] = _mq8_min; outputObject["mq8_avg"] = _mq8_avg; outputObject["mq8_max"] = _mq8_max; outputObject["mq9_min"] = _mq9_min; outputObject["mq9_avg"] = _mq9_avg; outputObject["mq9_max"] = _mq9_max;
	outputObject["mq135_min"] = _mq135_min; outputObject["mq135_avg"] = _mq135_avg; outputObject["mq135_max"] = _mq135_max; outputObject["deviceId"] = device_id;
	outputObject["temperature_min"] = _temperature_min; outputObject["temperature_avg"] = _temperature_avg; outputObject["temperature_max"] = _temperature_max; outputObject["humidity_min"] = _humidity_min; outputObject["humidity_avg"] = _humidity_avg; outputObject["humidity_max"] = _humidity_max;
	outputObject["lux_min"] = _lux_min; outputObject["lux_avg"] = _lux_avg; outputObject["lux_max"] = _lux_max; outputObject["uvb_min"] = _uvb_min; outputObject["uvb_avg"] = _uvb_avg; outputObject["uvb_max"] = _uvb_max;
	outputObject["err"] = _err; outputObject["pressure_min"] = _pressure_min; outputObject["pressure_max"] = _pressure_max; outputObject["pressure_avg"] = _pressure_avg; outputObject["dust_min"] = _dust_min; outputObject["dust_max"] = _dust_max; outputObject["dust_avg"] = _dust_avg;
	outputObject["dust1_min"] = _dust1_min; outputObject["dust1_max"] = _dust1_max; outputObject["dust1_avg"] = _dust1_avg; outputObject["dust25_min"] = _dust25_min; outputObject["dust25_max"] = _dust25_max; outputObject["dust25_avg"] = _dust25_avg;
	if (!outputObject.printTo(Serial))
	{
		strcat(err, "U");
		return false;
	}
	else
	{
		Serial.write((byte)'\r');
		Serial.write((byte)'\n');
		return true;
	}
}
//Constructs a json string sends that string over Serial1 interface, only available on Mega with a TTL bluetooth device connected
//Returns false if operation fails
boolean sendDataBT(char _err[], float _mq2_min, float _mq2_max, float _mq2_avg, float _mq3_min, float _mq3_max, float _mq3_avg, float _mq4_min, float _mq4_max, float _mq4_avg, float _mq5_min, float _mq5_max,
	float _mq5_avg, float _mq6_min, float _mq6_max, float _mq6_avg, float _mq7_min, float _mq7_max, float _mq7_avg, float _mq8_min, float _mq8_max, float _mq8_avg, float _mq9_min, float _mq9_max, float _mq9_avg,
	float _mq135_min, float _mq135_max, float _mq135_avg, float _temperature_min, float _temperature_max, float _temperature_avg, float _humidity_min, float _humidity_max, float _humidity_avg, float _lux_min, float _lux_max, float _lux_avg,
	float _uvb_min, float _uvb_max, float _uvb_avg, float _pressure_min, float _pressure_max, float _pressure_avg, float _dust_min, float _dust_max, float _dust_avg, float _dust25_min, float _dust25_max, float _dust25_avg, float _dust1_min, float _dust1_max, float _dust1_avg)
{
	StaticJsonBuffer<1024> jsonBuffer;
	JsonObject& outputObject = jsonBuffer.createObject();
	outputObject["mq2_min"] = _mq2_min; outputObject["mq2_avg"] = _mq2_avg; outputObject["mq2_max"] = _mq2_max; outputObject["mq3_min"] = _mq3_min; outputObject["mq3_avg"] = _mq3_avg; outputObject["mq3_max"] = _mq3_max;
	outputObject["mq4_min"] = _mq4_min; outputObject["mq4_avg"] = _mq4_avg; outputObject["mq4_max"] = _mq4_max; outputObject["mq5_min"] = _mq5_min; outputObject["mq5_avg"] = _mq5_avg; outputObject["mq5_max"] = _mq5_max;
	outputObject["mq6_min"] = _mq6_min; outputObject["mq6_avg"] = _mq6_avg; outputObject["mq6_max"] = _mq6_max; outputObject["mq7_min"] = _mq7_min; outputObject["mq7_avg"] = _mq7_avg; outputObject["mq7_max"] = _mq7_max;
	outputObject["mq8_min"] = _mq8_min; outputObject["mq8_avg"] = _mq8_avg; outputObject["mq8_max"] = _mq8_max; outputObject["mq9_min"] = _mq9_min; outputObject["mq9_avg"] = _mq9_avg; outputObject["mq9_max"] = _mq9_max;
	outputObject["mq135_min"] = _mq135_min; outputObject["mq135_avg"] = _mq135_avg; outputObject["mq135_max"] = _mq135_max; outputObject["deviceId"] = device_id;
	outputObject["temperature_min"] = _temperature_min; outputObject["temperature_avg"] = _temperature_avg; outputObject["temperature_max"] = _temperature_max; outputObject["humidity_min"] = _humidity_min; outputObject["humidity_avg"] = _humidity_avg; outputObject["humidity_max"] = _humidity_max;
	outputObject["lux_min"] = _lux_min; outputObject["lux_avg"] = _lux_avg; outputObject["lux_max"] = _lux_max; outputObject["uvb_min"] = _uvb_min; outputObject["uvb_avg"] = _uvb_avg; outputObject["uvb_max"] = _uvb_max;
	outputObject["err"] = _err; outputObject["pressure_min"] = _pressure_min; outputObject["pressure_max"] = _pressure_max; outputObject["pressure_avg"] = _pressure_avg; outputObject["dust_min"] = _dust_min; outputObject["dust_max"] = _dust_max; outputObject["dust_avg"] = _dust_avg;
	outputObject["dust1_min"] = _dust1_min; outputObject["dust1_max"] = _dust1_max; outputObject["dust1_avg"] = _dust1_avg; outputObject["dust25_min"] = _dust25_min; outputObject["dust25_max"] = _dust25_max; outputObject["dust25_avg"] = _dust25_avg;
	if (!outputObject.printTo(Serial1))
	{
		strcat(err, "B");
		return false;
	}
	else
	{
		Serial1.write((byte)'\r');
		Serial1.write((byte)'\n');
		return true;
	}
}
//Constructs a json string sends that string over Wifi using CC3000
//Returns false if operation fails
boolean sendDataWifi(char _err[], float _mq2_min, float _mq2_max, float _mq2_avg, float _mq3_min, float _mq3_max, float _mq3_avg, float _mq4_min, float _mq4_max, float _mq4_avg, float _mq5_min, float _mq5_max,
	float _mq5_avg, float _mq6_min, float _mq6_max, float _mq6_avg, float _mq7_min, float _mq7_max, float _mq7_avg, float _mq8_min, float _mq8_max, float _mq8_avg, float _mq9_min, float _mq9_max, float _mq9_avg,
	float _mq135_min, float _mq135_max, float _mq135_avg, float _temperature_min, float _temperature_max, float _temperature_avg, float _humidity_min, float _humidity_max, float _humidity_avg, float _lux_min, float _lux_max, float _lux_avg,
	float _uvb_min, float _uvb_max, float _uvb_avg, float _pressure_min, float _pressure_max, float _pressure_avg, float _dust_min, float _dust_max, float _dust_avg, float _dust25_min, float _dust25_max, float _dust25_avg, float _dust1_min, float _dust1_max, float _dust1_avg)
{
	StaticJsonBuffer<1024> jsonBuffer;
	JsonObject& outputObject = jsonBuffer.createObject();
	outputObject["mq2_min"] = _mq2_min; outputObject["mq2_avg"] = _mq2_avg; outputObject["mq2_max"] = _mq2_max; outputObject["mq3_min"] = _mq3_min; outputObject["mq3_avg"] = _mq3_avg; outputObject["mq3_max"] = _mq3_max;
	outputObject["mq4_min"] = _mq4_min; outputObject["mq4_avg"] = _mq4_avg; outputObject["mq4_max"] = _mq4_max; outputObject["mq5_min"] = _mq5_min; outputObject["mq5_avg"] = _mq5_avg; outputObject["mq5_max"] = _mq5_max;
	outputObject["mq6_min"] = _mq6_min; outputObject["mq6_avg"] = _mq6_avg; outputObject["mq6_max"] = _mq6_max; outputObject["mq7_min"] = _mq7_min; outputObject["mq7_avg"] = _mq7_avg; outputObject["mq7_max"] = _mq7_max;
	outputObject["mq8_min"] = _mq8_min; outputObject["mq8_avg"] = _mq8_avg; outputObject["mq8_max"] = _mq8_max; outputObject["mq9_min"] = _mq9_min; outputObject["mq9_avg"] = _mq9_avg; outputObject["mq9_max"] = _mq9_max;
	outputObject["mq135_min"] = _mq135_min; outputObject["mq135_avg"] = _mq135_avg; outputObject["mq135_max"] = _mq135_max; outputObject["deviceId"] = device_id;
	outputObject["temperature_min"] = _temperature_min; outputObject["temperature_avg"] = _temperature_avg; outputObject["temperature_max"] = _temperature_max; outputObject["humidity_min"] = _humidity_min; outputObject["humidity_avg"] = _humidity_avg; outputObject["humidity_max"] = _humidity_max;
	outputObject["lux_min"] = _lux_min; outputObject["lux_avg"] = _lux_avg; outputObject["lux_max"] = _lux_max; outputObject["uvb_min"] = _uvb_min; outputObject["uvb_avg"] = _uvb_avg; outputObject["uvb_max"] = _uvb_max;
	outputObject["err"] = _err; outputObject["pressure_min"] = _pressure_min; outputObject["pressure_max"] = _pressure_max; outputObject["pressure_avg"] = _pressure_avg; outputObject["dust_min"] = _dust_min; outputObject["dust_max"] = _dust_max; outputObject["dust_avg"] = _dust_avg;
	outputObject["dust1_min"] = _dust1_min; outputObject["dust1_max"] = _dust1_max; outputObject["dust1_avg"] = _dust1_avg; outputObject["dust25_min"] = _dust25_min; outputObject["dust25_max"] = _dust25_max; outputObject["dust25_avg"] = _dust25_avg;
	if (client.connect(server, 80))
	{
		sprintf(buffer, "POST /tables/%s HTTP/1.1", table_name);
		client.println(buffer);
		sprintf(buffer, "Host: %s", server);
		client.println(buffer);
		client.println("Connection: close"); // No keep alive
		client.println("ZUMO-API-VERSION: 2.0.0"); // Azure Mobile Services header,not required if your app is configuired not to check version
		client.println("Content-Type: application/json"); // JSON content type
		outputObject.printTo(buffer, sizeof(buffer));    // POST body
		client.print("Content-Length: ");  // Content length
		client.println(outputObject.measureLength());
		client.println();
		client.println(buffer);
		err[0] = '\0';
		return true;
	}
	else
	{
		strcat(err, "J");
		return false;
	}
}
//Initializes all other electronices
void setup()
{
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
	nextEEPROMaddress = START_ADDRESS;
	Serial.begin(115200);
	Serial1.begin(115200); // Apparently COM ports on windows only support upto 128000 baud ? and 115200 is the nearest to it which is supported by every module
	//Serial.println("Start setup");
	dsm501.begin(MIN_WIN_SPAN);
	//Serial.println("Initialize samyoung Dust");
	SharpDust.begin(SHARP_LED, SHARP_AO);
	//Serial.println("Initialize Sharp Dust");
	lastMillis_data = millis();
	lastMillis_sample = millis();
	lastMillis_send = millis();
	initWifi();
	//Serial.println("Initialize CC3000");
	initI2C(); // initialize I2C sensors
	//Serial.println("Initialize IIC");
	digitalWrite(LED_BUILTIN, LOW);
	delay(300);
	digitalWrite(LED_BUILTIN, HIGH);
	delay(300);
	digitalWrite(LED_BUILTIN, LOW);
	//Serial.println("Initialize IIC");
 // sendDataWifi("No Error" ,random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000),random(1,1000));

}
//Loops forever. Checks for any incoming serial data and the time to stop sampling and send the data using other functions.
int freeRam() {
	extern int __heap_start, *__brkval;
	int v;
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}
void loop()
{
	digitalWrite(LED_BUILTIN, HIGH);
	if (millis() - lastMillis_data > 300000 && count != 0)
	{
		sendSampledData();
		lastMillis_data = millis();
	}
	else if (millis() - lastMillis_sample > 500)
	{
		sampleData();
		lastMillis_sample = millis();
	}
	if (Serial.available() > 1)
	{
		SerialEvent();
	}
	if (Serial1.available() > 1)
	{
		SerialEvent1();
	}
	//Serial.println("Finish Serial");
	if (!Serial)
	{
		USBData = false;
	}
	if (!Serial1)
	{
		BTData = false;
	}
	if (millis() - lastMillis_send > 500 && mq2_min != 1024)
	{
		sendDataUSB();
		sendDataBT();
		lastMillis_send = lastMillis_send;
	}
	digitalWrite(LED_BUILTIN, LOW);
	//Serial.println("Exit loop");
}

//By default triggered when any data is received on Serial1(Bluetooth here) interface.
void SerialEvent1()
{
	String serialString = Serial1.readStringUntil('\n');
	StaticJsonBuffer<256> jsonBuffer;
	JsonObject& serialInput = jsonBuffer.parseObject(serialString);
	int cmd = serialInput["cmd"];
	const char* sec = serialInput["sec"];
	const char* ssid = serialInput["ssid"];
	const char* password = serialInput["password"];
	int _device_id = serialInput["device_id"];
	int data = serialInput["data"];
	if (cmd == 1)
	{
		if (_device_id != 0)
		{
			device_id = _device_id;
		}
		if (ssid != NULL && sec != NULL && password != NULL)
		{
			writeWifiEEPROM((char*)ssid, (char*)password, (char*)sec);
			connectWifi();
		}
	}
	else if (cmd == 0)
	{
		if (data == 1)
		{
			BTData = true;
		}
		else if (data == 0)
		{
			BTData = false;
		}
	}
}\
//By default triggered when any data is received on Serial(TTL-USB) interface/
void SerialEvent()
{
	String serialString = Serial.readStringUntil('\n');
	StaticJsonBuffer<256> jsonBuffer;
	JsonObject& serialInput = jsonBuffer.parseObject(serialString);
	int cmd = serialInput["cmd"];
	const char* sec = serialInput["sec"];
	const char* ssid = serialInput["ssid"];
	const char* password = serialInput["password"];
	int _device_id = serialInput["device_id"];
	int data = serialInput["data"];
	if (cmd == 1)
	{
		if (_device_id != 0)
		{
			device_id = _device_id;
		}
		if (ssid != NULL && sec != NULL && password != NULL)
		{
			writeWifiEEPROM((char*)ssid, (char*)password, (char*)sec);
			connectWifi();
		}
	}
	else if (cmd == 0)
	{
		if (data == 1)
		{
			USBData = true;
		}
		else if (data == 0)
		{
			USBData = false;
		}
	}
}


