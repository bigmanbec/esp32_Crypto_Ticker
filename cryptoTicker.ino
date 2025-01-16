#include <Wire.h> //For easy connection and initialization of display
#include <LiquidCrystal_I2C.h> //Library for interfacing with LCD display
#include <WiFi.h> //Library for interfacing with wifi
#include <HTTPClient.h> //Library for allowing http connection
#include <ArduinoJson.h> //Library for parsing JSON

//Wi-Fi credentials
const char* ssid = "A iPhone";
const char* password = "RaeBeans";

//CoinGecko API URL 
const char* apiUrl = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,ethereum,dogecoin&vs_currencies=usd";

//LCD configuration
LiquidCrystal_I2C lcd(0x27, 16, 2); //Adress for display, with width and height of display

//Array of cryptocurrencies to display
const String cryptos[] = {"Bitcoin", "Ethereum", "Dogecoin"};
const String cryptoIDs[] = {"bitcoin", "ethereum", "dogecoin"};
const int cryptoCount = sizeof(cryptos) / sizeof(cryptos[0]);

//Array to store the previous prices
float previousPrices[cryptoCount] = {0.0, 0.0, 0.0}; 
int currentCryptoIndex = 0;

//Custom characters for the up and down arrows
byte upArrow[8] = {
  0b00100, //   *
  0b01110, //  ***
  0b10101, // * * *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00000
};

byte downArrow[8] = {
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b00100, //   *
  0b10101, // * * *
  0b01110, //  ***
  0b00100, //   *
  0b00000
};

void setup() {
  //Initialize Serial Monitor
  Serial.begin(115200);

  //Initialize I2C pins
  Wire.begin(4, 5); // SDA = GPIO4, SCL = GPIO5

  //Initialize LCD and create custom characters
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, upArrow);   // Save up arrow as custom character 0
  lcd.createChar(1, downArrow); // Save down arrow as custom character 1
  lcd.print("Connecting..."); //Show that it is trying to connect with WiFi before moving to start of connection
  
  //Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(0, 1);
    lcd.print("WiFi...");
  }
  
  Serial.println("\nConnected to WiFi!");
  lcd.clear();
  lcd.print("WiFi Connected");
  delay(2000);
}

void loop() {
  //Check to make sure WiFi is connected, if it is makes API call
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(apiUrl);
    int httpResponseCode = http.GET();
    
    //If the HTTP request was successful
    if (httpResponseCode == 200) { 
      String payload = http.getString();
      Serial.println("API Response:");
      Serial.println(payload);

      //Parse the JSON response
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, payload);

      //If there was an error with JSON print error
      if (error) {
        Serial.print("JSON parsing error: ");
        Serial.println(error.c_str());
        lcd.clear();
        lcd.print("JSON Error");
      } 
      
      //Get the current cryptocurrency and price
      else {
        String cryptoName = cryptos[currentCryptoIndex];
        String cryptoID = cryptoIDs[currentCryptoIndex];
        float price = doc[cryptoID]["usd"];
        
        //Determine price trend, if it is going up or down from previous price
        byte trendChar;
        if (price > previousPrices[currentCryptoIndex]) {
          trendChar = 0; //Up arrow custom character
        } else if (price < previousPrices[currentCryptoIndex]) {
          trendChar = 1; //Down arrow custom character
        } else {
          trendChar = '='; //No change
        }

        //Update the previous price
        previousPrices[currentCryptoIndex] = price;

        //Display the data on the LCD
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(cryptoName);
        lcd.setCursor(0, 1);
        lcd.print("$");
        lcd.print(price, 2); //Show price with 2 decimal places
        lcd.print(" ");

        //Checks if there was a price change from previous price
        if (trendChar == 0 || trendChar == 1) {
          lcd.write(trendChar); //Write the custom arrow
        }

        //Scroll to the next cryptocurrency
        currentCryptoIndex = (currentCryptoIndex + 1) % cryptoCount;
      }

    } 
    
    //Checks if there was an with HTTP request and will display error
    else {
      Serial.print("Error in HTTP request: ");
      Serial.println(httpResponseCode);
      lcd.clear();
      lcd.print("HTTP Error");
    }

    //End HTTP client session
    http.end();
  } 
  
  //Checks if the WiFi was disconnected and displays error if it is
  else {
    Serial.println("WiFi disconnected!");
    lcd.clear();
    lcd.print("WiFi Error!");
  }

  //Show each cryptocurrency for 15 seconds then begin again
  delay(15000); 
}

