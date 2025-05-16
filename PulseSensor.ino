#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#define USE_ARDUINO_INTERRUPTS false  // <-- ESTO ES CRUCIAL PARA ESP8266
#include <PulseSensorPlayground.h>

const char* ssid     = "Totalplay-BB9D"; 
const char* password = "BB9DC070U2MK67u9"; 
const char* server   = "apex.oracle.com"; 

#define led D0
#define buzzer D1

WiFiClientSecure client;

const int PulseWire = A0;
int Threshold = 550;

PulseSensorPlayground pulseSensor;

void setup() {
  Serial.begin(115200);
  delay(100);

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
  
  client.setInsecure();  // Para evitar problemas con certificados HTTPS

  // Configurar sensor
  pulseSensor.analogInput(PulseWire);
  pulseSensor.setThreshold(Threshold);

  if (pulseSensor.begin()) {
    Serial.println("PulseSensor iniciado correctamente.");
  } else {
    Serial.println("Error iniciando PulseSensor.");
  }

  // Configurar pines de salida
  pinMode(led, OUTPUT);
  pinMode(buzzer, OUTPUT);
}

void loop() {
  latidos();
  delay(2000); // Intervalo de espera entre lecturas
}

void latidos(){
  if (pulseSensor.sawStartOfBeat()) {
    int myBPM = pulseSensor.getBeatsPerMinute();
    Serial.println("\n♥ Latido detectado");
    Serial.print("BPM: ");
    Serial.println(myBPM);

    sendData(42, "Pulso", myBPM);

    if (myBPM > 100){
      digitalWrite(led, HIGH);       // Enciende LED
      Serial.println("LED Encendido");
      digitalWrite(buzzer, HIGH);    // Enciende buzzer
      Serial.println("Buzzer Encendido");
      sendAct(1, 1);
      sendAct(2, 1);
    } else {
      digitalWrite(led, LOW);        // Apaga LED
      Serial.println("LED Apagado");
      digitalWrite(buzzer, LOW);     // Apaga buzzer
      Serial.println("Buzzer Apagado");
      sendAct(1, 0);
      sendAct(2, 0);
    }
  }
  delay(1000);  // Intervalo seguro de envío
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
    if (line == "\r") break;
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


