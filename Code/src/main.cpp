#include <TFT_eSPI.h>
#include <SPI.h>


#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <SensirionI2cScd4x.h>
#include <driver/i2s.h>
#include "Adafruit_PM25AQI.h"
///Users/mark/Documents/PlatformIO/Projects/250519-181440-adafruit_feather_esp32_v2/src
// AQI result structure
struct AQIResult {
   int aqi;
   const char* category;
   uint16_t color;
};


// Compute AQI based on PM2.5 concentration
AQIResult computeAQI_PM25(float pm25) {
   struct Range { float low, high; int aLow, aHigh; const char* cat; uint16_t col; };
   const Range ranges[] = {
       {   0.0f,   12.0f,   0,  50, "Good",      TFT_GREEN  },
       {  12.1f,   35.4f,  51, 100, "Moderate",  TFT_YELLOW },
       {  35.5f,   55.4f, 101, 150, "Unhealthy", TFT_ORANGE },
       {  55.5f,  150.4f, 151, 200, "Poor",      TFT_RED    },
       { 150.5f,  250.4f, 201, 300, "Very Poor", TFT_PURPLE },
       { 250.5f,  500.4f, 301, 500, "Hazardous", TFT_MAROON }
   };
   for (int i = 0; i < 6; ++i) {
       if (pm25 <= ranges[i].high) {
           int a = (ranges[i].aHigh - ranges[i].aLow)
                   * (pm25 - ranges[i].low)
                   / (ranges[i].high - ranges[i].low)
                 + ranges[i].aLow;
           return { a, ranges[i].cat, ranges[i].col };
       }
   }
   return { 500, "Hazardous", TFT_MAROON };
}

float latestCO2 = 0, latestTemp = 0, latestHum = 0, latestNoise = 0;
float lastAltitude = 0;
int latestPM1 = 0, latestPM25 = 0, latestPM10 = 0;

float readCO2()         { return latestCO2; }
float readTemperature() { return latestTemp; }
float readHumidity()    { return latestHum; }
float readNoise()       { return latestNoise; }
int   readAltitude()     { return lastAltitude; }
int   readPM1()         { return latestPM1; }
int   readPM25()        { return latestPM25; }
int   readPM10()        { return latestPM10; }

// Display and button pins
TFT_eSPI tft = TFT_eSPI();
const int BUTTON_A_PIN = A3;
const int BUTTON_B_PIN = A4;


// Page states
enum Page { HOME,SUMMARY_PAGE, Overall,Abnormal, PROJ, PROJ2, AIR, ENV, AQI_PAGE, NOISE_PAGE,
    TempHum_PAGE, ALTITUDECO2_PAGE, PRESSGAS_PAGE};
Page currentPage = HOME;
Page lastPage    = HOME;


// Button edge-detection state
bool prevA = false, prevB = false;


// Timing
unsigned long lastButtonPress = 0;
unsigned long startMillis      = 0;


// Display settings
uint8_t bgIndex    = 0;
uint8_t brightness = 0;
const uint16_t bgColors[] = { TFT_BLACK, TFT_BLUE, TFT_RED, TFT_GREEN, TFT_WHITE };
const char*    bgNames[]  = { "Black", "Blue", "Red", "Green", "White" };
uint16_t fontColor() { return (bgIndex == 4) ? TFT_BLACK : TFT_WHITE; }


// Layout parameters
const int lineH  = 28;
const int startY = 10;



// Simple debounce: returns true if button is pressed (LOW) in two consecutive reads
bool isButtonPressed(int pin) {
   if (digitalRead(pin) == LOW) {
       delay(20);  // debounce delay
       if (digitalRead(pin) == LOW) {
           return true;
       }
   }
   return false;
}




#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME680 bme;  // I2C


// ——— SCD-4X Definitions —————————————————————————————————————————————
#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0
SensirionI2cScd4x sensor;
static char errorMessage[64];
static int16_t error;


// ——— I2S Mic Definitions —————————————————————————————————————
#define SAMPLE_BUFFER_SIZE 512
#define SAMPLE_RATE        8000
#define I2S_MIC_SERIAL_CLOCK    A0
#define I2S_MIC_LEFT_RIGHT_CLOCK A1
#define I2S_MIC_SERIAL_DATA      A2
// PM
Adafruit_PM25AQI aqi = Adafruit_PM25AQI();


i2s_config_t i2s_config = {
 .mode                 = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
 .sample_rate          = SAMPLE_RATE,
 .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
 .channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT,
 .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
 .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
 .dma_buf_count        = 4,
 .dma_buf_len          = 1024,
 .use_apll             = false,
 .tx_desc_auto_clear   = false,
 .fixed_mclk           = 0
};


