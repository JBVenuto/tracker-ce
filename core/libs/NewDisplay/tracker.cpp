
#include "FT812.h"

#include "display.h"
#include "trackerControl.h"

#include <algorithm>
#include <cstring>

static void Number2Bitmaps(int16_t x, int16_t y, uint8_t font_x, uint8_t font_y, int16_t number);
static void Number2HexBitmaps(int16_t x, int16_t y, uint8_t font_x, uint8_t font_y, int16_t number);

#define ROW_NUM 1

static constexpr uint8_t text_font = 1;
static int16_t s_lastCell = -1;

#if ROW_NUM
// uint8_t fourParamsOffset[4] =		{	4,44,73,130,	};	// stara koncepcja z numerami
// wierszy uint8_t twoParamsOffsetFirst[4] =	{	9,9,9,9,		}; uint8_t
// twoParamsOffsetSecond[4] =	{	50,50,50,50,	};

static uint8_t fourParamsOffset[4] = {
    8,
    52,
    86,
    140,
};
static uint8_t twoParamsOffsetFirst[4] = {
    7,
    10,
    5,
    0,
};
static uint8_t twoParamsOffsetSecond[4] = {
    0,
    61,
    50,
    50,
};
static uint8_t oneParamsOffset[4] = {
    32,
    37,
    27,
    27,
};

static const uint8_t fourParamsSelOffset[4] = {
    1,
    44,
    78,
    132,
};
static const uint8_t fourParamsSelWidth[4] = {
    40,
    30,
    50,
    50,
};

static const uint8_t twoParamsSel1Offset[4] = {
    1,
    1,
    1,
    0,
};
static const uint8_t twoParamsSel1Width[4] = {
    40,
    33,
    43,
    0,
};

static const uint8_t twoParamsSel2Offset[4] = {
    0,
    52,
    44,
    44,
};
static const uint8_t twoParamsSel2Width[4] = {
    0,
    37,
    45,
    45,
};

static const uint8_t oneParamsSelOffset[4] = {
    24,
    29,
    19,
    19,
};
static const uint8_t oneParamsSelWidth[4] = {
    40,
    30,
    50,
    50,
};

#else

uint8_t fourParamsOffset[4] = {
    18,
    60,
    92,
    144,
};
uint8_t twoParamsOffsetFirst[4] = {
    9,
    9,
    6,
    0,
};
uint8_t twoParamsOffsetSecond[4] = {
    0,
    65,
    55,
    55,
};
uint8_t oneParamsOffset[4] = {
    35,
    40,
    30,
    30,
};

const uint8_t fourParamsSelOffset[4] = {
    10,
    52,
    84,
    136,
};
const uint8_t fourParamsSelWidth[4] = {
    40,
    30,
    50,
    50,
};

const uint8_t twoParamsSel1Offset[4] = {
    1,
    1,
    1,
    0,
};
const uint8_t twoParamsSel1Width[4] = {
    40,
    30,
    47,
    0,
};

const uint8_t twoParamsSel2Offset[4] = {
    0,
    57,
    49,
    49,
};
const uint8_t twoParamsSel2Width[4] = {
    0,
    30,
    47,
    47,
};

const uint8_t oneParamsSelOffset[4] = {
    27,
    32,
    22,
    22,
};
const uint8_t oneParamsSelWidth[4] = {
    40,
    30,
    50,
    50,
};

const uint8_t paramToStateOffset[4] = {
    1,
    2,
    4,
    8,
};

#endif

const uint8_t paramToStateOffset[4] = {
    1,
    2,
    4,
    8,
};

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
cTracker::cTracker(strControlProperties* properties)
  : cDisplayControl(controlIdTracker, 5)
{
    colorsCount = 21;
    colors = display.getTheme().patternTrackerColors;
    dimFx1Color = ((colors[8] & 0xFEFEFE) >> 1) + ((colors[1] & 0xFEFEFE) >> 1);
    dimFx2Color = ((colors[9] & 0xFEFEFE) >> 1) + ((colors[1] & 0xFEFEFE) >> 1);

    if (properties == nullptr)
    {
        posX = 0;
        posY = 0;
        width = 0;
        height = 0;
        tracks = nullptr;
        this->style = 0;
        value = 0;
        return;
    }

    setColors((properties->colors == nullptr) ? colors : properties->colors);

    tracks = (strTrackerPattern*)properties->data;
    posX = properties->x;
    posY = properties->y;
    width = properties->w;
    height = properties->h;
    value = properties->value;

    setStyle(properties->style);
}

