#pragma once

#include <functional>
#include <Arduino.h>


class Entry {
  private:
    std::function<void(String)> outFunction = NULL;
    std::function<String()> inFunction = NULL;
    std::function<void(Entry*, String)> publishFunction = NULL;

    char* name;
    int force = 0;
    int interval = -1;
    int forceUpdate = -1;
    unsigned long lastUpdate = 0;
    unsigned long lastPublish = 0;
    char* lastValue = NULL;
    bool persist = false;

    Entry* parent = NULL;
    Entry* next = NULL;
    Entry* children = NULL;

    std::function<void(Entry*, String)> getPublishFunction();

  protected:
    Entry(const char* name);
    Entry *getOrCreate(const char* name);
    Entry *getRoot();
    Entry *getParent();
    Entry *setParent(Entry& parent);
    Entry* addChild(Entry* child);
    void setPublishFunction(std::function<void(Entry*, String)> function);
    
  public:
    void debug(String key, bool value);
    void debug(String key, int value);
    void debug(String key, String value);
    void debug(String msg);

    void callback(const char* topic, uint8_t* payload, unsigned int length);
    
    /**
     * Request a updated value if needed
     */
    void update();
    void update(String payload);

    bool isIn();
    bool isOut();
    bool isRoot();
    bool isInternal();

    virtual String getTopic();

    int getInterval();
    void setInterval(int interval);
    void setInterval(int interval, int force);

    void setPersist(bool persist);

    int getForce();

    /**
     * Get last value
     */
    char *getValue();
    bool setValue(const char *value, bool force = false);

    /**
     * 
     */
    long getLastUpdate();

    /**
     * Publish value to mqtt
     */
    void publish(const char *message);
    
    /**
     * Iterate over each child, including sub children
     */
    void each(std::function<void(Entry*)> f);

    /**
     * Create or get the sub topic with the name {name}
     */
    Entry & get(const char* name);

    /**
     * Create or get the sub topic with the name {name}
     */
    Entry & operator[](int index);

    /**
     * Create or get the sub topic with the name {name}
     */
    Entry & operator[](const char* name);

    /**
     *  Read data from function and send it to mqtt
     */
    void operator<<(std::function<String()> inFunction);
    void operator<<(std::function<char *()> inFunction);
    void operator<<(std::function<float()> inFunction);

    /**
     *  Handle data comming from mqtt
     */
    void operator>>(std::function<void(String payload)> outFunction);
};
