#include "frame_epub.h"
#include "frame_epub_list.h"

#include "unzipLIB.h"
#include "tinyxml2.h"

UNZIP zip; // statically allocate the UNZIP structure (41K)
String text = "Error reading EPUB";

// Functions to access a file on the SD card
static File myfile;

//
// Callback functions needed by the unzipLIB to access a file system
// The library has built-in code for memory-to-memory transfers, but needs
// these callback functions to allow using other storage media
//

void *ZipOpen(const char *filename, int32_t *size)
{
    myfile = SD.open(filename);
    *size = myfile.size();
    return (void *)&myfile;
}

void ZipClose(void *p)
{
    ZIPFILE *pzf = (ZIPFILE *)p;
    File *f = (File *)pzf->fHandle;
    if (f)
        f->close();
}

int32_t ZipRead(void *p, uint8_t *buffer, int32_t length)
{
    ZIPFILE *pzf = (ZIPFILE *)p;
    File *f = (File *)pzf->fHandle;
    return f->read(buffer, length);
}

int32_t ZipSeek(void *p, int32_t position, int iType)
{
    ZIPFILE *pzf = (ZIPFILE *)p;
    File *f = (File *)pzf->fHandle;
    if (iType == SEEK_SET)
        return f->seek(position);
    else if (iType == SEEK_END)
    {
        return f->seek(position + pzf->iSize);
    }
    else
    { // SEEK_CUR
        long l = f->position();
        return f->seek(l + position);
    }
}

void key_exit_epub_cb(epdgui_args_vector_t &args)
{
    EPDGUI_Button *key = (EPDGUI_Button *)(args[1]);
    Frame_Base *frame = new Frame_EpubList();
    EPDGUI_PushFrame(frame);
    *((int *)(args[0])) = 0;
}

String getRootFile(String xml)
{
    // parse the meta data
    tinyxml2::XMLDocument meta_data_doc;
    auto result = meta_data_doc.Parse(xml.c_str());
    // finished with the data as it's been parsed
    if (result != tinyxml2::XML_SUCCESS)
    {
        log_d("Could not parse META-INF/container.xml");
        return "";
    }
    auto container = meta_data_doc.FirstChildElement("container");
    if (!container)
    {
        log_d("Could not find container element in META-INF/container.xml");
        return "";
    }
    auto rootfiles = container->FirstChildElement("rootfiles");
    if (!rootfiles)
    {
        log_d("Could not find rootfiles element in META-INF/container.xml");
        return "";
    }
    // find the root file that has the media-type="application/oebps-package+xml"
    auto rootfile = rootfiles->FirstChildElement("rootfile");
    while (rootfile)
    {
        const char *media_type = rootfile->Attribute("media-type");
        if (media_type && strcmp(media_type, "application/oebps-package+xml") == 0)
        {
            const char *full_path = rootfile->Attribute("full-path");
            if (full_path)
            {
                return full_path;
            }
        }
        rootfile = rootfile->NextSiblingElement("rootfile");
    }
    return "";
}

String readZipFile(String fileName, String file)
{
    String fileText = "";
    SPI.end();
    SPI.begin(14, 13, 12, 4);
    if (!SD.begin(4))
    {
        log_e("Card Mount Failed");
        return "";
    }
    const char *name = fileName.c_str();
    int len;

    if (zip.openZIP(name, ZipOpen, ZipClose, ZipRead, ZipSeek) != UNZ_OK)
        goto end;
    log_d("Open ok");

    if (zip.locateFile(file.c_str()) != UNZ_OK)
        goto end;
    log_d("Container ok");

    if (zip.openCurrentFile() != UNZ_OK)
        goto end;
    log_d("Open ZipFile ok");

    uint8_t buffer[100];

    len = zip.readCurrentFile(buffer, sizeof(buffer));
    while (len > 0)
    {
        for (int i = 0; i < len; i++)
            fileText += (char)buffer[i];
        len = zip.readCurrentFile(buffer, sizeof(buffer));
    }

    zip.closeCurrentFile();

end:
    zip.closeZIP();
    SD.end();
    SPI.end();
    SPI.begin(14, 13, 12, 15);
    return fileText;
}

Frame_Epub::Frame_Epub(String title)
{
    epubFile = title;
    _frame_name = "Frame_Epub";

    _canvas_title = new M5EPD_Canvas(&M5.EPD);
    _canvas_title->createCanvas(540, 64);
    _canvas_title->drawFastHLine(0, 64, 540, 15);
    _canvas_title->drawFastHLine(0, 63, 540, 15);
    _canvas_title->drawFastHLine(0, 62, 540, 15);
    _canvas_title->setTextSize(26);
    _canvas_title->setTextDatum(CC_DATUM);

    _canvas = new M5EPD_Canvas(&M5.EPD);
    _canvas->createCanvas(264, 60);
    _canvas->fillCanvas(0);
    _canvas->setTextSize(26);
    _canvas->setTextColor(15);
    _canvas->setTextDatum(CL_DATUM);

    epubText = new EPDGUI_Label(0, 71, 540, 960 - 71);
    epubText->SetTextMargin(16, 16, 16, 16);
    epubText->SetTextSize(30);

    text = readZipFile("/" + title, "META-INF/container.xml");

    tinyxml2::XMLDocument meta_data_doc;
    meta_data_doc.Parse(text.c_str());

    String rootFile = getRootFile(text);

    String index = readZipFile("/" + title, rootFile);

    epubText->SetText(index);

    _key_exit = new EPDGUI_Button(8, 12, 150, 48);
    _key_exit->CanvasNormal()->fillCanvas(0);
    _key_exit->CanvasNormal()->setTextSize(26);
    _key_exit->CanvasNormal()->setTextDatum(CL_DATUM);
    _key_exit->CanvasNormal()->setTextColor(15);
    _key_exit->CanvasNormal()->drawString("Exit", 47 + 13, 28);
    _key_exit->CanvasNormal()->pushImage(15, 8, 32, 32, ImageResource_item_icon_arrow_l_32x32);
    *(_key_exit->CanvasPressed()) = *(_key_exit->CanvasNormal());
    _key_exit->CanvasPressed()->ReverseColor();

    //_canvas->drawString(title, 11, 35);
    _canvas_title->drawString(title, 270, 34);

    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, &_is_run);
    _key_exit->AddArgs(EPDGUI_Button::EVENT_RELEASED, 1, _key_exit);
    _key_exit->Bind(EPDGUI_Button::EVENT_RELEASED, key_exit_epub_cb);

    log_d("EPUB_INIT");
}

Frame_Epub::~Frame_Epub(void)
{
}
int Frame_Epub::run()
{
    return 2;
}

int Frame_Epub::init(epdgui_args_vector_t &args)
{
    log_d("EPUB_init");
    _is_run = 1;
    M5.EPD.Clear();
    log_d("Cleared");
    _canvas_title->pushCanvas(0, 8, UPDATE_MODE_NONE);
    _canvas->pushCanvas(4, 380, UPDATE_MODE_NONE);
    EPDGUI_AddObject(_key_exit);
    EPDGUI_AddObject(epubText);
    EPDGUI_Draw(UPDATE_MODE_NONE);
    log_d("Drawed");
    while (M5.EPD.UpdateFull(UPDATE_MODE_GC16) != m5epd_err_t::M5EPD_OK)
        ;
    log_d("Updated");
    return 2;
}