#include <Wire.h>
#include <WiFi.h>
#include <Firebase.h>
#include <time.h>
#include <Preferences.h>

// Configurations WiFi et Firebase
#define WIFI_SSID "iPhone de Céline (2)"
#define WIFI_PASSWORD "cc290601"
#define REFERENCE_URL "https://iotproject-c37dd-default-rtdb.europe-west1.firebasedatabase.app/"

Firebase fb(REFERENCE_URL);

#define MOISTURE_SENSOR_PIN 34  // Broche analogique pour le capteur d'humidité du sol
#define RELAY_PIN 26
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

  // ---------- lancement du Wifi  ----------
  #if !defined(ARDUINO_UNOWIFIR4)
    WiFi.mode(WIFI_STA);
  #else
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
  #endif
    WiFi.disconnect();
    delay(1000);

    /* Connexion au WiFi */
    Serial.println();
    Serial.println("En cours de connexion à ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
      Serial.print("-");
      delay(500);
    }
    Serial.println();
    Serial.println("Connecté au Wifi");
    Serial.println();

  #if defined(ARDUINO_UNOWIFIR4)
    digitalWrite(LED_BUILTIN, LOW);
  #endif

  SERIAL.println("Setup completed");
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

  //Est-ce que la plante doit-être arrosée? 
  String id_plante = fb.getString("plantes/id_selectionne");
  Serial.print("Id plante:\t");
  Serial.println(id_plante);

  bool arrosage = fb.getBool("plantes/" + id_plante + "/arrosageAuto");
  Serial.print("arrosage");
  Serial.println(arrosage);

  if (arrosage){

  
    // Lire l'humidité du sol
    int moistureValue = analogRead(MOISTURE_SENSOR_PIN);
    
    // Convertir la valeur brute en pourcentage (en fonction de vos capteurs spécifiques)
    float humidity = map(moistureValue, 0, 2048, 0, 100);  // Conversion en pourcentage
    
    // Afficher l'humidité du sol
    Serial.print("Humidité du sol: ");
    Serial.print(humidity);
    Serial.println(" %");

    float ref_humidite = fb.getFloat("plantes/" + id_plante + "/refHumidite");
    Serial.print("ref_humidite");
    Serial.println(ref_humidite);

    fb.setFloat("plantes/" + String(id_plante) + "/humiditeActuelle", humidity);

    if (humidity<ref_humidite){
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
      fb.setFloat("plantes/eau_reservoir", trig_section * 5);


      //Est-ce qu'il y a assez d'eau? 
      if (trig_section*5 > 24){
        digitalWrite(RELAY_PIN, HIGH);
        Serial.println("Relais activé (ON), arrosage");
        // Attendre 2 secondes
        delay(4000);
        fb.setBool("plantes/eau_suffisante", true);
        // Désactiver le relais (OFF)
        digitalWrite(RELAY_PIN, LOW);
        Serial.println("Relais désactivé (OFF)");
        // Attendre 2 secondes
        delay(2000);
      }else{
        fb.setBool("plantes/eau_suffisante", false);
        //Sinon, prévenir sur l'application 
      }
      
    }
  }

//donner le taux actuel d'humidité
//dire s'il reste de l'eau 
//donner le taux d'eau qui reste dans lé réservoir



  delay(1000); // Attendre avant la prochaine itération*/
}
