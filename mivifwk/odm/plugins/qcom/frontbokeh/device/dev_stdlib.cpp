/**
 * @file        dev_stdlib.c
 * @brief       this file provides some Standard library APIs
 * @details
 * @author      LZL
 * @date        2016.05.17
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */
#include "string.h"
#include <ctype.h>
#include "dev_stdlib.h"

typedef enum {
    _CHAR_0 = '0', _CHAR_9 = '9', _CHAR_A = 'A', _CHAR_F = 'F', _END_OF_STR = '\0'
} _Enum_Char;

#define _IS_NUMBER(alpha_char)   \
    (((alpha_char >= _CHAR_0) && (alpha_char <= _CHAR_9) ) ? 1 : 0)

static char* __StrToUpper(char *str) {
    char *pCh = str;
    if (!str) {
        return NULL;
    }
    for (; *pCh != '\0'; pCh++) {
        if (((*pCh) >= 'a') && ((*pCh) <= 'z')) {
            *pCh = Dev_toupper(*pCh);
        }
    }
    return str;
}

U8 Dev_xstoi(U8 *str, U8 *val) {
    U16 i = 0;
    U8 temp = 0;
    if (NULL == str || NULL == val) {
        return FALSE;
    }
    __StrToUpper((char*) str);
    while (str[i] != '\0') {
        if (_IS_NUMBER(str[i])) {
            temp = (temp << 4) + (str[i] - _CHAR_0);
        } else if ((str[i] >= _CHAR_A) && (str[i] <= _CHAR_F)) {
            temp = (temp << 4) + ((str[i] - _CHAR_A) + 10);
        } else {
            return FALSE;
        }
        i++;
    }
    *val = temp;
    return TRUE;
}

void Dev_atox(const char *s, U8 *hex, U32 *len) {
    U8 szTmp[3];
    S32 iCount = (S32) (Dev_strlen(s) / 2) + (Dev_strlen(s) % 2 > 0 ? 1 : 0);
    S32 i, iLeft = Dev_strlen(s);
    if (!hex) {
        return;
    }
    for (i = 0; i < iCount; i++) {
        Dev_memset(szTmp, '\0', sizeof(szTmp));
        Dev_strncpy((char*) szTmp, s, 2 > iLeft ? iLeft : 2);
        szTmp[2 > iLeft ? iLeft : 2] = 0;
        Dev_xstoi(szTmp, &hex[i]);
        s += 2;
        iLeft -= 2;
    }
    *len = iCount;
}

int Dev_xtoa(const U8 *in, char *out, int len) {
    const char ascTable[17] = { "0123456789ABCDEF" };
    char *tmp_p = out;
    int i, pos;
    pos = 0;
    for (i = 0; i < len; i++) {
        tmp_p[pos++] = ascTable[in[i] >> 4];
        tmp_p[pos++] = ascTable[in[i] & 0x0F];
    }
    tmp_p[pos] = '\0';
    return pos;
}
/* Parse S into tokens separated by characters in DELIM.
   If S is NULL, the saved pointer in SAVE_PTR is used as
   the next starting point.  For example:
        char s[] = "-abc-=-def";
        char *sp;
        x = strtok_r(s, "-", &sp);      // x = "abc", sp = "=-def"
        x = strtok_r(NULL, "-=", &sp);  // x = "def", sp = NULL
        x = strtok_r(NULL, "=", &sp);   // x = NULL
                // s = "abc\0-def\0"
*/

char* Dev_strtok_r(char *s, const char *delim, char **save_ptr) {
    char *token;
    if (s == NULL)
        s = *save_ptr;
    /* Scan leading delimiters.  */
    s += strspn(s, delim);
    if (*s == '\0')
        return NULL;
    /* Find the end of the token.  */
    token = s;
    s = strpbrk(token, delim);
    if (s == NULL)
        /* This token finishes the string.  */
        *save_ptr = strchr(token, '\0');
    else {
        /* Terminate the token and make *SAVE_PTR point past it.  */
        *s = '\0';
        *save_ptr = s + 1;
    }
    return token;
}
