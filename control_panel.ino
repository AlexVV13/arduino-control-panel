#include <Keyboard.h>

// De knopjes zelf
const int dis1 = 2;
const int dis2 = 3;
const int gates = 4;
const int restraints = 5;
const int estop = 6;
const int reset = 7;
const int functie = 8;
const int power = 9;

// Lampjes
const int disl1 = 10;
const int disl2 = 11;
const int functionl = 12;
const int resetl = 13;

// Mogelijke opties
bool gatesOpen = false;
bool resOpen = false;
bool estopped = false;
bool canDispatch = true;
bool canMoveGatesRestraints = true;
bool canStartAdv = true;
bool trainParked = true;
bool keyboardState = false;

// Wat doen de poortjes en beugels?
int gatesMotionDirection = 0;
int restraintsMotionDirection = 0;

// Delays
const long GatesRestraintsDelay = 1000; // Tijd voor het bewegen van de beugels en poortjes
const long DispatchDelay = 1000; // Tijd tussen vertrek en stilstand volgende trein
const long AdvDelay = 2000; // Tijd tussen 2 enter moves?

unsigned long GatesStartTime = 0;
unsigned long RestraintsStartTime = 0;
unsigned long DispatchStartTime = 0;
unsigned long advStartTime = 0;
unsigned long currentMillis;

// Mogelijke errors
int restraintsError = 0;
int gatesError = 0;
int dispatchError = 0; //track when buttons are pressed at wrong time

// Voor knipperende LED
unsigned long previousBlinkMillis = 0;
const long blinkInterval = 500; // Interval LED
int ledState = LOW; //Staat voor knipperende leds

void setup() {
  // Zorg ervoor dat arduino de knopjes herkent
  pinMode(dis1, INPUT_PULLUP);
  pinMode(dis2, INPUT_PULLUP);
  pinMode(gates, INPUT_PULLUP);
  pinMode(restraints, INPUT_PULLUP);
  pinMode(estop, INPUT_PULLUP);
  pinMode(reset, INPUT_PULLUP);
  pinMode(functie, INPUT_PULLUP);
  pinMode(power, INPUT_PULLUP);

  // En ook de lampjes
  pinMode(disl1, OUTPUT);
  pinMode(disl2, OUTPUT);
  pinMode(functionl, OUTPUT);
  pinMode(resetl, OUTPUT);
}

// Reset storingen
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

// Reset fouten
void errorReset() {
  restraintsError = 0;
  gatesError = 0;
  dispatchError = 0;
  digitalWrite(resetl, LOW);
  Serial.write("RESETTING...\n");
  delay(200);
}

// ESTOP
void emergency() {
  Keyboard.press(0x73);
  delay(200);
  Keyboard.releaseAll();

  estopped = true;
  Serial.write("E-STOPPED\n");

  while (estopped == true) {
    if (digitalRead(reset) == LOW && digitalRead(estop) == LOW) {
      emergencyReset();
    }
    delay(300);
  }
}

void dispatch() {
  Keyboard.press(' ');
  Serial.write("DISPATCHED");
  delay(5000);
  Keyboard.releaseAll();
  trainParked = false;
  DispatchStartTime = currentMillis; // start bewegingstimer
}

void openGates() {
  Serial.write("OPEN GATES\n");
  Keyboard.press(KEY_RIGHT_ARROW);
  gatesOpen = true;
  gatesMotionDirection = -1; //-1 = Ga van dispatch naar laadpositie
  GatesStartTime = currentMillis; // start bewegingstimer
  delay(2000);
  Keyboard.releaseAll();
}

void closeGates() {
  Serial.write("CLOSE GATES\n");
  Keyboard.press(KEY_LEFT_ARROW);
  gatesOpen = false;
  gatesMotionDirection = 1; //1 = klaar voor dispatchpositie
  GatesStartTime = currentMillis; //start bewegingstimer
  delay(2000);
  Keyboard.releaseAll();
}

void openRestraints() {
  Serial.write("OPEN RESTRAINTS\n");
  Keyboard.press(KEY_UP_ARROW);
  resOpen = true;
  restraintsMotionDirection = -1; //-1 = Ga van dispatch naar laadpositie
  RestraintsStartTime = currentMillis; // Start bewegingstimer
  delay(1000);
  Keyboard.releaseAll();
  delay(1000);
}

void closeRestraints() {
  Serial.write("CLOSE RESTRAINTS\n");
  Keyboard.press(KEY_DOWN_ARROW);
  resOpen = false;
  restraintsMotionDirection = 1; // 1 = Ga naar dispatchpositie
  RestraintsStartTime = currentMillis; // Start bewegingstimer
  delay(1000);
  Keyboard.releaseAll();
  delay(1000);
}

void functieReturn() {
  Serial.write("ADVANCING\n");
  Keyboard.press(KEY_RETURN);
  canStartAdv = false;
  delay(1000);
  Keyboard.releaseAll();
  delay(1000);
  advStartTime = currentMillis; // Start bewegingstimer
}

