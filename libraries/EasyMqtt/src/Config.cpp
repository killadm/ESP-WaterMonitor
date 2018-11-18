#include "Config.h"
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include "FS.h"

Config::Config() {
  SPIFFS.begin();
}

void Config::load() {
  File f = SPIFFS.open("/config.cfg", "r");
  if (f) {
    while(f.available()) {
      String line = f.readStringUntil('\n');
      int pos = line.indexOf("=");
      String key = line.substring(0, pos);
      String value = line.substring(pos+1, line.length()-1);
      set(key.c_str(), value.c_str());
    }
    f.close();
  }
}

void Config::save() {
  File f = SPIFFS.open("/config.cfg", "w");
  if (f) {
    each([&](char *key, char *value) {
      f.print(key);
      f.print("=");
      f.println(value);
    });
    f.close();
  }
}

void Config::reset() {
  SPIFFS.remove("/config.cfg");
  ESP.restart();
}

void Config::each(std::function<void(char*, char*)> f) {
  struct element * ele = elements;
  while(ele) {
    f(ele->key, ele->value);
    ele = ele->next;
  }
}

int Config::getInt(const char *key, int defaultValue) {
  char defaultStr[16];
  sprintf(defaultStr, "%i", defaultValue);
  return atoi(get(key, defaultStr));
}
    
double Config::getDouble(const char *key, double defaultValue) {
  char defaultStr[16];
  sprintf(defaultStr, "%f", defaultValue);
  return atof(get(key, defaultStr));
}

long Config::getLong(const char *key, long defaultValue) {
  char defaultStr[16];
  sprintf(defaultStr, "%ld", defaultValue);
  return atol(get(key, defaultStr));
}

bool Config::getBool(const char *key, bool defaultValue) {
  return strcmp(get(key, defaultValue ? "true" : "false"), "true") == 0;
}

char * Config::get(const char *key, const char *defaultValue) {
  struct element * ele = elements;
  while(ele) {
    if(strcmp(key, ele->key) == 0) {
      return ele->value;
    }
    ele = ele->next;
  }
  return set(key, defaultValue);
}

char * Config::set(const char *key, const char *value){
  struct element * ele = elements;
  while(ele) {
    if(strcmp(key, ele->key) == 0) {
      if(ele->value) {
        free(ele->value);
      }
      ele->value = (char*)malloc(strlen(value)+1);
      strcpy(ele->value, value);
      return ele->value;
    }
    ele = ele->next;
  }
  ele = elements;
  elements = new element();
  elements->key = (char*)malloc(strlen(key)+1);
  strcpy(elements->key, key);
  elements->value = (char*)malloc(strlen(value)+1);
  strcpy(elements->value, value);
  elements->next = ele;
  return elements->value;
}
