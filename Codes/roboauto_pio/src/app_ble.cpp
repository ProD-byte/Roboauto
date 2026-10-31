#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "config.h"
#include "web.h"
#include "app_ble.h"

// Eigene UUIDs der Roboauto-App (in der App genauso eingetragen)
static const char* UUID_DIENST  = "8d3a1000-5b7e-4f2a-9c61-2e4b7a9d0c11";
static const char* UUID_BEFEHL  = "8d3a1001-5b7e-4f2a-9c61-2e4b7a9d0c11";   // App -> Auto
static const char* UUID_ANTWORT = "8d3a1002-5b7e-4f2a-9c61-2e4b7a9d0c11";   // Auto -> App

static BLEServer* server = nullptr;
static BLECharacteristic* antwortChar = nullptr;

// Befehle kommen im Bluetooth-Task an. Ausgefuehrt werden sie erst in loop(),
// damit nicht zwei Programmteile gleichzeitig am Auto drehen.
struct Anfrage { char text[256]; };
static QueueHandle_t warteschlange = nullptr;
static volatile bool getrennt = false;

class Verbindung : public BLEServerCallbacks {
  void onConnect(BLEServer*) override {}
  void onDisconnect(BLEServer* s) override {
    getrennt = true;            // loop() stoppt das Auto
    s->startAdvertising();      // gleich wieder sichtbar, damit die App neu verbinden kann
  }
};

class Empfang : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    String wert = c->getValue();
    Anfrage a;
    if (wert.length() >= sizeof(a.text)) return;          // zu lang -> verwerfen
    memcpy(a.text, wert.c_str(), wert.length() + 1);
    xQueueSend(warteschlange, &a, 0);                     // Schlange voll -> verwerfen (naechster Fahrbefehl kommt gleich)
  }
};

void appBleStarten() {
  warteschlange = xQueueCreate(8, sizeof(Anfrage));
  BLEDevice::init(BLE_NAME);
  BLEDevice::setMTU(517);                                 // grosse Pakete: Status passt in 2 Stuecke
  server = BLEDevice::createServer();
  server->setCallbacks(new Verbindung());

  BLEService* dienst = server->createService(UUID_DIENST);
  BLECharacteristic* befehl = dienst->createCharacteristic(
      UUID_BEFEHL, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  befehl->setCallbacks(new Empfang());
  antwortChar = dienst->createCharacteristic(UUID_ANTWORT, BLECharacteristic::PROPERTY_NOTIFY);
  antwortChar->addDescriptor(new BLE2902());
  dienst->start();

  BLEAdvertising* werbung = server->getAdvertising();
  werbung->addServiceUUID(UUID_DIENST);                   // App findet das Auto an dieser UUID
  werbung->setScanResponse(true);                         // Name kommt in der Scan-Antwort mit
  werbung->start();
  Serial.printf(">> Bluetooth fuer die App bereit: \"%s\"\n", BLE_NAME);
}

// Antwort in Stuecken passend zur MTU verschicken; '\n' am Ende = fertig
static void sende(const String& text) {
  if (server->getConnectedCount() == 0) return;
  int stueck = constrain((int)server->getPeerMTU(server->getConnId()) - 3, 20, 509);
  for (unsigned pos = 0; pos < text.length(); pos += stueck) {
    String teil = text.substring(pos, pos + stueck);
    antwortChar->setValue((uint8_t*)teil.c_str(), teil.length());
    antwortChar->notify();
  }
}

void appBleUpdate() {
  if (getrennt) { getrennt = false; webFahrStopp(); }

  Anfrage a;
  while (xQueueReceive(warteschlange, &a, 0) == pdTRUE) {
    // "<nr>|<pfad>?<parameter>"  -  nr 0 = keine Antwort
    String text(a.text);
    int strich = text.indexOf('|');
    if (strich < 0) continue;
    String nr = text.substring(0, strich);
    String antwort = webAppBefehl(text.substring(strich + 1));
    if (nr != "0") sende(nr + "|" + antwort + "\n");
  }
}
