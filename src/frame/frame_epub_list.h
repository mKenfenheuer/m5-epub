#ifndef _FRAME_EPUBLIST_H_
#define _FRAME_EPUBLIST_H_

#include "frame_base.h"
#include "../epdgui/epdgui.h"
#include "epub.h"

class Frame_EpubList : public Frame_Base
{
public:
    Frame_EpubList();
    ~Frame_EpubList();
    int init(epdgui_args_vector_t &args);
    int run();

private:
    void newListItem(epub_list_t *list);

private:
    std::vector<EPDGUI_Button*> _keys;
    bool _is_first;
    int _page_num = 1;
    int _current_page = 1;
};

#endif //_FRAME_EPUBLIST_H_

