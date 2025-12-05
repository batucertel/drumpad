#include <MIDIUSB.h>

// CONFIG
const int piezoPins[2] = {A0, A2};   // Two piezos
const int noteNumbers[2] = {36, 38}; // Assign a MIDI note for each piezo

// Sensitivity tuning
int thresholdHighA0 = 70;   
int thresholdHighA2  = 90;   

int thresholdLow  = 40;      // noise floor

unsigned long holdTime     = 15;   // ms (piezo active window)
unsigned long lockoutTime  = 60;   // ultra fast lockout

bool piezoActive[2] = {false, false};
unsigned long hitTime[2] = {0, 0};
unsigned long lastHit[2] = {0, 0};

// ----------------------------------------------------
// Better peak detection (4 fast samples)
// ----------------------------------------------------
int getPeak(int piezoPin) {
  int peak = 0;
  for (int i = 0; i < 4; i++) {
    int r = analogRead(piezoPin);
    if (r > peak) peak = r;
    delayMicroseconds(200);
  }
  return peak;
}

// ----------------------------------------------------
void setup() {
  pinMode(piezoPins[0], INPUT);
  pinMode(piezoPins[1], INPUT);
  Serial.begin(115200);
}

// ----------------------------------------------------
void loop() {
  unsigned long now = millis();

  for (int i = 0; i < 2; i++) {
    int reading = analogRead(piezoPins[i]);

    // SELECT correct thresholdHigh per piezo
    int thresholdHigh = (i == 0 ? thresholdHighA0 : thresholdHighA2);

    // Ultra-fast lockout
    if (now - lastHit[i] < lockoutTime) continue;

    // RISING EDGE (hit detected)
    if (!piezoActive[i] && reading >= thresholdHigh) {
      int peak = getPeak(piezoPins[i]);

      // Lower sensitivity velocity curve
      int velocity = map(peak, thresholdHigh, 1023, 1, 110);
      velocity = constrain(velocity, 1, 110);

      sendNoteOn(noteNumbers[i], velocity);
      MidiUSB.flush();

      Serial.print("Piezo ");
      Serial.print(i);
      Serial.print(" vel=");
      Serial.println(velocity);
      Serial.print(" NOTE= ");
      Serial.println(noteNumbers[i]);

      piezoActive[i] = true;
      hitTime[i] = now;
      lastHit[i] = now;
    }

    // RELEASE / RESET
    if (piezoActive[i] && (reading <= thresholdLow || now - hitTime[i] > holdTime)) {
      piezoActive[i] = false;
    }
  }
}

// ----------------------------------------------------
void sendNoteOn(byte note, byte velocity) {
  midiEventPacket_t msg = {0x09, 0x90, note, velocity};
  MidiUSB.sendMIDI(msg);
}
