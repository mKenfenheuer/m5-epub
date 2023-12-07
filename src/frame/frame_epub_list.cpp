#include "frame_epub_list.h"
#include "frame_epub.h"
#include "epub.h"

#define MAX_ITEM_NUM 12

std::map<String, epub_t> epubmap;

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "Arduino.h"

void key_epublist_select_cb(epdgui_args_vector_t &args)
{
    EPDGUI_Button *key = (EPDGUI_Button *)(args[1]);
    Frame_Base *frame = new Frame_Epub(key->GetCustomString());
    EPDGUI_PushFrame(frame);
    *((int *)(args[0])) = 0;
}

Frame_EpubList::Frame_EpubList()
{
    _frame_name = "Frame_EpubList";

    _is_first = true;
}

Frame_EpubList::~Frame_EpubList()
{
}

esp_err_t Frame_EpubList::updateEpubs(fs::FS &fs)
{
    // SPI.begin(14, 13, 12, 4);
    delay(500);
    SPI.end();
    SPI.begin(14, 13, 12, 4);
    if (!SD.begin(4))
    {
        log_e("Card Mount Failed");
        return -1;
    }
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        log_e("No SD card attached");
        return -1;
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    log_d("SD Card Size: %lluMB\n", cardSize);
    log_d("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    log_d("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));

    File root = fs.open("/");
    log_d("Checking directory %s", root.name());
    while (true)
    {
        File entry = root.openNextFile();
        if (!entry)
            break;

        log_d("Checking file %s", entry.name());

        if (entry.isDirectory())
            continue;

        if (!String(entry.name()).endsWith(".epub"))
            continue;

        epub_t epub;
        epub.id = entry.name();
        epub.title = entry.name();
        epubmap.insert(std::pair<String, epub_t>(epub.id, epub));
    }
    root.close();
    SD.end();
    SPI.end();
    SPI.begin(14, 13, 12, 15);
    return 0;
}

void Frame_EpubList::newListItem(epub_t *list)
{
    EPDGUI_Button *key = new EPDGUI_Button(4, 71 + (_keys.size() % MAX_ITEM_NUM) * 80, 532, 80);
    _keys.push_back(key);

    uint16_t len = list->title.length();
    uint16_t n = 0;
    int charcnt = 0;
    char buf[len];
    memcpy(buf, list->title.c_str(), len);
    String title = list->title;
    while (n < len)
    {
        uint16_t unicode = key->CanvasNormal()->decodeUTF8((uint8_t *)buf, &n, len - n);
        if ((unicode > 0) && (unicode < 0x7F))
        {
            charcnt += 1;
        }
        else
        {
            charcnt += 2;
        }
        if (charcnt > 24)
        {
            title = title.substring(0, n);
            title += "...";
            break;
        }
    }

    key->setBMPButton(title, "", ImageResource_item_icon_tasklist_32x32);
    key->SetCustomString(list->id);
    key->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, &_is_run);
    key->AddArgs(EPDGUI_Button::EVENT_RELEASED, 1, key);
    key->Bind(EPDGUI_Button::EVENT_RELEASED, key_epublist_select_cb);
    if (epubmap.size() == 0)
    {
        key->SetEnable(false);
    }
}

int Frame_EpubList::run()
{
    if (_is_first)
    {
        _current_page = 1;
        log_d("heap = %d, psram = %d", ESP.getFreeHeap(), ESP.getFreePsram());
        _is_first = false;
        // LoadingAnime_32x32_Start(254, 424);

        esp_err_t err = updateEpubs(SD);

        for (int i = 0; i < _keys.size(); i++)
        {
            delete _keys[i];
        }

        _keys.clear();
        EPDGUI_Clear();
        if (err != ESP_OK)
        {
            // LoadingAnime_32x32_Stop();
            UserMessage(MESSAGE_ERR, 20000, "ERROR", String(err), "Touch the screen to retry");
            _is_first = true;
            M5.EPD.Clear();

            _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
            M5.EPD.UpdateFull(UPDATE_MODE_GC16);
            return 1;
        }
        _page_num = (epubmap.size() - 1) / MAX_ITEM_NUM + 1;
        for (auto iter = epubmap.begin(); iter != epubmap.end(); iter++)
        {
            newListItem(&(iter->second));
        }

        int size = _keys.size() > MAX_ITEM_NUM ? MAX_ITEM_NUM : _keys.size();
        for (int i = 0; i < size; i++)
        {
            EPDGUI_AddObject(_keys[i]);
        }

        LoadingAnime_32x32_Stop();
        M5.EPD.Clear();

        _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
        EPDGUI_Draw(UPDATE_MODE_NONE);
        M5.EPD.UpdateFull(UPDATE_MODE_GC16);
    }

    M5.update();

    if (M5.BtnR.wasReleased())
    {
        if (_current_page != _page_num)
        {
            _current_page++;
            EPDGUI_Clear();
            int start = (_current_page - 1) * MAX_ITEM_NUM;
            int end = _keys.size() > (_current_page * MAX_ITEM_NUM) ? (_current_page * MAX_ITEM_NUM) : _keys.size();
            for (int i = start; i < end; i++)
            {
                EPDGUI_AddObject(_keys[i]);
            }
            M5.EPD.Clear();

            _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
            EPDGUI_Draw(UPDATE_MODE_NONE);
            M5.EPD.UpdateFull(UPDATE_MODE_GC16);
        }
    }
    if (M5.BtnL.wasReleased())
    {
        if (_current_page != 1)
        {
            _current_page--;
            EPDGUI_Clear();
            int start = (_current_page - 1) * MAX_ITEM_NUM;
            int end = _keys.size() > (_current_page * MAX_ITEM_NUM) ? (_current_page * MAX_ITEM_NUM) : _keys.size();
            for (int i = start; i < end; i++)
            {
                EPDGUI_AddObject(_keys[i]);
            }
            M5.EPD.Clear();
            EPDGUI_Draw(UPDATE_MODE_NONE);

            _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
            M5.EPD.UpdateFull(UPDATE_MODE_GC16);
        }
    }
    if (M5.BtnP.wasReleased())
    {
        M5.EPD.Clear();

        _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
        M5.EPD.UpdateFull(UPDATE_MODE_GL16);
        _is_first = true;
    }

    return 1;
}

int Frame_EpubList::init(epdgui_args_vector_t &args)
{
    _is_run = 1;
    M5.EPD.Clear();

    _canvas_title = new M5EPD_Canvas(&M5.EPD);
    _canvas_title->createCanvas(540, 64);
    _canvas_title->drawFastHLine(0, 64, 540, 15);
    _canvas_title->drawFastHLine(0, 63, 540, 15);
    _canvas_title->drawFastHLine(0, 62, 540, 15);
    _canvas_title->setTextSize(26);
    _canvas_title->setTextDatum(CC_DATUM);
    _canvas_title->drawString("EPUB List", 270, 34);

    _current_page = 1;
    int size = _keys.size() > MAX_ITEM_NUM ? MAX_ITEM_NUM : _keys.size();
    for (int i = 0; i < size; i++)
    {
        EPDGUI_AddObject(_keys[i]);
    }
    return 0;
}
