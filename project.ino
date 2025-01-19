/************************************************************************** 
   LED Pattern Game with FreeRTOS on ESP32 + Blynk + LCD
   -----------------------------------------------------
   - Shows a random pattern of LEDs for each level.
   - User replicates pattern via Blynk app’s virtual buttons.
   - LCD displays status: “Watch Sequence”, “Your Turn”, 
     level info, score, and mistakes.
   - Up to predefined max levels, each level increases pattern length by 1 
     (starting at 3).
   - There is a max number of mistakes (eg. 3) allowed per level before game over.
   - If the user completes the predefined max level, the game ends with a win message.
   - Press Start again to restart from level 1.

**************************************************************************/
#define BLYNK_PRINT Serial

/* Information of the device from Blynk Device Info */
#define BLYNK_TEMPLATE_ID "TMPL6CKg9NmVM"
#define BLYNK_TEMPLATE_NAME "Real Time Project"
#define BLYNK_AUTH_TOKEN "J6GATmpN2K-ANb02VE-bQZZVSAdfiget"

// -------------------- LIBRARIES --------------------
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <LiquidCrystal_I2C.h>

// Virtual pins in Blynk
#define VPIN_LED1_BUTTON V1       // RED button
#define VPIN_LED2_BUTTON V2       // YELLOW button
#define VPIN_LED3_BUTTON V3       // GREEN button
#define VPIN_LED4_BUTTON V4       // BLUE button
#define VPIN_START_BUTTON V5      // START button
#define VPIN_RESTART_BUTTON V6    // RESTART button

// -------------------- PIN ASSIGNMENTS --------------------
#define LED1_PIN  33 // RED
#define LED2_PIN  26 // YELLOW
#define LED3_PIN  25 // GREEN
#define LED4_PIN  27 // BLUE

// -------------------- GAME CONSTANTS --------------------
#define NUM_LEDS 4
#define DELAY_BETWEEN_LEDS 1000 // 1 sec
#define MAX_LEVEL 7 // THE MAX LEVEL TAHT WHILL WIN THE PLAYER IF HE COMPLETED
#define MAX_MISTAKES_LEVEL 3 // EVEVRY WRONG SEQUENCE IN THE LEVEL
#define MAX_MISTAKES_GAME 4 // EVERY WRONG SEQUENCE I NTHE WHOLE GAME


// -------------------- BLYNK CONFIG --------------------
char auth [] = BLYNK_AUTH_TOKEN ;
char ssid[] = "Waseem";
char pass[] = "123abc123";


// -------------------- SHARED GAME VARIABLES ---------------
int ledsPins[4] = {33, 26, 25, 27};
int ledPattern[MAX_LEVEL + 2] = {};
int led_index = 0;
int current_level = 0;
int score = 0;
int numBtnsClicked = 0;
int is_get_wrong = 0;
int num_mistakes_level = 0;
int num_mistakes_game = 0;
bool isWin = false;
bool firstTime = true;
bool isResult = false;
bool isGameStart =false;
bool gameActive    = false;   // True when game is running
bool userTurn      = false;   // True when waiting for user input
bool newLevelReady = false;   // Flag telling tasks we need new pattern
bool isLevelStart = false;
bool isHaveTry = false;

// LCD setup
LiquidCrystal_I2C lcd2(0x3F, 16, 2);

// -------------------- FREE RTOS HANDLES --------------------
TaskHandle_t ledSequenceTaskHandle   = NULL;
TaskHandle_t lcdDisplayTaskHandle    = NULL;
TaskHandle_t blynkTaskHandle    = NULL;

// Function prototypes
void startLevel();
void resetGameVariables();
void displayResults();
void printonLCD(int numColumns, int numRows, String msg);
void handleButtonPress(int buttonState, int buttonIndex, int ledPin, String colorName);

void printonLCD(int numColumns, int numRows, String msg) {
  lcd2.setCursor(numColumns, numRows);
  lcd2.print(msg);
  delay(500);
}

