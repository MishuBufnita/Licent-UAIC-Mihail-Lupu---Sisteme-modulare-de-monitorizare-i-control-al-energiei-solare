#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AM2320.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_VEML6070.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DigiPotX9Cxxx.h>

// Credentiale conectare Wi-Fi
const char* ssid = "Desktop-Mihail";
const char* password = "Licenta2024";

const char* panel_id = "c1r1";  // identificatorului panoului

// Adresa server
const char* serverName = "http://your_server_ip:8086/data/pv1";

// Pre-Initializarea senzorilor de pe i2c
Adafruit_AM2320 am2320 = Adafruit_AM2320();
Adafruit_TSL2561_Unified tsl2561 = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
Adafruit_VEML6070 veml6070 = Adafruit_VEML6070();

// Initializarea DigiPot
DigiPot pot(14, 13, 12); // pini select, direction, increment

void setup() {
  Serial.begin(115200);

  // Conectare la rețeaua Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectare la WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Conectat la WiFi");

  // Initializarea senzorilor I2C
  Wire.begin();
  if (!am2320.begin()) {
    Serial.println("Nu s-a putut initializa AM2320!");
  }

  if (!tsl2561.begin()) {
    Serial.println("Nu s-a putut initializa TSL2561!");
  }

  veml6070.begin(VEML6070_1_T);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    // Colectarea datelor de tensiune si curent
    float voltaj_in = analogRead(34) * (3.3 / 4095.0); 
    float curent_in = analogRead(35) * (3.3 / 4095.0); 

    // Colectarea datelor de la AM2320
    am2320.read();
    float temperature = am2320.temperature;
    float humidity = am2320.humidity;

    // Colectarea datelor de la TSL2561
    sensors_event_t event;
    tsl2561.getEvent(&event);
    uint16_t lux = event.light;

    // Colectarea datelor de la VEML6070
    uint16_t uv = veml6070.readUV();

    // Crearea JSON cu datele colectate
    String httpRequestData = "{\"panel_id\":\"" + String(panel_id) + 
                            "\", \"voltaj_in\":" + String(voltaj_in) + 
                              ", \"curent_in\":" + String(curent_in) +
                             ", \"temperature\":" + String(temperature) + 
                             ", \"humidity\":" + String(humidity) +
                             ", \"lux\":" + String(lux) + 
                             ", \"uv\":" + String(uv) + "}";

    // Trimitere JSON catre server
    int httpResponseCode = http.POST(httpRequestData);

    // Verificarea răspunsului de la server
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);

      // Procesarea răspunsului JSON
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);
      float voltaj_out = doc["voltaj_out"];
      Serial.print("Receptionat voltaj_out: ");
      Serial.println(voltaj_out);

      // Ajustarea rezistenței DigiPot pentru a obține tensiunea de ieșire 
      float target_voltaj_out = voltaj_out;
      float tolerance = target_voltaj_out * 0.1;  // 10% marjă de eroare
      float current_voltaj_out = voltaj_in;  

      while (abs(current_voltaj_out - target_voltaj_out) > tolerance) {
        if (current_voltaj_out < target_voltaj_out) {
          pot.increase(1);
        } else {
          pot.decrease(1);
        }

        delay(100);  // timp de asteptare pentru stabilizarea tensiunii de ieșire

        // Actualizează tensiunea de ieșire curentă 
        current_voltaj_out = analogRead(34) * (3.3 / 4095.0);  
      }
      
    } else {
      Serial.print("Eroare POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }
  delay(5000); // Trimitere date la interval de 5 secunde
}