i2s_pin_config_t i2s_mic_pins = {
 .bck_io_num   = I2S_MIC_SERIAL_CLOCK,
 .ws_io_num    = I2S_MIC_LEFT_RIGHT_CLOCK,
 .data_out_num = I2S_PIN_NO_CHANGE,
 .data_in_num  = I2S_MIC_SERIAL_DATA
};


int32_t raw_samples[SAMPLE_BUFFER_SIZE];


void PrintUint64(uint64_t& value) {
 Serial.print("0x");
 Serial.print((uint32_t)(value >> 32), HEX);
 Serial.print((uint32_t)(value & 0xFFFFFFFF), HEX);
}


int lastRead;


const char* tempAdvice(float t) {
    if (t < 64) return "Too cold!";
    if (t > 80) return "Too hot!";
    return "Comfortable";
}

const char* humAdvice(float h) {
    if (h < 35) return "Too dry!";
    if (h > 70) return "Too humid!";
    return "Comfortable";
}

void showHomeLoad();
void showHomeReady();

void setup() {
   Serial.begin(115200);
   delay(5000);
   pinMode(BUTTON_A_PIN, INPUT);
   pinMode(BUTTON_B_PIN, INPUT);


   lastRead = millis();


   tft.init();
   Serial.println("TFT init");
   tft.setRotation(1);


   startMillis = millis();
   showHomeLoad();

   // delay(5000);  // give USB-Serial time


   // —— Init I2C & BME680 ——
   Wire.begin();
   Serial.println("Initializing BME680...");
   if (!bme.begin()) {
     Serial.println("BME680 not found! Check wiring.");
     while (1) delay(10);
   }
   bme.setTemperatureOversampling(BME680_OS_8X);
   bme.setHumidityOversampling(BME680_OS_2X);
   bme.setPressureOversampling(BME680_OS_4X);
   bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
   bme.setGasHeater(320, 150);
  //   // —— Init SCD-4X ——
   Serial.println("Initializing SCD4X...");
   sensor.begin(Wire, SCD41_I2C_ADDR_62);
   sensor.wakeUp();
   sensor.stopPeriodicMeasurement();
   sensor.reinit();
    uint64_t sn = 0;
   sensor.getSerialNumber(sn);
    // start periodic (5 s) mode
   sensor.startPeriodicMeasurement();
   Serial.println("Waiting 5 s for first SCD4X reading…");
 
   // —— Init I2S Mic ——
   Serial.println("Initializing I2S mic...");
   i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
   i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);
   i2s_zero_dma_buffer(I2S_NUM_0);
   i2s_start(I2S_NUM_0);
    Serial.println("Setup complete");
   Serial.println("============================");
    // PMAS
   if (! aqi.begin_I2C()) {      // connect to the sensor over I2C
       Serial.println("Could not find PM 2.5 sensor!");
       while (1) delay(10);
     }
  
     Serial.println("PM25 found!");
    //delay(5000);
}

// Function prototypes

void showProjectData();
void showProjectData2();

