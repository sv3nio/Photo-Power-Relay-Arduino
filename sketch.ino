/*
PHOTO POWER LIGHT RELAY (Arduino)
====================================================================================
A simple photosensitive power relay for controlling lights with BLE for wireless
command and control.
*/

#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"

#if SOFTWARE_SERIAL_AVAILABLE
#include <SoftwareSerial.h>
#endif

#define MINIMUM_FIRMWARE_VERSION "0.7"
#define MODE_LED_BEHAVIOUR "MODE"
bool pwrStatus = 0;          // Tracking status of lights
bool pwrOverride = 0;        // Tracking manual override of the light control
int pwrOverrideType = 0;     // Tracking override type (0 = off, 1 = day, 2 = night).
int pwrTimeLimit = 21600;    // Default 6hr time limit
int pwrTimeLimitOld = 21600; // Used to restore time limit after override
int pwrTimer = 0;            // Timer for the limit
int photoThreshold = 125;    // Default light sensivity value
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

// A simple error handler
void error(const __FlashStringHelper *err)
{
  Serial.println(err);
  while (1)
    ;
}

// After much testing (and some frustration!) I found that BLE's isConnected() function
// was too unreliable for flow control. So, this quick-and-dirty solution uses the
// AT+BLEGETPEERADDR command to watch the MAC address. Please note that while the MAC addr
// returned by the BLE device while disconnected is not always zero, this value does not
// fluctuate wildly. Consequently, it can be used as a check. However, in order to achieve
// reliability in light of this flucuation the function checks 3x before returning.
bool bleConnected()
{
  int hit = 0;
  for (int i = 0; i < 3; i++)
  {
    ble.println("AT+BLEGETPEERADDR");
    ble.readline();
    // While testing, I noticed that the flucuating MAC address returned by Nordic (BLE) almost
    // always started and ended with zeros. This changes only occasionally and appears never to
    // maintain a non-zero value in both positions for more than a single sampling. Consequently,
    // sampling both characters gives a high probability of success in detecting a connection.
    int connData1 = int(ble.buffer[0]);
    int connData2 = int(ble.buffer[16]);
    if ((connData1 != 48) && (connData2 != 48))
    {
      hit++;
    }
    delay(100);
  }
  ble.waitForOK();
  if (hit == 3)
  {
    return true;
  }
  return false;
}

// A simple UI hack that pushes the last round of text out of view.
void clrScreen()
{
  ble.print("AT+BLEUARTTX=");
  ble.println("\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n\\r\\n");
  ble.waitForOK();
  delay(50);
}

// Without an easy way to flush the BLE buffer, this simple
// function runs it again to reset it back to "OK".
void resetBLEBuffer()
{
  ble.println("AT+BLEUARTRX");
  ble.readline();
  ble.waitForOK();
  delay(100);
}

// Light toggling. Returns the status.
bool switchLights(int status)
{
  if (status == 1)
  {
    digitalWrite(A0, HIGH);
    return 1;
  }
  else if (status == 0)
  {
    digitalWrite(A0, LOW);
    return 0;
  }
  else
  {
    return 0;
  }
}

// UI: Main Menu
void mainMenu(bool pwr_status, int pwr_time_limit, int photoThresholdReport)
{
  int pwrTimeLimitReport = (pwr_time_limit / 60) / 60;
  char pwrStatusReport[4] = "On";
  if (!pwr_status)
  {
    strcpy(pwrStatusReport, "Off");
  }
  delay(50);
  clrScreen();
  ble.print("AT+BLEUARTTX=MAIN MENU\\r\\n==============================\\r\\n");
  ble.print("[1]  Toggle lights            [");
  ble.print(pwrStatusReport);
  ble.print("]\\r\\n");
  ble.print("[2]  Set new time limit       [");
  ble.print(pwrTimeLimitReport);
  ble.println("hrs]\\r\\n");
  ble.print("AT+BLEUARTTX=[3]  Set new light threshold  [");
  ble.print(photoThresholdReport);
  ble.print("]\\r\\n");
  ble.print("[4]  Show realtime light levels\\r\\n");
  ble.print("[5]  Reset all settings to default\\r\\n\\r\\n");
  ble.println("Enter a number (1-5):\\r\\n");
  ble.waitForOK();
}

