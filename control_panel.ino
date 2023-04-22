#include <Keyboard.h>
 
// Knopjes
const int power = 13;
const int dis1 = 2;
const int dis2 = 3;
const int gates = 4;
const int restraints = 5;
const int estop = 6;
const int reset = 7;
const int functie1 = 8;
 
// Lampjes
const int dis1l = 9;
const int dis2l = 10;
const int resetl = 11;
const int functionl = 12;
 
// Opties
bool gatesOpen = false; // Poortjes zijn dicht
bool resOpen = false; // Beugels zijn dicht
bool estopped = false; // Zou t raar vinden als ie ge estopped begint?
bool canDispatch = true; // Zie onderstaand
bool trainParked = true; // Je begint altijd met trein in dispatch modus
bool lightTest = false; // Bij true zou de lamptest al gedaan zijn
bool enableStationCTRL = true; // NOOIT OP FALSE ZETTEN
bool systemError = false; // Als je deze aan zou zetten zou je moeten beginnen met een reset
bool keyboardState = false;
bool preLoad = true; // In principe zou je vanaf het begin meteen moeten kunnen preloaden
 
// Dispatch
const long DispatchDelay = 10000; // 10 seconden voor de trein weer vrijgegeven kan worden
unsigned long DispatchStartTime = 0;

// PreLoad
const long PreLoadDelay = 10000; // 10 seconden tussen enter functies
unsigned long PreLoadStartTime = 0;
 
// Ledjes
unsigned long currentMillis;
unsigned long previousBlinkMillis = 0;
const long blinkInterval = 500; // led interval, 25ms is fury, 500 is normaal
int ledState = LOW; // Led begint altijd uit
 
void setup() {
  // Pins knopjes
  pinMode(power, INPUT_PULLUP);
  pinMode(dis1, INPUT_PULLUP);
  pinMode(dis2, INPUT_PULLUP);
  pinMode(gates, INPUT_PULLUP);
  pinMode(restraints, INPUT_PULLUP);
  pinMode(estop, INPUT_PULLUP);
  pinMode(reset, INPUT_PULLUP);
  pinMode(functie1, INPUT_PULLUP);
 
  // Pins lampjes
  pinMode(dis1l, OUTPUT);
  pinMode(dis2l, OUTPUT);
  pinMode(resetl, OUTPUT);
  pinMode(functionl, OUTPUT);
}
 
// E-stop reset
void emergencyReset() {
  estopped = false;
  digitalWrite(resetl, LOW);
  Serial.write("RESETTING...\n");
  Keyboard.press(0x72);
  delay(5000);
  Serial.write("RESET!\n");
  Keyboard.releaseAll();
  delay(200);
}

// Reset fault doet nog niks
void faultReset() {
  systemError = false;
  digitalWrite(resetl, LOW);
  Serial.write("RESET!\n");
  delay(200);
}
 
// E-stop
void emergency() {
  Keyboard.press(0x73);
  delay(200);
  Keyboard.releaseAll();
 
  estopped = true;
  digitalWrite(resetl, HIGH);
  digitalWrite(dis1l, LOW);
  digitalWrite(dis2l, LOW);
  Serial.write("E-STOPPED\n");
 
  while (estopped == true) {
    if (digitalRead(reset) == LOW && digitalRead(estop) == LOW) {
      emergencyReset();
    }
    delay(300);
  }
}

// Vrijgave
void dispatch() {
  Keyboard.press(' ');
  Serial.write("DISPATCHED");
  delay(5000);
  Keyboard.releaseAll();
  trainParked = false;
  DispatchStartTime = currentMillis;
}

// Functie/ advance
void functie() {
  Keyboard.press(KEY_RETURN);
  Serial.write("ADVANCED");
  delay(1000);
  Keyboard.releaseAll();
  preLoad = false;
  PreLoadStartTime = currentMillis;
}
 
// Poortjes
void openGates() {
  if (enableStationCTRL) {
    Serial.write("OPEN GATES\n");
    Keyboard.press(KEY_RIGHT_ARROW);
    gatesOpen = true;
    delay(2000);
    Keyboard.releaseAll();
  } else {
    systemError = true;
  }
}
 
void closeGates() {
  if (enableStationCTRL) {
    Serial.write("CLOSE GATES\n");
    Keyboard.press(KEY_LEFT_ARROW);
    gatesOpen = false;
    delay(2000);
    Keyboard.releaseAll();
  } else {
    systemError = true;
  }
}
 