void showAirQuality();
void showEnvQuality();
void showOverall();
void redrawPage(Page p);
void showAbnormal();
void showTempHumPage();
void showAQIPage();
void showNoisePage();
void showPressGasPage();
void showAltitudeCO2Page();
void showSummaryPage();
bool bmeDone = false, scdDone = false, pmDone = false, micDone = false;
bool readyShown = false;
void loop() {
    if (millis() >= lastRead + 10000) {
        lastRead = millis();

        PM25_AQI_Data data;
        Serial.println("AQI reading success");
        if (!aqi.read(&data)) {
            Serial.println("Could not read from AQI");
            delay(500);
            return;
        }
        Serial.println(F("---------------------------------------"));
        Serial.println(F("Concentration Units (standard)"));
        Serial.print(F("PM 1.0: ")); Serial.print(data.pm10_standard);
        Serial.print(F("\t\tPM 2.5: ")); Serial.print(data.pm25_standard);
        Serial.print(F("\t\tPM 10: ")); Serial.println(data.pm100_standard);
        Serial.println(F("---------------------------------------"));
        Serial.println(F("Concentration Units (environmental)"));
        Serial.print(F("PM 1.0: ")); Serial.print(data.pm10_env);
        Serial.print(F("\t\tPM 2.5: ")); Serial.print(data.pm25_env);
        Serial.print(F("\t\tPM 10: ")); Serial.println(data.pm100_env);
        Serial.println(F("---------------------------------------"));
        Serial.print(F("Particles > 0.3um / 0.1L air:")); Serial.println(data.particles_03um);
        Serial.print(F("Particles > 0.5um / 0.1L air:")); Serial.println(data.particles_05um);
        Serial.print(F("Particles > 1.0um / 0.1L air:")); Serial.println(data.particles_10um);
        Serial.print(F("Particles > 2.5um / 0.1L air:")); Serial.println(data.particles_25um);
        Serial.print(F("Particles > 5.0um / 0.1L air:")); Serial.println(data.particles_50um);
        Serial.print(F("Particles > 10 um / 0.1L air:")); Serial.println(data.particles_100um);
        Serial.println(F("---------------------------------------"));
        Serial.println(F("AQI"));
        Serial.print(F("PM2.5 AQI US: ")); Serial.print(data.aqi_pm25_us);
        Serial.print(F("\tPM10  AQI US: ")); Serial.println(data.aqi_pm100_us);
        Serial.println(F("---------------------------------------"));
        Serial.println();

        latestPM1  = data.pm10_standard;
        latestPM25 = data.pm25_standard;
        latestPM10 = data.pm100_standard;
        bmeDone = true;
        // —— 1) CO₂ (non-blocking poll) ——
        uint16_t co2; float t, h;
        sensor.readMeasurement(co2, t, h);
        Serial.print("CO₂ [ppm]: ");    Serial.println(co2);
        Serial.print("Temp_SCD [°C]: "); Serial.println(t, 2);
        Serial.print("RH_SCD [%]: ");    Serial.println(h, 2);
        Serial.println();
        scdDone = true;
        latestCO2  = co2;
        latestTemp = t;
        latestHum  = h;

        // —— 2) BME680 ——
        unsigned long end = bme.beginReading();
        if (end) {
            delay(50);
            if (bme.endReading()) {
                if(latestTemp > bme.temperature) latestTemp = bme.temperature;
                Serial.print("Temp_BME [°C]: ");  Serial.println(bme.temperature, 2);
                Serial.print("Pressure [hPa]: "); Serial.println(bme.pressure / 100.0, 2);
                Serial.print("Humidity [%]: ");   Serial.println(bme.humidity, 2);
                Serial.print("Gas [KΩ]: ");       Serial.println(bme.gas_resistance / 1000.0, 2);
                lastAltitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
                Serial.print("Altitude [m]: ");   Serial.println(lastAltitude, 2);
                Serial.println();
                pmDone = true;
            } else {
                Serial.println("Failed BME680 reading");
            }
        } else {
            Serial.println("Failed to start BME680 reading");
        }
        latestTemp -= 2;
        latestTemp = latestTemp * 1.8 + 32;
        Serial.print("Display Temp in F: ");  Serial.println(latestTemp);

        // —— 3) I2S mic —— ///Users/mark/Documents/PlatformIO/Projects/250519-181440-adafruit_feather_esp32_v2/src/main.cpp
        size_t bytesRead = 0;
        i2s_read(I2S_NUM_0, raw_samples, sizeof(raw_samples), &bytesRead, portMAX_DELAY);
        int count = bytesRead / sizeof(int32_t);
        double sumSq = 0;
        for (int i = 0; i < count; i++) {
            int16_t s = raw_samples[i] >> 8;
            sumSq += double(s) * s;
        }
        double rms = sqrt(sumSq / count);
        double dbfs = 20.0 * log10(rms / 32767.0);
        double dbSPL = dbfs + 40.0;
        Serial.print("Mic [dB SPL]: "); Serial.println(dbSPL, 2);

        latestNoise = 2*dbSPL;

        micDone = true;
        Serial.println("----------------------------");
    }
    if (!readyShown && bmeDone && scdDone && pmDone && micDone) {
        delay(3000);
        showHomeReady();
        readyShown = true;
    }
        bool currA = isButtonPressed(BUTTON_A_PIN);
    bool currB = isButtonPressed(BUTTON_B_PIN);
    bool pressA = currA && !prevA;
    bool pressB = currB && !prevB;

    const int totalPages = PRESSGAS_PAGE + 1;
    if (pressB) {
        Serial.println("pressB");
        currentPage = Page((currentPage - 1 + totalPages) % totalPages);
        redrawPage(currentPage);
        lastButtonPress = millis();
    } else if (pressA) {
        Serial.println("pressA");
        currentPage = Page((currentPage + 1) % totalPages);
        redrawPage(currentPage);
        lastButtonPress = millis();
    }

    if (millis() - lastButtonPress >= 10000) {
        switch (currentPage) {
            case PROJ:            showProjectData();      break;
            case PROJ2:           showProjectData2();     break;
            case AQI_PAGE:        showAQIPage();          break;
            case NOISE_PAGE:      showNoisePage();        break;
            case TempHum_PAGE:    showTempHumPage();      break;
            case Abnormal:    showAbnormal();      break;
            case Overall:   showOverall();         break;

            case ALTITUDECO2_PAGE:showAltitudeCO2Page();  break;
            case PRESSGAS_PAGE:   showPressGasPage();     break;
            case AIR:             showAirQuality();      break;
            case ENV:             showEnvQuality();         break;
            case SUMMARY_PAGE:   showSummaryPage();         break;

            default:              break;
        }
        lastButtonPress = millis();
    }
    prevA = currA;
    prevB = currB;
}



