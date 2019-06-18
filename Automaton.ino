#include <HID-Project.h>
#include <HID-Settings.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"
#include <avr/wdt.h>
#include "ArduinoJson.h"
#include <avr/wdt.h>

#define CURRENTVERSION 18
#define DEFAULTCOMMAND "/Volumes/mds/run"
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

#ifdef SHOWFREEMEM
int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}
#endif
enum mode_types {RECOVERY, DEP, CLI} current_mode;

struct settings_t
{
  int version;
  char command[255];
  char firmware_password[32];
  long startup_delay;
  long pre_command_delay;
  bool autorun;
} settings;

void reboot() {
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}
}

void setup() {
  pinMode(13, OUTPUT);

}

char wifi_ssid[32];
char wifi_password[32];

void flash_led(int count) {

  for (int i = 0; i < count; i++) {
    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);
    delay(100);
  }
#ifdef SHOWFREEMEM
     Serial.print("Free memory: ");
    Serial.println(freeMemory());
#endif
}
void presskey(KeyboardKeycode key, int times, int delayms) {

  for (int i = 0; i < times; i++) {
    flash_led(1);
    BootKeyboard.press(key);
    delay(delayms);
    BootKeyboard.releaseAll();
    delay(100);

  }
#ifdef SHOWFREEMEM
    Serial.print("Free memory: ");
    Serial.println(freeMemory());
#endif
}
void showusage() {

  Serial.println(F("\nCopyright 2018 Twocanoes Software, Inc."));
  Serial.println(F("help: this message"));
  Serial.println(F("show: show current settings"));
  Serial.println(F("reset: reset settings to defaults"));
  Serial.println(F("reboot: reboot the device"));
  Serial.println(F("set_command <command>: set command to run in recovery."));
  Serial.println(F("set_firmware_password <password>: set firmware password to enter prior to booting to recovery."));
  Serial.println(F("set_startup_delay <seconds>: how many seconds to wait from booting in recovery to launching terminal."));
  Serial.println(F("set_pre_command_delay <seconds>: how many seconds to wait after launching terminal until typing command."));
  Serial.println(F("set_settings <json>: Provide all settings in JSON format."));
  Serial.println(F("get_settings: Get all settings in JSON format."));
  Serial.println(F("dep Automatically configure DEP in setup assistant"));
  Serial.println(F("recovery: provide keyboard commands to boot into recovery and run resources from external volume or remote disk image.\n"));
  Serial.println(F("set_autorun <on|off>: automatically enter recovery mode after admin time.\n"));


}

