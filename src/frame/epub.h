#ifndef _EPUB_H_
#define _EPUB_H_

#include "Arduino.h"
#include <vector>
#include <map>
#include <time.h>
#include "nvs.h"

typedef struct
{
    String title;
    String id;
}epub_t;

typedef struct
{
    String display_name;
    String id;
    std::map<String, epub_t> epubmap;
}epub_list_t;

#endif // _EPUB_H_