// Redraw helper
void redrawPage(Page p) {
   switch (p) {
       case HOME:     showHomeReady();         break;
       case PROJ:     showProjectData();  break;
       case PROJ2:     showProjectData2();  break;
       case AIR:      showAirQuality();   break;
       case ENV:    showEnvQuality();break;
       case TempHum_PAGE: showTempHumPage(); break;
       case AQI_PAGE: showAQIPage(); break;
       case NOISE_PAGE: showNoisePage(); break;
       case PRESSGAS_PAGE: showPressGasPage(); break;
       case ALTITUDECO2_PAGE: showAltitudeCO2Page(); break;
       case SUMMARY_PAGE: showSummaryPage(); break;
       case Overall: showOverall(); break;
       case Abnormal: showAbnormal(); break;
   }
}


void showHomeLoad() {
   tft.fillScreen(bgColors[bgIndex]);
   tft.setTextSize(3);
   tft.setTextColor(fontColor(), bgColors[bgIndex]);
   tft.setCursor(10, 50);  tft.print("Indoor Monitoring");
   tft.setTextSize(3);
   tft.setTextColor(fontColor(), TFT_RED);
   tft.setCursor(20, 130);  tft.print("Loading...");}


void showHomeReady() {
    tft.fillScreen(bgColors[bgIndex]);
    tft.setTextSize(3);
    tft.setTextColor(fontColor(), bgColors[bgIndex]);
    tft.setCursor(10, 50);  tft.print("Indoor Monitoring");
    tft.setTextSize(2);
    tft.fillCircle(20, 100, 10, TFT_GREEN);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(60, 97);  tft.print("Ready");
    tft.setTextSize(2);
    tft.drawFastHLine(0, 130, 300, fontColor());
    tft.setTextColor(fontColor(), bgColors[bgIndex]);
    tft.setCursor(20,140);  tft.print("Up Button: Next Page");
    //tft.setCursor(20,160);  tft.print("Down Button: Previous Page");
 }




