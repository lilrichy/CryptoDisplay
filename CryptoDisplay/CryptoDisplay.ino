#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <gfxfont.h>
#include <Adafruit_SPITFT_Macros.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_GFX.h>
#include "ESP8266WiFi.h"
#include "WiFiClientSecure.h"
#include "ArduinoJson.h"
#include <stdio.h>

// Reset pin not used but needed for library
#define OLED_RESET -1
Adafruit_SSD1306 display(OLED_RESET);

// We always wait a bit between updates of the display
#define  DELAYTIME  5000  // in milliseconds

// Total Number of coins must match amount of coins in configureCoins below.
#define TOTAL_COINS 4

struct Coin {
	String url;
	String ticker;
};

struct Coin cryptoCoins[TOTAL_COINS];
//Change this to add, edit, or remove coins to the display remember to update TOTAL_COINS above.
void configureCoins() {

	//To add coin change Coin Name, Coin, Currency, Coin Ticker Symbol values inside of <>:

	//Coin <Coin Name> = { "/products/<Coin>-<Currency>/ticker", "<Coin Ticker Symbol>" };
	
	Coin BTC = { "/products/BTC-USD/ticker", "BTC" };
	Coin XTZ = { "/products/XTZ-USD/ticker", "XTZ" };
	Coin XRP = { "/products/XRP-USD/ticker", "XRP" };
	Coin BAT = { "/products/BAT-USDC/ticker", "BAT" };

	//Add coin to list here and uptate total number of coins above.
	cryptoCoins[0] = BTC;
	cryptoCoins[1] = XTZ;
	cryptoCoins[2] = XRP;
	cryptoCoins[3] = BAT;

}

//Update Wifi Info
const char* ssid = "LiLRichyNetz";
const char* password = "Monster1984";

const char* coinbaseHost = "api.pro.coinbase.com";
const char* coinbaseFingerprint = "9C B0 72 05 A4 F9 D7 4E 5A A4 06 5E DD 1F 1C 27 5D C2 F1 48";


void connectToWIFI() {

	display.clearDisplay();
	display.setTextSize(2);
	display.setCursor(0, 0);
	display.print("Connecting");
	display.setCursor(0, 20);
	display.print("   WiFi");
	display.display();

	Serial.println();
	Serial.print("connecting to ");
	Serial.println(ssid);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());

	display.clearDisplay();
	display.setCursor(0, 0);
	display.print("Connected");
	display.setCursor(0, 20);
	display.print(ssid);
	display.display();
	delay(1000);
}

JsonObject& getJsonObject(String url) {
	const size_t capacity = JSON_OBJECT_SIZE(2) + 74;
	DynamicJsonBuffer jsonBuffer(capacity);

	// Use WiFiClientSecure class to create TLS connection
	WiFiClientSecure client;
	client.setTimeout(10000);
	client.setFingerprint(coinbaseFingerprint);
	client.setInsecure();

	if (!client.connect(coinbaseHost, 443)) {
		Serial.println("connection failed");

		display.clearDisplay();
		display.setTextSize(2);
		display.setCursor(0, 20);
		display.print("Connection");
		display.setCursor(0, 40);
		display.print("Failed!");
		display.display();

		// No further work should be done if the connection failed
		return jsonBuffer.parseObject(client);
	}
	Serial.println(F("Connected!"));

	// Send HTTP Request
	String httpEnding = /*" HTTP/1.1\r\n"*/ " HTTP/1.0\r\n";
	client.print(String("GET ") + url + httpEnding +
		"Host: " + coinbaseHost + "\r\n" +
		"User-Agent: BuildFailureDetectorESP8266\r\n" +
		"Connection: close\r\n\r\n");
	Serial.println("request sent");

	// Check HTTP Status
	char status[32] = { 0 };
	client.readBytesUntil('\r', status, sizeof(status));
	if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
		Serial.print(F("Unexpected response: "));
		Serial.println(status);

		display.clearDisplay();
		display.setTextSize(2);
		display.setCursor(0, 20);
		display.print("Response:");
		display.setCursor(0, 40);
		display.print(status);
		display.display();

		return jsonBuffer.parseObject(client);
	}

	// Skip HTTP headers
	char endOfHeaders[] = "\r\n\r\n";
	if (!client.find(endOfHeaders)) {
		Serial.println(F("Invalid response"));

		display.clearDisplay();
		display.setTextSize(2);
		display.setCursor(0, 20);
		display.print("Invalid");
		display.setCursor(0, 40);
		display.print("response!");
		display.display();

	}

	// Parse JSON object
	JsonObject& root = jsonBuffer.parseObject(client);
	if (!root.success()) {
		Serial.println(F("Parsing failed!"));

		display.clearDisplay();
		display.setTextSize(2);
		display.setCursor(0, 20);
		display.print("Parsing");
		display.setCursor(0, 40);
		display.print("Failed!");
		display.display();

	}

	// Disconnect
	client.stop();
	jsonBuffer.clear();
	return root;
}

void getCoinPrice(String url, String cryptoName) {
	JsonObject& root = getJsonObject(url);
	Serial.println("==========");
	Serial.println(F("Response:"));
	Serial.print("Symbol: ");
	Serial.println(root["symbol"].as<char*>());
	Serial.print("Price: ");
	Serial.println(root["price"].as<char*>());

	float cryptoPrice = root["price"].as<float>();
	Serial.println(cryptoPrice);
	Serial.println("==========");
	String output = cryptoName + " $" + String(cryptoPrice);
	Serial.println(output);
	output = "$" + String(cryptoPrice);

	char* cstr = new char[output.length() + 1];
	strcpy(cstr, output.c_str());

	display.clearDisplay();
	display.setTextSize(4);
	display.setCursor(0, 0);
	display.print(" " + cryptoName);

	display.setTextSize(2);
	display.setCursor(0, 40);
	display.print(cstr);
	display.display();

	delay(DELAYTIME);

	delete[] cstr;
}

void getAllCoinPrices() {
	Coin currentCoin;
	for (int index = 0; index < TOTAL_COINS; index++) {
		currentCoin = cryptoCoins[index];
		getCoinPrice(currentCoin.url, currentCoin.ticker);
	}
}

void setup() {
	Serial.begin(115200);

	// Start Wire library for I2C
	Wire.begin();

	// initialize OLED with I2C addr 0x3C
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

	// Clear the display
	display.clearDisplay();
	//Set the color - always use white despite actual display color
	display.setTextColor(WHITE);
	//Set the font size
	display.setTextSize(2);
	//Set the cursor coordinates
	display.setCursor(0, 24);
	display.print("  LiLRichy");
	display.display();
	delay(4000);

	connectToWIFI();
	configureCoins();
}

void loop() {

	getAllCoinPrices();

}