void setdefaults() {
  strcpy(settings.command, DEFAULTCOMMAND);
  strcpy(settings.firmware_password, "");
  settings.version = CURRENTVERSION;
  settings.startup_delay = 180;
  settings.autorun = true;
  settings.pre_command_delay = 6;
  EEPROM_writeAnything(0, settings);
  EEPROM_readAnything(0, settings);

}
void enter_cli() {
  
  Serial.begin(9600);

  Serial.println(F("Configuration Mode. Enter help for assistance. Copyright 2018 Twocanoes Software, Inc."));
  while (current_mode == CLI) {
    Serial.setTimeout(1000000);
    flash_led(1);
    Serial.print(">");
    String s = Serial.readStringUntil('\n');
    s.trim();
    Serial.println(s);
 #ifdef SHOWFREEMEM
    Serial.print("Free memory: ");
    Serial.println(freeMemory());
#endif


    if (s.startsWith("show") == true) {
      Serial.print("Version: ");
      Serial.println(settings.version, DEC);
      Serial.print(F("Command:"));
      Serial.println(settings.command);
      Serial.print(F("Startup Delay:"));
      Serial.println(settings.startup_delay, DEC);
      Serial.print(F("Pre Commmand Delay:"));
      Serial.println(settings.pre_command_delay, DEC);
      Serial.print(F("Firmware is Set:"));
      if (strlen(settings.firmware_password) > 0) Serial.println("YES");
      else Serial.println(F("NO"));
      Serial.print(F("Autorun:"));
      if (settings.autorun == 1) Serial.println("on");
      else Serial.println("off");
    }
    else if (s.startsWith("set_command") == true) {

      String new_command = s.substring(12);
      strncpy(settings.command, new_command.c_str(), 255);
      EEPROM_writeAnything(0, settings);

    }
    else if (s.startsWith("set_firmware_password") == true) {

      String new_firmware_password = s.substring(22);
      strncpy(settings.firmware_password, new_firmware_password.c_str(), 32);
      EEPROM_writeAnything(0, settings);

    }
    else if (s.startsWith(F("help")) == true) {
      showusage();

    }
    else if (s.startsWith(F("set_startup_delay")) == true) {
      String new_command = s.substring(18);
      settings.startup_delay = new_command.toInt();
      EEPROM_writeAnything(0, settings);

    }
    else if (s.startsWith(F("set_pre_command_delay")) == true) {
      String new_command = s.substring(18);
      settings.pre_command_delay = new_command.toInt();
      EEPROM_writeAnything(0, settings);

    }

    else if (s.startsWith(F("reset")) == true) {
      setdefaults();
    }
    else if (s.startsWith(F("recovery")) == true) {
      current_mode = RECOVERY;
    }
    else if (s.startsWith(F("bootloader")) == true) {
      enter_bootloader();
    }
    else if (s.startsWith(F("dep")) == true) {
      current_mode = DEP;
    }
    else if (s.startsWith(F("set_autorun")) == true) {
      char *line = (char *)s.c_str();
      char *ptr = NULL;
      byte index = 0;
      char *tokens[2];
      ptr = strtok(line, " ");  // takes a list of delimiters
      while (ptr != NULL && index < 2) {
        tokens[index] = ptr;
        index++;
        ptr = strtok(NULL, " ");  // takes a list of delimiters
      }
      if (index != 2) {

        Serial.println(F("Invalid commmand. Please provide on or off for the set_autorun command"));
      }
      else {
        if (strncmp(tokens[1], "on", 3) == 0) {

          settings.autorun = true;
          Serial.println(F("Autorun turned on"));
          EEPROM_writeAnything(0, settings);
        }
        else if (strncmp(tokens[1], "off", 3) == 0) {
          settings.autorun = false;
          Serial.println(F("Autorun turned off"));
          EEPROM_writeAnything(0, settings);
        }
        else {
          Serial.println(F("Invalid value. Please specify on or off"));
        }
      }

    }
    else if (s.startsWith(F("reboot")) == true) {

      reboot();
    }
    else if (s.length() == 0) {


    }
    else if (s.startsWith(F("set_settings")) == true) {
      Serial.println(F("set_settings"));

#ifdef SHOWFREEMEM
    Serial.print("Free memory before JSON buffer: ");
    Serial.println(freeMemory());
#endif
      StaticJsonDocument<600> doc;
      
      
      String json = s.substring(13);
#ifdef SHOWFREEMEM
    Serial.print("Free memory: ");
    Serial.println(freeMemory());
#endif

      deserializeJson(doc, json);
      JsonObject root = doc.as<JsonObject>();

      const char *command = root["command"];

      const char*firmware_password = root["firmware_password"];

      if (command) {
        strncpy(settings.command, command, sizeof(settings.command));

        if (firmware_password) {
          strncpy(settings.firmware_password, firmware_password, sizeof(settings.firmware_password));

        }

        if (root[F("startup_delay")]) settings.startup_delay  =  root[F("startup_delay")];
        if (root[F("pre_command_delay")]) settings.pre_command_delay  =  root[F("pre_command_delay")];

        settings.autorun  =  root[F("autorun")];
        Serial.println(F("Writing Settings"));
        EEPROM_writeAnything(0, settings);
      }
      else {
        Serial.println("invalid JSON");
        Serial.println(json);
      }
    }

    else if (s.startsWith("get_settings") == true) {
      StaticJsonDocument<600> doc;

      //JsonObject root = doc.as<JsonObject>();
      doc["command"] = settings.command;
      if (strlen(settings.firmware_password) > 0) doc["firmware_password_set"] = true;
      else doc["firmware_password_set"] = false;
      doc["startup_delay"] = settings.startup_delay;
      doc["pre_command_delay"] = settings.pre_command_delay;
      doc["version"] = settings.version;
      doc["autorun"] = settings.autorun;
      serializeJson(doc, Serial);
      Serial.println();
    }
    else {
      Serial.println(F("Invalid command. Enter help for usage."));

    }
  }
  Serial.end();
}
void hold_recovery_key(int seconds) {
  int j;
  int led_flash_delay = 200; // ms
  int key_press_interval = 500; // ms
  BootKeyboard.begin();
  for (j = 0; j < (seconds * 1000) / key_press_interval; j++) {

    BootKeyboard.press(KEY_LEFT_GUI);
    BootKeyboard.press('r');
    delay(key_press_interval - led_flash_delay);
    flash_led(1);
    BootKeyboard.releaseAll();
  }
}
void enter_dep() {
  // Enter setup assistant. Assumes all screens that can be are skipped. 
  flash_led(3);
  BootKeyboard.begin();
  // Country picker - tab once, Continue
  presskey(KEY_TAB, 1, 40);
  presskey(KEY_SPACE, 1, 40);
  // Keyboard layout picker - tab x3, Continue
  presskey(KEY_TAB, 3, 40);
  presskey(KEY_SPACE, 1, 40);
  // Remote Management screen - tab x3, Continue
  presskey(KEY_TAB, 3, 40);
  presskey(KEY_SPACE, 1, 40);

  BootKeyboard.end();

  current_mode = CLI;

}
void enter_bootloader(){
  uint16_t bootKey = 0x7777;
  uint16_t *const bootKeyPtr = (uint16_t *)0x0800;

// Stash the magic key
*bootKeyPtr = bootKey;

// Set a watchdog timer
wdt_enable(WDTO_120MS);

while(1) {} // This infinite loop ensures nothing else
            // happens before the watchdog reboots us

}
void enter_recovery() {
  Serial.println(F("Running..."));
  Serial.end();
  BootKeyboard.begin();
  delay(500);


  int firmware_pw_len = strlen(settings.firmware_password);
  if (firmware_pw_len > 0) {
    int i;
    delay(500);
    for (i = 0; i < firmware_pw_len; i++) {

      BootKeyboard.press(settings.firmware_password[i]);
      delay(20);
      BootKeyboard.releaseAll();
      delay(500);
    }
    BootKeyboard.press(KEY_RETURN);
    delay(500);
    BootKeyboard.releaseAll();
  }
  delay(500);
  hold_recovery_key(30);
  long delay_completed=0;
  long total_delay = settings.startup_delay * 1000;
  int led_flash_delay = 200; // ms
  int led_flash_interval = 1000; // ms

  while(delay_completed<total_delay) {
    flash_led(2);
    delay(led_flash_interval - led_flash_delay * 2);
    delay_completed += led_flash_interval;
  }

  //not sure why theses escapes are here
  presskey(KEY_ESC, 1, 400);
  presskey(KEY_ESC, 1, 400);
  delay(500);

  //if desktop, need to hit space to dismiss mouse setup. otherwise it is just ignored
  presskey(KEY_SPACE, 1, 400);
  delay(10000);

  //skip language
  BootKeyboard.println("\n");
  delay(30000);

  //turn on menu navigation
  BootKeyboard.press(KEY_LEFT_CTRL);
  BootKeyboard.press(KEY_F2);
  BootKeyboard.releaseAll();

  //move to utilities menu
  presskey(KEY_RIGHT_ARROW, 4, 50);
  //move to terminal menu
  presskey(KEY_DOWN_ARROW, 4, 400);
  //go go activate!
  BootKeyboard.println("\n");

  //run command
  delay(settings.pre_command_delay * 1000);
  BootKeyboard.println(settings.command);
  BootKeyboard.end();

  //done, so go back to CLI
  Serial.begin(9600);
  delay(2000);
  current_mode = CLI;

}
void loop() {
//wdt_enable(WDTO_15MS);
//while(1) {};
digitalWrite(13, HIGH);
  Serial.begin(9600);
  delay(2000);
digitalWrite(13, LOW);
digitalWrite(13, HIGH);
  EEPROM_readAnything(0, settings);
  if (settings.version != CURRENTVERSION) {
    setdefaults();
  }
  Serial.println(F("Copyright 2018 Twocanoes Software, Inc."));
  Serial.println(F("Press <return> to enter configuration mode."));
  int i;
  current_mode = RECOVERY;
  if (settings.autorun == true) {
    for (i = 0; i < 10; i++) {
      if (Serial.available()) {
        while (Serial.available()) {
          Serial.read();
        }
        current_mode = CLI;
        break;
      }
      else {
        delay(500);
      }
    }
    
  }
  else {
    current_mode = CLI;
  }
digitalWrite(13, LOW);
  if (current_mode != CLI) {
    Serial.end();
  }
  while (1) {
    switch (current_mode) {

      case CLI:
        enter_cli();
        break;
      case DEP:
        enter_dep();
        break;
      case RECOVERY:
        enter_recovery();
        break;
    }
  }
}
