# Projet IoT - API RESTful sur TTGO T-Display ESP32

## Description

Système de surveillance de température et luminosité avec API RESTful implémentée sur microcontrôleur ESP32 TTGO T-Display. Le système permet la lecture de capteurs, le contrôle de LEDs et la configuration de seuils d'alerte via HTTP.

##  Matériel Requis

### Microcontrôleur
- **TTGO T-Display ESP32** avec écran TFT intégré (135x240 pixels)

### Capteurs
- **NTC Thermistor** (EPCOS B57861S, 10kΩ @ 25°C, β=3988K)
- **LDR** (photorésistance)

### Composants
- 4× LEDs (2 rouges, 2 vertes)
- 2× Résistances 1kΩ (pour LEDs et LDR)
- 1× Résistance 47kΩ (pour NTC)
- Câbles de connexion
- Breadboard

### Kit 
[Adafruit Kit #2975](https://www.adafruit.com/product/2975)

##  Schéma de Câblage

Pour faire le cablage nous avons utiliser Fri

```
ESP32 Pin Assignments:
├── GPIO 36 (ADC1_CH0) ← LDR (pont diviseur avec R=1kΩ)
├── GPIO 39 (ADC1_CH3) ← NTC (pont diviseur avec R=47kΩ)
├── GPIO 12 ← LED Rouge Température
├── GPIO 13 ← LED Verte Température
├── GPIO 2  ← LED Rouge Luminosité
├── GPIO 15 ← LED Verte Luminosité
├── GPIO 0  ← Bouton Gauche (intégré TTGO)
└── GPIO 35 ← Bouton Droit (intégré TTGO)
```
Le diagramme de câblage ci-dessous a été conçu avec Fritzing :

![Schéma de câblage](./Cablage/Pr_iot.png)

*Fichier source : [projet_iot.fzz](./Cablage/Pr_iot.fzz)*

### Ponts Diviseurs

**NTC (Température):**
```
3.3V ──[R_fix=47kΩ]── Vout (GPIO39) ──[NTC]── GND
```

**LDR (Luminosité):**
```
3.3V ──[R_fix=1kΩ]── Vout (GPIO36) ──[LDR]── GND
```

## Installation

### 1. Configuration Arduino IDE

1. Installer [Arduino IDE](https://www.arduino.cc/en/software)
2. Ajouter le support ESP32:
   - `Fichier` → `Préférences` → `URL de gestionnaire de cartes`
   - Ajouter: `https://dl.espressif.com/dl/package_esp32_index.json`
3. Installer la carte ESP32:
   - `Outils` → `Type de carte` → `Gestionnaire de cartes`
   - Chercher "ESP32" et installer

### 2. Bibliothèques Requises

Installer via `Croquis` → `Inclure bibliothèque` → `Gérer les bibliothèques`:

- **TFT_eSPI** (version ≥2.5.0)
- **ArduinoJson** (version ≥6.0.0)
- **WiFi** (intégré ESP32)
- **WebServer** (intégré ESP32)

### 3. Configuration TFT_eSPI

Éditer `Arduino/libraries/TFT_eSPI/User_Setup_Select.h`:
```cpp
// Commenter la ligne par défaut:
// #include <User_Setup.h>

// Décommenter la ligne TTGO T-Display:
#include <User_Setups/Setup25_TTGO_T_Display.h>
```

### 4. Configuration WiFi

Modifier dans `Projet_iot.ino`:
```cpp
const char* ssid     = "VOTRE_SSID";
const char* password = "VOTRE_MOT_DE_PASSE";
```

### 5. Téléversement

1. Connecter le TTGO T-Display via USB
2. Sélectionner:
   - `Outils` → `Type de carte` → `TTGO T1`
   - `Outils` → `Upload speed` → `115200`
   - `Outils` → `Port` → (sélectionner le port USB)
3. Cliquer sur `Téléverser`

##  API RESTful - Endpoints ESP32

### Lister les Capteurs
```http
GET http://<ESP32_IP>/sensors
```
**Réponse:**
```json
{
  "sensors": ["ldr", "thermistor"]
}
```

### Lire la Température
```http
GET http://<ESP32_IP>/temp
```
**Réponse:**
```json
{
  "raw": 2048,
  "voltage": 1.65,
  "celsius": 23.5
}
```

### Lire la Luminosité
```http
GET http://<ESP32_IP>/ldr
```
**Réponse:**
```json
{
  "raw": 1500,
  "voltage": 1.21,
  "res_ohm": 450.5
}
```

### Configurer les Seuils de Température
```http
GET http://<ESP32_IP>/config/temp?min=15&max=28
```
**Réponse:**
```json
{
  "TEMP_MIN": 15.0,
  "TEMP_MAX": 28.0
}
```

### Configurer le Seuil de Luminosité
```http
GET http://<ESP32_IP>/config/ldr?min=50
```
**Réponse:**
```json
{
  "rLdr_MIN": 50.0
}
```


## Interface Utilisateur

### Écran TFT
Affiche en temps réel:
- Température (en °C)
- Résistance LDR (en Ω)
- Rafraîchissement: 1 fois/seconde

### LEDs Indicatrices

| LED | Couleur | Condition |
|-----|---------|-----------|
| Température | 🟢 Verte | TEMP_MIN ≤ T ≤ TEMP_MAX |
| Température | 🔴 Rouge | T < TEMP_MIN ou T > TEMP_MAX |
| Luminosité | 🟢 Verte | R_LDR ≥ R_MIN |
| Luminosité | 🔴 Rouge | R_LDR < R_MIN |

### Boutons (TTGO T-Display)
 **Bouton Gauche (GPIO 0):**
 **Bouton Droit (GPIO 35):**

##  Formules de Calcul

### Conversion ADC → Tension
```
V_out = (ADC_raw × V_ref) / ADC_max
```
Avec: V_ref = 3.3V, ADC_max = 4095 (12 bits)

### Résistance NTC
```
R_NTC = R_fix × (V_ref - V_out) / V_out
```

### Température (Équation de Steinhart-Hart simplifiée)
```
1/T = (1/T₀) + (1/β) × ln(R_NTC / R₀)
T_celsius = T - 273.15
```
Avec: T₀ = 298.15K (25°C), β = 3988K, R₀ = 10kΩ

### Résistance LDR

```
R_LDR = R_fix × V_out / (V_ref - V_out)
```

##  Tests
 Pour les teste nous avon cree une autre API en python utilisant le framework FastAPI l'application permet de tester les endpoints de l'API RESTful de l'ESP32.


##  API Python (FastAPI)

### Installation
```bash
pip install -r requirements.txt
```

### Configuration
Modifier `API.py`:
```python
ESP32_IP = "http://<VOTRE_IP_ESP32>"
```

### Lancement
```bash
uvicorn API:app --reload --host 0.0.0.0 --port 8000
```



## Groupe 15 
Auteurs: 

MERAD Amira Djihane
CHBOUK Yassir
AMARA Rafik
Goussem Ayoub