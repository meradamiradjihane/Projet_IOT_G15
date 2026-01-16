from fastapi import FastAPI
import requests

app = FastAPI(title="API IoT – ESP32")

ESP32_IP = "http://10.100.21.50"


def esp32_get(path: str, params=None, timeout=2):
    try:
        r = requests.get(f"{ESP32_IP}{path}", params=params, timeout=timeout)
        r.raise_for_status()
        return r.json()
    except requests.exceptions.RequestException as e:
        return {"error": "ESP32 non joignable", "details": str(e)}


# ====== CAPTEURS ======
@app.get("/temperature")
def get_temperature():
    data = esp32_get("/temp")

    temp = data.get("celsius")
    if temp is None:
        status = "Capteur indisponible"
    elif temp < 10:
        status = "Température basse"
    elif temp > 30:
        status = "Température élevée"
    else:
        status = "Température normale"

    return {
        "temperature_celsius": temp,
        "etat": status,
        "source": "ESP32"
    }


@app.get("/luminosite")
def get_luminosite():
    data = esp32_get("/ldr", timeout=10)

    r_ldr = data.get("res_ohm")
    if r_ldr is None:
        status = "Capteur indisponible"
    elif r_ldr < 20:
        status = "Environnement sombre"
    else:
        status = "Luminosité normale"

    return {
        "resistance_ohm": r_ldr,
        "etat": status,
        "source": "ESP32"
    }


# ====== CONFIGURATION DES SEUILS ======
@app.post("/config/temperature")
def set_temperature(min: float, max: float):
    data = esp32_get(
        "/config/temp",
        params={"min": min, "max": max}
    )

    return {
        "TEMP_MIN": min,
        "TEMP_MAX": max,
        "esp32_response": data
    }


@app.post("/config/luminosite")
def set_luminosity(min: float):
    data = esp32_get(
        "/config/ldr",
        params={"min": min},
        timeout=10
    )

    return {
        "LDR_min": min,
        "esp32_response": data
    }

@app.get("/mode")
def get_mode():
    """
    Retourne le mode actuel de l'ESP32 (auto / manual)
    """
    return esp32_get("/mode")


@app.post("/mode/{value}")
def set_mode(value: str):
    """
    Force le mode de l'ESP32 : auto ou manual
    """
    if value not in ["auto", "manual"]:
        return {"error": "mode invalide (auto/manual)"}

    return esp32_get("/mode", params={"set": value})


VALID_LEDS = ["1R", "1V", "2R", "2V", "all"]

@app.post("/led/{led_id}/{state}")
def control_led(led_id: str, state: str):
    if led_id not in VALID_LEDS:
        return {
            "error": "LED inconnue",
            "led_id": led_id,
            "valid_leds": VALID_LEDS
        }

    if state not in ["on", "off", "toggle"]:
        return {"error": "state invalide"}

    r = requests.get(
        f"{ESP32_IP}/led",
        params={"id": led_id, "state": state},
        timeout=20
    )

    return {
        "led": led_id,
        "state": state,
        "esp32_response": r.json()
    }