#include "epdgui_label.h"
#include "Free_Fonts.h"

uint32_t EPDGUI_Label::_textbox_touching_id = 0;

EPDGUI_Label::EPDGUI_Label(int16_t x, int16_t y, int16_t w, int16_t h) : EPDGUI_Base(x, y, w, h)
{
    _canvas = new M5EPD_Canvas(&M5.EPD);

    _size = 26;
    _thiscreat = false;

    _canvas->createCanvas(_w, _h);

    _canvas->fillCanvas(15);
    //_canvas->drawRect(0, 0, _w, _h, 15);
    if (!_canvas->isRenderExist(_size))
    {
        _canvas->createRender(_size, 60);
        _thiscreat = true;
    }
    _canvas->setTextSize(_size);

    _canvas->setTextDatum(TL_DATUM);
    _canvas->setTextColor(15);

    for (int i = 0; i < 26; i++)
    {
        _canvas->preRender('a' + i);
        _canvas->preRender('A' + i);
    }

    _margin_left = 8;
    _margin_right = 8;
    _margin_top = 8;
    _margin_bottom = 8;

    _state = EVENT_NONE;
}

EPDGUI_Label::~EPDGUI_Label()
{
    delete _canvas;
}

void EPDGUI_Label::SetTextMargin(int16_t left, int16_t top, int16_t right, int16_t bottom)
{
    _margin_left = left;
    _margin_top = top;
    _margin_right = right + left;
    _margin_bottom = bottom + top;
}

void EPDGUI_Label::SetTextSize(uint16_t size)
{
    if (_thiscreat)
    {
        _canvas->destoryRender(_size);
    }
    _size = size;
    if (!_canvas->isRenderExist(_size))
    {
        _canvas->createRender(_size, 60);
        _thiscreat = true;
    }
    else
    {
        _thiscreat = false;
    }

    _canvas->setTextSize(_size);
    for (int i = 0; i < 26; i++)
    {
        _canvas->preRender('a' + i);
        _canvas->preRender('A' + i);
    }
    Draw(UPDATE_MODE_GC16);
}

void EPDGUI_Label::drawWordWrap(int maxX)
{
    float sizeperChar = 10.5f * _size / 26.0f;
    int index = 0;
    int sPosX = 0;
    while (index < _data.length())
    {
        String nextWord = _data.substring(index, _data.indexOf(" ", index + 1));
        index += nextWord.length();
        int width = nextWord.length() * sizeperChar;
        if(sPosX < _canvas->getCursorX())
        {
            sPosX = _canvas->getCursorX();
        }
        if (sPosX + width > maxX)
        {
            _canvas->println();
            nextWord.trim();
            sPosX = 0;
        }
        sPosX += width;
        _canvas->print(nextWord);
    }
}

void EPDGUI_Label::Draw(m5epd_update_mode_t mode)
{
    if (_ishide)
    {
        return;
    }

    if (_state == EVENT_NONE)
    {
        _canvas->setTextSize(_size);
        _canvas->fillCanvas(0);
        _canvas->drawRect(0, 0, _w, _h, 15);
        _canvas->setTextArea(_margin_left, _margin_top, _w - _margin_right, _h - _margin_bottom);
        int maxX = _w - _margin_right - _margin_left;
        drawWordWrap(maxX);
        _canvas->pushCanvas(_x, _y, mode);
    }
    else
    {
        _canvas->setTextSize(_size);
        _canvas->fillCanvas(0);
        _canvas->drawRect(0, 0, _w, _h, 15);
        _canvas->drawRect(1, 1, _w - 2, _h - 2, 15);
        _canvas->drawRect(2, 2, _w - 4, _h - 4, 15);

        _canvas->setTextArea(_margin_left, _margin_top, _w - _margin_right, _h - _margin_bottom);
        int maxX = _w - _margin_right - _margin_left;
        drawWordWrap(maxX);
        _canvas->pushCanvas(_x, _y, mode);
    }
}

void EPDGUI_Label::Draw(M5EPD_Canvas *canvas)
{
    if (_ishide)
    {
        return;
    }

    if (_state == EVENT_NONE)
    {
        _canvas->setTextSize(_size);
        _canvas->fillCanvas(0);
        _canvas->drawRect(0, 0, _w, _h, 15);
        _canvas->setTextWrap(true, true);
        _canvas->setTextArea(_margin_left, _margin_top, _w - _margin_right, _h - _margin_bottom);
        int maxX = _w - _margin_right - _margin_left;
        drawWordWrap(maxX);
        _canvas->pushToCanvas(_x, _y, canvas);
    }
    else
    {
        _canvas->setTextSize(_size);
        _canvas->fillCanvas(0);
        _canvas->drawRect(0, 0, _w, _h, 15);
        _canvas->drawRect(1, 1, _w - 2, _h - 2, 15);
        _canvas->drawRect(2, 2, _w - 4, _h - 4, 15);
        _canvas->setTextWrap(true, true);
        _canvas->setTextArea(_margin_left, _margin_top, _w - _margin_right, _h - _margin_bottom);
        int maxX = _w - _margin_right - _margin_left;
        drawWordWrap(maxX);
        _canvas->pushToCanvas(_x, _y, canvas);
    }
}

void EPDGUI_Label::Bind(int16_t event, void (*func_cb)(epdgui_args_vector_t &))
{
}

void EPDGUI_Label::UpdateState(int16_t x, int16_t y)
{
    if (!_isenable)
    {
        return;
    }

    int16_t state = _state;
    if ((_state == EVENT_PRESSED) && (_textbox_touching_id != _id))
    {
        state = EVENT_NONE;
    }

    if (isInBox(x, y))
    {
        _textbox_touching_id = _id;
        state = EVENT_PRESSED;
    }

    SetState(state);
}

void EPDGUI_Label::SetState(int16_t state)
{
    if (state != _state)
    {
        if (state == EVENT_PRESSED)
        {
            _textbox_touching_id = _id;
        }
        _state = state;
    }
}

void EPDGUI_Label::SetText(String text)
{
    if (text != _data)
    {
        _data = text;
        Draw(UPDATE_MODE_A2);
    }
}

void EPDGUI_Label::Remove(int16_t idx)
{
    uint16_t n = 0, last_n = 0;
    uint8_t *buf = (uint8_t *)_data.c_str();
    uint16_t len = strlen((char *)buf);
    uint16_t cnt = 0;
    while (n < len)
    {
        last_n = n;
        _canvas->decodeUTF8(buf, &n, len - n);
        if (cnt == idx)
        {
            _data.remove(last_n, n - last_n);
            return;
        }
        cnt++;
    }
    if (idx == -1)
    {
        _data.remove(last_n, n - last_n);
    }
}

void EPDGUI_Label::AddText(String text)
{
    if (text.length() == 0)
    {
        return;
    }

    uint8_t *buf = (uint8_t *)text.c_str();
    uint16_t len = strlen((char *)buf);
    uint16_t n = 0, last_n = 0;

    while (n < len)
    {
        last_n = n;
        uint16_t uniCode = _canvas->decodeUTF8(buf, &n, len - n);
        if (uniCode == 0x0008)
        {
            Remove(-1);
        }
        else
        {
            _data += text.substring(last_n, n);
        }
    }

    Draw(UPDATE_MODE_A2);
}