void resetGameVariables() {
  led_index = 0;
  current_level = 0;
  score = 0;
  numBtnsClicked = 0;
  gameActive = false;
  newLevelReady = false;
  num_mistakes_level = 0;
  num_mistakes_game = 0;
  isWin=false;
  firstTime=true;
  isResult=false;
  isGameStart=false;
  isLevelStart=false;
  isHaveTry = false;

  for (int i = 0; i < MAX_LEVEL +2; i++) {
    ledPattern[i]   = 0;
  }
}

void generateLEDPattern(){
  int len = current_level + 2;
  for (int i = 0; i < len; i++) {
    // Each "LED step" is an integer from 1..4
    int pin = random(1, NUM_LEDS + 1);
    ledPattern[i] = pin;
    Serial.println(pin);
    lightUpLED(ledsPins[pin - 1]);
  }
}

void lightUpLED(int pinToLight) {
  // Turn on, wait, turn off
  digitalWrite(pinToLight, HIGH);
  delay(DELAY_BETWEEN_LEDS);
  digitalWrite(pinToLight, LOW);
  delay(DELAY_BETWEEN_LEDS);
}

void displayResults() {
  String msg = "Scr:";
  msg += score;
  msg += " Mstks:";
  msg+= num_mistakes_game;

  printonLCD(0, 1, msg);
}

void handleButtonPress(int buttonState, int buttonIndex, int ledPin, String colorName) {
  // button clicked
  if (userTurn){ // to ignore any clicked when the sequance is not generated yet

    if ( buttonState == 0) {
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
      Serial.println(colorName + " LED CLICKED");

      numBtnsClicked++;
      if (ledPattern[led_index] != buttonIndex) {
        is_get_wrong = 1;
      }
      led_index++;

      if (numBtnsClicked == current_level + 2) {
        if (!is_get_wrong) { // Correct Sequence
          score++;
          isResult=true;
          delay(2000);
          isResult=false;
          if (current_level == MAX_LEVEL) { // finish the game and win
            isWin = true;
            gameActive = false;
            userTurn = false;
          } else { // continue
            newLevelReady =true;
            //startLevel();
          }

        } else { // Wrong Sequence
          num_mistakes_level++;
          num_mistakes_game++;
          isResult=true;
          delay(2000);
          isResult=false;
          if (num_mistakes_level < MAX_MISTAKES_LEVEL && num_mistakes_game < MAX_MISTAKES_GAME) {
            // allow to another try
            isHaveTry = true;
            delay(2000);
            isHaveTry = false;

            numBtnsClicked = 0;
            is_get_wrong = 0;
            led_index = 0;

          } else { // finish the game with display game over 
            isWin = false;
            gameActive = false;
            userTurn = false;
          }
        }
      }
    }

  }
}


// -------------------- FREE RTOS TASKS --------------------

/**
 * Task: LED Sequence
 *  - If a new level is ready, generate a pattern and display it.
 *  - Then sets `userTurn = true` to let the other task check input.
 */
