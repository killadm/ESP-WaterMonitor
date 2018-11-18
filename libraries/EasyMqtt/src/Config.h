#pragma once

#include <functional>

class Config {
  private:
    struct element {
      char * key;
      char * value;
      struct element * next;
    };
    struct element * elements = nullptr;

  public:
    Config();
    void load();
    void save();
    void reset();

    void each(std::function<void(char*, char*)> f);

    int getInt(const char *key, int defaultValue);
    double getDouble(const char *key, double defaultValue);
    long getLong(const char *key, long defaultValue);
    bool getBool(const char *key, bool defaultValue);
    char * get(const char *key, const char *defaultValue);
    char * set(const char *key, const char *value);
};