// UI: Timer Menu
void timerMenu()
{
  clrScreen();
  ble.print("AT+BLEUARTTX=");
  ble.print("\\r\\nTIMER SETTING\\r\\n==============================\\r\\n");
  ble.print("Please enter the number of hours to have the lights stay on after dark:\\r\\n\\r\\n");
  ble.println("- Default is 6hrs (resets on boot).\\r\\n");
  ble.print("AT+BLEUARTTX=");
  ble.print("- Recommended is 4-8.\\r\\n");
  ble.print("- Must be less than 23.\\r\\n");
  ble.print("- Enter 999 to disable the timer.\\r\\n\\r\\n");
  ble.println("Enter a number (1-23 or 999):\\r\\n");
  ble.waitForOK();
}

// UI: Light Threshold Menu
void lightThresholdMenu()
{
  clrScreen();
  ble.print("AT+BLEUARTTX=");
  ble.print("\\r\\nLIGHT THRESHOLD SETTING\\r\\n==============================\\r\\n");
  ble.println("Please enter a light-sensitivity value for when the lights should turn on:\\r\\n\\r\\n");
  ble.print("AT+BLEUARTTX=");
  ble.print("- Default is 125 (dusk, resets on boot).\\r\\n");
  ble.print("- Pitch black is <5.\\r\\n");
  ble.print("- Noon (full sun) is >600.\\r\\n\\r\\n");
  ble.println("Enter a number (1 - 600):\\r\\n");
  ble.waitForOK();
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=======================================================\n");
  Serial.println(F("Setting up...\n"));

  // PIN config
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(A0, OUTPUT);
  Serial.println(F(" --> Pins configured."));

  // Initialize & configure BLE module
  if (!ble.begin(VERBOSE_MODE))
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println(F(" --> BLE initialized."));
  ble.factoryReset();
  ble.sendCommandCheckOK(F("AT+GAPDEVNAME=Backyard Light Relay"));
  ble.reset();
  ble.sendCommandCheckOK(F("AT+BLEPOWERLEVEL=4"));
  Serial.println(F(" --> BLE reset complete."));
  ble.echo(false);
  ble.verbose(false);
  Serial.println(F(" --> BLE ready for connection."));
  Serial.println(F("\nPlease connect with the Adafruit Bluefruit LE app in UART mode."));
  Serial.println(F("Then enter '0' to begin."));
  Serial.println("\n=======================================================\n");

  // LED Activity command is only supported from 0.6.6
  if (ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION))
  {
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }
}