void showProjectData() {
    tft.fillScreen(bgColors[bgIndex]);
    tft.setTextColor(fontColor(), bgColors[bgIndex]);

    tft.setTextSize(2);
    tft.setCursor(10, 5);
    tft.print("Indoor Monitoring");
    tft.setTextSize(1);
    tft.setCursor(235, 10);
    tft.print("uptime:");
    tft.print((millis()-startMillis)/1000);
    tft.print("s");

   // tft.printf("uptime %s",(millis()-startMillis)/1000);

    // card
    const int w = 130, h = 70, gap = 10;
    int x = 10, y = 40;

    // 2) CO₂ 
    tft.fillRect(x, y, w, h, TFT_DARKGREY);
    tft.setTextSize(2);
    tft.setCursor(x+5, y+5);      tft.print("CO2");
    tft.setTextSize(2);
    tft.setCursor(x+50, y+5);     tft.printf("%.0f", readCO2());
    tft.setTextSize(1);           tft.print("ppm");

    // 3) Temp 
    x += w + gap;
    tft.fillRect(x, y, w, h, TFT_NAVY);
    tft.setTextSize(2);
    tft.setCursor(x+5, y+5);      tft.print("Temp");
    tft.setTextSize(2);
    tft.setCursor(x+60, y+5);     tft.printf("%.1f", readTemperature());
    tft.setTextSize(1);           tft.print("F");

    x = 10;
    y += h + gap;

    tft.fillRect(x, y, w, h, TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(x+5, y+5);      tft.print("Humidity");
    tft.setTextSize(2);
    tft.setCursor(x+70, y+25);     tft.printf("%.0f%", readHumidity());
    tft.setTextSize(1);           tft.print("%");

    x += w + gap;
    int pm25 = readPM25();
    auto a = computeAQI_PM25(pm25);
    tft.fillRect(x, y, w, h, a.color);
    tft.setTextSize(2);
    tft.setCursor(x+5, y+5);      tft.print("PM2.5:");
    tft.setTextSize(2);
    tft.setCursor(x+80, y+5);     tft.printf("%d", pm25);

    tft.drawFastHLine(0, 200, 300, fontColor());
    tft.setTextSize(1);
    tft.setCursor(10, 205);    tft.print("Up Button: Next Page");
    tft.setCursor(140,205);    tft.print("Down Button: Previous Page");
}


void showProjectData2() {
    tft.fillScreen(bgColors[bgIndex]);
    tft.setTextColor(fontColor(), bgColors[bgIndex]);

    tft.setTextSize(2);
    tft.setCursor(10, 5);
    tft.print("Indoor Monitoring");
    tft.setTextSize(1);
    tft.setCursor(235, 10);

    tft.print("uptime:");
    tft.print((millis()-startMillis)/1000);
    tft.print("s");

    const int w = 130, h = 70, gap = 10;
    int x = 10, y = 40;

    // 1) Pressure
    float p = bme.pressure / 100.0;
    tft.fillRect(x, y, w, h, TFT_SKYBLUE);
    tft.setTextSize(2);
    tft.setCursor(x + 5, y + 5);      tft.print("Pressure");
    tft.setCursor(x + 40, y + 25);     tft.printf("%.1f", p);
    tft.setTextSize(1);               tft.print("hPa");

    // 2) Gas
    x += w + gap;
    float g = bme.gas_resistance / 1000.0;
    tft.fillRect(x, y, w, h, TFT_DARKGREEN);
    tft.setTextSize(2);
    tft.setCursor(x + 5, y + 5);      tft.print("Gas");
    tft.setCursor(x + 50, y + 5);     tft.printf("%.1f", g);
    tft.setTextSize(1);               tft.print("kΩ");

    // 3) AQI
    x = 10;
    y += h + gap;
    int pm25 = readPM25();
    auto a = computeAQI_PM25(pm25);
    tft.fillRect(x, y, w, h, TFT_SILVER);
    tft.setTextSize(2);
    tft.setCursor(x + 5, y + 5);      tft.print("AQI ");
    tft.setCursor(x + 70, y + 5);     tft.printf("%d", a.aqi);
    tft.setTextSize(2);

    tft.setCursor(x + 5, y + 40);     tft.print(a.category);

    // 4) Noise
    x += w + gap;
    float db = latestNoise;
    tft.fillRect(x, y, w, h, TFT_ORANGE);
    tft.setTextSize(2);
    tft.setCursor(x + 5, y + 5);      tft.print("Noise");
    tft.setCursor(x + 60, y + 25);     tft.printf("%.1f", db);
    tft.setTextSize(1);               tft.print("dB");

    tft.drawFastHLine(0, 200, 300, fontColor());
    tft.setTextSize(1);
    tft.setCursor(10, 205);    tft.print("Up Button: Next Page");
    tft.setCursor(140, 205);   tft.print("Down Button: Previous Page");
}


void showAirQuality() {
   tft.fillScreen(bgColors[bgIndex]);
   tft.setTextSize(2);
   tft.setTextColor(fontColor(), bgColors[bgIndex]);
   int y = startY;
   auto aqi = computeAQI_PM25(readPM25());
   tft.setCursor(4, y);    tft.printf("AQI: %d %s", aqi.aqi, aqi.category); y += lineH;
   tft.setCursor(4, y);    tft.printf("PM1.0: %d ug/m3", readPM1());       y += lineH;
   tft.setCursor(4, y);    tft.printf("PM2.5: %d ug/m3", readPM25());      y += lineH;
   tft.setCursor(4, y);    tft.printf("PM10: %d ug/m3", readPM10());       y += lineH;
   tft.setCursor(4, y);    tft.printf("CO2: %.0f ppm", readCO2());         y += lineH;
   tft.setCursor(4, y);    tft.printf("Gas: %.1f k", (bme.gas_resistance / 1000.0)); y += lineH;
   tft.setCursor(4, y);    tft.printf("Pressure: %.1f hpa", (bme.pressure / 100.0)); y += lineH;
   tft.drawFastHLine(0, 200, 300, fontColor());
   tft.setTextSize(1);
   tft.setCursor(10, 203);    tft.print("Indoor Air Quality");
   tft.setCursor(10, 215);    tft.print("Up Button: Next Page");
   tft.setCursor(140, 215);   tft.print("Down Button: Previous Page");
}

void showEnvQuality() {
    tft.fillScreen(bgColors[bgIndex]);
    tft.setTextSize(2);
    tft.setTextColor(fontColor(), bgColors[bgIndex]);
    int y = startY;
    auto aqi = computeAQI_PM25(readPM25());
    float alt = readAltitude();
    tft.setCursor(4, y);    tft.printf("Sound: %.1f dB", readNoise());     y += lineH;
    tft.setCursor(4, y);    tft.printf("Temperature: %.1f F", readTemperature()); y += lineH;
    tft.setCursor(4, y);    tft.printf("Humidity: %.1f%%", readHumidity()); y += lineH;
    tft.setCursor(4, y);    tft.printf("Altitude: %.0f m", alt);  y += lineH;

    tft.drawFastHLine(0, 200, 300, fontColor());
    tft.setTextSize(1);
    tft.setCursor(10, 203);    tft.print("Indoor Environmental Quality");
    tft.setCursor(10, 215);    tft.print("Up Button: Next Page");
    tft.setCursor(140, 215);   tft.print("Down Button: Previous Page");
 }


 void showTempHumPage() {
    tft.fillScreen(TFT_DARKGREY);

    // Temperature section
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 10);
    tft.print("Temp:");
    tft.setTextSize(4);
    tft.setCursor(10, 40);
    tft.printf("%.1f F", latestTemp);
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, 90);
    tft.print(tempAdvice(latestTemp));

    // Draw divider line
    tft.drawFastHLine(0, 120, 300, fontColor());

    // Humidity section
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 140);
    tft.print("Humidity:");
    tft.setTextSize(4);
    tft.setCursor(10, 170);
    tft.printf("%.0f%%", latestHum);
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, 220);
    tft.print(humAdvice(latestHum));
}


