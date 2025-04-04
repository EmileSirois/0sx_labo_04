#pragma region constantes

#include <AccelStepper.h>
#define MOTOR_INTERFACE_TYPE 4
#define IN_1 8
#define IN_2 9
#define IN_3 10
#define IN_4 11
AccelStepper myStepper(MOTOR_INTERFACE_TYPE, IN_1, IN_3, IN_2, IN_4);

#include <HCSR04.h>
#define TRIGGER_PIN 6
#define ECHO_PIN 7
HCSR04 hc(TRIGGER_PIN, ECHO_PIN);

#include <LCD_I2C.h>
LCD_I2C lcd(0x27, 16, 2);

enum AppState {TOOCLOSE , AUTOMATIC , TOOFAR};
AppState appState = AUTOMATIC;

const int SETUP_DELAY = 1000;
const int REFRESH_RATE = 100;
const int NO_DISTANCE = 0;
const long MAX_ANGLE = 170;
const long MIN_ANGLE = 10;
const long MAX_DISTANCE = 60;
const long MIN_DISTANCE = 30;
const int MAX_SPEED = 500;
const int MAX_ACCEL = 200;
const int TOUR = 2038;
const float MAX_POSITION = 960;
const float MIN_POSITION = 50;


unsigned int currentTime = 0;
int distance;
int angle;
String message = "";

#pragma endregion

#pragma region Modèles
void xTask(unsigned long ct) {
  static unsigned long lastTime = 0;
  unsigned long rate = 500;
  
  if (ct - lastTime < rate) {
    return;
  }
  
  lastTime = ct;
  
  // Faire le code de la tâche ici
  
}
#pragma endregion

#pragma region functions
//revoie l'angle auquel le stepper dois bouger selon la distance
int angleReturnTask(int distance){
  int angle = map(distance, 30, 60, 10, 170);
  return angle;
}
//lire et renvoyer la distance perçue par le capteur
int distanceTaskReturn(unsigned long ct) {
  static unsigned long lastTime = 0;
  unsigned long rate = 50;
  static int lastResult = 0;
  float result = 0;
  
  if (ct - lastTime < rate) {
    return lastResult;
  }
  
  lastTime = ct;
  
  // Faire le code de la tâche ici

  result = hc.dist();

  //vérifier si la distance est valide
  if(result == 0.0) result = 1;
  
  lastResult = result;
  return result;  
}
//afficher les informations sur la led
void ledTask(unsigned long ct, int distance){
  static unsigned long lastTime = 0;
  const int rate = 150;

  if(ct - lastTime < rate) return;
  lastTime = ct;

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Dist : ");

  lcd.setCursor(7, 0);
  lcd.print(distance);

  lcd.setCursor(10, 0);
  lcd.print("cm");

  lcd.setCursor(0, 1);
  lcd.print("Obj  :");

  lcd.setCursor(7, 1);
  lcd.print(message);

  if(appState != AUTOMATIC) return;

  lcd.setCursor(10, 1);
  lcd.print("deg");
}
//indique au stepper motor où pointer et le fais bouger
void automaticStepperTask(int distance) {
  //si l'application n'est pas en automatic, arrêter le moteur et 
  if(appState != AUTOMATIC){
    return;
  } 

  float goTo = map(distance, MIN_DISTANCE, MAX_DISTANCE, MIN_POSITION, MAX_POSITION);
  //goTo = constrain(goTo, MIN_POSITION, MAX_POSITION);

  //Serial.println(goTo);
  //Serial.println(myStepper.distanceToGo());

  myStepper.moveTo(goTo);
}

//le stepper retourne à 0 lorsque trop proche
void tooCloseStepperTask() {

  myStepper.moveTo(MIN_POSITION);
}

//le stepper retourne à 2038 lorsque trop loin
void tooFarStepperTask() {

  myStepper.moveTo(MAX_POSITION);
}

//dispatch les états du programme
void stateManager(unsigned long ct) {
  // Adapter selon votre situation!
  bool tooClose = distance < 30;
  bool tooFar = distance > 60;

  switch (appState) {
    case TOOCLOSE:
      if(!tooClose){
        appState = AUTOMATIC;
      }
      message = "trop pres";
      tooCloseStepperTask();
      break;
    case AUTOMATIC:
      if(tooClose){
        appState = TOOCLOSE;
      }
      if(tooFar){
        appState = TOOFAR;
      }
      message = angle;
      automaticStepperTask(distance);
      break;
    case TOOFAR:
      if(!tooFar){
        appState = AUTOMATIC;
      }
      message = "trop loin";
      tooFarStepperTask();
      break;
  }
}
#pragma endregion

#pragma region setup-loop
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  lcd.begin();
  lcd.backlight();

  myStepper.setMaxSpeed(MAX_SPEED);  
  myStepper.setAcceleration(MAX_ACCEL); 

  setupTask();
}
//message au début du programme affiché sur le lcd
void setupTask(){
  lcd.setCursor(0, 0);
  lcd.print("6229147");

  lcd.setCursor(0, 1);
  lcd.print("labo 4B");

  delay(SETUP_DELAY);

  lcd.clear();
}
//messages à la console
void SerialTask(unsigned long ct){
  static unsigned long lastTime = 0;

  if(ct - lastTime < REFRESH_RATE) return;

  Serial.print("etd:6229147");
  Serial.print(",dist:");
  Serial.print(distance);
  Serial.print(",deg:");
  Serial.println(angle);
}

void loop() {
  currentTime = millis();
  distance = distanceTaskReturn(currentTime);
  angle = angleReturnTask(distance);

  //SerialTask(currentTime);
  
  stateManager(currentTime);
  
  ledTask(currentTime, distance);

  myStepper.run();

  if(myStepper.distanceToGo() == NO_DISTANCE){
    //myStepper.disableOutputs();
  }                

}
#pragma endregion