void loop()
{
  // Take a reading from the photoresistor
  int photoVal = analogRead(A1);

  if (!bleConnected())
  {

    // Report to serial
    Serial.print("Light: ");
    Serial.print(photoVal);
    Serial.print("/");
    Serial.print(photoThreshold);
    Serial.print("  ||  Power: ");
    Serial.print(pwrStatus);
    Serial.print("  ||  Timer: ");
    Serial.print(pwrTimer);
    Serial.print("/");
    Serial.print(pwrTimeLimit);
    Serial.print("  ||  Override[Type]: ");
    Serial.print(pwrOverride);
    Serial.print("[");
    Serial.print(pwrOverrideType);
    Serial.println("]");

    // Blink LED to show photoresistor processing
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(750);

    // Automatic schedule (not-overridden)
    if (!pwrOverride)
    {

      // Reset timer at dawn (otherwise, the lights will turn back on again when the timer runs out).
      //
      // IMPORTANT NOTE: It turns out that the photoVal does not rise or fall linearly throughout the day.
      // Instead, it flucuates as the values trend up or down with the sun. Therefore, in order to
      // prevent a bug that turns the lights ON at DAWN when the photoVal *happens* to dip below the
      // photoThreshold (i.e. the unit thinks it's dusk), the condition below is increased by a reasonable
      // margin to delay resetting the pwrTimer. This, in turn, prevents the lights from turning back on
      // until the real dusk threshold.
      if ((photoVal > photoThreshold + 30) && (pwrTimer != 0))
      {
        pwrTimer = 0;
      }

      // Normal (not-overridden), power-on at dusk
      if ((pwrStatus == 0) && (photoVal < photoThreshold) && (pwrTimer <= pwrTimeLimit))
      {
        pwrStatus = switchLights(1);
        return;
      }

      // Normal (not-overridden), time-based power-off
      if ((pwrStatus == 1) && (pwrTimer > pwrTimeLimit))
      {
        pwrStatus = switchLights(0);
        return;
      }

      // Normal (not-overridden), light-based power-off with timer (i.e. what happens
      // if dawn comes before the timer runs out?).
      //
      // IMPORTANT NOTE: Turning the lights on causes ambient light levels to rise. This,
      // in certain situations, MAY be sufficient to cause the photoVal to cross back over
      // the photoThreshold, (since the unit thinks it's dawn). Therefore, to prevent the
      // lights from immediately turning off after they turn on, the photoThreshold condition
      // is increased by a reasonable margin to account for this situation.
      if ((pwrStatus == 1) && (photoVal > photoThreshold + 30) && (pwrTimer < pwrTimeLimit))
      {
        pwrStatus = switchLights(0);
        return;
      }

      // Normal (not-overridden), dusk-till-dawn (light-based, disabled timer) power-off
      if ((pwrStatus == 1) && (photoVal > photoThreshold + 30) && (pwrTimeLimit == 0))
      {
        pwrStatus = switchLights(0);
        pwrTimeLimit = 0; // This shouldn't matter, but just to be sure.
        pwrTimer = 0;     // Confirm timer is reset in case the user re-enables the time limit
        return;
      }

      // Normal (not-overridden) time tracking while lights are on & the timer is NOT off.
      if ((pwrStatus == 1) && (pwrTimeLimit != 0))
      {
        pwrTimer++;
        return;
      }
    }
    // Override Scheduling
    // Regardless of when the lights were overridden or their current power status, the
    // override should reset on the next crossing of the photoThreshold in a manner
    // consistant with the automatic schedule and then turn over handling back to the
    // automatic scheduler (above).
    else
    {
      // Overridden lights during day.
      if ((pwrOverrideType == 1) && (photoVal < photoThreshold))
      {
        pwrStatus = switchLights(1);
        pwrOverride = 0;
        pwrOverrideType = 0;
        pwrTimer = 0;
        return;
      }
      // Overridden lights during night.
      if ((pwrOverrideType == 2) && (photoVal > photoThreshold))
      {
        pwrStatus = switchLights(0);
        pwrOverride = 0;
        pwrOverrideType = 0;
        pwrTimer = 0;
        return;
      }
    }
  }

  else
  { // If BLE connection
    ble.println("AT+BLEUARTRX");
    ble.readline();
    ble.waitForOK();

    // Main Menu
    if (!strcmp(ble.buffer, "0"))
    { // strcmp returns 0 on success
      mainMenu(pwrStatus, pwrTimeLimit, photoThreshold);
      return;
    }

    // Light Override (Toggle)
    if (!strcmp(ble.buffer, "1"))
    {
      resetBLEBuffer();
      pwrOverride = 1;
      pwrTimer = 0;
      if (photoVal < photoThreshold)
      {
        pwrOverrideType = 2;
      }
      else
      {
        pwrOverrideType = 1;
      }
      if (!pwrStatus)
      {
        pwrStatus = switchLights(1);
        clrScreen();
        ble.print("AT+BLEUARTTX=Lights turned on.\\r\\nThis override will last until the next threshold.\\r\\n\\r\\n");
        ble.println("Enter '0' to return to the Main Menu.\\r\\n\\r\\n");
        return;
      }
      else
      {
        pwrStatus = switchLights(0);
        clrScreen();
        ble.print("AT+BLEUARTTX=Lights turned off.\\r\\nThis override will last until the next threshold.\\r\\n\\r\\n");
        ble.println("Enter '0' to return to the Main Menu.\\r\\n\\r\\n");
        return;
      }
      return;
    }

    // Timer Limit Updater
    if (!strcmp(ble.buffer, "2"))
    {
      resetBLEBuffer();
      timerMenu(); // Show the menu
      while (strcmp(ble.buffer, "OK") == 0)
      { // Wait for imput from the user
        ble.println("AT+BLEUARTRX");
        ble.readline();
        delay(100);
      }
      ble.waitForOK();
      int data = atoi(ble.buffer); // Convert user input to INT
      if (data == 999)
      { // If the user wants to disable the timer
        pwrTimeLimit = 0;
        pwrTimer = 0;
        ble.println("AT+BLEUARTTX=\\r\\n\\r\\nUpdate Successful!\\r\\n\\r\\nEnter '0' to return to the Main Menu.\\r\\n\\r\\n");
      }
      else if ((data > 0) && (data <= 23))
      { // Input validation
        pwrTimeLimitOld = pwrTimeLimit;
        pwrTimeLimit = (data * 60) * 60;
        ble.println("AT+BLEUARTTX=\\r\\n\\r\\nUpdate Successful!\\r\\n\\r\\nEnter '0' to return to the Main Menu.\\r\\n\\r\\n");
      }
      else
      {
        ble.println("AT+BLEUARTTX=\\r\\n\\r\\nInvalid Input! Exited.\\r\\n\\r\\nEnter '2' to try again or '0' for the Main Menu.\\r\\n\\r\\n");
      }
      ble.waitForOK();
      resetBLEBuffer();
      return;
    }

    // Threshold sensitivity updater
    if (!strcmp(ble.buffer, "3"))
    {
      resetBLEBuffer();
      lightThresholdMenu();
      while (strcmp(ble.buffer, "OK") == 0)
      {
        ble.println("AT+BLEUARTRX");
        ble.readline();
        delay(100);
      }
      int data = atoi(ble.buffer);
      if ((data > 0) && (data <= 600))
      {
        photoThreshold = data;
        ble.println("AT+BLEUARTTX=\\r\\n\\r\\nUpdate Successful!\\r\\n\\r\\nEnter '0' to return to the Main Menu.\\r\\n\\r\\n");
      }
      else
      {
        ble.println("AT+BLEUARTTX=\\r\\n\\r\\nInvalid Input! Exited.\\r\\n\\r\\nEnter '3' to try again or '0' for the Main Menu.\\r\\n\\r\\n");
      }
      ble.waitForOK();
      resetBLEBuffer();
      return;
    }

    // Realtime Light Reporting
    if (!strcmp(ble.buffer, "4"))
    {
      resetBLEBuffer();
      clrScreen();
      ble.println("AT+BLEUARTTX=Initiating realtime reporting...\\r\\nSubmit anything to stop.\\r\\n\\r\\n");
      ble.waitForOK();

      while (strcmp(ble.buffer, "OK") == 0)
      { // While there is no input from the user
        ble.print("AT+BLEUARTTX=\\r\\n  ");
        ble.println(int(analogRead(A1)));
        ble.waitForOK();

        ble.println("AT+BLEUARTRX");
        ble.readline();
        delay(100);
      }

      ble.println("AT+BLEUARTTX=\\r\\n\\r\\nReporting Stopped.\\r\\n\\r\\nEnter '0' to return to the Main Menu.\\r\\n\\r\\n");
      ble.waitForOK();
      resetBLEBuffer();
      return;
    }

    // Reset to default
    if (!strcmp(ble.buffer, "5"))
    {
      resetBLEBuffer();
      clrScreen();
      ble.println("AT+BLEUARTTX=### WARNING!! ###\\r\\n\\r\\nThis will clear all custom settings, apply the built-in defalts and restart the timer from 0.\\r\\n\\r\\nAre you sure?\\r\\n\\r\\nEnter 'Y' again to confirm,\\r\\nanything else will abort:\\r\\n");
      ble.waitForOK();

      while (strcmp(ble.buffer, "OK") == 0)
      {
        ble.println("AT+BLEUARTRX");
        ble.readline();
        delay(100);
      }
      clrScreen();
      if (strcmp(ble.buffer, "Y") == 0)
      {
        photoThreshold = 20;
        pwrTimeLimit = 21600;
        pwrTimeLimitOld = 21600;
        pwrTimer = 0;
        pwrOverride = 0;
        pwrOverrideType = 0;
        pwrStatus = switchLights(0);
        ble.println("AT+BLEUARTTX=\\r\\n\\r\\nReset Successful.\\r\\n\\r\\nEnter '0' to return to the Main Menu.\\r\\n\\r\\n");
      }
      else
      {
        ble.println("AT+BLEUARTTX=\\r\\n\\r\\nReset Aborted.\\r\\n\\r\\nEnter '5' to try again or '0' for the Main Menu.\\r\\n\\r\\n");
      }
      ble.waitForOK();
      resetBLEBuffer();
      return;
    }
  }
}
