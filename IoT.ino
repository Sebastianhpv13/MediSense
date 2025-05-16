#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>

// Pines
#define RST_PIN D1  // GPIO5
#define SS_PIN  D2  // GPIO4
#define DHTPIN D3
#define DHTTYPE DHT11 
#define FAN_PIN D4   // GPIO16 para el ventilador (relay)

DHT dht(DHTPIN, DHTTYPE);
MFRC522 rfid(SS_PIN, RST_PIN);  // Instancia del lector

int sensorFuerza = A0;

const char* ssid     = "Totalplay-BB9D"; 
const char* password = "BB9DC070U2MK67u9"; 
const char* server = "apex.oracle.com"; 
WiFiClientSecure client;

void setup() {
  Serial.begin(9600);
  delay(100);

  pinMode(FAN_PIN, OUTPUT);         // ← Configura el pin como salida
  digitalWrite(FAN_PIN, LOW);       // ← Asegura que inicie apagado

  dht.begin();
  SPI.begin();
  rfid.PCD_Init();     
  Serial.println("Listo para leer tarjetas RFID...");
  
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  client.setInsecure();  
}

void loop() {
  tempHum();
  fuerza();
  tarjeta();
}

void tarjeta(){
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uidString = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uidString += "0";
    uidString += String(rfid.uid.uidByte[i], HEX);
  }
  uidString.toUpperCase();

  Serial.print("UID detectado: ");
  Serial.println(uidString);

  if (uidString == "7F61B40C") {
    sendData(337, "Credencial", 1);
  } else if (uidString == "43EEFE28") {
    sendData(337, "Credencial", 2);
  }else {
    sendData(0, "Credencial", 0);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void tempHum(){
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    Serial.println("Error leyendo del sensor DHT!");
    delay(5000);
    return;
  }

  Serial.print("Temp: "); Serial.print(temp); Serial.print("°C | ");
  Serial.print("Hum: "); Serial.print(hum); Serial.println("%");

  int tempRounded = round(temp);
  int humRounded = round(hum);

  sendData(1, "Temperature", tempRounded);
  delay(3000);
  sendData(1, "Humedad", humRounded);
  delay(10000); 

  // 🔵 Control del ventilador:
  if (tempRounded > 28) {
    digitalWrite(FAN_PIN, HIGH);  // Enciende el ventilador
    Serial.println("🔵 Ventilador ENCENDIDO");
    sendAct(3, 1);
  } else {
    digitalWrite(FAN_PIN, LOW);   // Apágalo si no es necesario
    Serial.println("⚪ Ventilador APAGADO");
    sendAct(3, 0);
  }
}

void fuerza(){
  int lectura = analogRead(sensorFuerza);
  Serial.print("Lectura Analogica = ");
  Serial.println(lectura);

  int disponibilidad = (lectura > 80) ? 1 : 0;

  sendData(41, "Fuerza", disponibilidad);
  delay(2000);
}

void sendData(int sensor, String tipo, int valor) {
  if (!client.connect(server, 443)) {
    Serial.println("Fallo la conexión al servidor!");
    return;
  }

  String url = "/pls/apex/a00838407tec/sendreg/senddata?pcods=" + String(sensor) + "&ptipo=" + tipo + "&pvalor=" + String(valor);

  Serial.print("Enviando GET a: ");
  Serial.println(url);

  client.println("GET " + url + " HTTP/1.0");
  client.println("Host: apex.oracle.com");
  client.println("Connection: close");
  client.println();

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  while (client.available()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
  }

  client.stop();
  Serial.println("Conexión cerrada\n");
}

void sendAct(int actuador, int valor) {
  if (!client.connect(server, 443)) {
    Serial.println("Fallo la conexión al servidor!");
    return;
  }

  String url = "/pls/apex/a00838407tec/sendestatus/senddata?pcode=" + String(actuador) + "&pvalor=" + String(valor);

  Serial.print("Enviando GET a: ");
  Serial.println(url);

  client.println("GET " + url + " HTTP/1.0");
  client.println("Host: apex.oracle.com");
  client.println("Connection: close");
  client.println();

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  while (client.available()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
  }

  client.stop();
  Serial.println("Conexión cerrada\n");
}
