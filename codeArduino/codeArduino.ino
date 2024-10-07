#include <DHT.h>  
#include <Wire.h>
#define DHTTYPE DHT22 

#define DHTPIN 15

#define RELAY_PIN 26

DHT dht(DHTPIN, DHTTYPE);

#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
#define SERIAL SerialUSB
#else
#define SERIAL Serial
#endif

unsigned char low_data[8] = {0};
unsigned char high_data[12] = {0};

#define NO_TOUCH       0xFE
#define THRESHOLD      100
#define ATTINY1_HIGH_ADDR   0x78
#define ATTINY2_LOW_ADDR   0x77

void getHigh12SectionValue(void)
{
  memset(high_data, 0, sizeof(high_data));
  Wire.requestFrom(ATTINY1_HIGH_ADDR, 12);
  while (12 != Wire.available());

  for (int i = 0; i < 12; i++) {
    high_data[i] = Wire.read();
  }
  delay(10);
}

void getLow8SectionValue(void)
{
  memset(low_data, 0, sizeof(low_data));
  Wire.requestFrom(ATTINY2_LOW_ADDR, 8);
  while (8 != Wire.available());

  for (int i = 0; i < 8 ; i++) {
    low_data[i] = Wire.read(); // recevoir un octet comme caractère
  }
  delay(10);
}

void setup() {
  SERIAL.begin(9600);
  SERIAL.println("Setup completed");
  dht.begin();
  delay(2000);
  analogReadResolution(12);  // Pour ESP32
  Wire.begin();
  // Initialiser la broche du relais en tant que sortie
  pinMode(RELAY_PIN, OUTPUT);
  
  // Relais désactivé au démarrage
  digitalWrite(RELAY_PIN, LOW);
}

void loop()
{
  delay(2000);

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Vérifier si les données sont valides
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Erreur de lecture du capteur DHT !");
  } else {
    // Afficher les résultats du capteur DHT
    Serial.print("Humidité: ");
    Serial.print(humidity);
    Serial.print(" %\t");
    Serial.print("Température: ");
    Serial.print(temperature);
    Serial.println(" °C");
  }

  // 2. Mesurer et afficher le niveau d'eau

  int sensorvalue_min = 250;
  int sensorvalue_max = 255;
  int low_count = 0;
  int high_count = 0;
  uint32_t touch_val = 0;
  uint8_t trig_section = 0;
  
  // Lire les données des capteurs I2C (8 sections et 12 sections)
  getLow8SectionValue();
  getHigh12SectionValue();

  // Afficher les valeurs des 8 sections basses
  Serial.println("low 8 sections value = ");
  for (int i = 0; i < 8; i++) {
    Serial.print(low_data[i]);
    Serial.print(".");
    if (low_data[i] >= sensorvalue_min && low_data[i] <= sensorvalue_max) {
      low_count++;
    }
    if (low_count == 8) {
      Serial.print("      PASS");
    }
  }
  Serial.println("  ");

  // Afficher les valeurs des 12 sections hautes
  Serial.println("high 12 sections value = ");
  for (int i = 0; i < 12; i++) {
    Serial.print(high_data[i]);
    Serial.print(".");

    if (high_data[i] >= sensorvalue_min && high_data[i] <= sensorvalue_max) {
      high_count++;
    }
    if (high_count == 12) {
      Serial.print("      PASS");
    }
  }
  Serial.println("  ");

  // Calcul du niveau d'eau en fonction des données des capteurs
  for (int i = 0 ; i < 8; i++) {
    if (low_data[i] > THRESHOLD) {
      touch_val |= 1 << i;
    }
  }
  for (int i = 0 ; i < 12; i++) {
    if (high_data[i] > THRESHOLD) {
      touch_val |= (uint32_t)1 << (8 + i);
    }
  }

  // Calculer la section d'eau déclenchée
  while (touch_val & 0x01) {
    trig_section++;
    touch_val >>= 1;
  }

  // Afficher le niveau d'eau
  SERIAL.print("Niveau d'eau = ");
  SERIAL.print(trig_section * 5);  // Multiplier par 5 pour obtenir le pourcentage
  SERIAL.println("% ");
  SERIAL.println("*********************************************************");

  digitalWrite(RELAY_PIN, HIGH);
  Serial.println("Relais activé (ON)");
  
  // Attendre 2 secondes
  delay(2000);
  
  // Désactiver le relais (OFF)
  digitalWrite(RELAY_PIN, LOW);
  Serial.println("Relais désactivé (OFF)");
  
  // Attendre 2 secondes
  delay(2000);

  delay(1000); // Attendre avant la prochaine itération
}
