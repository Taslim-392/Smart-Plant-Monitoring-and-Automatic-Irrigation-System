#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

#define SOIL_PIN         A0
#define TANK_LEVEL_PIN   A1

#define DHT_PIN          2
#define DHT_TYPE         DHT11

#define RELAY_PIN        7
#define BUZZER_PIN       8

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHT_TYPE);

const int DRY_THRESHOLD = 650;
const int WET_THRESHOLD = 450;

const int CAL_POINTS = 6;
const int calADC[CAL_POINTS] = {   0, 480, 600, 680, 720, 750 };
const int calPCT[CAL_POINTS] = {   0,  20,  40,  60,  80, 100 };

const int TANK_LOW_PERCENT = 20;

const unsigned long MAX_PUMP_TIME          = 10000;
const unsigned long DHT_SKIP_AFTER_PUMP_MS = 15000;
const unsigned long DISPLAY_ROTATE_MS      = 3000;
const unsigned long LOOP_DELAY_MS          = 1000;

bool pumpRunning = false;
bool safetyLock  = false;

unsigned long pumpStartTime = 0;

float lastValidTemp = 25.0;
float lastValidHum  = 50.0;
bool  haveValidDHT  = false;

// ---------- Tank Level Stability ----------
int  lastStableTankPct = 0;
int  stableCount       = 0;

// =====================================================
void setup()
{
  Serial.begin(9600);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  dht.begin();

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Plant");
  lcd.setCursor(0, 1);
  lcd.print("System Ready");
  delay(2000);
  lcd.clear();

  Serial.println("=== Smart Plant System Ready ===");
}

// =====================================================
void loop()
{
  int soilValue = analogRead(SOIL_PIN);
  int tankLevel = readTankLevelFiltered();
  int tankPct   = getStableTankPct(tankLevel);

  // ---------- DHT ----------
  float temperature;
  float humidity;
  bool  dhtError = false;

  bool dhtSkipWindow = pumpRunning &&
                       (millis() - pumpStartTime < DHT_SKIP_AFTER_PUMP_MS);

  if (dhtSkipWindow)
  {
    temperature = lastValidTemp;
    humidity    = lastValidHum;
    dhtError    = false;
    Serial.println("[DHT] Skipped (pump just started)");
  }
  else
  {
    temperature = dht.readTemperature();
    humidity    = dht.readHumidity();
    dhtError    = isnan(temperature) || isnan(humidity);

    if (!dhtError)
    {
      lastValidTemp = temperature;
      lastValidHum  = humidity;
      haveValidDHT  = true;
    }
    else if (haveValidDHT)
    {
      temperature = lastValidTemp;
      humidity    = lastValidHum;
    }
  }

  bool soilDry = soilValue >= DRY_THRESHOLD;
  bool soilWet = soilValue <= WET_THRESHOLD;
  bool tankLow = tankPct <= TANK_LOW_PERCENT;

  if (dhtError)
  {
    stopPump();
    digitalWrite(BUZZER_PIN, HIGH);
    displayError("DHT ERROR");
    Serial.println("[ERROR] DHT sensor failed!");
    delay(LOOP_DELAY_MS);
    return;
  }

  if (tankLow)
  {
    stopPump();
    digitalWrite(BUZZER_PIN, HIGH);

    displayData(soilValue, temperature, humidity, tankPct);
    printSerialData(soilValue, temperature, humidity, tankLevel, tankPct);

    Serial.println("[ALERT] Tank is LOW!");
    delay(LOOP_DELAY_MS);
    return;
  }

  digitalWrite(BUZZER_PIN, LOW);

  if (safetyLock)
  {
    stopPump();
    if (soilWet)
    {
      safetyLock = false;
      Serial.println("[SAFETY] Lock released (soil wet)");
    }
  }
  else
  {
    if (!pumpRunning && soilDry) startPump();
    if (pumpRunning && soilWet)  stopPump();

    if (pumpRunning && (millis() - pumpStartTime >= MAX_PUMP_TIME))
    {
      stopPump();
      safetyLock = true;
      Serial.println("[SAFETY] Max pump time reached! Lock ON");
    }
  }

  displayData(soilValue, temperature, humidity, tankPct);
  printSerialData(soilValue, temperature, humidity, tankLevel, tankPct);

  delay(LOOP_DELAY_MS);
}

