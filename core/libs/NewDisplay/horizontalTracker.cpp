#include "FT812.h"

#include "display.h"
#include "horizotnalTrackerControl.h"

#include <algorithm>
#include <cstring>

static void Number2Bitmaps(int16_t x, int16_t y, uint8_t font_x, uint8_t font_y, int16_t number);
static void Number2HexBitmaps(int16_t x, int16_t y, uint8_t font_x, uint8_t font_y, int16_t number);

/*
static const uint32_t defaultColors[] =

{
    0x000000, // 0 linie
    0x232323, // 1 background  /numery wierszy juz nie bo nie ma/

    0x00e8be, // 2 nuta
    0xfef749, // 3 instrument
    0xbb58f1, // 4 effekt1
    0x57f1ff, // 5 effekt2

    0x2c6861, // 6 nuta nieaktywna
    0x756a47, // 7 instrument nieaktywny
    0x614b69, // 8 effekt1 nieaktywny
    0x4b6977, // 9 effekt2 nieaktywny

    0xf13c3c, // 10 zaznaczenie
    0x2a2a2a, // 11 podzialka
    0x333333, // 12 nieaktywny
    0x000000, // 13 playhead

    0xffffff, // 14 tekst zaznaczenia
    0xf13c3c, // 15 playhead rec mode
};
*/

// uint8_t fourParamsOffset[4] =		{	4,44,73,130,	};	// stara koncepcja z numerami
// wierszy uint8_t twoParamsOffsetFirst[4] =	{	9,9,9,9,		}; uint8_t
// twoParamsOffsetSecond[4] =	{	50,50,50,50,	};

static uint8_t fourParamsOffsetX[4] = {
    10,
    10,
    10,
    10,
};
/*
static uint8_t fourParamsOffsetY[4] =		{	12,  36,  60,  84,	};
static uint8_t twoParamsOffsetFirstY[4] =	{	12,  12,  12,  12,	};
static uint8_t twoParamsOffsetSecondY[4] =	{	36,  36,  36,  36,	};
static uint8_t oneParamsOffsetY[4] =		{	24,  24,  24,  24,	};
*/

static const uint8_t fourParamsSelOffsetX[4] = {
    1,
    1,
    1,
    1,
};
static const uint8_t fourParamsSelOffsetY[4] = {
    2,
    26,
    50,
    74,
};
static const uint8_t fourParamsSelWidth[4] = {
    53,
    53,
    53,
    53,
};
static const uint8_t fourParamsSelHight[4] = {
    22,
    22,
    22,
    22,
};

static const uint8_t oneParamsSelOffsetY[4] = {
    12,
    12,
    12,
    12,
};

static const uint8_t twoParamsSel1OffsetY[4] = {
    2,
    2,
    2,
    2,
};
static const uint8_t twoParamsSel2OffsetY[4] = {
    25,
    25,
    25,
    25,
};