// Beugels
void openRestraints() {
  if (enableStationCTRL) {
    Serial.write("OPEN RESTRAINTS\n");
    Keyboard.press(KEY_UP_ARROW);
    resOpen = true;
    delay(1000);
    Keyboard.releaseAll();
  } else {
    systemError = true;
  }
}
 
void closeRestraints() {
  if (enableStationCTRL) {
    Serial.write("CLOSE RESTRAINTS\n");
    Keyboard.press(KEY_DOWN_ARROW);
    resOpen = false;
    delay(1000);
    Keyboard.releaseAll();
  } else {
    systemError = true;
  }
}

// Controleer de statussen van de knoppen, en zet timers na vrijgaves en functiekeys
void updateStates() {
  // Dispatch
  if ((currentMillis - DispatchStartTime >= DispatchDelay) || DispatchStartTime == 0) {
    trainParked = true;
  }

  // PreLoad
  if ((currentMillis - PreLoadStartTime >= PreLoadDelay) || PreLoadStartTime == 0) {
    preLoad = true;
  }

  // Controleer of er gedispatcht kan worden
  if (!resOpen && !estopped && !gatesOpen && trainParked && !systemError) {
    canDispatch = true;
  } else {
    canDispatch = false;
  }

  // Controleer of er beugels en poortjes open kunnen
  if (trainParked) {
    enableStationCTRL = true;
  } else {
    enableStationCTRL = false;
  }
}

// Update LED
void updateLights() {
  if (currentMillis - previousBlinkMillis >= blinkInterval) {
    previousBlinkMillis = currentMillis;
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
  }
  // Knipperende LED functie
  if (preLoad && !estopped) {
    digitalWrite(functionl, ledState);
  } else {
    digitalWrite(functionl, LOW);
  }
  // Knipperende LED dispatch
  if (canDispatch) {
    digitalWrite(dis1l, ledState);
    digitalWrite(dis2l, ledState);
  } else {
    digitalWrite(dis1l, LOW);
    digitalWrite(dis2l, LOW);
  }
  // Knipperende LED reset -- Enkelt bij fault, estop gaat vanzelf en brandt CONSTANT
  if (systemError) {
    digitalWrite(resetl, ledState);
  } else {
    digitalWrite(resetl, LOW);
  }
}
 
void loop() {
  currentMillis = millis();
  // Power
  if (digitalRead(power) == LOW) {
    // Lamptest
    if (!lightTest) {
      digitalWrite(dis1l, HIGH);
      digitalWrite(dis2l, HIGH);
      digitalWrite(resetl, HIGH);
      digitalWrite(functionl, HIGH);
      delay(1000);
      digitalWrite(dis1l, LOW);
      digitalWrite(dis2l, LOW);
      digitalWrite(resetl, LOW);
      digitalWrite(functionl, LOW);
      lightTest = true;
    }
    updateLights();
    updateStates();
    // Zet keyboard aan
    if (!keyboardState) {
      Serial.begin(9600);
      Keyboard.begin();
      keyboardState = true;
    }
    // Dispatch
    if (digitalRead(dis1) == LOW && digitalRead(dis2) == LOW) {
      if (canDispatch) {
        dispatch();
        delay(1000);
      } else {
        systemError = true;
      }
    }
 
  // Functie
  if (digitalRead(functie1) == LOW) {
    if (!estopped) {
      functie();
    }
  }

  // Poortjes
  if (digitalRead(gates) == HIGH) {
    if (gatesOpen == false && trainParked) {
      openGates();
    }
  }
 
  if (digitalRead(gates) == LOW) {
    if (gatesOpen == true && trainParked) {
      closeGates();
    }
  }
 
  // Beugels
  if (digitalRead(restraints) == HIGH) {
    if (resOpen == false && trainParked) {
      openRestraints();
    }
  }
 
  if (digitalRead(restraints) == LOW) {
    if (resOpen == true && trainParked) {
      closeRestraints();
    }
  }
 
  // E-stop
  if (digitalRead(estop) == HIGH && estopped == false) {
    emergency();
  }

  // Reset
  if (digitalRead(reset) == LOW && systemError == true) {
    faultReset();
  }

  } else {
    // Zet alles uit
    if (keyboardState) {
      Keyboard.end();
      keyboardState = false;
      Serial.end();
    }
    digitalWrite(dis1l, LOW);
    digitalWrite(dis2l, LOW);
    digitalWrite(resetl, LOW);
    digitalWrite(functionl, LOW);
    lightTest = false;
  }
}
