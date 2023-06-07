#include <xmlutil.h>

xmlDocPtr xml_parser(const char * file_name)
{
    return xmlParseFile(file_name);
}

void xml_free(xmlDocPtr docPtr)
{
    xmlFreeDoc(docPtr);
}

bool xml_get_node(xmlDocPtr docPtr, xmlNodePtr *fatherPtr, xmlNodePtr *cur, bool root)
{
    if (root) {
        if (!*fatherPtr) {
            *fatherPtr = xmlDocGetRootElement(docPtr);
            *cur = *fatherPtr;
            return true;
        }
        else {
            return false;
        }
    }

    if (!*cur)
        *cur = xmlFirstElementChild(*fatherPtr);
    else
        *cur = xmlNextElementSibling(*cur);

    if (*cur)
        return true;
    else
        return false;
}

bool xml_get_item(xmlDocPtr docPtr, xmlNodePtr cur, char **key, char **value)
{
    *key = (char *)cur->name;
    *value = (char *)xmlNodeListGetString(docPtr, cur->xmlChildrenNode, 1);
    return true;
}

bool xml_get_prop(xmlNodePtr cur, const char *name, char **value)
{
    *((xmlChar **)value) = xmlGetProp(cur, (const xmlChar*)name);
    if (*value)
        return true;
    else
        return false;
}