// Zet lampen uit
void lightsOut() {
  digitalWrite(disl1, LOW);
  digitalWrite(disl2, LOW);
  digitalWrite(functionl, LOW);
  digitalWrite(resetl, LOW);
}

void processLights() {
  if (currentMillis - previousBlinkMillis >= blinkInterval) { // Controleer elke halve seconde of er iets moet knipperen
    previousBlinkMillis = currentMillis;
    if (ledState == LOW) {
      ledState = HIGH;
    } else { // update de led Status om te knipperen of niet
      ledState = LOW;
    }
  }
  if (trainParked) {
    // De epische trein mag weg
    if (canDispatch) {
      digitalWrite(disl1, ledState);
      digitalWrite(disl2, ledState);
    } else {
      // Trein kan niet weg
      digitalWrite(disl1, LOW);
      digitalWrite(disl2, LOW);
    }

    if (canStartAdv) {
      // Enteren met de meiden
      digitalWrite(functionl, ledState);
    } else {
      // De enter toets kan niet bedrukt worden
      digitalWrite(functionl, LOW);
    }

    if (estopped) {
      // Welke mongool heeft de estop aangeraakt?
      digitalWrite(resetl, ledState);
    } else {
      // Joepie we mogen weer
      digitalWrite(resetl, LOW);
    }

    if ((gatesError + restraintsError + dispatchError) > 0) {
      // Reset aanzetten
      digitalWrite(resetl, ledState);
    } else {
      // Weer veilig
      digitalWrite(resetl, LOW);
    }
  }
}

void updateStates() {
  // Trein dispatch lampjes
  if (!gatesOpen && !resOpen && trainParked && !estopped) {
    canDispatch = true;
  } else { // Kan de trein weg?
    canDispatch = false;
  }

  // Timer voor poortjes
  if ((currentMillis - GatesStartTime >= GatesRestraintsDelay) || GatesStartTime == 0) {
    if (gatesMotionDirection == -1) { // Controleer of de poortjes openen
      gatesOpen = true;
    }
    if (gatesMotionDirection == 1) { // Controleer of de poortjes sluiten
      gatesMotionDirection = 0;
      gatesOpen = false;
    }
  }

  // Timer voor beugels
  if ((currentMillis - RestraintsStartTime >= GatesRestraintsDelay) || RestraintsStartTime == 0) {
    if (restraintsMotionDirection == -1) { // Controleer of de beugels openen
      restraintsMotionDirection = 0;
      resOpen = true;
    }
    if (restraintsMotionDirection == 1) { // Controleer of de beugels sluiten
      restraintsMotionDirection = 0;
      resOpen = false;
    }
  }

  // Timer voor Dispatch
  if ((currentMillis - DispatchStartTime >= DispatchDelay) || DispatchStartTime == 0) {
    trainParked = true; // Controleer wanneer dispatch voltooid is
  }

  // Timer voor functie
  if ((currentMillis - advStartTime >= AdvDelay) || advStartTime == 0) {
    canStartAdv = true; // Je mag weer enteren
  }
}

void loop() {
  
  currentMillis = millis();
  // Zonder power switch gaat niks moeten werken
  if (digitalRead(power) == LOW) { 
      // Zet het toetsenbord aan
      if (keyboardState == false) {
          Serial.begin(9600);
          Keyboard.begin();
          keyboardState = true;
      }

      // Doe dingen met lampjes
      processLights();
      updateStates();

      // Dispatch
      if (digitalRead(dis1) == LOW && digitalRead(dis2) == LOW && trainParked && !estopped && canDispatch) {
        dispatch();
        delay(1000);
      } else {
        dispatchError = 1;
      }

      // Open poortjes
      if (digitalRead(gates) == LOW && gatesOpen == false && trainParked) {
        openGates();
      } else {
        gatesError = 1;
      }

      // Sluit poortjes
      if (digitalRead(gates) == HIGH && gatesOpen && trainParked) {
        closeGates();
      } else {
        gatesError = 1;
      }

      // Open beugels
      if (digitalRead(restraints) == LOW && !resOpen && trainParked) {
        openRestraints();
      } else {
        restraintsError = 1;
      }

      // Sluit beugels
      if (digitalRead(restraints) == HIGH && resOpen && trainParked) {
        closeRestraints();
      } else {
        restraintsError = 1;
      }

      // E-stop
      if (digitalRead(estop) == HIGH && estopped == false) {
        emergency();
      }

      // Functie
      if (digitalRead(functie) == LOW && estopped == false) {
        functieReturn();
      }

      // Error reset
      if (digitalRead(reset) == LOW && ((gatesError + restraintsError + dispatchError) > 0)) {
        errorReset();
      }
  } else {
      // Zet het toetsenbord aan
      if (keyboardState == true) {
          Serial.end();
          Keyboard.end();
          keyboardState = false;
      }
      // Zet de lampjes uit :/
      lightsOut();
  }
}