void showAQIPage() {
    AQIResult r = computeAQI_PM25(latestPM25);

    tft.fillScreen(TFT_PINK);
    tft.setTextColor(TFT_BLACK);

    tft.setTextSize(3);
    tft.setCursor(10, 10);
    tft.print("Air Quality");

    tft.setTextSize(4);
    tft.setCursor(10, 50);
    tft.printf("AQI %d", r.aqi);

    tft.setTextSize(3);
    tft.setCursor(75, 100);
    tft.print(r.category);
    tft.fillCircle(33, 105, 23, r.color);

    tft.setTextSize(2);
    tft.setCursor(10, 145);
    tft.printf("PM1.0 : %d", latestPM1);
    tft.setCursor(10, 165);
    tft.printf("PM2.5 : %d", latestPM25);
    tft.setCursor(10, 185);
    tft.printf("PM10  : %d", latestPM10);
}

void showNoisePage() {
    tft.fillScreen(TFT_BROWN);

    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 10);
    tft.print("Noise Level");

    tft.setTextSize(4);
    tft.setCursor(10, 50);
    tft.printf("%.1f dB", latestNoise);

    tft.drawFastHLine(0, 100, 300, fontColor());

    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, 110);
    if (latestNoise <= 30) {
        tft.setCursor(10, 110);
        tft.print("Very Quiet");
        tft.setCursor(10, 130);
        tft.print("Ideal for study");
    } else if (latestNoise <= 55) {
        tft.setCursor(10, 110);
        tft.print("Moderate");
        tft.setCursor(10, 130);
        tft.print("Normal conversation");
    } else if (latestNoise <= 70) {
        tft.setCursor(10, 110);
        tft.print("Loud");
        tft.setCursor(10, 130);
        tft.print("May cause fatigue");
    } else if (latestNoise <= 85) {
        tft.setCursor(10, 110);
        tft.print("Very Loud: limit exposure");
        tft.setCursor(10, 130);
        tft.print("Limit exposure");
    } else {
        tft.setCursor(10, 110);
        tft.print("Danger");
        tft.setCursor(10, 130);
        tft.print("hearing risk!");
    }

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 160);
    tft.print("Tips:");
    tft.setTextSize(1);
    tft.setCursor(10, 180);
    tft.print("- Avoid long exposure above 85dB");
    tft.setCursor(10, 195);
    tft.print("- Use earplugs in loud environments");
}

void showPressGasPage() {
    float p = bme.pressure / 100.0;
    float g = bme.gas_resistance / 1000.0;

    tft.fillScreen(TFT_DARKCYAN);

    // Pressure section
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 10);
    tft.print("Pressure:");
    tft.setTextSize(4);
    tft.setCursor(10, 40);
    tft.printf("%.1f hPa", p);
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, 90);
    if (p < 1000)
        tft.print("Low: headaches");
    else if (p > 1030)
        tft.print("High: discomfort");
    else
        tft.print("Normal Range");

    //draw line
    tft.drawFastHLine(0, 120, 300, fontColor());
    tft.setTextSize(1);
    // Gas section
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 140);
    tft.print("Gas:");
    tft.setTextSize(4);
    tft.setCursor(10, 170);
    tft.printf("%.1f k", g);
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, 220);
    tft.print(g > 50 ? "Pollutants high" : "Air is fresh");
}

void showAltitudeCO2Page() {
    float alt = readAltitude();

    tft.fillScreen(TFT_PURPLE);

    // Altitude section
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 10);
    tft.print("Altitude:");
    tft.setTextSize(4);
    tft.setCursor(10, 40);
    tft.printf("%.0f m", alt);
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, 90);
    tft.print(alt > 2000 ? "High ALT: Low Oxygen" : "Sea level range: Good");

    // Divider line
    tft.drawFastHLine(0, 120, 300, fontColor());

    // CO2 section
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 140);
    tft.print("CO2:");
    tft.setTextSize(4);
    tft.setCursor(10, 170);
    tft.printf("%.0f ppm", latestCO2);
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(10, 220);
    tft.print(latestCO2 <= 1000 ? "Comfortable" : "High: Ventilate!");
}


