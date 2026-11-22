#include "FT812.h"

#include "display.h"
#include "displayControls.h"
#include "list.h"

#include "Tracker.h"
#include <string.h>

#define TEXT_SCROLL_SPEED_DIV 30
#define TEXT_SCROLL_SPACER    30
#define TEXT_SCROLL_FONT_WIDTH_CORRECTION                                                          \
    (-2) // strFont::width has to be corrected to be used for stride calculation

cList::cList(strControlProperties* properties)
  : cDisplayControl(controlIdList, 1)
{
    colorsCount = 7;
    colors = display.getTheme().activeListColors;

    listPosition = 0;

    listAnimationStep = 0;
    disableBar = 0;

    if (properties == nullptr)
    {
        posX = 0;
        posY = 0;
        text = nullptr;
        value = 0;
        width = 0;
        height = 0;
        style = 0;
        return;
    }

    colors = (properties->colors == nullptr) ? colors : properties->colors;

    posX = properties->x;
    posY = properties->y;

    // text = properties->text;
    value = properties->value;

    width = properties->w;
    height = properties->h;

    textScrollTime = 0;

    setStyle(properties->style);

    setData(properties->data);
}

cList::~cList()
{
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
void cList::setStyle(uint32_t style)
{
    this->style = style;

    textStyle = (style & controlStyleCenterX ? OPT_CENTERX : 0)
                // | (style & controlStyleCenterY ? OPT_CENTERY : 0)
                | (style & controlStyleRightX ? OPT_RIGHTX : 0);

    int8_t textFont = FONT_INDEX_FROM_STYLE;
    textFont = (textFont >= 0) ? textFont : 0;
    font = &fonts[textFont];
}

void cList::setText(const char* text)
{
    this->text = text;
}

void cList::setValue(int value)
{
    if (value >= list->length)
    {
        if (list->length > 0)
            listPosition = list->length - 1;
        else
            listPosition = 0;

        disableBar = 0;
    }
    else if (value < 0)
    {
        listPosition = 0;
        disableBar = 1;
    }
    else
    {
        listPosition = value;
        disableBar = 0;
    }
}

void cList::setColors(const uint32_t* colors)
{
    this->colors = colors;
}

void cList::setData(void* data)
{
    if (data == nullptr)
        return;
    list = (strList*)data;
    listAnimationStep = 0;

    if (list->start < 0)
    {
        list->start = 0;
        listPosition = 0;
        disableBar = 1;
    }
    else
    {
        listPosition = list->start;
        disableBar = 0;
    }

    if (listPosition == 0) // pierwsza pozycja listy*
    {
        barPos = 0;
        textListPos = 0;
    }
    else if (listPosition == 1) // druga pozycja listy
    {
        barPos = 1;
        textListPos = 0;
    }
    else if (listPosition >= list->length - 1) // ostatnia pozycja listy
    {
        barPos = (list->length > list->linesCount ? list->linesCount - 1 : list->length - 1);
        textListPos = (list->length > list->linesCount ? list->length - (list->linesCount) : 0);
    }
    else if (listPosition >= list->length - 2) // przed ostatnia pozycja listy
    {
        barPos = (list->length > list->linesCount ? list->linesCount - 2 : list->length - 2);
        textListPos = (list->length > list->linesCount ? list->length - (list->linesCount) : 0);
    }
    else
    {
        barPos = (list->length > list->linesCount
                    ? (listPosition > list->linesCount - 2 ? list->linesCount - 2 : listPosition)
                    : listPosition);
        textListPos =
          (list->length > list->linesCount
             ? (listPosition > list->linesCount - 2 ? listPosition - (list->linesCount - 2) : 0)
             : 0);
    }
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
void cList::update()
{
    if (list == nullptr)
        return;

    Timer* timer = Tracker::getInstance()->getPlatform()->getTimer();
    // int16_t posX = this->posX;
    int16_t posY = this->posY;

    if (textScrollTimeStart != 0)
    {

        textScrollTime = timer->millis() / TEXT_SCROLL_SPEED_DIV;
    }

    if (style & controlStyleCenterY)
    {
        posY = this->posY - (height / 2);
    }

    int16_t x_pos = posX, y_pos, h_row = 27; // font->height+8;
    uint16_t w_bar = width - 6;              // szerokosc ramki
    uint8_t lines;

    if (list->length > list->linesCount)
        w_bar = width - 13;

    // tlo listy
    if (style & controlStyleBackground)
    {
        API_COLOR(colors[3]);
        API_LINE_WIDTH(16);
        API_BEGIN(RECTS);
        API_VERTEX2F(posX, posY);
        API_VERTEX2F(posX + width, posY + height);
        API_END();

        API_BLEND_FUNC(SRC_ALPHA, ZERO);

        if (style & controlStyleBottomShadow)
        {
            API_SAVE_CONTEXT();
            uint16_t grad_y = posY + height - 10;
            uint16_t grad_h = 12;
            API_SCISSOR_XY(posX, grad_y);
            API_SCISSOR_SIZE(width + 1, grad_h);
            API_CMD_GRADIENT(0, grad_y, colors[3], 0, grad_y + grad_h, 0x0);
            API_RESTORE_CONTEXT();
        }

        API_BLEND_FUNC(SRC_ALPHA, ONE_MINUS_SRC_ALPHA);
    }

    posY += 4; // top margin

    if (list->start == listPosition)
    {
        // geneorwanie poczatkowego menu
        listAnimationStep = 0;
        autoRefresh = false;
        highPrioRefresh = false;

        if (!disableBar)
        {
            x_pos = posX + 3;
            y_pos = posY + (barPos * h_row);

            // ramka
            if (list->selectionActive)
            {
                API_COLOR(colors[1]);
            }
            else
            {
                API_COLOR(colors[0]);
            }

            API_LINE_WIDTH(kRedFrameWidth);
            API_BEGIN(LINE_STRIP);
            API_VERTEX2F(x_pos, y_pos);
            API_VERTEX2F(x_pos + w_bar, y_pos);
            API_VERTEX2F(x_pos + w_bar, y_pos + h_row);
            API_VERTEX2F(x_pos, y_pos + h_row);
            API_VERTEX2F(x_pos, y_pos);
            API_END();

            {
                // set back old width 8, to avoid regressions
                // TRK-205 b#2
                API_LINE_WIDTH(8);
            }
        }

        // lista tekstow
        x_pos = posX + 13;
        y_pos = posY + h_row / 2;

        lines = (list->length >= list->linesCount) ? list->linesCount : list->length;

        if (textScrollItem != list->start)
        {
            textScrollTimeStart = 0;
            textScrollItem = list->start;
        }

        API_SAVE_CONTEXT();

        API_COLOR(colors[4]);

        API_SCISSOR_XY(posX, posY);
        API_SCISSOR_SIZE(w_bar, h_row * list->linesCount);

        for (uint8_t i = 0; i < lines; i++)
        {
            uint16_t rowIdx = i + textListPos;
            if (list->selectTab != NULL && list->selectTab[rowIdx])
            {
                API_COLOR(colors[5] != colors[4] ? colors[5] : display.getTheme().red);
            }
            else if (!disableBar && rowIdx == list->start)
            {
                API_COLOR(colors[5]);
            }
            else
            {
                API_COLOR(colors[4]);
            }

            char** row_text = (list->data + rowIdx);

            if (row_text != nullptr)
            {
                char* text = *row_text;
                size_t textMaxWidth = list->lineWidthMax;
                int xOfs = 0;
                bool scrollActive = false;
                size_t textLen = strlen(text);
                size_t textWidth = 0;

                if (textMaxWidth > 0 && textScrollItem == i + textListPos)
                {
                    textWidth = getTextWidth((strFont*)font, text, textLen);
                    if (textWidth > textMaxWidth)
                    {
                        if (textScrollTimeStart == 0)
                        {
                            textScrollTimeStart = timer->millis() / TEXT_SCROLL_SPEED_DIV;
                            textScrollTime = textScrollTimeStart;
                        }

                        uint32_t deltaTime = (textScrollTime - textScrollTimeStart);
                        deltaTime = deltaTime < 11
                                      ? (deltaTime * deltaTime) / 30
                                      : deltaTime - 7; // smooth out start with quadratic function
                        text += xOfs > font->data[(uint8_t)text[0]] ? 1 : 0;
                        xOfs = deltaTime % (textWidth + TEXT_SCROLL_SPACER);
                        scrollActive = true;
                        autoRefresh = true;

                        API_SCISSOR_XY(x_pos, y_pos + (i * h_row) - h_row / 2);
                        API_SCISSOR_SIZE(list->lineWidthMax, h_row);
                    }
                }
                else
                {
                    textLen = list->lineTextLenMax;
                }

                API_CMD_TEXT(x_pos - xOfs,
                             y_pos + (i * h_row),
                             font->handle,
                             textStyle | OPT_CENTERY,
                             text,
                             textLen);

                if (scrollActive)
                {
                    // display wrap-around
                    size_t currentTextLen = textWidth + TEXT_SCROLL_SPACER;
                    textMaxWidth = list->lineWidthMax - currentTextLen;
                    if (textMaxWidth > 0)
                    {
                        API_CMD_TEXT(x_pos - xOfs + currentTextLen,
                                     y_pos + (i * h_row),
                                     font->handle,
                                     textStyle | OPT_CENTERY,
                                     *row_text,
                                     list->lineTextLenMax); // + 2);	// + first and last
                                                            // character cut by scissors
                    }

                    API_SCISSOR_XY(posX, posY);
                    API_SCISSOR_SIZE(w_bar, h_row * list->linesCount);
                }
            }

            //			if(list->icon.useSpecialIcons)
            //			{
            //				if((list->icon.iconPositionInList) == i+textListPos)
            //				{
            //					uint8_t idx = list->icon.iconNum;
            //
            //					API_BITMAP_SOURCE(bitmaps[idx].address);
            //					API_BITMAP_LAYOUT(bitmaps[idx].format, bitmaps[idx].linestride,
            // bitmaps[idx].height); 					API_BITMAP_SIZE(NEAREST, BORDER, BORDER,
            // bitmaps[idx].width, bitmaps[idx].height);
            //
            //					API_BEGIN(BITMAPS);
            //					API_VERTEX2F(x_pos, y_pos + ((i*h_row) - bitmaps[idx].height/2) +
            // 1); 					API_END();
            //
            //				}
            //			}
        }

        API_RESTORE_CONTEXT();
    }
    else
    {
        int8_t dir, mode;   // kieurnek przewijania ; tryb/rodzaj przewijania
        uint16_t diffrence; // skok belki/listy w pixelach przy jedym odsiwezeniu listy

        // Geneorwanie zmieny menu.

        // okreslenie kierunku i skoku listy
        if (list->start > listPosition)
        {
            dir = -1;
            diffrence = list->start - listPosition;
        }
        else
        {
            dir = 1;
            diffrence = listPosition - list->start;
        }

        if (diffrence > 8)
            diffrence = 75;
        else if (diffrence > 4)
            diffrence = 50;
        else if (diffrence > 1)
            diffrence = 25;
        else
            diffrence = 8; // 2;

        mode = 0;
        if (barPos == 0 || barPos == list->linesCount - 1)
            mode = 0;
        else if (barPos == 1)
            mode = ((dir == 1 || list->start == 1) ? 0 : 1);
        else if (barPos == list->linesCount - 2)
            mode = ((dir == -1 || list->start == list->length - 2) ? 0 : 1);
        else
            mode = 0;

        if (mode == 0) // ruch belka(mode = 0)
        {
            listAnimationStep = listAnimationStep + (dir * diffrence);
            if (listAnimationStep >= h_row)
            {
                listAnimationStep = 0;
                barPos = barPos + 1;
                list->start = list->start + 1;
            }
            else if (listAnimationStep <= -1 * h_row)
            {
                listAnimationStep = 0;
                barPos = barPos - 1;
                list->start = list->start - 1;
            }
        }
        else // ruch tekstem(mode = 1)
        {
            listAnimationStep = listAnimationStep + (dir * diffrence);
            if (listAnimationStep >= h_row)
            {
                listAnimationStep = 0;
                list->start = list->start + 1;
                textListPos = textListPos + 1;
            }
            else if (listAnimationStep <= -1 * h_row)
            {
                listAnimationStep = 0;
                list->start = list->start - 1;
                if (textListPos != 0)
                {
                    textListPos = textListPos - 1;
                }
            }
        }
        highPrioRefresh = true;

        if (!disableBar)
        {
            x_pos = posX + 3;
            y_pos = posY + (barPos * h_row) + (mode ? 0 : listAnimationStep);
            if (list->length > list->linesCount)
                w_bar = width - 13;

            // ramka
            if (list->selectionActive)
            {
                API_COLOR(colors[1]);
            }
            else
            {
                API_COLOR(colors[0]);
            }
            API_LINE_WIDTH(kRedFrameWidth);
            API_BEGIN(LINE_STRIP);
            API_VERTEX2F(x_pos, y_pos);
            API_VERTEX2F(x_pos + w_bar, y_pos);
            API_VERTEX2F(x_pos + w_bar, y_pos + h_row);
            API_VERTEX2F(x_pos, y_pos + h_row);
            API_VERTEX2F(x_pos, y_pos);
            API_END();

            {
                // set back old width 8, to avoid regressions
                // TRK-205 b#2
                API_LINE_WIDTH(8);
            }
        }

        x_pos = posX + 13;
        y_pos = posY + h_row / 2 + (mode ? ((-1 * h_row) - listAnimationStep) : 0);

        API_SAVE_CONTEXT();

        API_COLOR(colors[4]);

        API_SCISSOR_XY(posX, posY);
        API_SCISSOR_SIZE(w_bar, h_row * list->linesCount);

        lines = (list->length >= list->linesCount) ? list->linesCount : list->length;

        if (mode == 1)
        {
            if (dir == 1) // tekst porusza sie w gore (wartosci w dol)
            {
                y_pos += h_row;

                for (int8_t i = 0; i < lines; i++)
                {
                    uint16_t rowIdx = i + textListPos;
                    if (list->selectTab != NULL && list->selectTab[rowIdx])
                    {
                        API_COLOR(colors[5] != colors[4] ? colors[5] : display.getTheme().red);
                    }
                    else if (!disableBar && rowIdx == list->start)
                    {
                        API_COLOR(colors[5]);
                    }
                    else
                    {
                        API_COLOR(colors[4]);
                    }

                    API_CMD_TEXT(x_pos,
                                 y_pos + (i * h_row),
                                 font->handle,
                                 textStyle | OPT_CENTERY,
                                 *(list->data + rowIdx),
                                 list->lineTextLenMax);

                    //					if(list->icon.useSpecialIcons)
                    //					{
                    //						if((list->icon.iconPositionInList) == i+textListPos)
                    //						{
                    //							uint8_t idx = list->icon.iconNum;
                    //
                    //							API_BITMAP_SOURCE(bitmaps[idx].address);
                    //							API_BITMAP_LAYOUT(bitmaps[idx].format,
                    // bitmaps[idx].linestride,
                    // bitmaps[idx].height); 							API_BITMAP_SIZE(NEAREST,
                    // BORDER, BORDER, bitmaps[idx].width, bitmaps[idx].height);
                    //
                    //							API_BEGIN(BITMAPS);
                    //							API_VERTEX2F(x_pos, y_pos + ((i*h_row) -
                    // bitmaps[idx].height/2
                    //+ 1)); 							API_END();
                    //						}
                    //					}
                }
            }
            else if (dir == -1) // tekst porusza sie w dol (wartosci w gore)
            {
                for (int8_t i = 0; i < (lines + 1); i++)
                {
                    int16_t table_offset = textListPos - 1;
                    if (table_offset < 0)
                        table_offset = 0;

                    uint16_t rowIdx = i + table_offset;
                    if (list->selectTab != NULL && list->selectTab[rowIdx])
                    {
                        API_COLOR(colors[5] != colors[4] ? colors[5] : display.getTheme().red);
                    }
                    else if (!disableBar && rowIdx == list->start)
                    {
                        API_COLOR(colors[5]);
                    }
                    else
                    {
                        API_COLOR(colors[4]);
                    }

                    API_CMD_TEXT(x_pos,
                                 y_pos + (i * h_row),
                                 font->handle,
                                 textStyle | OPT_CENTERY,
                                 *(list->data + rowIdx),
                                 list->lineTextLenMax);

                    //					if(list->icon.useSpecialIcons)
                    //					{
                    //						if((list->icon.iconPositionInList) == i + table_offset)
                    //						{
                    //							uint8_t idx = list->icon.iconNum;
                    //							API_BITMAP_SOURCE(bitmaps[idx].address);
                    //							API_BITMAP_LAYOUT(bitmaps[idx].format,
                    // bitmaps[idx].linestride,
                    // bitmaps[idx].height); 							API_BITMAP_SIZE(NEAREST,
                    // BORDER, BORDER, bitmaps[idx].width, bitmaps[idx].height);
                    //
                    //							API_BEGIN(BITMAPS);
                    //							API_VERTEX2F(x_pos, y_pos + ((i*h_row) -
                    // bitmaps[idx].height/2
                    //+ 1)); 							API_END();
                    //						}
                    //					}
                }
            }
        }
        else
        {
            for (uint8_t i = 0; i < lines; i++)
            {
                uint16_t rowIdx = i + textListPos;
                if (list->selectTab != NULL && list->selectTab[rowIdx])
                {
                    API_COLOR(colors[5] != colors[4] ? colors[5] : display.getTheme().red);
                }
                else if (!disableBar && rowIdx == list->start)
                {
                    API_COLOR(colors[5]);
                }
                else
                {
                    API_COLOR(colors[4]);
                }

                API_CMD_TEXT(x_pos,
                             y_pos + (i * h_row),
                             font->handle,
                             textStyle | OPT_CENTERY,
                             *(list->data + rowIdx),
                             list->lineTextLenMax);

                if (list->icon.useSpecialIcons)
                {
                    if ((list->icon.iconPositionInList) == i + textListPos)
                    {
                        uint8_t idx = list->icon.iconNum;

                        API_BITMAP_SOURCE(bitmaps[idx].address);
                        API_BITMAP_LAYOUT(
                          bitmaps[idx].format, bitmaps[idx].linestride, bitmaps[idx].height);
                        API_BITMAP_SIZE(
                          NEAREST, BORDER, BORDER, bitmaps[idx].width, bitmaps[idx].height);

                        API_BEGIN(BITMAPS);
                        API_VERTEX2F(x_pos, y_pos + ((i * h_row) - bitmaps[idx].height / 2 + 1));
                        API_END();
                    }
                }
            }
        }

        API_RESTORE_CONTEXT();
    }

    // pasek przewijania
    if (list->length > list->linesCount)
    {
        int16_t y_length = ((height - 14) * list->linesCount) / list->length;
        y_pos = posY + 6 + (list->start * ((height - 14) - y_length)) / list->length;

        // API_BLEND_FUNC(SRC_ALPHA , ZERO);

        API_COLOR(colors[6]);
        API_LINE_WIDTH(16);
        API_BEGIN(RECTS);
        API_VERTEX2F(posX + (width - 6), posY + 4);
        API_VERTEX2F(posX + (width - 3), (posY + height) - 10);
        API_END();

        API_COLOR(colors[2]);
        API_LINE_WIDTH(8);
        API_BEGIN(LINES);
        // API_LINE_WIDTH(24);
        // API_BEGIN(RECTS);
        API_VERTEX2F(posX + (width - 5), y_pos);
        API_VERTEX2F(posX + (width - 5), y_pos + y_length);

        API_VERTEX2F(posX + (width - 4), y_pos);
        API_VERTEX2F(posX + (width - 4), y_pos + y_length);

        API_END();

        // API_BLEND_FUNC(SRC_ALPHA, ONE_MINUS_SRC_ALPHA);
    }
}
