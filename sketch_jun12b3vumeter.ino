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

// Array per evitare di calcolare la trigonometria in tempo reale
float cosOut[MAX_BARS];
float sinOut[MAX_BARS];
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
  
  // Precalcola angoli e colori una volta per tutte
  for(int i = 0; i < MAX_BARS; i++) {
    oldLevels[i] = 0;
    
    // Angolo in radianti per ogni barra (distribuite sui 360 gradi)
    float angle = (i * 360.0 / MAX_BARS) * (PI / 180.0);
    
    // Il display ha la Y invertita (0 in alto), quindi mettiamo il meno sul seno
    cosOut[i] = cos(angle);
    sinOut[i] = -sin(angle); 
    
    // Assegna a ciascuna barra un colore fisso dall'arcobaleno
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
      
      // Mappa i valori nella lunghezza fisica
      int oldH = map(oldLevels[i], 0, MAX_VALUE, 0, R_MAX_ADD);
      int newH = map(newLevels[i], 0, MAX_VALUE, 0, R_MAX_ADD);
      
      // Calcola i punti solo se serve aggiornare
      if (newH > oldH) {
        // La barra si ALLUNGA: disegna l'aggiunta colorata
        int x1 = CX + (R_MIN + oldH) * cosOut[i];
        int y1 = CY + (R_MIN + oldH) * sinOut[i];
        int x2 = CX + (R_MIN + newH) * cosOut[i];
        int y2 = CY + (R_MIN + newH) * sinOut[i];
        
        // Disegno la riga 3 volte con un leggero offset per renderla più spessa
        tft.drawLine(x1, y1, x2, y2, barColors[i]);
        tft.drawLine(x1+1, y1, x2+1, y2, barColors[i]);
        tft.drawLine(x1, y1+1, x2, y2+1, barColors[i]);
      } 
      else if (newH < oldH) {
        // La barra si RITIRA: colora di nero la parte eccedente
        int x1 = CX + (R_MIN + newH) * cosOut[i];
        int y1 = CY + (R_MIN + newH) * sinOut[i];
        int x2 = CX + (R_MIN + oldH) * cosOut[i];
        int y2 = CY + (R_MIN + oldH) * sinOut[i];
        
        tft.drawLine(x1, y1, x2, y2, TFT_BLACK);
        tft.drawLine(x1+1, y1, x2+1, y2, TFT_BLACK);
        tft.drawLine(x1, y1+1, x2, y2+1, TFT_BLACK);
      }
      
      oldLevels[i] = newLevels[i];
    }
  }
}