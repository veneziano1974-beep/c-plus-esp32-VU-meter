#include <SPI.h>
#include <TFT_eSPI.h>
#include <math.h>

TFT_eSPI tft = TFT_eSPI();

#define MAX_BARS 32
#define MAX_VALUE 255

// Parametri geometrici dello schermo (CYD 320x240)
#define CX 160          // Centro X 
#define CY 120          // Centro Y
#define R_MIN 45        // Raggio del "buco" nero centrale
#define R_MAX_ADD 70    // Lunghezza massima dell'escursione della barra

uint8_t oldLevels[MAX_BARS];
uint8_t newLevels[MAX_BARS];

// Due set di array: uno per la metà destra, uno per la metà sinistra
float cosR[MAX_BARS];
float sinR[MAX_BARS];
float cosL[MAX_BARS];
float sinL[MAX_BARS];
uint16_t barColors[MAX_BARS];

// Funzione di supporto per creare un arcobaleno perfetto
uint16_t getRainbowColor(byte position) {
  byte r, g, b;
  if (position < 85) {
    r = 255 - position * 3; g = 0; b = position * 3;
  } else if (position < 170) {
    position -= 85; r = 0; g = position * 3; b = 255 - position * 3;
  } else {
    position -= 170; r = position * 3; g = 255 - position * 3; b = 0;
  }
  return tft.color565(r, g, b);
}

void setup() {
  Serial.begin(115200); 
  Serial.setTimeout(50);
  
  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true); // Croma inverso dei CYD
  tft.fillScreen(TFT_BLACK);
  
  // Precalcola angoli e colori per la simmetria
  for(int i = 0; i < MAX_BARS; i++) {
    oldLevels[i] = 0;
    
    // I bassi (i=0) partono dal basso centrale, ovvero a 90 gradi (PI / 2)
    // Distribuiamo i 32 valori su un arco di 180 gradi (PI) per lato
    float angleRight = (PI / 2.0) - (i * PI / MAX_BARS);
    float angleLeft  = (PI / 2.0) + (i * PI / MAX_BARS);
    
    // Salviamo le coordinate precalcolate
    cosR[i] = cos(angleRight);
    sinR[i] = sin(angleRight);
    
    cosL[i] = cos(angleLeft);
    sinL[i] = sin(angleLeft);
    
    // Colore uguale per le barre speculari
    byte colorPos = (i * 256 / MAX_BARS) & 255;
    barColors[i] = getRainbowColor(colorPos);
  }

  // Disegna l'anello centrale decorativo
  tft.drawCircle(CX, CY, R_MIN - 2, TFT_DARKGREY);
}

void loop() {
  if (Serial.available() > 0) {
    if (Serial.read() == 0xFF) {
      
      uint32_t t = millis();
      while(Serial.available() == 0 && millis() - t < 10) {} 
      
      if (Serial.available() > 0 && Serial.read() == 0xAA) {
        
        t = millis();
        while(Serial.available() == 0 && millis() - t < 10) {}
        
        if (Serial.available() > 0) {
          uint8_t count = Serial.read(); 
          
          if (count > 0 && count <= MAX_BARS) {
            int received = Serial.readBytes((char*)newLevels, count);
            
            if (received == count) {
              drawRadialMeter(count);
            }
          }
        }
      }
    }
  }
}

void drawRadialMeter(uint8_t count) {
  for (int i = 0; i < count; i++) {
    if (oldLevels[i] != newLevels[i]) {
      
      int oldH = map(oldLevels[i], 0, MAX_VALUE, 0, R_MAX_ADD);
      int newH = map(newLevels[i], 0, MAX_VALUE, 0, R_MAX_ADD);
      
      if (newH > oldH) {
        // --- DISEGNA IL LATO DESTRO ---
        int x1R = CX + (R_MIN + oldH) * cosR[i];
        int y1R = CY + (R_MIN + oldH) * sinR[i];
        int x2R = CX + (R_MIN + newH) * cosR[i];
        int y2R = CY + (R_MIN + newH) * sinR[i];
        
        tft.drawLine(x1R, y1R, x2R, y2R, barColors[i]);
        tft.drawLine(x1R+1, y1R, x2R+1, y2R, barColors[i]);
        tft.drawLine(x1R, y1R+1, x2R, y2R+1, barColors[i]);

        // --- DISEGNA IL LATO SINISTRO ---
        int x1L = CX + (R_MIN + oldH) * cosL[i];
        int y1L = CY + (R_MIN + oldH) * sinL[i];
        int x2L = CX + (R_MIN + newH) * cosL[i];
        int y2L = CY + (R_MIN + newH) * sinL[i];
        
        tft.drawLine(x1L, y1L, x2L, y2L, barColors[i]);
        tft.drawLine(x1L-1, y1L, x2L-1, y2L, barColors[i]);
        tft.drawLine(x1L, y1L+1, x2L, y2L+1, barColors[i]);
      } 
      else if (newH < oldH) {
        // --- CANCELLA IL LATO DESTRO ---
        int x1R = CX + (R_MIN + newH) * cosR[i];
        int y1R = CY + (R_MIN + newH) * sinR[i];
        int x2R = CX + (R_MIN + oldH) * cosR[i];
        int y2R = CY + (R_MIN + oldH) * sinR[i];
        
        tft.drawLine(x1R, y1R, x2R, y2R, TFT_BLACK);
        tft.drawLine(x1R+1, y1R, x2R+1, y2R, TFT_BLACK);
        tft.drawLine(x1R, y1R+1, x2R, y2R+1, TFT_BLACK);

        // --- CANCELLA IL LATO SINISTRO ---
        int x1L = CX + (R_MIN + newH) * cosL[i];
        int y1L = CY + (R_MIN + newH) * sinL[i];
        int x2L = CX + (R_MIN + oldH) * cosL[i];
        int y2L = CY + (R_MIN + oldH) * sinL[i];
        
        tft.drawLine(x1L, y1L, x2L, y2L, TFT_BLACK);
        tft.drawLine(x1L-1, y1L, x2L-1, y2L, TFT_BLACK);
        tft.drawLine(x1L, y1L+1, x2L, y2L+1, TFT_BLACK);
      }
      
      oldLevels[i] = newLevels[i];
    }
  }
}