cTracker::~cTracker()
{
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
void cTracker::setStyle(uint32_t style)
{
    this->style = style;

    int8_t textFont = FONT_INDEX_FROM_STYLE;
    textFont = (textFont >= 0) ? textFont : 0;
    font = &fonts[textFont];
}

void cTracker::setText(const char* text)
{
}

void cTracker::setValue(int value)
{
    this->value = value;
}

void cTracker::setColors(const uint32_t* colors)
{
    this->colors = colors;
    dimFx1Color = ((colors[8] & 0xFEFEFE) >> 1) + ((colors[1] & 0xFEFEFE) >> 1);
    dimFx2Color = ((colors[9] & 0xFEFEFE) >> 1) + ((colors[1] & 0xFEFEFE) >> 1);
}

void cTracker::setData(void* data)
{
    tracks = (strTrackerPattern*)data;
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// void cTracker::update()
//{
//    update1();
//
//    //    // API_LIB_BeginDirectDL();
//    //    displayDriver->beginCoProListNoCheck();
//    //    API_CMD_DLSTART();
//    //
//    //    API_SAVE_CONTEXT();
//    //
//    //    switch (refreshStep)
//    //    {
//    //        case 0: refresh1(); break;
//    //        case 1: refresh2(); break;
//    //        case 2: refresh3(); break;
//    //        case 3: refresh4(); break;
//    //        case 4: refresh5(); break;
//    //        default: refreshStep = 0; break;
//    //    }
//    //
//    //    API_RESTORE_CONTEXT();
//    //
//    //    displayDriver->endCoProList();
//}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
void cTracker::update()
{
    // colors[10] = tracks->selectColor;
    displayMode = value;

    lastFontHandle = -1;
    s_lastCell = -1; // zerowanie ostatnio uzywanej czcionki

    rightOffset = 0;
    leftOffset = 0;

#if ROW_NUM
    rightOffset = 27;
    leftOffset = 27;
#endif

    stepHight = height / 15;

    if (displayMode > 0)
    {
        tracksWidth = (800 - (rightOffset + leftOffset)) / 8;
        columnsCount = (tracks->popupMode & 2) ? 6 : 8;
        columnsCount = (tracks->popupMode & 4) ? 4 : columnsCount;
        paramCount = (displayMode & 1) + ((displayMode & 2) >> 1) + ((displayMode & 4) >> 2) +
                     ((displayMode & 8) >> 3);
    }
    else
    {
        columnsCount = (tracks->popupMode & 2) ? 3 : 4;
        columnsCount = (tracks->popupMode & 4) ? 2 : columnsCount;
        tracksWidth = (800 - (rightOffset + leftOffset)) / 4;
        paramCount = 4;
    }

    const uint8_t activeTracks =
      (tracks->tracksCount > 0 && tracks->tracksCount <= strTrackerPattern::kTrackCount)
        ? tracks->tracksCount
        : strTrackerPattern::kTrackCount;
    const uint8_t remainingTracks =
      (tracks->firstVisibleTrack < activeTracks) ? (activeTracks - tracks->firstVisibleTrack) : 0;
    columnsCount = std::min<uint8_t>(columnsCount, remainingTracks);

    if (tracks->popupMode & 1)
    {
        rowCount = 8;
    }
    else
    {
        rowCount = 15;
    }

    // counting row numbers here
    // used in several further procedures
    bool showTrackEndLines = false;
    uint16_t rowNumbers[rowCount];

    for (uint16_t i = 0; i < rowCount; i++)
    {
        if (tracks->track[0].row[i].note[0] == 0)
        {
            rowNumbers[i] = 0;
            continue;
        }

        int16_t row = tracks->actualStep - 6 + i;

        if (row < 1)
        {
            showTrackEndLines = true;
            row = tracks->patternLength + (row % tracks->patternLength);
        }
        else if (row > tracks->patternLength)
        {
            showTrackEndLines = true;
            row = (row - 1) % tracks->patternLength + 1;
        }

        rowNumbers[i] = row;
    }

    backgroundDivider(rowNumbers);

    calculateSelection();

    playHead();

    lines();

#if ROW_NUM
    drawRowNumbers(rowNumbers, showTrackEndLines);
#endif

    selection();
}

//------------------------------------------------------------------------------------------
void cTracker::update2()
{
    if (displayMode == 0 || displayMode & 1)
    {
        notes();
    }
}

void cTracker::update3()
{
    if (displayMode == 0 || displayMode & 2)
    {
        instruments();
    }
}

void cTracker::update4()
{
    if (displayMode == 0 || displayMode & 4)
    {
        // volumes();
        fxes1();
    }
}

void cTracker::update5()
{
    if (displayMode == 0 || displayMode & 8)
    {
        fxes2();
    }

    tracksNumbers();
}

//-------------------------------------------------------------------------------------
// podzialka
void cTracker::backgroundDivider(uint16_t* rowNumbers)
{
    // tlo calosci
    API_COLOR(colors[1]);
    API_LINE_WIDTH(16);
    API_BEGIN(RECTS);

    API_VERTEX2F(posX + 1, posY);
    API_VERTEX2F(width - 2, posY + height - 2);

    if (tracks->patternLength > 1)
    {
        // podzialka
        API_COLOR(colors[11]);
        API_LINE_WIDTH(16);

        for (uint8_t i = 0; i < rowCount; i++)
        {
            if (rowNumbers[i] == 0)
            {
                continue;
            }

            if ((rowNumbers[i] - 1) % tracks->stepDevider == 0)
            {
                API_VERTEX2F(1, posY + (i * stepHight) + 1);
                API_VERTEX2F(798, posY + (i * stepHight) + 26);
            }
        }
    }

    API_END();

    if (style & controlStyleBottomShadow)
    {
        // gradient-cien dolny
        API_SAVE_CONTEXT();
        API_SCISSOR_XY(1, 413);
        API_SCISSOR_SIZE(798, 10);
        API_CMD_GRADIENT(0, 413, colors[1], 0, 423, 0x0);
        API_RESTORE_CONTEXT();
    }
}

//-------------------------------------------------------------------------------------
// // linie
void cTracker::lines()
{
    API_COLOR(colors[15]);
    API_LINE_WIDTH(16);
    API_BEGIN(LINES);

#if ROW_NUM
    API_VERTEX2F(rightOffset, posY);
    API_VERTEX2F(rightOffset, posY + stepHight * 15);

    API_VERTEX2F(799 - leftOffset, posY);
    API_VERTEX2F(799 - leftOffset, posY + stepHight * 15);
#endif

    uint16_t x;
    for (uint8_t i = 0; i < columnsCount; i++)
    {
        x = (rightOffset + tracksWidth * i);
        API_VERTEX2F(x, posY);
        API_VERTEX2F(x, posY + height);
    }
    if (columnsCount > 0)
    {
        x = rightOffset + tracksWidth * columnsCount;
        if (x < (799 - leftOffset))
        {
            API_VERTEX2F(x, posY);
            API_VERTEX2F(x, posY + height);
        }
    }

    API_END();
}

#define DISP_RGB(red, green, blue)                                                                 \
    ((((red) & 255UL) << 16) | (((green) & 255UL) << 8) | (((blue) & 255UL) << 0))

//-------------------------------------------------------------------------------------
// playhead
void cTracker::playHead()
{
    if (!tracks->showPlayhead)
    {
        return;
    }

    if (tracks->playheadPosition > tracks->actualStep - 8 &&
        tracks->playheadPosition < tracks->actualStep + 8)
    {
        uint8_t row = tracks->playheadPosition - (tracks->actualStep - 8);
        uint16_t phy1 = posY + stepHight * (row - 1);
        uint16_t phy2 = posY + stepHight * row;
        int16_t phx1 = 1;
        uint16_t phw = 798;

        const uint8_t activeTracks =
          (tracks->tracksCount > 0 && tracks->tracksCount <= strTrackerPattern::kTrackCount)
            ? tracks->tracksCount
            : strTrackerPattern::kTrackCount;
        if (tracks->playheadStartTrack > 0 || (tracks->playheadEndTrack + 1) < activeTracks)
        {
            int16_t phx2;
            if (tracks->playheadStartTrack < tracks->playheadEndTrack)
            {
                phx1 = tracks->playheadStartTrack;
                phx2 = tracks->playheadEndTrack;
            }
            else
            {
                phx1 = tracks->playheadEndTrack;
                phx2 = tracks->playheadStartTrack;
            }

            if (phx1 >= (tracks->firstVisibleTrack + columnsCount) ||
                phx2 < tracks->firstVisibleTrack)
            {
                return;
            }

            phx1 = leftOffset + (phx1 - tracks->firstVisibleTrack) * tracksWidth + 1;
            phx1 = phx1 < (leftOffset + 1) ? leftOffset + 1 : phx1;

            phx2 = leftOffset + (phx2 + 1 - tracks->firstVisibleTrack) * tracksWidth;
            const int16_t maxPhx2 =
              std::min<int16_t>(800 - rightOffset - 1, leftOffset + columnsCount * tracksWidth - 1);
            phx2 = phx2 > maxPhx2 ? maxPhx2 : phx2;
            if (phx2 <= phx1)
            {
                return;
            }

            phw = phx2 - phx1;
        }

        if (style & controlStyleGradient)
        {
            API_SAVE_CONTEXT();

            // API_LINE_WIDTH(16);
            API_SCISSOR_XY(phx1, phy1 - 11);
            API_SCISSOR_SIZE(phw, 11);
            API_CMD_GRADIENT(0, phy1 - 11, colors[1], 0, phy1, colors[13]);

            API_RESTORE_CONTEXT();

            API_SAVE_CONTEXT();

            // API_LINE_WIDTH(16);
            API_SCISSOR_XY(phx1, phy2);
            API_SCISSOR_SIZE(phw, 11);
            API_CMD_GRADIENT(0, phy2, colors[13], 0, phy2 + 11, colors[1]);

            API_RESTORE_CONTEXT();
        }
        else
        {
            API_COLOR(colors[13]);
            API_LINE_WIDTH(16);
            API_BEGIN(RECTS);
            API_VERTEX2F(phx1, phy1);
            API_VERTEX2F(phx1 + phw, phy2);
            API_END();
        }
    }
}

void cTracker::calculateSelection()
{
    if (tracks->selectState == 2)
    {
        selectActive = 1;

        if (tracks->selectStartTrack > tracks->selectEndTrack)
        {
            select1_x = tracks->selectEndTrack;
            select2_x = tracks->selectStartTrack;
        }
        else
        {
            select1_x = tracks->selectStartTrack;
            select2_x = tracks->selectEndTrack;
        }

        if (tracks->selectStartStep > tracks->selectEndStep)
        {
            select1_y = tracks->selectEndStep;
            select2_y = tracks->selectStartStep;
        }
        else
        {
            select1_y = tracks->selectStartStep;
            select2_y = tracks->selectEndStep;
        }

        select1_x = select1_x - tracks->firstVisibleTrack;
        select1_x = rightOffset + select1_x * tracksWidth + 1;

        if (select1_x < rightOffset + 1)
        {
            select1_x = rightOffset + 1;
        }

        select2_x = select2_x - tracks->firstVisibleTrack;
        select2_x = rightOffset + select2_x * tracksWidth + tracksWidth - 2;

        const int16_t maxSelect2X =
          std::min<int16_t>(800 - rightOffset - 4, rightOffset + columnsCount * tracksWidth - 2);
        if (select2_x > maxSelect2X)
        {
            select2_x = maxSelect2X;
        }

        select1_y = select1_y - tracks->actualStep + 7;
        select1_y = posY + select1_y * stepHight;

        select2_y = select2_y - tracks->actualStep + 7;
        select2_y = posY + select2_y * stepHight + stepHight;
    }
    else
    {
        selectActive = 0;
    }
}
//-------------------------------------------------------------------------------------
// ZAZNACZENIE ACTUAL
void cTracker::selection()
{
    uint32_t selection_color = colors[10];
    if (tracks->selectColor == 1)
        selection_color = colors[17];
    else if (tracks->selectColor == 2)
        selection_color = colors[18];

    //--------------------
    // ZAZNACZENIE

    if (tracks->selectState == 2)
    {
        // pole pod edytowanym parametrem
        API_COLOR(selection_color);
        API_LINE_WIDTH(kRedFrameWidth);
        API_BEGIN(RECTS);

        int16_t sel2_y_temp = select2_y;

        if (sel2_y_temp > posY + stepHight * 15) // dolna
        {
            sel2_y_temp = posY + stepHight * 15;
        }

        if (paramCount == 1 && ((1 << tracks->selectedParam) &
                                displayMode)) // jesli jest na ekranie zaznaczony parametr
        {
            for (uint8_t i = 0; i < 8; i++)
            {
                uint16_t SelPosX =
                  select1_x + oneParamsSelOffset[tracks->selectedParam] + (tracksWidth * i);
                if (SelPosX < select2_x && SelPosX > select1_x)
                {
                    API_VERTEX2F(SelPosX, select1_y + 1);
                    API_VERTEX2F(SelPosX + oneParamsSelWidth[tracks->selectedParam], sel2_y_temp);
                }
            }
        }
        else if (paramCount == 2 && ((1 << tracks->selectedParam) & displayMode))
        {
            uint8_t param = whichParamFromTwoSelected();
            uint8_t offset = param == 0 ? twoParamsSel1Offset[tracks->selectedParam]
                                        : twoParamsSel2Offset[tracks->selectedParam];
            uint8_t width = param == 0 ? twoParamsSel1Width[tracks->selectedParam]
                                       : twoParamsSel2Width[tracks->selectedParam];

            for (uint8_t i = 0; i < 8; i++)
            {
                uint16_t SelPosX = select1_x + offset + (tracksWidth * i);
                if (SelPosX < select2_x && SelPosX > select1_x)
                {
                    API_VERTEX2F(SelPosX, select1_y + 1);
                    API_VERTEX2F(SelPosX + width, sel2_y_temp);
                }
            }
        }
        else if (paramCount == 4)
        {
            for (uint8_t i = 0; i < 8; i++)
            {
                uint16_t SelPosX =
                  select1_x + fourParamsSelOffset[tracks->selectedParam] + (tracksWidth * i);
                if (SelPosX < select2_x && SelPosX > select1_x)
                {
                    API_VERTEX2F(SelPosX, select1_y + 1);
                    API_VERTEX2F(SelPosX + fourParamsSelWidth[tracks->selectedParam], sel2_y_temp);
                }
            }
        }
        API_END();

        // ramka zaznaczenia
        API_BEGIN(LINES);

        if (select1_y >= posY) // gorna
        {
            API_VERTEX2F(select1_x, select1_y);
            API_VERTEX2F(select2_x, select1_y);
        }
        else
        {
            select1_y = posY;
        }

        if (select2_y <= posY + stepHight * 15) // dolna
        {
            API_VERTEX2F(select2_x, select2_y);
            API_VERTEX2F(select1_x, select2_y);
        }
        else
        {
            select2_y = posY + stepHight * 15;
        }

        if (select2_x <= rightOffset + tracksWidth * columnsCount) // prawa
        {
            API_VERTEX2F(select2_x, select1_y);
            API_VERTEX2F(select2_x, select2_y);
        }
        if (select1_x >= rightOffset) // lewa
        {
            API_VERTEX2F(select1_x, select2_y);
            API_VERTEX2F(select1_x, select1_y);
        }

        API_END();
    }

    // aktualnie modyfikowany step
    if (tracks->selectState && tracks->actualTrack >= tracks->firstVisibleTrack &&
        tracks->actualTrack < tracks->firstVisibleTrack + columnsCount)
    {
        uint16_t select_x =
          rightOffset + (tracks->actualTrack - tracks->firstVisibleTrack) * tracksWidth + 2;
        uint16_t select1_y = posY + stepHight * 7;
        uint16_t select2_y = select1_y + stepHight - 1;
        uint16_t select_w = tracksWidth - 5;
        uint16_t rect_select_x = 0;
        uint16_t rect_select_width = 0;

        if (paramCount == 1)
        {
            rect_select_x = select_x + oneParamsSelOffset[tracks->selectedParam];
            rect_select_width = oneParamsSelWidth[tracks->selectedParam];
        }
        else if (paramCount == 2)
        {
            uint8_t param = whichParamFromTwoSelected();
            rect_select_x = select_x + (param == 0 ? twoParamsSel1Offset[tracks->selectedParam]
                                                   : twoParamsSel2Offset[tracks->selectedParam]);
            rect_select_width = (param == 0 ? twoParamsSel1Width[tracks->selectedParam]
                                            : twoParamsSel2Width[tracks->selectedParam]);
        }
        else if (paramCount == 4)
        {
            rect_select_x = select_x + fourParamsSelOffset[tracks->selectedParam];
            rect_select_width = fourParamsSelWidth[tracks->selectedParam];
        }

        API_COLOR(selection_color);
        API_LINE_WIDTH(kRedFrameWidth);
        API_BEGIN(RECTS);
        API_VERTEX2F(rect_select_x, select1_y + 1);
        API_VERTEX2F(rect_select_x + rect_select_width - 1, select2_y - 1);
        API_END();

        // ramka tylko w okolo aktualnego stepa
        if (tracks->selectState == 1)
        {

            API_BEGIN(LINE_STRIP);

            /*
                    API_LINE_WIDTH(16);
                    API_VERTEX2F(rect_select_x-2, select1_y);
                    API_VERTEX2F(rect_select_x+rect_select_width, select1_y);
                    API_VERTEX2F(rect_select_x+rect_select_width, select2_y);
                    API_VERTEX2F(rect_select_x-2, select2_y);
                    API_VERTEX2F(rect_select_x-2, select1_y);
            */

            API_LINE_WIDTH(kRedFrameWidth);
            API_VERTEX2F(select_x, select1_y);
            API_VERTEX2F(select_x + select_w, select1_y);
            API_VERTEX2F(select_x + select_w, select2_y);
            API_VERTEX2F(select_x, select2_y);
            API_VERTEX2F(select_x, select1_y);

            API_END();
        }
    }
}

uint8_t cTracker::whichParamFromTwoSelected()
{
    if (displayMode & 1)
    {
        if (tracks->selectedParam == 0)
            return 0;
        else
            return 1;
    }
    else if (displayMode & 2)
    {
        if (tracks->selectedParam == 1)
            return 0;
        else
            return 1;
    }
    else if (displayMode & 4)
    {
        if (tracks->selectedParam == 2)
            return 0;
        else
            return 1;
    }

    return 1;
}

//-------------------------------------------------------------------------------------
// step number
void cTracker::drawRowNumbers(uint16_t* rowNumbers, bool showTrackEndLines)
{
    API_SAVE_CONTEXT();

    strFont* numFont = &fonts[0];

    API_COLOR(colors[16]);
    API_BITMAP_HANDLE(numFont->handle);
    API_BEGIN(BITMAPS);

    // show right row number column only if it's not obstructed
    uint8_t showRight = !((tracks->popupMode & 4) | (tracks->popupMode & 2));
    bool hexMode = isStyleSet(controlStyleHexNumbers);
    bool startFrom1 = isStyleSet(controlStyleStartNumbersFrom1);

    for (uint16_t i = 0; i < rowCount; i++)
    {
        if (rowNumbers[i] == 0)
        {
            continue;
        }

        if (hexMode)
        {
            Number2HexBitmaps(3,
                              posY + (i * stepHight) + 12,
                              7,
                              numFont->height,
                              startFrom1 ? rowNumbers[i] : rowNumbers[i] - 1);
            if (showRight)
            {
                Number2HexBitmaps((799 - 23),
                                  posY + (i * stepHight) + 12,
                                  7,
                                  numFont->height,
                                  startFrom1 ? rowNumbers[i] : rowNumbers[i] - 1);
            }
        }
        else
        {
            Number2Bitmaps(3,
                           posY + (i * stepHight) + 12,
                           7,
                           numFont->height,
                           startFrom1 ? rowNumbers[i] : rowNumbers[i] - 1);
            if (showRight)
            {
                Number2Bitmaps((799 - 23),
                               posY + (i * stepHight) + 12,
                               7,
                               numFont->height,
                               startFrom1 ? rowNumbers[i] : rowNumbers[i] - 1);
            }
        }
    }
    API_END();

    // --------------------------------------------
    // track end lines

    if (showTrackEndLines)
    {
        API_COLOR(colors[20]);
        API_LINE_WIDTH(8);
        API_BEGIN(LINES);

        API_BLEND_FUNC(SRC_ALPHA, ZERO);

        for (uint16_t i = 1; i < rowCount; i++)
        {
            if (rowNumbers[i] == 1)
            {
                int trackEndLineY = posY + (i * stepHight);
                API_VERTEX2F(0, trackEndLineY);
                API_VERTEX2F(799, trackEndLineY);
            }
        }

        API_END();
    }

    API_RESTORE_CONTEXT();
}

//-------------------------------------------------------------------------------------
// tracks numbers
void cTracker::tracksNumbers()
{
    if (!tracks->showTrackNames || columnsCount == 0)
    {
        return;
    }

    int16_t x, y, w, h;
    w = 24;
    h = 27;

    // pole/tlo
    y = posY; //-1

    // wyznaczanie najduższej nazwy

    API_LINE_WIDTH(24);
    API_BEGIN(RECTS);

    int textLetersCount[columnsCount];

    for (uint8_t i = 0; i < columnsCount; i++)
    {
        if (tracks->inactive[tracks->firstVisibleTrack + i])
            API_COLOR(colors[12]);
        else
            API_COLOR(colors[19]);

        int trackIdx = tracks->firstVisibleTrack + i;
        const char* text = trackIdx < tracks->kTrackCount ? tracks->tracksNames[trackIdx] : "";
        int letersCount = strlen(text);
        w = getTextWidth((strFont*)font, text, letersCount);

        while (w > tracksWidth - 2)
        {
            if (letersCount > 1)
            {
                letersCount--;
            }
            else
            {
                break;
            }
            w = getTextWidth((strFont*)font, text, letersCount);
        }

        x = rightOffset + (i * tracksWidth) + 1;

        textLetersCount[i] = letersCount;

        API_VERTEX2F(x, y);
        API_VERTEX2F(x + w + 2, y + h);
    }

    API_END();

    // numer/nazwa
    y = posY + 12 - font->height / 2;
    actualColor = lastFontColor = colors[0];

    API_SAVE_CONTEXT();
    API_COLOR(actualColor);
    lastFontHandle = -1;
    s_lastCell = -1;
    API_BEGIN(BITMAPS);

    for (uint8_t i = 0; i < columnsCount; i++)
    {
        x = rightOffset + (i * tracksWidth) + 4;
        int trackIdx = tracks->firstVisibleTrack + i;
        const char* trackName = trackIdx < tracks->kTrackCount ? tracks->tracksNames[trackIdx] : "";
        string2Bitmaps(x, y, trackName, textLetersCount[i], false); //, tracksWidth-fonts[3].width
    }

    API_END();
    API_RESTORE_CONTEXT();
}

//===============================================================================================================================
void cTracker::notes()
{
    API_SAVE_CONTEXT();
    API_COLOR(colors[2]);
    actualColor = lastFontColor = colors[2];
    // API_BITMAP_HANDLE(fonts[1].handle);
    lastFontHandle = -1;
    s_lastCell = -1;
    API_BEGIN(BITMAPS);

    uint8_t offset_y = posY + 12;
    uint8_t offset_x = rightOffset + fourParamsOffset[0];

    // if(paramCount == 1) 		offset_x = 27 + ((tracksSpace/2)-(param_length[1]*12)/2);
    if (paramCount == 1)
        offset_x = rightOffset + oneParamsOffset[0];
    else if (paramCount == 2)
        offset_x = rightOffset + twoParamsOffsetFirst[0];

    int8_t mark = -1;
    if ((tracks->selectedParam == 0 /*|| tracks->selectedParam == 4*/) && tracks->selectState)
    {
        mark = tracks->actualTrack - tracks->firstVisibleTrack;
    }

    for (uint16_t i = 0; i < columnsCount; i++)
    {
        for (uint16_t j = 0; j < rowCount; j++)
        {
            int16_t param_x = offset_x + i * tracksWidth;
            int16_t param_y = offset_y + j * stepHight;

            char* text = tracks->track[tracks->firstVisibleTrack + i].row[j].note;

            if (selectActive && mark >= 0 && param_x > select1_x && param_x < select2_x &&
                param_y > select1_y && param_y < select2_y)
                actualColor = colors[14];
            else if (j == 7 && i == mark)
                actualColor = colors[14];
            else
            {
                if (tracks->inactive[tracks->firstVisibleTrack + i])
                    actualColor = colors[12];
                else if (*text == '-')
                    actualColor = colors[6];
                else
                    actualColor = colors[2];
            }

            String2Bitmaps(param_x, param_y, &fonts[text_font], text, param_length[0]);
        }
    }
    API_END();
    API_RESTORE_CONTEXT();
}

//===============================================================================================================================
void cTracker::instruments()
{
    API_SAVE_CONTEXT();
    API_COLOR(colors[3]);
    actualColor = lastFontColor = colors[3];
    // API_BITMAP_HANDLE(fonts[1].handle);
    lastFontHandle = -1;
    s_lastCell = -1;
    API_BEGIN(BITMAPS);

    uint8_t offset_y = posY + 12;
    uint8_t offset_x = rightOffset + fourParamsOffset[1]; // 59
    if (paramCount == 1)
        offset_x = rightOffset + oneParamsOffset[1];
    else if (paramCount == 2)
        offset_x = (displayMode & 1) ? rightOffset + twoParamsOffsetSecond[1]
                                     : rightOffset + twoParamsOffsetFirst[1];

    int8_t mark = -1;
    if ((tracks->selectedParam == 1 /*|| tracks->selectedParam == 4*/) && tracks->selectState)
    {
        mark = tracks->actualTrack - tracks->firstVisibleTrack;
    }

    for (uint16_t i = 0; i < columnsCount; i++)
    {
        uint8_t track = tracks->firstVisibleTrack + i;
        for (uint16_t j = 0; j < rowCount; j++)
        {
            int16_t param_x = offset_x + i * tracksWidth;
            int16_t param_y = offset_y + j * stepHight;
            // uint8_t midi_channel  =  tracks->track[track].row[j].instr[3];
            char* text = tracks->track[track].row[j].instr;
            if (selectActive && mark >= 0 && param_x > select1_x && param_x < select2_x &&
                param_y > select1_y && param_y < select2_y)
                actualColor = colors[14];
            else if (j == 7 && i == mark)
                actualColor = colors[14];
            else
            {
                if (tracks->inactive[track])
                    actualColor = colors[12];
                else if (*text == '-')
                    actualColor = colors[7];
                else
                    actualColor = colors[3];
            }

            if (tracks->track[track].row[j].instr[2] > 0)
            {
                String2Bitmaps(param_x - (fonts[0].data['M'] - 6),
                               param_y,
                               &fonts[0],
                               text,
                               param_length[1] + 1);
            }
            else
            {
                String2Bitmaps(param_x, param_y, &fonts[text_font], text, param_length[1]);
            }
        }
    }
    API_END();
    API_RESTORE_CONTEXT();
}

//===============================================================================================================================
void cTracker::fxes1()
{
    API_SAVE_CONTEXT();
    API_COLOR(colors[4]);
    actualColor = lastFontColor = colors[4];
    lastFontHandle = -1;
    s_lastCell = -1;
    // API_BITMAP_HANDLE(fonts[1].handle);
    API_BEGIN(BITMAPS);

    uint8_t offset_y = posY + 12;
    uint8_t offset_x = rightOffset + fourParamsOffset[2]; // 99
    if (paramCount == 1)
        offset_x = rightOffset + oneParamsOffset[2];
    else if (paramCount == 2)
        offset_x = (displayMode & 8) ? rightOffset + twoParamsOffsetFirst[2]
                                     : rightOffset + twoParamsOffsetSecond[2];

    int8_t mark = -1;
    if ((tracks->selectedParam == 2 /*|| tracks->selectedParam == 4*/) && tracks->selectState)
    {
        mark = tracks->actualTrack - tracks->firstVisibleTrack;
    }

    for (uint16_t i = 0; i < columnsCount; i++)
    {
        for (uint16_t j = 0; j < rowCount; j++)
        {

            int16_t param_x = offset_x + i * tracksWidth;
            int16_t param_y = offset_y + j * stepHight;
            char* text = &tracks->track[tracks->firstVisibleTrack + i].row[j].fx[0][0];

            if (selectActive && mark >= 0 && param_x > select1_x && param_x < select2_x &&
                param_y > select1_y && param_y < select2_y)
                actualColor = colors[14];
            else if (j == 7 && i == mark)
                actualColor = colors[14];
            else
            {
                if (tracks->inactive[tracks->firstVisibleTrack + i])
                    actualColor = colors[12];
                else if (*text == '-')
                    actualColor = colors[8];
                else if (text[1] == '-' && text[2] == '-')
                    actualColor = dimFx1Color;
                else
                    actualColor = colors[4];
            }
            String2Bitmaps(param_x, param_y, &fonts[text_font], text, param_length[3]);
        }
    }
    API_END();
    API_RESTORE_CONTEXT();
}
void cTracker::fxes2()
{
    API_SAVE_CONTEXT();
    API_COLOR(colors[5]);
    actualColor = lastFontColor = colors[5];
    lastFontHandle = -1;
    s_lastCell = -1;
    // API_BITMAP_HANDLE(fonts[1].handle);
    API_BEGIN(BITMAPS);

    uint8_t offset_y = posY + 12;
    uint8_t offset_x = rightOffset + fourParamsOffset[3];
    if (paramCount == 1)
        offset_x = rightOffset + oneParamsOffset[3];
    else if (paramCount == 2)
        offset_x = rightOffset + twoParamsOffsetSecond[3];

    int8_t mark = -1;
    if ((tracks->selectedParam == 3 /*|| tracks->selectedParam == 4*/) && tracks->selectState)
    {
        mark = tracks->actualTrack - tracks->firstVisibleTrack;
    }

    for (uint16_t i = 0; i < columnsCount; i++)
    {
        for (uint16_t j = 0; j < rowCount; j++)
        {

            int16_t param_x = offset_x + i * tracksWidth;
            int16_t param_y = offset_y + j * stepHight;
            char* text = &tracks->track[tracks->firstVisibleTrack + i].row[j].fx[1][0];

            if (selectActive && mark >= 0 && param_x > select1_x && param_x < select2_x &&
                param_y > select1_y && param_y < select2_y)
                actualColor = colors[14];
            else if (j == 7 && i == mark)
                actualColor = colors[14];
            else
            {
                if (tracks->inactive[tracks->firstVisibleTrack + i])
                    actualColor = colors[12];
                else if (*text == '-')
                    actualColor = colors[9];
                else if (text[1] == '-' && text[2] == '-')
                    actualColor = dimFx2Color;
                else
                    actualColor = colors[5];
            }

            String2Bitmaps(param_x, param_y, &fonts[text_font], text, param_length[3]);
        }
    }
    API_END();
    API_RESTORE_CONTEXT();
}
//===============================================================================================================================
//===============================================================================================================================
//===============================================================================================================================
static inline void draw_char(uint16_t x, uint16_t y, uint8_t charr)
{
    if (x > 511 || y > 511)
    {
        API_CELL(charr);
        API_VERTEX2F(x, y);
    }
    else
    {
        API_VERTEX2II(x, y, fonts[0].handle, charr);
    }
}

/*
Note: this is not cDisplayControl::string2Bitmaps

This one has support for multiple fonts and optimizations for switching bitmap handles only
when necessary.
*/
void cTracker::String2Bitmaps(int16_t x,
                              int16_t y,
                              const strFont* font,
                              char* string,
                              int8_t length,
                              int16_t w_limit)
{
    if (actualColor != lastFontColor)
    {
        API_COLOR(actualColor);
        lastFontColor = actualColor;
    }
    if (lastFontHandle != font->handle)
    {
        API_BITMAP_HANDLE(font->handle);
        lastFontHandle = font->handle;
    }

    w_limit += x;
    y = y - font->height / 2;
    uint8_t strPtr = 0;

    x += fonts[1].width - font->width; // text_font

    unsigned char ch;
    for (uint8_t i = 0; i < length; i++)
    {
        ch = string[strPtr++];
        if (ch == 0 || x > w_limit)
            return;

        if (0 <= x && x <= 799 && 0 <= y && y <= 479)
        {
            if ((x > 511 || y > 511) && font->data[ch])
            {
                if (s_lastCell != (int16_t)ch)
                {
                    API_CELL(ch);
                    s_lastCell = (int16_t)ch;
                }
                API_VERTEX2F(x, y);
            }
            else if (font->data[ch])
            {
                s_lastCell = -1;
                API_VERTEX2II(x, y, font->handle, ch);
            }
        }

        // if(font == &fonts[0])
        if ((string[0] == '-' && string[1] == '-') || (string[1] == '-' && string[2] == '-'))
        {
            x += 11; // font->data['8'];
        }
        else
        {
            x += font->data[ch];
            // x += font->data['C'];
        }
    }
}

static void Number2HexBitmaps(int16_t x, int16_t y, uint8_t font_x, uint8_t font_y, int16_t number)
{
    y = y - font_y / 2;
    uint8_t digit, ascii_offset;

    if (number < 0)
    {
        number *= -1;
        draw_char(x, y, 45);
        x += font_x;
    }

    // note: only prints the last two hex digits of the number (00-FF)
    number = number % 256;
    digit = number / 16;
    number = number % 16;

    // Xx
    ascii_offset = digit > 9 ? 55 : 48;
    draw_char(x, y, digit + ascii_offset);
    x += font_x;

    // xX
    ascii_offset = number > 9 ? 55 : 48;
    draw_char(x, y, number + ascii_offset);
}

static void Number2Bitmaps(int16_t x, int16_t y, uint8_t font_x, uint8_t font_y, int16_t number)
{
    y = y - font_y / 2;
    uint8_t digit, flag = 0;

    if (number < 0)
    {
        number = number * (-1);
        draw_char(x, y, 45);
        x += font_x;
    }

    //	digit =  number/10000;
    //	number = number%10000;
    //	if(digit > 0) {draw_char(x,y,digit+48); x+=font_x; flag = 1;}

    //	digit =  number/1000;
    //	number = number%1000;
    //	if(digit > 0 || flag) {draw_char(x,y,digit+48); x+=font_x; flag = 1;}

    digit = number / 100;
    number = number % 100;
    if (digit > 0 || flag)
    {
        draw_char(x, y, digit + 48);
        x += font_x - 1;
        flag = 1;
    } // skrocony odstep po 1

    digit = number / 10;
    number = number % 10;
    if (digit > 0 || flag)
    {
        draw_char(x, y, digit + 48);
        x += font_x;
        flag = 1;
    }

    draw_char(x, y, number + 48);
}
