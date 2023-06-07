#ifndef __MIJSON_H__
#define __MIJSON_H__

#include <map>
#include <string>
#include <vector>

#include "dataAdapter.h"

using namespace std;

class JsonObject
{
public:
    JsonObject();
    JsonObject(string);

    /*
     * set Json object format string to analize
     * the format like this:
     * { "key1":"value1", "key2", "value2"  }
     */
    void setJsonString(string);

    /*
     * get value as string by key
     */
    const char *getString(string);

    /*
     * get value as int by key
     */
    int getInt(string);

    /*
     * for the result which can not ensure type
     */
    StringDataAdapter getStringDataAdapter(string key);

    /*
     * get value as float by key
     */
    float getFloat(string);

    unsigned int size();

private:
    map<string, string> json;
};

class JsonArray
{
public:
    JsonArray();
    JsonArray(string);

    /*
     * set Json array format string to analize
     * the format like this:
     * ["value1", "value2"]
     */
    void setJsonString(string);

    /*
     *  get string value by index
     */
    const char *getString(unsigned int);

    /*
     *  get int value by index
     */
    int getInt(unsigned int);

    /*
     *  get float value by index
     */
    float getFloat(unsigned int);

    /*
     * for the result which can not ensure type
     */
    StringDataAdapter getStringDataAdapter(unsigned int);
    unsigned int size();

private:
    vector<string> json;
};
#endif
