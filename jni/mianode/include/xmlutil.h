#ifndef _XML_UTIL_H_
#define _XML_UTIL_H_
#include <libxml/parser.h>

xmlDocPtr xml_parser(const char * file_name);
void xml_free(xmlDocPtr docPtr);
bool xml_get_node(xmlDocPtr docPtr, xmlNodePtr *fatherPtr, xmlNodePtr *cur, bool root);
bool xml_get_item(xmlDocPtr docPtr, xmlNodePtr cur, char **key, char **value);
bool xml_get_prop(xmlNodePtr cur, const char *name, char **value);
#endif