void showAbnormal() {
    tft.fillScreen(bgColors[bgIndex]);

    // Title
    tft.setTextSize(3); 
    tft.setTextColor(TFT_WHITE); 
    tft.setCursor(10, 10);
    tft.print("Abnormal Report");

    // Divider
    tft.drawLine(10, 45, 320, 45, TFT_WHITE);  

    int y = 60; 
    bool any = false;

    // Temperature
    if (latestTemp < 65 || latestTemp > 80) {
        any = true;
        tft.setTextSize(2);
        if (latestTemp < 65) tft.setTextColor(TFT_SKYBLUE); // cold
        else tft.setTextColor(TFT_ORANGE);            // hot
        tft.setCursor(10, y);
        tft.print("Temp     : ");
        tft.print(latestTemp, 1);
        tft.print(" F");
        y += 22;
    }

    // Humidity
    if (latestHum < 25 || latestHum > 75) {
        any = true;
        tft.setTextColor(TFT_YELLOW);
        tft.setCursor(10, y);
        tft.print("Humidity : ");
        tft.print(latestHum, 0);
        tft.print(" %");
        y += 22;
    }

    // CO2
    if (latestCO2 > 1200) {
        any = true;
        tft.setTextColor(TFT_RED);
        tft.setCursor(10, y);
        tft.print("CO2  : ");
        tft.print(latestCO2);
        tft.print(" ppm");
        y += 22;
    }

    // PM2.5
    if (latestPM25 > 35) {
        any = true;
        tft.setTextColor(TFT_MAGENTA);
        tft.setCursor(10, y);
        tft.print("PM2.5   : ");
        tft.print(latestPM25);
        y += 22;
    }

    // Noise
    if (latestNoise > 45) {
        any = true;
        tft.setTextColor(TFT_ORANGE);
        tft.setCursor(10, y);
        tft.print("Noise  : ");
        tft.print(latestNoise, 0);
        tft.print("dB");
        y += 22;
    }

    // All normal
    if (!any) {
        tft.setTextColor(TFT_GREEN);
        tft.setTextSize(3); 
        tft.setCursor(10, y);
        tft.print("All normal");
    }
}


void showOverall() {
    tft.fillScreen(bgColors[bgIndex]);
    tft.setTextSize(3); 
    tft.setTextColor(TFT_WHITE); 
    tft.setCursor(10, 10);
    tft.println("Overall Report");
    tft.drawLine(0, 45, 320, 45, TFT_WHITE);  

    int y = 50;
    bool any = false;
    bool okForStudy = true;

    // Temperature in Fahrenheit
    if (latestTemp < 65 || latestTemp > 80) {
        any = true;
        okForStudy = false;
        tft.setTextSize(2); 
        if (latestTemp < 64) {
            tft.setTextColor(TFT_SKYBLUE);
            tft.setCursor(15, y);
            tft.println("Too Cold");
        } else {
            tft.setTextColor(TFT_RED);
            tft.setCursor(15, y);
            tft.println("Too Hot");
        }
        y += 20;
    }

    // Humidity
    if (latestHum < 25) {
        any = true;
        okForStudy = false;
        tft.setTextColor( TFT_ORANGE);
        tft.setCursor(15, y);
        tft.println("Too Dry");
        y += 20;
    } else if (latestHum > 75) {
        any = true;
        okForStudy = false;
        tft.setTextColor(TFT_SKYBLUE);
        tft.setCursor(15, y);
        tft.println("Too Humid");
        y += 20;
    }

    // CO2
    if (latestCO2 > 1200) {
        any = true;
        okForStudy = false;
        tft.setTextColor(TFT_PURPLE);
        tft.setCursor(15, y);
        tft.println("Too Much CO2");
        y += 20;
    }

    // PM2.5
    if (latestPM25 > 35) {
        any = true;
        okForStudy = false;
        tft.setTextColor(TFT_DARKGREY);
        tft.setCursor(15, y);
        tft.println("Dusty Air");
        y += 20;
    }

    // Noise
    if (latestNoise > 45) {
        any = true;
        okForStudy = false;
        tft.setTextColor(TFT_ORANGE);
        tft.setCursor(15, y);
        tft.println("Too Noisy");
        y += 20;
    }

    // All normal
    if (!any) {
        tft.setTextColor(TFT_GREEN);
        tft.setTextSize(2);
        tft.setCursor(10, y);
        tft.print("All normal");
        y += 30;
    }

    // Overall Evaluation
    tft.setTextSize(3);
    tft.drawFastHLine(0, y, 320, TFT_WHITE);
    tft.drawFastHLine(0, y+2, 320, TFT_WHITE);

    if (okForStudy) {
        tft.setTextColor(TFT_GREEN);
        tft.setCursor(15, y+3);
        tft.print("=Good to study=");
        //tft.fillCircle(16, y+22, 12, TFT_GREEN);
        tft.setTextSize(2);
        tft.setCursor(32, y + 37);
        tft.print("--- Let's Study ---");
    } else {
        tft.setTextColor(TFT_RED);
        tft.setCursor(18, y+8);
        tft.print("= Bad to study =");
        //tft.fillCircle(16, y+22, 12, TFT_RED);
        tft.setTextSize(2);
        tft.setCursor(28, y + 37);
        tft.setTextColor(TFT_YELLOW);
        tft.print("--- Avoid this Room ---");
    }
}