// =====================================================
//   FILTER: Median + Average (Noise-proof)
// =====================================================
int readTankLevelFiltered()
{
  const int N = 15;
  int samples[N];

  for (int i = 0; i < N; i++)
  {
    samples[i] = analogRead(TANK_LEVEL_PIN);
    delay(3);
  }

  // Bubble sort
  for (int i = 0; i < N - 1; i++)
  {
    for (int j = 0; j < N - 1 - i; j++)
    {
      if (samples[j] > samples[j + 1])
      {
        int temp = samples[j];
        samples[j] = samples[j + 1];
        samples[j + 1] = temp;
      }
    }
  }

  // Middle 7 average
  long sum = 0;
  int start = (N - 7) / 2;
  for (int i = start; i < start + 7; i++)
  {
    sum += samples[i];
  }
  return (int)(sum / 7);
}

// =====================================================
//   STABLE TANK % — Pump ON হলে freeze
// =====================================================
int getStableTankPct(int tankADC)
{
  int rawPct = adcToPercent(tankADC);

  // Pump চললে freeze — পুরনো মান দেখাও
  if (pumpRunning)
  {
    return lastStableTankPct;
  }

  // Pump বন্ধ — stable কিনা চেক করো
  if (abs(rawPct - lastStableTankPct) <= 2)
  {
    stableCount++;
    if (stableCount >= 2)   // 2 বার same হলে accept
    {
      lastStableTankPct = rawPct;
    }
  }
  else
  {
    stableCount = 0;
    // বড় পরিবর্তন হলে সাথে সাথে update
    if (abs(rawPct - lastStableTankPct) > 10)
    {
      lastStableTankPct = rawPct;
    }
  }

  return lastStableTankPct;
}

// =====================================================
int adcToPercent(int adc)
{
  if (adc <= calADC[0])              return calPCT[0];
  if (adc >= calADC[CAL_POINTS - 1]) return calPCT[CAL_POINTS - 1];

  for (int i = 0; i < CAL_POINTS - 1; i++)
  {
    if (adc >= calADC[i] && adc <= calADC[i + 1])
    {
      float ratio = (float)(adc - calADC[i]) /
                    (float)(calADC[i + 1] - calADC[i]);
      float pct = calPCT[i] + ratio * (calPCT[i + 1] - calPCT[i]);
      return (int)(pct + 0.5);
    }
  }
  return 0;
}

// =====================================================
void startPump()
{
  digitalWrite(RELAY_PIN, LOW);
  pumpRunning   = true;
  pumpStartTime = millis();
  Serial.println("[PUMP] ON — Tank % frozen, DHT skipped");
}

void stopPump()
{
  digitalWrite(RELAY_PIN, HIGH);
  pumpRunning = false;
}

// =====================================================
void displayData(int soil, float temperature, float humidity, int tankPct)
{
  static unsigned long lastDisplay = 0;
  static int screen = 0;

  if (millis() - lastDisplay >= DISPLAY_ROTATE_MS)
  {
    lastDisplay = millis();
    screen++;
    if (screen > 2) screen = 0;
    lcd.clear();
  }

  if (screen == 0)
  {
    lcd.setCursor(0, 0);
    lcd.print("Moist:");
    lcd.print(soil);
    lcd.print("    ");

    lcd.setCursor(0, 1);
    lcd.print("Pump:");
    if (pumpRunning) lcd.print("ON ");
    else             lcd.print("OFF");
  }
  else if (screen == 1)
  {
    lcd.setCursor(0, 0);
    lcd.print("Temp:");
    lcd.print(temperature, 1);
    lcd.print("C   ");

    lcd.setCursor(0, 1);
    lcd.print("Hum:");
    lcd.print(humidity, 0);
    lcd.print("%    ");
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print("Water Level:   ");

    lcd.setCursor(0, 1);
    lcd.print(tankPct);
    lcd.print(" %        ");
  }
}

// =====================================================
void displayError(const char *message)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SYSTEM ERROR");
  lcd.setCursor(0, 1);
  lcd.print(message);
}

// =====================================================
void printSerialData(int soil, float temperature, float humidity,
                     int tankADC, int tankPct)
{
  Serial.print("Soil: ");
  Serial.print(soil);

  Serial.print(" | Temp: ");
  Serial.print(temperature, 1);

  Serial.print(" C | Hum: ");
  Serial.print(humidity, 0);

  Serial.print(" % | TankADC: ");
  Serial.print(tankADC);

  Serial.print(" | Tank: ");
  Serial.print(tankPct);
  Serial.print(" %");

  if (pumpRunning) Serial.print(" [FROZEN]");

  Serial.print(" | Pump: ");
  Serial.println(pumpRunning ? "ON" : "OFF");
}