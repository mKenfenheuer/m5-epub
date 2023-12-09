#ifndef _FRAME_EPUB_H_
#define _FRAME_EPUB_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"
#include "epub.h"

class Frame_Epub : public Frame_Base
{
public:
    Frame_Epub(String file);
    ~Frame_Epub();
    int init(epdgui_args_vector_t &args);
    int run();
    int NextPage();
    int PrevPage();

private:
    void newListItem(epub_t *epub);
    esp_err_t updateEpubs(fs::FS &fs);

private:
    M5EPD_Canvas *_canvas;
    String epubFile;
    EPDGUI_Label *epubText;
    std::vector<EPDGUI_Button *> _keys;
    bool _is_first;
    int _page_num = 1;
    int _current_page = 1;
};

#endif //_FRAME_EPUB_H_
