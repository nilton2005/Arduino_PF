#include <WiFi.h>
#include <WebSocketsClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <HTTPClient.h>
// Pines del RFID
#define SS_PIN 21
#define RST_PIN 22

// Crear instancias para RFID y LCD
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);   // Dirección I2C del LCD (ajustar si es diferente)

// Conexión WiFi
const char* ssid = "Redmi 8A";
const char* password = "niltontaco";
const char* serverAddress = "192.168.43.73";  // Dirección IP del servidor WebSocket
const int serverPort = 8080;                  // Puerto del servidor WebSocket
const char* serverStore = "https://api-of-administration.onrender.com/api/rfid";
const String apiURL = "https://api-of-administration.onrender.com/api/productos/";

// Crear instancia para WebSocket
WebSocketsClient webSocket;

// ID único del ESP32
String id_esp32;

// Manejar mensajes del WebSocket
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket desconectado");
      break;

    case WStype_CONNECTED:
      Serial.println("WebSocket conectado");
      webSocket.sendTXT("ESP32");  // Identificar al ESP32 en el servidor
      break;

    case WStype_TEXT:
      Serial.println("Mensaje recibido del servidor:");
      Serial.println((char*)payload);

      // Procesar JSON recibido del servidor
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      if(strcmp((char*))){

      }

      if (!error) {
        if (doc.containsKey("rfid")) {
          String rfid = doc["rfid"];
          Serial.println("UID recibido del servidor: " + rfid);

          // Mostrar el UID recibido en la pantalla LCD
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("UID registrado:");
          lcd.setCursor(0, 1);
          lcd.print(rfid);  


        }
      } else {
        Serial.println("Error al deserializar JSON recibido.");
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // Esperar conexión WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando al WiFi...");
  }
  Serial.println("WiFi conectado");

  // Configurar I2C en los pines 17 y 16
  Wire.begin(17, 16);

  // Inicializar la pantalla LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Esperando RFID...");

  // Obtener ID único del ESP32
  uint64_t chipid = ESP.getEfuseMac();
  id_esp32 = String((uint16_t)(chipid >> 32), HEX) + String((uint32_t)chipid, HEX);

  // Inicializar RFID
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("Módulo RFID inicializado. Esperando tarjeta...");

  // Conectar al servidor WebSocket
  webSocket.begin(serverAddress, serverPort, "/");
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();

  // Verificar si hay una tarjeta RFID presente
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  // Leer el UID de la tarjeta RFID
  String uidString = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uidString += String(rfid.uid.uidByte[i], HEX);
  }
  uidString.toUpperCase();

  // Enviar el UID al servidor de websocket
  String jsonData = "{\"id_esp32\":\"" + id_esp32 + "\", \"rfid\":\"" + uidString + "\"}";
  webSocket.sendTXT(jsonData);

  Serial.println("Datos enviados al servidor: " + jsonData);

  //Mostrar el UID en la pantalla LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enviando UID:");
  lcd.setCursor(0, 1);
  lcd.print(uidString);
          Serial.println("*****************************");
          Serial.println("Eviando UID:");
          Serial.println(uidString);
          Serial.println("*****************************");
  // Detener lectura del RFID
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // hacemos un POST para actualizar el stock del producot
  if (WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    http.begin(serverStore);
    http.addHeader("Content-Type", "application/json");
    String jsonData = "{\"status\":\"hola\", \"message\":\""+uidString+ "\"}";
    int httpResponseCode = http.POST(jsonData);
    if(httpResponseCode > 0){
      String response = http.getString();
      Serial.println("-------------------------");
       Serial.println(httpResponseCode);
      Serial.println(response);
      Serial.println(uidString);
      Serial.println("------------------------");

    }else{
      Serial.print("Error al hacer el POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }
  fetchProducData(uidString);

  // Mstramos los detalles despúes de pasar el producto
  delay(5000); // Tiempo entre lecturas
}


void fetchProducData(String rfidUid){
 //construimo la url
   String url = apiURL + "?idNFC=" + rfidUid;
   Serial.println("Cosultando: "+ url);

   if(WiFi.status() != WL_CONNECTED){
    Serial.println("Wifi conectado, procedemos a mostrar los detalles");
    return;
   }

   HTTPClient http;
   http.begin(url);
   int httpResponseCode = http.GET();

  // valida el request del servidor
   if(httpResponseCode >0){
    String response = http.getString();
    Serial.println("Respuesta del Servidor:");
    Serial.print(response);

    //Paseamos el json para extrae sus contenido

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);
    Serial.println("////////////////////////////////");
    JsonArray array = doc.as<JsonArray>();
    Serial.println("el Array despues de desarializar");
    for (JsonObject obj : array){
      Serial.println("Nombre: "+ obj["name"].as<String>());
      Serial.println("stock:"+ String(obj["stock"].as<int>()));
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Nombre: "+ obj["name"].as<String>());
        lcd.setCursor(0, 1);
        lcd.print("stock:"+ String(obj["stock"].as<int>()));
    }


   }else{
    Serial.println("Error al hace la solicitud");
   }

 }