void showSummaryPage(){
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);

    const int radius     = 40;
    const int innerR     = radius - 6;
    const int startAngle = 90;
    const int sweepAngle = 320;

    const int X[5] = {  60, 160, 260, 60, 260 };
    const int Y[5] = {  60,  60,  60, 170, 170 };

    struct Gauge {
      const char* label;
      float val, minV, maxV;
      float normMin, normMax;
    };
    AQIResult aqi = computeAQI_PM25(latestPM25);
    Gauge gs[5] = {
      { "AQI",  aqi.aqi,      0,   125,  0,   75 },
      { "Noise",latestNoise,  0,   100,  0,   45 },
      { "Temp", latestTemp,  0,    110,  64,  80 },
      { "Hum",  latestHum,   0,    100,  35,  70 },
      { "CO2",  latestCO2,  0,  2000, 0, 1200 }
    };

    auto drawGauge = [&](int x, int y, Gauge &g) {
        tft.drawArc(x, y, radius, innerR,
                    startAngle, startAngle + sweepAngle,
                    TFT_SKYBLUE, TFT_BLUE, true);

        // cal pin color
        uint16_t ptrCol;
        if (g.val < g.minV || g.val > g.maxV) {
            ptrCol = TFT_RED;
        }
        else if (g.val >= g.normMin && g.val <= g.normMax) {
            ptrCol = TFT_GREEN;
        }
        else {
            ptrCol = TFT_YELLOW;
        }

        // pin location
        float norm = constrain((g.val - g.minV)/(g.maxV - g.minV), 0.0, 1.0);
        int ang = startAngle + int(norm * sweepAngle);
        float rad = ang * PI/180.0;
        int x2p = x + cos(rad)*(radius-8);
        int y2p = y + sin(rad)*(radius-8);
        tft.drawLine(x, y, x2p, y2p, ptrCol);
        tft.fillCircle(x, y, 3, ptrCol);

        // label and values
        char buf[16];
        snprintf(buf, sizeof(buf), "%s: %.0f", g.label, g.val);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE);
        tft.drawString(buf, x, y + radius + 10);
    };

    for (int i = 0; i < 5; i++) {
        drawGauge(X[i], Y[i], gs[i]);
    }
    float score = 100;

    // PM2.5
    if (latestPM25 <= 15)
        score -= 0;
    else if (latestPM25 <= 35)
        score -= 5;
    else if (latestPM25 <= 75)
        score -= 15;
    else if (latestPM25 <= 150)
        score -= 70;
    else
        score -= 100;

    // CO2
    if (latestCO2 <= 800)
        score -= 0;
    else if (latestCO2 <= 1200)
        score -= 15;
    else if (latestCO2 <= 1500)
        score -= 30;
    else if (latestCO2 <= 2000)
        score -= 60;
    else
        score -= 100;

    // Noise
    if (latestNoise <= 35)
        score -= 0;
    else if (latestNoise <= 45)
        score -= 5;
    else if (latestNoise <= 60)
        score -= 15;
    else if (latestNoise <= 80)
        score -= 30;
    else
        score -= 40;

    // temp
    if (latestTemp < 65 || latestTemp > 80) {
        score -= 10;
        if (latestTemp < 60 || latestTemp > 85)
            score -= 10;
    }
    if (latestTemp < 55 || latestTemp > 90) {
        score -= 20;
        if (latestTemp < 50 || latestTemp > 95)
            score -= 25;
    }
    // hum
    if (latestHum < 25 || latestHum > 75) {
        score -= 10;
        if (latestHum < 15 || latestHum > 85)
            score -= 20;
    }
    //if(score <=  0) score = 0;
    if(score < 80) score*=1.2;
    score = constrain(score, 0.0f, 100.0f);


    // color for score
    const char* comfortLabel;
    uint16_t col;
    if (score > 70) {
        comfortLabel = "Comfort";
        col = TFT_GREEN;
    } else if (score > 50) {
        comfortLabel = "Mild";
        col = 0xAFE5; 
    } else if (score > 30) {
        comfortLabel = "Poor";
        col = TFT_ORANGE;
    } else if (score > 15) {
        comfortLabel = "Bad";
        col = 0xFD20; 
    } else {
        comfortLabel = "Danger";
        col = TFT_RED;
    }

    // show label
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(col);
    tft.drawString(comfortLabel, 160, 150);

    // score
    tft.setTextSize(2);
    tft.drawString("SCORE", 160, 220);

    //for fake score test
    // score = xx;
    tft.setTextSize(4);
    tft.drawNumber(int(score), 160, 185);
}