void ledSequenceTask(void *pvParameters) {
  for(;;) {
    if (gameActive && newLevelReady) {
      newLevelReady = false;   // We’re going to handle it now
      userTurn       = false;
      numBtnsClicked = 0;
      num_mistakes_level = 0;
      current_level++;
      led_index = 0;
      is_get_wrong = 0;
      isGameStart = false;
      isLevelStart = true;
      // Generate & Display the LED pattern
      Serial.printf("Generating pattern for level %d...\n", current_level);
      generateLEDPattern();  

      // Now it’s user’s turn
      isLevelStart = false;
      userTurn = true;
    }

    // Minimal task delay
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}


/**
 * Task: LCD Display
 *  - Periodically updates the LCD with current game status (level, score, mistakes, etc).
 */
void lcdDisplayTask(void *pvParameters) {
  lcd2.init();
  lcd2.backlight();
  
  for(;;) {
    lcd2.clear();
    if (!gameActive) {
      // Game is not active
      if (firstTime){
        printonLCD(0,0,"Press START");
        printonLCD(0,1,"in Blynk App");
      }
      else if (isWin){
        printonLCD(0, 0, "Max Level Reached!");
        printonLCD(0, 1, "Press 'New Round'");
        delay(1000);
      }
      else {
        printonLCD(0, 0, "Game Over!");
        printonLCD(0, 1, "Press 'Restart'");
        delay(1000);
      }

    } else {
      // Game is active
      if(isGameStart){
        printonLCD(0, 0, "Start in 3 sec");
        printonLCD(0, 1, "Watch closely!");
        delay(3000);
        newLevelReady = true;
        isLevelStart = true;
      }
      else if (isResult){
        if (is_get_wrong)
          printonLCD(0, 0, "Wrong Sequence");
        else
            printonLCD(0, 0, "Correct Sequence");
        displayResults();
      }

      else if (isLevelStart){
        String msg = "Start Level ";
        msg += current_level;
        msg += "...";

        lcd2.clear();
        printonLCD(0, 0, msg);
        printonLCD(0, 1, "Watch closely!");
        delay(1000);
      }
      else if (isHaveTry){
          printonLCD(0, 0, "Try again, you");
          String message = "Have ";
          message += (MAX_MISTAKES_LEVEL - num_mistakes_level);
          message += "tries left";
          printonLCD(0, 1, message);
      }
      else if(userTurn){
        printonLCD(0, 0, "ENTER SEQUNCE BY");
        printonLCD(0, 1, "CLICKING BUTTONS");
      }
    
    }
    // Update every half second (adjust as needed)
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}


// -------------------- BLYNK CALLBACKS --------------------
BLYNK_WRITE(VPIN_LED1_BUTTON) { // red
  int buttonState = param.asInt(); // Read button state (0 or 1)
  handleButtonPress(buttonState, 1, LED1_PIN, "RED"); 
}

BLYNK_WRITE(VPIN_LED2_BUTTON) { // yellow
  int buttonState = param.asInt(); // Read button state (0 or 1)
  handleButtonPress(buttonState, 2, LED2_PIN, "YELLOW"); 
}

BLYNK_WRITE(VPIN_LED3_BUTTON) { // green
  int buttonState = param.asInt(); // Read button state (0 or 1)
  handleButtonPress(buttonState, 3, LED3_PIN, "GREEN"); 
}

BLYNK_WRITE(VPIN_LED4_BUTTON) { 
  int buttonState = param.asInt(); // Read button state (0 or 1)
  handleButtonPress(buttonState, 4, LED4_PIN, "BLUE"); 
}

BLYNK_WRITE ( VPIN_START_BUTTON ) {
  int buttonState = param.asInt(); // Read button state (0 or 1)
  if ( buttonState == 1) {
  } else {
    // start game button clicked
    Serial.println("Game started!");
    resetGameVariables();
    gameActive = true;
    isGameStart = true;
    firstTime=false;
  }
}

BLYNK_WRITE ( VPIN_RESTART_BUTTON ) {
  int buttonState = param.asInt(); // Read button state (0 or 1)
  if ( buttonState == 1) {
  } else {
    // re start game button clicked
    Serial.println("Game started!");
    resetGameVariables();
    gameActive = true;
    isGameStart = true;
    firstTime=false;
  }
}

// -------------------- SETUP & LOOP --------------------
void setup() {
  Serial.begin(9600);

  // Initialize LED pins
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(LED4_PIN, OUTPUT);

  // Initialize Blynk
  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);

  // Initialize random seed
  randomSeed(esp_random());

  // Initialize LCD
  lcd2.init();
  lcd2.backlight();

  // Create FreeRTOS tasks
  xTaskCreate(
    ledSequenceTask,       // Task function
    "LEDSequence",         // Task name
    2048,                  // Stack size
    NULL,                  // Parameters
    1,                     // Priority
    &ledSequenceTaskHandle // Task handle
  );

  xTaskCreate(
    blynkRunTask,
    "BlynkRun",
    2048,
    NULL,
    1,
    &blynkTaskHandle
  );

  xTaskCreate(
    lcdDisplayTask,
    "LCDDisplay",
    2048,
    NULL,
    1,
    &lcdDisplayTaskHandle
  );

}

void blynkRunTask(void *pvParameters) {
  Blynk.run();
}

void loop(){

}
