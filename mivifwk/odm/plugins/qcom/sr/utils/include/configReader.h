#ifndef __CONFIGREADER_H__
#define __CONFIGREADER_H__

class ConfigReader
{
public:
    ConfigReader(string path);
    ConfigReader *update();
    ConfigReader *trim();
    const char *getContent();

private:
    const string path;
    string content;
};

#endif