const uint8_t paramToStateOffset[4] = {
    1,
    2,
    4,
    8,
};

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
cHorizontalTracker::cHorizontalTracker(strControlProperties* properties)
  : cDisplayControl(controlIdArrangement, 5)
{
    colorsCount = 20;
    colors = display.getTheme().patternTrackerColors;
    dimFx1Color = ((colors[8] & 0xFEFEFE) >> 1) + ((colors[1] & 0xFEFEFE) >> 1);
    dimFx2Color = ((colors[9] & 0xFEFEFE) >> 1) + ((colors[1] & 0xFEFEFE) >> 1);

    refreshStep = 0;

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

cHorizontalTracker::~cHorizontalTracker()
{
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
void cHorizontalTracker::setStyle(uint32_t style)
{
    this->style = style;

    int8_t textFont = FONT_INDEX_FROM_STYLE;
    textFont = (textFont >= 0) ? textFont : 0;
    font = &fonts[textFont];
}

void cHorizontalTracker::setText(const char* text)
{
}

void cHorizontalTracker::setValue(int value)
{
    this->value = value;
}

void cHorizontalTracker::setColors(const uint32_t* colors)
{
    this->colors = colors;
    dimFx1Color = ((colors[8] & 0xFEFEFE) >> 1) + ((colors[1] & 0xFEFEFE) >> 1);
    dimFx2Color = ((colors[9] & 0xFEFEFE) >> 1) + ((colors[1] & 0xFEFEFE) >> 1);
}

void cHorizontalTracker::setData(void* data)
{
    tracks = (strTrackerPattern*)data;
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
void cHorizontalTracker::update()
{
    displayMode = value;
    lastFontHandle = -1; // zerowanie ostatnio uzywanej czcionki

    visibleStepsCount = 12;
    actualStepPosition = 5;
    stepsDataStructOffset = 7 - (actualStepPosition);

    if (tracks->showTrackNames)
        trackHeadWidth = 90;
    else
        trackHeadWidth = 0;

    stepWidth = (800 - trackHeadWidth) / visibleStepsCount;

    visibleTracksCount = 4;
    firstVisibleTrack = tracks->firstVisibleTrack;

    stepsNumbersHight = 16;
    // trackHight = (420-stepsNumbersHight)/visibleTracksCount; // 423
    // paramCount = 4;

    //-------------------------------------------------------------------------------------------
    //

    visibleStepsCount = (tracks->popupMode & 2) ? 8 : 12;                // param short popup
    visibleStepsCount = (tracks->popupMode & 4) ? 5 : visibleStepsCount; // param long popup

    if (displayMode > 0) // 1-2 parametry na step / 8 trackow
    {
        if (tracks->popupMode & 1)
            visibleTracksCount = 4; // fill
        else
            visibleTracksCount = 8;

        trackHight = (height - 7 - stepsNumbersHight) / 8;
        paramCount = (displayMode & 1) + ((displayMode & 2) >> 1) + ((displayMode & 4) >> 2) +
                     ((displayMode & 8) >> 3);
    }
    else // 4 parametry na step / 4 tracki
    {
        if (tracks->popupMode & 1)
            visibleTracksCount = 2; // fill
        else
            visibleTracksCount = 4;

        trackHight = (height - 7 - stepsNumbersHight) / 4;
        paramCount = 4;
    }

    const uint8_t activeTracks =
      (tracks->tracksCount > 0 && tracks->tracksCount <= strTrackerPattern::kTrackCount)
        ? tracks->tracksCount
        : strTrackerPattern::kTrackCount;
    const uint8_t remainingTracks =
      (firstVisibleTrack < activeTracks) ? (activeTracks - firstVisibleTrack) : 0;
    visibleTracksCount = std::min<uint8_t>(visibleTracksCount, remainingTracks);

    backgroundDivider();

    calculateSelection();

    playHead();

    selection();

    stepNumbers();
}

//------------------------------------------------------------------------------------------
void cHorizontalTracker::update2()
{
    if (displayMode == 0 || displayMode & 1)
    {
        notes();
    }
}

void cHorizontalTracker::update3()
{
    if (displayMode == 0 || displayMode & 2)
    {
        instruments();
    }
}

void cHorizontalTracker::update4()
{
    if (displayMode == 0 || displayMode & 4)
    {
        fxes1();
    }
}

void cHorizontalTracker::update5()
{
    if (displayMode == 0 || displayMode & 8)
    {
        fxes2();
    }

    tracksNames();

    API_BLEND_FUNC(SRC_ALPHA, ZERO);
    lines();

    API_BLEND_FUNC(SRC_ALPHA, ONE_MINUS_SRC_ALPHA);
}

//-------------------------------------------------------------------------------------
// podzialka
void cHorizontalTracker::backgroundDivider()
{
    int16_t div_row = tracks->middleVisibleStep - actualStepPosition;

    // tlo calosci
    API_COLOR(colors[1]);
    API_LINE_WIDTH(16);
    API_BEGIN(RECTS);

    API_VERTEX2F(posX + 1, posY);
    API_VERTEX2F(width - 2, posY + height - 2);

    // podzialka
    API_LINE_WIDTH(16);
    API_COLOR(colors[11]);

    for (uint8_t i = 0; i < visibleStepsCount; i++)
    {
        if (div_row < 0 || div_row > tracks->patternLength - 1)
        {
            div_row++;
            continue;
        }

        if (div_row % tracks->stepDevider == 0)
        {
            API_VERTEX2F(trackHeadWidth + (i * stepWidth) + 1, posY);
            API_VERTEX2F(trackHeadWidth + (i * stepWidth) + stepWidth - 1,
                         posY + height - (style & controlStyleBottomShadow ? 10 : 2));
        }

        div_row++;
    }

    API_END();

    // API_BLEND_FUNC(SRC_ALPHA, ONE_MINUS_SRC_ALPHA);
    // API_BLEND_FUNC(SRC_ALPHA, ZERO);

    if (style & controlStyleBottomShadow)
    {
        // gradient-cien dolny
        API_SAVE_CONTEXT();
        API_SCISSOR_XY(1, 413);
        API_SCISSOR_SIZE(798, 10);
        API_CMD_GRADIENT(0, 413, colors[1], 0, 423, 0x0);
        API_RESTORE_CONTEXT();
    }

    // API_BLEND_FUNC(SRC_ALPHA, ONE_MINUS_SRC_ALPHA);
}

//-------------------------------------------------------------------------------------
// // linie oddzielajace tracki
void cHorizontalTracker::lines()
{
    API_COLOR(colors[15]);
    API_LINE_WIDTH(1);
    API_BEGIN(RECTS);

    uint16_t y;
    for (uint8_t i = 0; i < visibleTracksCount; i++)
    {
        y = posY + stepsNumbersHight + trackHight * i;

        API_VERTEX2F(0, y);
        API_VERTEX2F(799, y);
    }
    const uint8_t baseMaxRows = (displayMode > 0) ? 8 : 4;
    if (visibleTracksCount > 0 && visibleTracksCount < baseMaxRows)
    {
        y = posY + stepsNumbersHight + trackHight * visibleTracksCount;
        API_VERTEX2F(0, y);
        API_VERTEX2F(799, y);
    }

    API_END();
}

#define DISP_RGB(red, green, blue)                                                                 \
    ((((red) & 255UL) << 16) | (((green) & 255UL) << 8) | (((blue) & 255UL) << 0))

//-------------------------------------------------------------------------------------
// playhead
void cHorizontalTracker::playHead()
{
    if (!tracks->showPlayhead)
    {
        return;
    }

    if (tracks->playheadPosition > tracks->middleVisibleStep - actualStepPosition - 1 &&
        tracks->playheadPosition <
          tracks->middleVisibleStep + (visibleStepsCount - actualStepPosition))
    {
        uint8_t step = tracks->playheadPosition - (tracks->middleVisibleStep - actualStepPosition);
        int16_t phy1 = posY;
        const int16_t phy2max = posY + height - (style & controlStyleBottomShadow ? 10 : 0);
        int16_t phy2 = phy2max;
        uint16_t phx1 = trackHeadWidth + (step)*stepWidth;
        uint16_t phx2 = trackHeadWidth + (step + 1) * stepWidth;

        const uint8_t activeTracks =
          (tracks->tracksCount > 0 && tracks->tracksCount <= strTrackerPattern::kTrackCount)
            ? tracks->tracksCount
            : strTrackerPattern::kTrackCount;
        if (tracks->playheadStartTrack > 0 || (tracks->playheadEndTrack + 1) < activeTracks)
        {
            if (tracks->playheadStartTrack < tracks->playheadEndTrack)
            {
                phy1 = tracks->playheadStartTrack;
                phy2 = tracks->playheadEndTrack;
            }
            else
            {
                phy1 = tracks->playheadEndTrack;
                phy2 = tracks->playheadStartTrack;
            }

            if (phy1 >= (firstVisibleTrack + visibleTracksCount) || phy2 < firstVisibleTrack)
            {
                return;
            }

            phy1 = phy1 - firstVisibleTrack;
            phy1 = posY + stepsNumbersHight + phy1 * trackHight;
            phy1 = phy1 < (posY + stepsNumbersHight) ? posY + stepsNumbersHight : phy1;

            phy2 = phy2 - firstVisibleTrack;
            phy2 = posY + stepsNumbersHight + phy2 * trackHight + trackHight;
            const int16_t maxPhy2 = std::min<int16_t>(
              phy2max, posY + stepsNumbersHight + visibleTracksCount * trackHight);
            phy2 = phy2 > maxPhy2 ? maxPhy2 : phy2;
            if (phy2 <= phy1)
            {
                return;
            }
        }
        if (style & controlStyleGradient)
        {
            API_SAVE_CONTEXT();

            // API_LINE_WIDTH(16);
            API_SCISSOR_XY(phx1 - 11, phy1);
            API_SCISSOR_SIZE(11, phy2 - phy1);
            API_CMD_GRADIENT(phx1 - 11, 0, colors[1], phx1, 0, 0x0);

            API_RESTORE_CONTEXT();

            API_SAVE_CONTEXT();

            // API_LINE_WIDTH(16);
            API_SCISSOR_XY(phx2, phy1);
            API_SCISSOR_SIZE(11, phy2 - phy1);
            API_CMD_GRADIENT(phx2, 0, 0x0, phx2 + 11, 0, colors[1]);

            API_RESTORE_CONTEXT();
        }
        else
        {
            API_COLOR(colors[13]);
            API_LINE_WIDTH(16);
            API_BEGIN(RECTS);
            API_VERTEX2F(phx1, phy1);
            API_VERTEX2F(phx2, phy2 - 2);
            API_END();
        }
    }
}

void cHorizontalTracker::calculateSelection()
{
    if (tracks->selectState == 2)
    {
        selectActive = 1;

        if (tracks->selectStartTrack > tracks->selectEndTrack)
        {
            select1_y = tracks->selectEndTrack;
            select2_y = tracks->selectStartTrack;
        }
        else
        {
            select1_y = tracks->selectStartTrack;
            select2_y = tracks->selectEndTrack;
        }

        if (tracks->selectStartStep > tracks->selectEndStep)
        {
            select1_x = tracks->selectEndStep;
            select2_x = tracks->selectStartStep;
        }
        else
        {
            select1_x = tracks->selectStartStep;
            select2_x = tracks->selectEndStep;
        }

        select1_y = select1_y - firstVisibleTrack;
        select1_y = posY + stepsNumbersHight + select1_y * trackHight;

        if (select1_y < posY + stepsNumbersHight)
        {
            select1_y = posY + stepsNumbersHight;
        }

        select2_y = select2_y - firstVisibleTrack;
        select2_y = posY + stepsNumbersHight + select2_y * trackHight + trackHight;

        if (select2_y > posY + stepsNumbersHight + trackHight * visibleTracksCount)
        {
            select2_y = posY + stepsNumbersHight + trackHight * visibleTracksCount;
        }

        select1_x = select1_x - tracks->middleVisibleStep + actualStepPosition;
        select1_x = trackHeadWidth + select1_x * stepWidth;

        if (select1_x < trackHeadWidth)
        {
            select1_x = trackHeadWidth;
        }

        select2_x = select2_x - tracks->middleVisibleStep + actualStepPosition;
        select2_x = trackHeadWidth + select2_x * stepWidth + stepWidth;

        if (select2_x > trackHeadWidth + visibleStepsCount * stepWidth)
        {
            select2_x = trackHeadWidth + visibleStepsCount * stepWidth;
        }
    }
    else
    {
        selectActive = 0;
    }
}
//-------------------------------------------------------------------------------------
// ZAZNACZENIE ACTUAL
void cHorizontalTracker::selection()
{
    // API_BLEND_FUNC(SRC_ALPHA, ZERO);

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

        if (paramCount == 1 && ((1 << tracks->selectedParam) &
                                displayMode)) // jesli jest na ekranie zaznaczony parametr
        {
            for (uint8_t i = 0; i < 8; i++)
            {
                uint16_t SelPosY = select1_y + (trackHight * i) + (trackHight / 4);
                if (SelPosY <= select2_y && SelPosY >= select1_y)
                {
                    API_VERTEX2F(select1_x + 2, SelPosY);
                    API_VERTEX2F(select2_x - 2, SelPosY + (trackHight / 2));
                }
            }
        }
        else if (paramCount == 2 && ((1 << tracks->selectedParam) & displayMode))
        {
            uint8_t param = whichParamFromTwoSelected();
            uint8_t offset = param == 0 ? 2 : (trackHight / 2);

            for (uint8_t i = 0; i < 8; i++)
            {
                uint16_t SelPosY = select1_y + offset + (trackHight * i);
                if (SelPosY <= select2_y && SelPosY >= select1_y)
                {
                    API_VERTEX2F(select1_x + 2, SelPosY);
                    API_VERTEX2F(select2_x - 2, SelPosY + (trackHight / 2) - 2);
                }
            }
        }
        else if (paramCount == 4)
        {
            for (uint8_t i = 0; i < 8; i++)
            {
                uint16_t SelPosY =
                  select1_y + (trackHight * i) + (trackHight / 4) * tracks->selectedParam + 2;

                if (SelPosY <= select2_y && SelPosY >= select1_y)
                {
                    API_VERTEX2F(select1_x + 2, SelPosY);
                    API_VERTEX2F(select2_x - 2, SelPosY + (trackHight / 4) - 3);
                }
            }
        }
        API_END();

        // ramka zaznaczenia
        API_BEGIN(LINES);

        if (select1_y >= posY + stepsNumbersHight) // gorna
        {
            API_VERTEX2F(select1_x + 1, select1_y + 1);
            API_VERTEX2F(select2_x - 2, select1_y + 1);
        }
        else
        {
            select1_y = posY + stepsNumbersHight;
        }

        if (select2_y <= posY + stepsNumbersHight + trackHight * visibleTracksCount) // dolna
        {
            API_VERTEX2F(select2_x - 2, select2_y - 1);
            API_VERTEX2F(select1_x + 1, select2_y - 1);
        }
        else
        {
            select2_y = posY + stepsNumbersHight + trackHight * visibleTracksCount;
        }

        if (select2_x <= trackHeadWidth + stepWidth * visibleStepsCount) // prawa
        {
            API_VERTEX2F(select2_x - 2, select1_y + 1);
            API_VERTEX2F(select2_x - 2, select2_y - 1);
        }

        if (select1_x >= trackHeadWidth) // lewa
        {
            API_VERTEX2F(select1_x + 1, select2_y - 1);
            API_VERTEX2F(select1_x + 1, select1_y + 1);
        }

        API_END();
    }

    // aktualnie modyfikowany step
    if (tracks->selectState && tracks->actualTrack >= firstVisibleTrack &&
        tracks->actualTrack < firstVisibleTrack + visibleTracksCount)
    {
        uint16_t select_x =
          trackHeadWidth + ((tracks->actualStep - tracks->middleVisibleStep) + 5) * stepWidth + 2;
        uint16_t select1_y =
          posY + stepsNumbersHight + (tracks->actualTrack - firstVisibleTrack) * trackHight;
        uint16_t select2_y = select1_y + trackHight;
        uint16_t select_w = stepWidth - 4;
        uint16_t rect_select_x = select_x + 1;
        uint16_t rect_select_width = stepWidth - 6;
        uint16_t rect_select_y = 0;
        uint16_t rect_select_hight = 0;

        if (paramCount == 1)
        {
            rect_select_y = select1_y + (trackHight / 4);
            rect_select_hight = (trackHight / 2);
        }
        else if (paramCount == 2)
        {
            uint8_t param = whichParamFromTwoSelected();
            rect_select_y = select1_y + (param == 0 ? 2 : (trackHight / 2) * 1);

            rect_select_hight = (trackHight / 2) - 2;
        }
        else if (paramCount == 4)
        {
            rect_select_y = select1_y + (trackHight / 4) * tracks->selectedParam +
                            2 /*fourParamsSelOffsetY[tracks->selectedParam]*/;
            rect_select_hight = (trackHight / 4) - 3;
        }

        API_COLOR(selection_color);
        API_LINE_WIDTH(kRedFrameWidth);
        API_BEGIN(RECTS);
        API_VERTEX2F(rect_select_x, rect_select_y);
        API_VERTEX2F(rect_select_x + rect_select_width, rect_select_y + rect_select_hight);
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
            API_VERTEX2F(select_x, select1_y + 1);
            API_VERTEX2F(select_x + select_w, select1_y + 1);
            API_VERTEX2F(select_x + select_w, select2_y - 1);
            API_VERTEX2F(select_x, select2_y - 1);
            API_VERTEX2F(select_x, select1_y + 1);

            API_END();
        }
    }
}

uint8_t cHorizontalTracker::whichParamFromTwoSelected()
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
void cHorizontalTracker::stepNumbers()
{
    int16_t stepIdx = (tracks->middleVisibleStep - actualStepPosition) + 1;
    API_SAVE_CONTEXT();

    strFont* numFont = &fonts[0];

    API_COLOR(colors[16]);
    API_BITMAP_HANDLE(numFont->handle);
    API_BEGIN(BITMAPS);

    bool hexMode = isStyleSet(controlStyleHexNumbers);
    bool startFrom1 = isStyleSet(controlStyleStartNumbersFrom1);

    for (uint16_t i = 0; i < visibleStepsCount; i++)
    {

        if (stepIdx < 1 || stepIdx > tracks->patternLength)
        {
            stepIdx++;
            continue;
        }

        if (hexMode)
        {
            Number2HexBitmaps(trackHeadWidth + 12 + i * stepWidth,
                              posY + 6,
                              7,
                              numFont->height,
                              startFrom1 ? stepIdx : stepIdx - 1);
        }
        else
        {
            Number2Bitmaps(trackHeadWidth + 12 + i * stepWidth,
                           posY + 6,
                           7,
                           numFont->height,
                           startFrom1 ? stepIdx : stepIdx - 1);
        }
        stepIdx++;
    }
    API_END();

    API_RESTORE_CONTEXT();
}

//-------------------------------------------------------------------------------------
// tracks names
void cHorizontalTracker::tracksNames()
{
    if (!tracks->showTrackNames)
    {
        return;
    }

    int16_t x, y, w, h;
    w = trackHeadWidth - 2;
    h = 18;

    x = 0;

    API_LINE_WIDTH(32);
    API_BEGIN(RECTS);

    for (uint8_t i = 0; i < visibleTracksCount; i++)
    {
        if (tracks->inactive[firstVisibleTrack + i])
            API_COLOR(colors[12]);
        else
            API_COLOR(colors[19]);

        // if(w > tracksSpace-2) w = tracksSpace-2;

        // x = rightOffset+(i*tracksSpace)+1;
        y = posY + stepsNumbersHight + trackHight * i + 2;

        API_VERTEX2F(x, y);
        API_VERTEX2F(x + w, y + h);
    }

    API_END();

    // numer/nazwa
    y = 0;
    x = 6;
    actualColor = lastFontColor = colors[0];

    API_SAVE_CONTEXT();
    API_COLOR(actualColor);
    lastFontHandle = -1;
    API_BEGIN(BITMAPS);

    for (uint8_t i = 0; i < visibleTracksCount; i++)
    {
        y = posY + stepsNumbersHight + (i * trackHight) + 10;
        const char* trackName = tracks->tracksNames[firstVisibleTrack + i];
        String2Bitmaps(x, y, &fonts[3], trackName, 7, trackHeadWidth - fonts[3].width);
    }

    API_END();
    API_RESTORE_CONTEXT();
}

//===============================================================================================================================
void cHorizontalTracker::notes()
{
    API_SAVE_CONTEXT();
    API_COLOR(colors[2]);
    actualColor = lastFontColor = colors[2];
    // API_BITMAP_HANDLE(fonts[1].handle);
    lastFontHandle = -1;
    API_BEGIN(BITMAPS);

    uint8_t offset_x = trackHeadWidth + fourParamsOffsetX[0];
    uint8_t offset_y = posY + stepsNumbersHight + trackHight / 8;
    if (paramCount == 1)
        offset_y = posY + stepsNumbersHight + (trackHight / 2);
    else if (paramCount == 2)
        offset_y = posY + stepsNumbersHight + (trackHight / 2) - (trackHight / 4);

    int8_t mark_t = -1;
    int8_t mark_s = (tracks->actualStep - tracks->middleVisibleStep) + actualStepPosition;
    if ((tracks->selectedParam == 0 /*|| tracks->selectedParam == 4*/) && tracks->selectState)
    {
        mark_t = tracks->actualTrack - firstVisibleTrack;
    }

    for (uint16_t i = 0; i < visibleStepsCount; i++)
    {
        for (uint16_t j = 0; j < visibleTracksCount; j++)
        {
            int16_t param_x = offset_x + i * stepWidth;
            int16_t param_y = offset_y + j * trackHight;

            char* text = tracks->track[firstVisibleTrack + j].row[i + stepsDataStructOffset].note;

            if (selectActive && mark_t >= 0 && param_x > select1_x && param_x < select2_x &&
                param_y > select1_y && param_y < select2_y)
                actualColor = colors[14];
            else if (i == mark_s && j == mark_t)
                actualColor = colors[14];
            else
            {
                if (tracks->inactive[firstVisibleTrack + j])
                    actualColor = colors[12];
                else if (*text == '-')
                    actualColor = colors[6];
                else
                    actualColor = colors[2];
            }

            String2Bitmaps(param_x, param_y, &fonts[1], text, param_length[0]);
        }
    }
    API_END();
    API_RESTORE_CONTEXT();
}

//===============================================================================================================================
void cHorizontalTracker::instruments()
{
    API_SAVE_CONTEXT();
    API_COLOR(colors[3]);
    actualColor = lastFontColor = colors[3];
    // API_BITMAP_HANDLE(fonts[1].handle);
    lastFontHandle = -1;
    API_BEGIN(BITMAPS);

    uint8_t offset_x = trackHeadWidth + fourParamsOffsetX[1];
    uint8_t offset_y = posY + stepsNumbersHight + (trackHight / 4) + (trackHight / 8);
    if (paramCount == 1)
        offset_y = posY + stepsNumbersHight + (trackHight / 2);
    else if (paramCount == 2)
        offset_y = posY + stepsNumbersHight +
                   ((displayMode & 1) ? (trackHight / 2) + (trackHight / 4)
                                      : (trackHight / 2) - (trackHight / 4));

    int8_t mark_t = -1;
    int8_t mark_s = (tracks->actualStep - tracks->middleVisibleStep) + actualStepPosition;
    if ((tracks->selectedParam == 1 /*|| tracks->selectedParam == 4*/) && tracks->selectState)
    {
        mark_t = tracks->actualTrack - firstVisibleTrack;
    }

    for (uint16_t i = 0; i < visibleStepsCount; i++)
    {
        for (uint16_t j = 0; j < visibleTracksCount; j++)
        {
            uint8_t track = firstVisibleTrack + j;
            int16_t param_x = offset_x + i * stepWidth;
            int16_t param_y = offset_y + j * trackHight;
            // uint8_t midi_channel  =  tracks->track[track].row[j].instr[3];
            char* text = tracks->track[track].row[i + stepsDataStructOffset].instr;
            if (selectActive && mark_t >= 0 && param_x > select1_x && param_x < select2_x &&
                param_y > select1_y && param_y < select2_y)
                actualColor = colors[14];
            else if (i == mark_s && j == mark_t)
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

            if (tracks->track[track].row[i + stepsDataStructOffset].instr[2] > 0)
            {
                String2Bitmaps(param_x, param_y, &fonts[1], text, param_length[1] + 1);
            }
            else
            {
                String2Bitmaps(param_x, param_y, &fonts[1], text, param_length[1]);
            }
        }
    }
    API_END();
    API_RESTORE_CONTEXT();
}

//===============================================================================================================================
void cHorizontalTracker::fxes1()
{
    API_SAVE_CONTEXT();
    API_COLOR(colors[4]);
    actualColor = lastFontColor = colors[4];
    lastFontHandle = -1;
    // API_BITMAP_HANDLE(fonts[1].handle);
    API_BEGIN(BITMAPS);

    uint8_t offset_x = trackHeadWidth + fourParamsOffsetX[2];
    uint8_t offset_y = posY + stepsNumbersHight + (trackHight / 2) + (trackHight / 8);
    if (paramCount == 1)
        offset_y = posY + stepsNumbersHight + (trackHight / 2);
    else if (paramCount == 2)
        offset_y = posY + stepsNumbersHight +
                   ((displayMode & 8) ? (trackHight / 2) - (trackHight / 4)
                                      : (trackHight / 2) + (trackHight / 4));

    int8_t mark_t = -1;
    int8_t mark_s = (tracks->actualStep - tracks->middleVisibleStep) + actualStepPosition;
    if ((tracks->selectedParam == 2 /*|| tracks->selectedParam == 4*/) && tracks->selectState)
    {
        mark_t = tracks->actualTrack - firstVisibleTrack;
    }

    for (uint16_t i = 0; i < visibleStepsCount; i++)
    {
        for (uint16_t j = 0; j < visibleTracksCount; j++)
        {
            int16_t param_x = offset_x + i * stepWidth;
            int16_t param_y = offset_y + j * trackHight;
            char* text =
              &tracks->track[firstVisibleTrack + j].row[i + stepsDataStructOffset].fx[0][0];

            if (selectActive && mark_t >= 0 && param_x > select1_x && param_x < select2_x &&
                param_y > select1_y && param_y < select2_y)
                actualColor = colors[14];
            else if (i == mark_s && j == mark_t)
                actualColor = colors[14];
            else
            {
                if (tracks->inactive[firstVisibleTrack + j])
                    actualColor = colors[12];
                else if (*text == '-')
                    actualColor = colors[8];
                else if (text[1] == '-' && text[2] == '-')
                    actualColor = dimFx1Color;
                else
                    actualColor = colors[4];
            }
            String2Bitmaps(param_x, param_y, &fonts[1], text, param_length[3]);
        }
    }
    API_END();
    API_RESTORE_CONTEXT();
}

void cHorizontalTracker::fxes2()
{
    API_SAVE_CONTEXT();
    API_COLOR(colors[5]);
    actualColor = lastFontColor = colors[5];
    lastFontHandle = -1;
    // API_BITMAP_HANDLE(fonts[1].handle);
    API_BEGIN(BITMAPS);

    uint8_t offset_x = trackHeadWidth + fourParamsOffsetX[3];
    uint8_t offset_y = posY + stepsNumbersHight + (trackHight / 4) * 3 + (trackHight / 8);
    if (paramCount == 1)
        offset_y = posY + stepsNumbersHight + (trackHight / 2);
    else if (paramCount == 2)
        offset_y = posY + stepsNumbersHight + (trackHight / 2) + (trackHight / 4);

    int8_t mark_t = -1;
    int8_t mark_s = (tracks->actualStep - tracks->middleVisibleStep) + actualStepPosition;
    if ((tracks->selectedParam == 3 /*|| tracks->selectedParam == 4*/) && tracks->selectState)
    {
        mark_t = tracks->actualTrack - firstVisibleTrack;
    }

    for (uint16_t i = 0; i < visibleStepsCount; i++)
    {
        for (uint16_t j = 0; j < visibleTracksCount; j++)
        {
            int16_t param_x = offset_x + i * stepWidth;
            int16_t param_y = offset_y + j * trackHight;
            char* text =
              &tracks->track[firstVisibleTrack + j].row[i + stepsDataStructOffset].fx[1][0];

            if (selectActive && mark_t >= 0 && param_x > select1_x && param_x < select2_x &&
                param_y > select1_y && param_y < select2_y)
                actualColor = colors[14];
            else if (i == mark_s && j == mark_t)
                actualColor = colors[14];
            else
            {
                if (tracks->inactive[firstVisibleTrack + j])
                    actualColor = colors[12];
                else if (*text == '-')
                    actualColor = colors[9];
                else if (text[1] == '-' && text[2] == '-')
                    actualColor = dimFx2Color;
                else
                    actualColor = colors[5];
            }

            String2Bitmaps(param_x, param_y, &fonts[1], text, param_length[3]);
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

This one has support for multiple fonts and optimizations for switching bitmap handles only when
necessary.
*/
void cHorizontalTracker::String2Bitmaps(int16_t x,
                                        int16_t y,
                                        const strFont* font,
                                        const char* string,
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

    x += fonts[1].width - font->width;

    unsigned char ch;
    for (uint8_t i = 0; i < length; i++)
    {
        ch = string[strPtr++];
        if (ch == 0 || x > w_limit)
            return;

        if (x >= 0 && x <= 799 && y >= 0 && y <= 479)
        {
            if ((x > 511 || y > 511) && font->data[ch])
            {
                API_CELL(ch);
                API_VERTEX2F(x, y);
            }
            else if (font->data[ch])
            {
                API_VERTEX2II(x, y, font->handle, ch);
            }
        }

        if ((string[0] == '-' && string[1] == '-') || (string[1] == '-' && string[2] == '-'))
        {
            x += 11; // font->data['o'];
        }
        else
        {
            x += font->data[ch];
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
