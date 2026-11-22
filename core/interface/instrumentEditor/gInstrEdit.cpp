#include "core/songTimer.h"
#include "instrumentEditor/instrumentEditor.h"
#include "mtWavetableEnvelopes.h"
#include "fileManager.h"
#include <stdio.h>

#include "core/interfacePopups.h"
#include "polyassert.h"
#include "string.h"
#include "util_math.h"
#include "util_string.h"

extern const char* const InstrumentCCNames[47];

static const uint16_t framesPlaces[8][4] = {
    { 0 + 1, 31, 800 / 8 - 3, 391 },
    { (800 / 8) * 1 + 1, 31, 800 / 8 - 3, 391 },
    { (800 / 8) * 2 + 1, 31, 800 / 8 - 3, 391 },
    { (800 / 8) * 3 + 1, 31, 800 / 8 - 3, 391 },
    { (800 / 8) * 4 + 1, 31, 800 / 8 - 3, 391 },
    { (800 / 8) * 5 + 1, 31, 800 / 8 - 3, 391 },
    { (800 / 8) * 6 + 1, 31, 800 / 8 - 3, 391 },
    { (800 / 8) * 7 + 1, 31, 800 / 8 - 3, 391 },
};

static const uint16_t framesPlacesAutomation[7][4] = {
    { 0 + 1, 31, 2 * (800 / 8) - 3, 391 },       { (800 / 8) * 2 + 1, 31, 800 / 8 - 3, 391 },
    { (800 / 8) * 3 + 1, 31, 800 / 8 - 3, 391 }, { (800 / 8) * 4 + 1, 31, 800 / 8 - 3, 391 },
    { (800 / 8) * 5 + 1, 31, 800 / 8 - 3, 391 }, { (800 / 8) * 6 + 1, 31, 800 / 8 - 3, 391 },
    { (800 / 8) * 7 + 1, 31, 800 / 8 - 3, 391 },
};

static const uint16_t framesPlacesCC[2][4] = {
    { 0 + 1, 80, (800 / 8) * 4 - 3, 342 },
    { (800 / 8) * 4 + 1, 80, (800 / 8) * 3 - 3, 342 },
};

static const uint16_t framesPlacesSynthEdit[1][4] = {
    { (800 / 8) * 4 + 1, 31, (800 / 8) * 3 - 3, 391 },
};

static const uint16_t framesPlacesSynthMacroEdit[3][4] = {
    { 0 + 1, 31, (800 / 8) * 2 - 3, 391 },
    { (800 / 8) * 2 + 1, 31, (800 / 8) * 2 - 3, 391 },
    { (800 / 8) * 4 + 1, 31, (800 / 8) * 2 - 3, 391 },
};

const char* const filterModeFunctLabels[filterModeCount] = {
    "Disabled",
    "Low-pass",
    "High-pass",
    "Band-pass",
};

const char* const ccNameLabels[ccNumbersCount] = {
    "0 Bank",   "1 ModWh", "2 Brth",   "3",        "4 Foot",   "5 Glide",  "6 Data",  "7 ChVol",
    "8 Balnc",  "9",       "10 Pan",   "11 Exp",   "12",       "13",       "14",      "15",
    "16",       "17",      "18",       "19",       "20",       "21",       "22",      "23",
    "24",       "25",      "26",       "27",       "28",       "29",       "30",      "31",
    "32 Bank",  "33 Modw", "34 Brth",  "35",       "36 Foot",  "37 Glid",  "38 Data", "39 ChVo",
    "40 Blnc",  "41",      "42 Pan",   "43 Exp",   "44",       "45",       "46",      "47",
    "48",       "49",      "50",       "51",       "52",       "53",       "54",      "55",
    "56",       "57",      "58",       "59",       "60",       "61",       "62",      "63",
    "64 Sust",  "65 Port", "66 Sste",  "67 SftP",  "68 Lgto",  "69 Hld2",  "70",      "71",
    "72",       "73",      "74",       "75",       "76",       "77",       "78",      "79",
    "80",       "81",      "82",       "83",       "84 Porta", "85",       "86",      "87",
    "88 HDVel", "89",      "90",       "91",       "92",       "93",       "94",      "95",
    "96 Dat+",  "97 Dat-", "98 NRPNL", "99 NRPNM", "100 RPNL", "101 RPNM", "102",     "103",
    "104",      "105",     "106",      "107",      "108",      "109",      "110",     "111",
    "112",      "113",     "114",      "115",      "116",      "117",      "118",     "119",
    "120 SOff", "121 Rst", "122 Locl", "123 NOff", "124",      "125",      "126",     "127",
    "Prg Chg",  "Chan AT", "Poly AT",
};

const char* const envelopesLabels[autoDestCount] = {
    "Volume",
    "Panning",
    "Cutoff",
    "Wavetable Position",
    "Granular Position",
    "Granular Length",
    "Finetune"
};

const char* const envStateLabels[5] = { "Off", "Envelope", "LFO", "WT Env", "WT LFO" };

const char* const lfoShapeLabels[lfoShapeCount] = { "Rev Saw",
                                                    "Saw",
                                                    "Triangle",
                                                    "Square",
                                                    "Random" };

const char* const lfoSpeedLabels[ENVELOPE_SPEED_OTHER_COUNT] = {
    "128 steps", "96 steps",  "64 steps",  "48 steps",  "32 steps", "24 steps",
    "16 steps",  "12 steps",  "8 steps",   "6 steps",   "4 steps",  "3 steps",
    "2 steps",   "3/2 step",  "1 step",    "3/4 step",  "1/2 step", "3/8 step",
    "1/3 step",  "1/4 step",  "3/16 step", "1/6 step",  "1/8 step", "1/12 step",
    "1/16 step", "1/24 step", "1/32 step", "1/48 step", "1/64 step"
};

const char* const instrSynthNoteText = "Note: To use MIDI Mapping, the sequence must be stopped.";

void cInstrumentEditor::initDisplayControls()
{
    for (uint8_t i = 0; i < 5; i++)
    {
        localActiveBarColors[i] = display.getTheme().activeBarColors[i];
        localInactiveBarColors[i] = display.getTheme().inactiveBarColors[i];
    }

    strControlProperties prop;
    frameData.placesCount = 8;
    frameData.startPlace = 0;
    frameData.places[0] = &framesPlaces[0][0];
    frameData.places[1] = &framesPlaces[1][0];
    frameData.places[2] = &framesPlaces[2][0];
    frameData.places[3] = &framesPlaces[3][0];
    frameData.places[4] = &framesPlaces[4][0];
    frameData.places[5] = &framesPlaces[5][0];
    frameData.places[6] = &framesPlaces[6][0];
    frameData.places[7] = &framesPlaces[7][0];
    prop.style = 0;
    prop.value = 0;
    prop.colors = nullptr;
    prop.data = &frameData;
    frameControl = display.createControl<cFrame>(&prop);

    labelArrow.bitmaps[0].bitmapIndex = displayArrowU;
    labelArrow.bitmaps[0].xValue = (800 / 8) * 0 + (800 / 16) - 10;
    labelArrow.bitmaps[0].yValue = 450;
    labelArrow.bitmaps[1].bitmapIndex = displayArrowD;
    labelArrow.bitmaps[1].xValue = (800 / 8) * 1 + (800 / 16) + 10;
    labelArrow.bitmaps[1].yValue = 450;

    labelDataMore.dotIndicator.count = 2;
    labelDataMore.dotIndicator.current = 0;

    strControlProperties prop3;
    prop3.colors = display.getTheme().activeLabelsColors;
    prop3.style =
      (controlStyleCenterX | controlStyleCenterY | controlStyleFont3 | controlStyleShowBitmap);
    prop3.x = (800 / 8) * (0) + (800 / 16) * 2;
    prop3.w = 2 * (800 / 8) - 6;
    prop3.y = 424;
    prop3.h = 55;
    prop3.value = 0;
    prop3.data = &labelArrow;
    doubleLabelAutomation = display.createControl<cLabel>(&prop3);

    strControlProperties prop2;
    for (uint8_t i = 0; i < 8; i++)
    {
        prop2.colors = display.getTheme().activeLabelsColors;
        prop2.style = (controlStyleCenterX | controlStyleCenterY | controlStyleFont3);
        prop2.x = (800 / 8) * i + (800 / 16);
        prop2.w = 800 / 8 - 6;
        prop2.y = 424;
        prop2.h = 55;
        prop2.value = 1;

        label[i] = display.createControl<cLabel>(&prop2);

        prop2.value = 0;
        prop2.colors = localActiveBarColors;
        prop2.x = (800 / 8) * i + 1;
        prop2.y = 29;
        prop2.w = 800 / 8 - 3;
        prop2.style = controlStyleValue_0_100 | controlStyleBackground |
                      (display.getTheme().barShadow > 0 ? controlStyleGradient : 0) |
                      (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
        prop2.h = 394;
        barControl[i] = display.createControl<cBar>(&prop2);
    }

    prop2.text = nullptr;
    prop2.colors = nullptr;
    prop2.value = 255;
    prop2.style = controlStyleNoTransparency | controlStyleShow;
    prop2.x = 0;
    prop2.w = 800;
    prop2.y = 425;
    prop2.h = 54;
    bgLabel = display.createControl<cBgLabel>(&prop2);

    filterModeList.linesCount = 5;
    filterModeList.start = 0;
    filterModeList.length = filterModeCount;
    filterModeList.data = (char**)filterModeFunctLabels;

    prop.style = controlStyleBackground |
                 (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
    prop.x = (800 / 8) * (4) + 1;
    prop.y = 29;
    prop.w = 800 / 8 - 3;
    prop.h = 394;
    prop.data = &filterModeList;
    filterModeListControl = display.createControl<cList>(&prop);

    list1Data.linesCount = autoDestCount;
    list1Data.start = 0;
    list1Data.length = autoDestCount;
    list1Data.data = (char**)envelopesLabels;
    prop.x = (800 / 8) * (0) + 1;
    prop.y = 29;
    prop.w = 2 * (800 / 8) - 3;
    prop.h = 394;
    prop.data = &list1Data;
    list1Control = display.createControl<cList>(&prop);

    list2Data.linesCount = 5;
    list2Data.start = 0;
    list2Data.length = 5;
    list2Data.data = (char**)envStateLabels;
    prop.x = (800 / 8) * (2) + 1;
    prop.y = 29;
    prop.w = 800 / 8 - 3;
    prop.h = 394;
    prop.data = &list2Data;
    list2Control = display.createControl<cList>(&prop);

    list3Data.linesCount = lfoShapeCount;
    list3Data.start = 0;
    list3Data.length = lfoShapeCount;
    list3Data.data = (char**)lfoShapeLabels;
    prop.x = (800 / 8) * (3) + 1;
    prop.y = 29;
    prop.w = 800 / 8 - 3;
    prop.h = 394;
    prop.data = &list3Data;
    list3Control = display.createControl<cList>(&prop);

    list4Data.linesCount = 14;
    list4Data.start = 0;
    list4Data.length = 20;
    list4Data.data = (char**)lfoSpeedLabels;
    prop.x = (800 / 8) * (4) + 1;
    prop.y = 29;
    prop.w = 800 / 8 - 3;
    prop.h = 394;
    prop.data = &list4Data;
    list4Control = display.createControl<cList>(&prop);

    instSynthCCData.linesCount = 12;
    instSynthCCData.start = 0;
    instSynthCCData.length = std::size(InstrumentCCNames);
    instSynthCCData.data = (char**)InstrumentCCNames;
    prop.x = (800 / 8) * (0) + 1;
    prop.y = 80;
    prop.w = 800 / 8 * 4 - 3;
    prop.h = 335;
    prop.data = &instSynthCCData;
    instrumentSynthCCListControl = display.createControl<cList>(&prop);

    instData.linesCount = 12;
    instData.start = 0;
    instData.length = kSampleInstrumentsCount;
#if GUI_HANDLE_SYNTH
    instData.data = interfaceGlobals.ptrIntrumentsNames + kSynthInstrumentsCount;
#else
    instData.data = interfaceGlobals.ptrIntrumentsNames;
#endif
    prop.x = (800 / 8) * (4) + 1;
    prop.y = 80;
    prop.w = 800 / 8 * 3 - 3;
    prop.h = 335;
    prop.data = &instData;
    instrumentListControl = display.createControl<cList>(&prop);

    prop.style |= controlStyleFont1;
    prop.x = (800 / 8) * 1 + 1;
    prop.y = 29;
    prop.w = (800 / 8) * 2 - 3;
    prop.h = 394;
    prop.data = nullptr;
    paramsListControl = display.createControl<NewList>(&prop);

    prop.x = (800 / 8) * 0 + 1;
    prop.y = 29;
    prop.w = (800 / 8) * 1 - 3;
    prop.h = 394;
    prop.data = nullptr;
    modelListControl = display.createControl<NewList>(&prop);

    prop.style |= controlStyleCenterX;
    prop.x = (800 / 16) * 1;
    prop.y = 97;
    prop.w = (800 / 8) * 3;
    prop.h = 308;
    prop.data = &imgGridData;
    imgGridControl = display.createControl<ImgGrid>(&prop);

    synthCCArrows.bitmaps[0].bitmapIndex = displayArrowU;
    synthCCArrows.bitmaps[0].xValue = (800 / 8) * 0 + (800 / 16);
    synthCCArrows.bitmaps[0].yValue = 450;
    synthCCArrows.bitmaps[1].bitmapIndex = displayArrowD;
    synthCCArrows.bitmaps[1].xValue = (800 / 8) * 3 + (800 / 16);
    synthCCArrows.bitmaps[1].yValue = 450;

    instArrows.bitmaps[0].bitmapIndex = displayArrowU;
    instArrows.bitmaps[0].xValue = (800 / 8) * 4 + (800 / 16);
    instArrows.bitmaps[0].yValue = 450;
    instArrows.bitmaps[1].bitmapIndex = displayArrowD;
    instArrows.bitmaps[1].xValue = (800 / 8) * 6 + (800 / 16);
    instArrows.bitmaps[1].yValue = 450;

    for (uint8_t i = 0; i < 6; i++)
    {
        ccNameListData[i].linesCount = 14;
        ccNameListData[i].start = 0;
        ccNameListData[i].length = ccNumbersCount;
        ccNameListData[i].data = (char**)ccNameLabels;

        prop.style = controlStyleBackground |
                     (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
        prop.x = (800 / 8) * (i + 2) + 1;
        prop.y = 29;
        prop.w = 800 / 8 - 3;
        prop.h = 394;
        prop.data = &ccNameListData[i];
        ccNameListControl[i] = display.createControl<cList>(&prop);
    }

    prop2.value = 0;
    prop2.data = nullptr;
    prop2.text = instrSynthNoteText;
    prop2.colors = display.getTheme().blankSpaceCoverLabelColors;
    prop2.style = controlStyleFont1;
    noteLabel = display.createControl<cLabel>(&prop2);

    prop2.value = 0;
    prop2.data = nullptr;
    prop2.text = nullptr;
    prop2.colors = display.getTheme().blankSpaceCoverLabelColors;
    prop2.style = controlStyleBackground |
                  (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
    prop2.x = (800 / 8) * 7 + 1;
    prop2.w = 800 / 8 - 2;
    prop2.y = 29;
    prop2.h = 394;
    blankSpaceCover[0] = display.createControl<cLabel>(&prop2);
    blankSpaceCover[1] = display.createControl<cLabel>(&prop2);
}

void cInstrumentEditor::showTitleBar()
{
    songTimer.show();

    const char* label = nullptr;

    switch (mode)
    {
        case mtInstEditModeParams:
            if (isEffectsWindow)
            {
                label = "Instrument Effects";
            }
            else
            {
                label = "Instrument Parameters 1/2";
            }
            break;
        case mtInstEditModeEnv: label = "Instrument Automation 2/2"; break;
        case mtInstEditModeMidi:
            label = "MIDI Instrument Parameters";
            songTimer.hide();
            break;
        case mtInstEditModeInstrSynth: label = "Instrument MIDI Mapping"; break;
#if GUI_HANDLE_SYNTH
        case mtInstEditModeSynth: label = "Synth Parameters 1/2"; break;
        case mtInstEditModeSynthMacros: label = "Synth Macros 2/2"; break;
        case mtInstEditModeSynthPatchEdit:
        {
            int syntPartIdx = mtProject.values.lastUsedInstrument - SYNTH_INSTRUMENTS_START_IDX;
            assert(syntPartIdx >= 0 && syntPartIdx < kSynthInstrumentsCount);
            snprintf(synthPatchNameText,
                     sizeof(synthPatchNameText),
                     "Edit Patch: %s",
                     synth->getPatchName(syntPartIdx).c_str());
            label = synthPatchNameText;
            songTimer.hide();
            break;
        }
#endif
        default: label = "Instrument Parameters"; break;
    }

    titleBar->show(true);
    titleBar->setTitle(label);
    titleBar->showCurrentInstrumentOnRightLabel();
}

void cInstrumentEditor::showInstrumentEnv()
{
    showTitleBar();
    refreshFramePosition();

    for (uint8_t i = 0; i < 8; i++)
    {
        display.setControlColors(label[i], display.getTheme().activeLabelsColors);
        display.setControlColors(barControl[i], localActiveBarColors);
        display.setRemoveControlStyle(label[i], controlStyleLimitTextWidth);
    }

    display.setControlHide(label[0]);
    display.setControlHide(label[1]);

    display.setControlText(doubleLabelAutomation, "Destination");
    display.setControlShow(doubleLabelAutomation);

    display.setControlText(label[2], "Type");

    display.setControlText2(label[2], "");
    display.setControlText2(label[3], "");
    display.setControlText2(label[4], "");
    display.setControlText2(label[5], "");
    display.setControlText2(label[6], "");
    display.setControlText2(label[7], "");
    display.setControlText2(doubleLabelAutomation, "");

    display.setRemoveControlStyle(label[1], controlStyleShowBitmap);
    display.setRemoveControlStyle(label[6], controlStyleShowBitmap);
    display.setRemoveControlStyle(label[6], controlStyleSpecialFx2);
    display.setRemoveControlStyle(label[7], controlStyleSpecialFx2);

    display.setControlPosition(label[1], (800 / 8) * 1 + (800 / 16), -1);
    display.setControlPosition(label[6], (800 / 8) * 6 + (800 / 16), -1);

    display.setControlHide(instrumentSynthCCListControl);
    display.setControlHide(instrumentListControl);

    showEnvList();
    showEnvState();

    list1Data.lineWidthMax = 0;
    list1Data.linesCount = autoDestCount;
    list1Data.start = selectedEnvelope;
    list1Data.length = autoDestCount;
    list1Data.data = (char**)envelopesLabels;
    list1Data.lineTextLenMax = 0;

    display.setControlPosition(list1Control, (800 / 8) * 0 + 1, -1);
    display.setControlSize(list1Control, 800 / 4 - 3, -1);
    display.setControlData(list1Control, &list1Data);
    display.setControlColors(list1Control, display.getTheme().activeListColors);
    display.setControlShow(list1Control);

    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    uint8_t curStateIdx = 0;
    if (!editorInstrument->isAutomationDestEnabled(selectedEnvelope))
        curStateIdx = 0;
    else if (editorInstrument->envelope[envSlot].loop == 0)
        curStateIdx = 1;
    else if (editorInstrument->envelope[envSlot].loop == 1)
        curStateIdx = 2;
    else if (editorInstrument->envelope[envSlot].loop == 2)
        curStateIdx = 3;
    else if (editorInstrument->envelope[envSlot].loop == 3)
        curStateIdx = 4;

    list2Data.linesCount = 5;
    list2Data.start = curStateIdx;
    list2Data.length = 5;
    list2Data.data = (char**)envStateLabels;
    list2Data.lineTextLenMax = 0;

    display.setControlData(list2Control, &list2Data);
    display.setControlPosition(list2Control, (800 / 8) * 2 + 1, -1);
    display.setControlSize(list2Control, 800 / 8 - 3, -1);
    display.setControlColors(list2Control, display.getTheme().activeListColors);
    display.setControlShow(list2Control);

    display.setControlHide(filterModeListControl);

    display.setControlHide(blankSpaceCover[0]);
    display.setControlHide(noteLabel);
    display.setControlHide(paramsListControl);
    display.setControlHide(modelListControl);
    display.setControlHide(imgGridControl);

    //**********************************
    uint32_t shadow = (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
    uint32_t gradient = (display.getTheme().barShadow > 0 ? controlStyleGradient : 0);
    for (int i = 3; i < 8; ++i)
    {
        display.setControlStyle(
          barControl[i],
          (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground) | shadow |
            gradient);
    }

    uint8_t curLoop = editorInstrument->envelope[envSlot].loop;
    if (curLoop == 0)
    {
        display.setControlText(label[3], "Attack");
        display.setControlText(label[4], "Decay");
        display.setControlText(label[5], "Sustain");
        display.setControlText(label[6], "Release");
        display.setControlText(label[7], "Amount");

        display.setControlValue(bgLabel, 252); // 252
        display.refreshControl(bgLabel);

        display.setControlShow(barControl[3]);
        display.setControlShow(barControl[4]);
        display.setControlShow(barControl[5]);
        display.setControlShow(barControl[6]);
        display.setControlShow(barControl[7]);

        showEnvAttack();
        showEnvDecay();
        showEnvSustain();
        showEnvRelease();
        showEnvAmount();

        display.setControlHide(list3Control);
        display.setControlHide(list4Control);
    }
    else if (curLoop == 1)
    {
        display.setControlText(label[3], "Shape");
        display.setControlText(label[4], "Speed");
        display.setControlText(label[5], "Amount");
        display.setControlText(label[6], "");
        display.setControlText(label[7], "");

        display.setControlValue(bgLabel, 252); // 124
        display.refreshControl(bgLabel);

        display.setControlHide(barControl[3]);
        display.setControlHide(barControl[4]);
        display.setControlShow(barControl[5]);
        display.setControlHide(barControl[6]);
        display.setControlHide(barControl[7]);

        uint8_t curSpeed = editorInstrument->lfo[envSlot].speed;
        uint8_t maxSpeed = (envSlot == envAmp) ? CONSTRAIN_SPEED_AMP_ENVELOPE : CONSTRAIN_SPEED_OTHER_ENVELOPE;
        if (curSpeed > maxSpeed)
        {
            curSpeed = 0;
            editorInstrument->lfo[envSlot].speed = 0;
        }

        if (envSlot == envAmp)
        {
            list4Data.data = (char**)(lfoSpeedLabels + ENVELOPE_SPEED_CONSTRAIN_OFFSET);
            list4Data.length = ENVELOPE_SPEED_AMP_COUNT;
        }
        else
        {
            list4Data.data = (char**)lfoSpeedLabels;
            list4Data.length = ENVELOPE_SPEED_OTHER_COUNT;
        }
        list4Data.start = curSpeed;
        display.setControlData(list4Control, &list4Data);

        showLfoShape();
        showLfoSpeed();
        showLfoAmount();

        display.setControlShow(list3Control);
        display.setControlShow(list4Control);

        display.setControlPosition(blankSpaceCover[0], (800 / 8) * 6 + 1, -1);
        display.setControlSize(blankSpaceCover[0], 800 / 4 - 3, 394);
        display.setControlShow(blankSpaceCover[0]);
        display.refreshControl(blankSpaceCover[0]);
    }
    else if (curLoop == 2) // WT Env
    {
        display.setControlText(label[3], "Wave");
        display.setControlText(label[4], "Duration");
        display.setControlText(label[5], "Warp X/Y");
        display.setControlText(label[6], "Sustain");
        display.setControlText(label[7], "Amount");

        display.setControlValue(bgLabel, 252);
        display.refreshControl(bgLabel);

        display.setControlShow(barControl[3]);
        display.setControlShow(barControl[4]);
        display.setControlShow(barControl[5]);
        display.setControlShow(barControl[6]);
        display.setControlShow(barControl[7]);

        display.setControlHide(list3Control);
        display.setControlHide(list4Control);

        showWtEnvWave();
        showEnvDecay();
        showWtWarpXY();
        showWtSustain();
        showEnvAmount();
    }
    else if (curLoop == 3) // WT LFO
    {
        display.setControlText(label[3], "Wave");
        display.setControlText(label[4], "Speed");
        display.setControlText(label[5], "Warp X");
        display.setControlText(label[6], "Warp Y");
        display.setControlText(label[7], "Amount");

        display.setControlValue(bgLabel, 252);
        display.refreshControl(bgLabel);

        display.setControlShow(barControl[3]);
        display.setControlHide(barControl[4]);
        display.setControlStyle(
          barControl[5],
          (controlStyleShow | controlStyleValueLeftRight_100_100 | controlStyleBackground) | shadow |
            gradient);
        display.setControlStyle(
          barControl[6],
          (controlStyleShow | controlStyleValueLeftRight_100_100 | controlStyleBackground) | shadow |
            gradient);
        display.setControlShow(barControl[7]);

        display.setControlHide(list3Control);
        display.setControlShow(list4Control);

        uint8_t curSpeed = editorInstrument->lfo[envSlot].speed;
        uint8_t maxSpeed = (envSlot == envAmp) ? CONSTRAIN_SPEED_AMP_ENVELOPE : CONSTRAIN_SPEED_OTHER_ENVELOPE;
        if (curSpeed > maxSpeed)
        {
            curSpeed = 0;
            editorInstrument->lfo[envSlot].speed = 0;
        }

        if (envSlot == envAmp)
        {
            list4Data.data = (char**)(lfoSpeedLabels + ENVELOPE_SPEED_CONSTRAIN_OFFSET);
            list4Data.length = ENVELOPE_SPEED_AMP_COUNT;
        }
        else
        {
            list4Data.data = (char**)lfoSpeedLabels;
            list4Data.length = ENVELOPE_SPEED_OTHER_COUNT;
        }
        list4Data.start = curSpeed;
        display.setControlData(list4Control, &list4Data);

        showWtEnvWave();
        showLfoSpeed();
        showWtWarpX();
        showWtWarpY();
        showLfoAmount();
    }

    display.setControlStyle2(doubleLabelAutomation, controlStyleCenterX | controlStyleFont2);
    display.setControlShow(doubleLabelAutomation);
    display.refreshControl(doubleLabelAutomation);

    display.refreshControl(list3Control);
    display.refreshControl(list4Control);

    display.setControlHide(barControl[0]);
    display.setControlHide(barControl[1]);
    display.setControlHide(barControl[2]);

    for (uint8_t i = 2; i < 8; i++)
    {
        display.setControlValue(label[i], 1);
        display.setControlStyle2(label[i],
                                 controlStyleCenterX | controlStyleCenterY | controlStyleFont2);
        display.setControlShow(label[i]);
        display.refreshControl(label[i]);
        display.refreshControl(barControl[i]);
    }

    display.setControlShow(bgLabel);
    display.refreshControl(bgLabel);

    display.synchronizeRefresh();
}

// 2nd page of Instrument Params Editor
void cInstrumentEditor::showEffectScreen()
{
    showTitleBar();
    showSendScreenFrame();
    frameData.placesCount = 4;

    for (uint8_t i = 0; i < 8; i++)
    {
        display.setControlColors(label[i], display.getTheme().activeLabelsColors);
        display.setControlColors(barControl[i], localActiveBarColors);
        display.setRemoveControlStyle(label[i], controlStyleLimitTextWidth);
    }

    display.setRemoveControlStyle(label[6], controlStyleSpecialFx2);
    display.setAddControlStyle(label[7], controlStyleSpecialFx2);
    display.setControlColors(label[7], display.getTheme().moreButtonColors);

    uint32_t shadow = (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
    uint32_t gradient = (display.getTheme().barShadow > 0 ? controlStyleGradient : 0);

    display.setControlStyle(barControl[0],
                            (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground) |
                              shadow | gradient);
    display.setControlStyle(barControl[1],
                            (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground) |
                              shadow | gradient);
    display.setControlStyle(barControl[2],
                            (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground) |
                              shadow | gradient);
    display.setControlStyle(barControl[3],
                            (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground) |
                              shadow | gradient);

    display.setControlShow(barControl[0]);
    display.setControlShow(barControl[1]);
    display.setControlShow(barControl[2]);
    display.setControlShow(barControl[3]);
    display.setControlHide(filterModeListControl);
    display.setControlHide(barControl[5]);
    display.setControlHide(barControl[6]);

    display.setControlText(label[0], "Overdrive");
    display.setControlText(label[1], "Bit depth");
    display.setControlText(label[2], "Reverb send");
    display.setControlText(label[3], "Delay send");
    display.setControlText(label[4], "Instrument");
    display.setControlText2(label[4], "MIDI Mapping");

    display.setControlText(label[5], "");
    display.setControlText2(label[5], "");
    display.setControlText(label[6], "Save .pti");
    display.setControlText2(label[6], "");
    display.setControlText(label[7], "Back");
    display.setControlText2(label[7], "");

    labelDataMore.dotIndicator.count = 2;
    labelDataMore.dotIndicator.current = 1;
    display.setControlData(label[7], &labelDataMore);

    showParamsDelaySend();
    showParamsReverbSend();
    showParamsOverdrive();
    showParamsBitDepth();

    display.setRemoveControlStyle(label[1], controlStyleShowBitmap);
    display.setRemoveControlStyle(label[6], controlStyleShowBitmap);

    display.setControlPosition(label[1], (800 / 8) * 1 + (800 / 16), -1);
    display.setControlPosition(label[6], (800 / 8) * 6 + (800 / 16), -1);

    display.setControlHide(instrumentSynthCCListControl);
    display.setControlHide(instrumentListControl);

    for (uint8_t i = 0; i < 8; i++)
    {
        display.setControlValue(label[i], 1);
        display.setControlStyle2(label[i],
                                 controlStyleCenterX | controlStyleCenterY | controlStyleFont2);
        display.setControlShow(label[i]);
        display.refreshControl(label[i]);
        display.refreshControl(barControl[i]);
    }

    display.setControlStyle2(label[4],
                             controlStyleCenterX | controlStyleCenterY | controlStyleFont3);
    display.setControlColors(label[4], display.getTheme().activeButtonLabelsColors);

    const bool canSavePti = (editorInstrument != nullptr && editorInstrument->isActive &&
                             editorInstrument->sample.length > 0);
    display.setControlColors(label[6],
                             canSavePti ? display.getTheme().activeButtonLabelsColors
                                        : display.getTheme().inactiveButtonLabelsColors);
    display.refreshControl(label[6]);

    display.setControlValue(bgLabel, 255); // 0b10111111
    display.setControlShow(bgLabel);
    display.refreshControl(bgLabel);

    display.setControlHide(noteLabel);
    display.setControlHide(paramsListControl);
    display.setControlHide(modelListControl);
    display.setControlHide(imgGridControl);

    display.setControlPosition(blankSpaceCover[0], (800 / 8) * 4 + 1, -1);
    display.setControlSize(blankSpaceCover[0], (800 / 8) * 4 - 3, 394);
    display.setControlShow(blankSpaceCover[0]);
    display.refreshControl(blankSpaceCover[0]);

    display.synchronizeRefresh();
}

void cInstrumentEditor::showSendScreenFrame()
{
    if (selectedPlaceSends > 3)
        return;

    display.setControlValue(frameControl, selectedPlaceSends);
    display.setControlShow(frameControl);
    display.refreshControl(frameControl);
}

// 1st page of Instrument Params Editor
void cInstrumentEditor::showInstrumentParams()
{
    showTitleBar();

    refreshFramePosition();

    for (uint8_t i = 0; i < 8; i++)
    {
        display.setControlColors(label[i], display.getTheme().activeLabelsColors);
        display.setControlColors(barControl[i], localActiveBarColors);
        display.setRemoveControlStyle(label[i], controlStyleLimitTextWidth);
    }

    display.setRemoveControlStyle(label[6], controlStyleSpecialFx2);
    display.setAddControlStyle(label[7], controlStyleSpecialFx2);
    display.setControlColors(label[7], display.getTheme().moreButtonColors);

    display.setControlValue(label[0], 1);
    display.setControlValue(label[1], 1);
    display.setControlValue(label[2], 1);
    display.setControlValue(label[3], 1);
    display.setControlValue(label[4], 1);
    display.setControlValue(label[5], 1);
    display.setControlValue(label[6], 1);
    display.setControlValue(label[7], 1);

    display.setRemoveControlStyle(label[1], controlStyleShowBitmap);
    display.setRemoveControlStyle(label[6], controlStyleShowBitmap);

    display.setControlPosition(label[1], (800 / 8) * 1 + (800 / 16), -1);
    display.setControlPosition(label[6], (800 / 8) * 6 + (800 / 16), -1);

    display.setControlHide(instrumentSynthCCListControl);
    display.setControlHide(instrumentListControl);

    display.setControlShow(label[0]);
    display.setControlShow(label[1]);
    display.setControlHide(doubleLabelAutomation);

    display.setControlText(label[0], "Volume");
    display.setControlText(label[1], "Panning");
    display.setControlText(label[2], "Tune");
    display.setControlText(label[3], "Finetune");
    display.setControlText(label[4], "Filter Type");
    display.setControlText(label[5], "Cutoff");
    display.setControlText(label[6], "Resonance");
    display.setControlText(label[7], "More");

    display.setControlText2(label[0], "");
    display.setControlText2(label[1], "");
    display.setControlText2(label[2], "");
    display.setControlText2(label[3], "");
    display.setControlText2(label[4], "");
    display.setControlText2(label[5], "");
    display.setControlText2(label[6], "");
    display.setControlText2(label[7], "");

    labelDataMore.dotIndicator.count = 2;
    labelDataMore.dotIndicator.current = 0;
    display.setControlData(label[7], &labelDataMore);

    display.setControlHide(noteLabel);

    display.setControlPosition(blankSpaceCover[0], (800 / 8) * 7 + 1, -1);
    display.setControlSize(blankSpaceCover[0], 800 / 8 - 3, 394);
    display.setControlShow(blankSpaceCover[0]);
    display.refreshControl(blankSpaceCover[0]);

    showParamsVolume();
    showParamsPanning();
    showParamsTune();
    showParamsFineTune();
    showFilterType();
    showFilterCutOff();
    showFilterResonance();

    //-------------------------------------

    display.setControlShow(barControl[0]);
    display.setControlShow(barControl[1]);
    display.setControlShow(barControl[2]);
    display.setControlShow(barControl[3]);
    display.setControlHide(barControl[4]);
    display.setControlShow(barControl[5]);
    display.setControlShow(barControl[6]);
    display.setControlHide(barControl[7]);

    display.setControlHide(list1Control);
    display.setControlHide(list2Control);
    display.setControlHide(list3Control);
    display.setControlHide(list4Control);
    display.setControlShow(filterModeListControl);
    display.setControlHide(doubleLabelAutomation);
    display.setControlHide(paramsListControl);
    display.setControlHide(modelListControl);
    display.setControlHide(imgGridControl);

    for (uint8_t i = 0; i < 6; i++)
    {
        display.setControlHide(ccNameListControl[i]);
    }

    uint32_t shadow = (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
    uint32_t gradient = (display.getTheme().barShadow > 0 ? controlStyleGradient : 0);

    display.setControlStyle(
      barControl[0],
      (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground) | shadow |
        gradient);
    display.setControlStyle(
      barControl[1],
      (controlStyleShow | controlStyleValueLeftRight_100_100 | controlStyleBackground) | shadow |
        gradient);
    display.setControlStyle(
      barControl[2],
      (controlStyleShow | controlStyleValueLeftRight_100_100 | controlStyleBackground) | shadow |
        gradient);
    display.setControlStyle(
      barControl[3],
      (controlStyleShow | controlStyleValueLeftRight_100_100 | controlStyleBackground) | shadow |
        gradient);
    display.setControlStyle(
      barControl[5],
      (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground) | shadow |
        gradient);
    display.setControlStyle(
      barControl[6],
      (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground) | shadow |
        gradient);

    //-------------------------------------

    for (uint8_t i = 0; i < 8; i++)
    {
        display.setControlValue(label[i], 1);
        display.setControlStyle2(label[i],
                                 controlStyleCenterX | controlStyleCenterY | controlStyleFont2);
        display.setControlShow(label[i]);
        display.refreshControl(label[i]);
        display.refreshControl(barControl[i]);
    }
    display.setControlValue(bgLabel, 255);
    display.setControlShow(bgLabel);
    display.refreshControl(bgLabel);

    display.synchronizeRefresh();
}

// Instrument Synthesizer screen
void cInstrumentEditor::showInstrumentSynth()
{
    showTitleBar();

    refreshFramePosition();

    mtPopups.listInstruments(stepPopupInstr);

    for (uint8_t i = 0; i < 8; i++)
    {
        display.setControlColors(label[i], display.getTheme().activeLabelsColors);
        display.setControlColors(barControl[i], localActiveBarColors);
        display.setRemoveControlStyle(label[i], controlStyleLimitTextWidth);
    }

    display.setControlValue(label[0], 1);
    display.setControlValue(label[1], 1);
    display.setControlValue(label[2], 1);
    display.setControlValue(label[3], 1);
    display.setControlValue(label[4], 1);
    display.setControlValue(label[5], 1);
    display.setControlValue(label[6], 1);
    display.setControlValue(label[7], 1);

    display.setControlData(label[1], &synthCCArrows);
    display.setControlData(label[6], &instArrows);
    display.setAddControlStyle(label[1], controlStyleShowBitmap);
    display.setAddControlStyle(label[6], controlStyleShowBitmap);
    display.setRemoveControlStyle(label[6], controlStyleSpecialFx2);
    display.setRemoveControlStyle(label[7], controlStyleSpecialFx2);

    display.setControlPosition(label[1], 200, -1);
    display.setControlPosition(label[6], 600, -1);

    display.setControlText(label[0], "");
    display.setControlText(label[1], "CC List");
    display.setControlText(label[2], "");
    display.setControlText(label[3], "");
    display.setControlText(label[4], "");
    display.setControlText(label[5], "Instruments");
    display.setControlText(label[6], "");
    display.setControlText(label[7], "Back");

    display.setControlText2(label[0], "");
    display.setControlText2(label[1], "");
    display.setControlText2(label[2], "");
    display.setControlText2(label[3], "");
    display.setControlText2(label[4], "");
    display.setControlText2(label[5], "");
    display.setControlText2(label[6], "");
    display.setControlText2(label[7], "");

    display.setControlData(noteLabel, nullptr);
    display.setControlStyle(noteLabel, controlStyleFont1);
    display.setControlColors(noteLabel, display.getTheme().activeButtonLabelsColors);
    display.setControlText(noteLabel, instrSynthNoteText);
    display.setControlValue(noteLabel, 0);
    display.setControlPosition(noteLabel, 10, 30);
    display.setControlSize(noteLabel, 780, 30);
    display.setControlShow(noteLabel);
    display.refreshControl(noteLabel);

    display.setControlPosition(blankSpaceCover[0], 0, -1);
    display.setControlSize(blankSpaceCover[0], 800, 394);
    display.setControlShow(blankSpaceCover[0]);
    display.refreshControl(blankSpaceCover[0]);

    display.setControlHide(barControl[0]);
    display.setControlHide(barControl[1]);
    display.setControlHide(barControl[2]);
    display.setControlHide(barControl[3]);
    display.setControlHide(barControl[4]);
    display.setControlHide(barControl[5]);
    display.setControlHide(barControl[6]);
    display.setControlHide(barControl[7]);

    display.setControlHide(list1Control);
    display.setControlHide(list2Control);
    display.setControlHide(list3Control);
    display.setControlHide(list4Control);
    display.setControlHide(filterModeListControl);
    display.setControlHide(doubleLabelAutomation);
    display.setControlHide(paramsListControl);
    display.setControlHide(modelListControl);
    display.setControlHide(imgGridControl);

    for (uint8_t i = 0; i < 6; i++)
    {
        display.setControlHide(ccNameListControl[i]);
    }

    for (uint8_t i = 0; i < 8; i++)
    {
        display.setControlValue(label[i], 1);
        display.setControlStyle2(label[i],
                                 controlStyleCenterX | controlStyleCenterY | controlStyleFont2);
        display.setControlShow(label[i]);
        display.refreshControl(label[i]);
    }

    display.setControlValue(label[1], 0);
    display.setControlValue(label[5], 0);
    display.setControlValue(label[7], 0);

    display.setControlShow(frameControl);

    display.setControlShow(instrumentSynthCCListControl);
    display.setControlValue(instrumentSynthCCListControl, instSynthCCListPos);
    display.refreshControl(instrumentSynthCCListControl);

    display.setControlShow(instrumentListControl);
    display.setControlValue(instrumentListControl, instListPos);
    display.refreshControl(instrumentListControl);

    display.setControlValue(bgLabel, 0b10010000);
    display.setControlShow(bgLabel);
    display.refreshControl(bgLabel);

    display.synchronizeRefresh();
}

void cInstrumentEditor::showInstrumentMidiParams()
{
    showTitleBar();
    refreshFramePosition();

    for (uint8_t i = 0; i < 8; i++)
    {
        display.setControlValue(label[i], 1);
        display.setControlColors(label[i], display.getTheme().activeLabelsColors);
        display.setRemoveControlStyle(label[i], controlStyleLimitTextWidth);
        display.setControlStyle2(label[i],
                                 controlStyleCenterX | controlStyleCenterY | controlStyleFont2);
        display.setControlHide(barControl[i]);
    }

    uint32_t shadow = (display.getTheme().bottomScreenShadow > 0 ? controlStyleBottomShadow : 0);
    uint32_t gradient = (display.getTheme().barShadow > 0 ? controlStyleGradient : 0);

    display.setControlStyle(barControl[0],
                            (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground) |
                              shadow | gradient);
    display.setControlStyle(barControl[1],
                            (controlStyleShow | controlStyleValue_0_100 | controlStyleBackground) |
                              shadow | gradient);
    display.setControlShow(barControl[0]);
    display.setControlShow(barControl[1]);

    display.setControlValue(label[0], 1);
    display.setControlValue(label[1], 1);
    display.setControlValue(label[2], 1);
    display.setControlValue(label[3], 1);
    display.setControlValue(label[4], 1);
    display.setControlValue(label[5], 1);
    display.setControlValue(label[6], 1);
    display.setControlValue(label[7], 1);

    display.setControlText(label[0], "Velocity");
    display.setControlText(label[1], "Offset");
    display.setControlText(label[2], "CC A");
    display.setControlText(label[3], "CC B");
    display.setControlText(label[4], "CC C");
    display.setControlText(label[5], "CC D");
    display.setControlText(label[6], "CC E");
    display.setControlText(label[7], "CC F");

    display.setControlText2(label[0], "");
    display.setControlText2(label[1], "");
    display.setControlText2(label[2], "");
    display.setControlText2(label[3], "");
    display.setControlText2(label[4], "");
    display.setControlText2(label[5], "");
    display.setControlText2(label[6], "");
    display.setControlText2(label[7], "");

    showParamsVelocity();
    showParamsOffset();

    display.setRemoveControlStyle(label[1], controlStyleShowBitmap);
    display.setRemoveControlStyle(label[6], controlStyleShowBitmap);
    display.setRemoveControlStyle(label[7], controlStyleSpecialFx2);
    display.setRemoveControlStyle(label[6], controlStyleSpecialFx2);

    display.setControlPosition(label[1], (800 / 8) * 1 + (800 / 16), -1);
    display.setControlPosition(label[6], (800 / 8) * 6 + (800 / 16), -1);

    for (uint8_t i = 0; i < 8; i++)
    {
        display.setControlShow(label[i]);
        display.refreshControl(label[i]);
    }

    //-------------------------------------

    for (uint8_t i = 0; i < 6; i++)
    {
        showParamsCC(i);
        ccNameListData[i].start = getActualMidiInstrumentCCValue(i);
        display.setControlData(ccNameListControl[i], &ccNameListData[i]);
        display.setControlShow(ccNameListControl[i]);
        display.refreshControl(ccNameListControl[i]);
    }

    //-------------------------------------

    display.setControlValue(bgLabel, 255);
    display.setControlShow(bgLabel);
    display.refreshControl(bgLabel);

    display.setControlHide(list1Control);
    display.setControlHide(list2Control);
    display.setControlHide(list3Control);
    display.setControlHide(list4Control);
    display.setControlHide(filterModeListControl);
    display.setControlHide(doubleLabelAutomation);
    display.setControlHide(paramsListControl);
    display.setControlHide(modelListControl);
    display.setControlHide(instrumentSynthCCListControl);
    display.setControlHide(instrumentListControl);
    display.setControlHide(noteLabel);
    display.setControlHide(blankSpaceCover[0]);
    display.setControlHide(imgGridControl);

    display.synchronizeRefresh();
}

//==============================================================================================================
void cInstrumentEditor::activateLabelsBorder()
{
    if (selectedPlace[mode] > frameData.placesCount - 1)
        return;

    display.setControlValue(frameControl, selectedPlace[mode]);
    display.setControlShow(frameControl);
    display.refreshControl(frameControl);
}

//==============================================================================================================
void cInstrumentEditor::showSynthCCList()
{
    display.setControlValue(instrumentSynthCCListControl, instSynthCCListPos);
    display.refreshControl(instrumentSynthCCListControl);
}

void cInstrumentEditor::showInstList()
{
    display.setControlValue(instrumentListControl, instListPos);
    display.refreshControl(instrumentListControl);
}

void cInstrumentEditor::showEnvList()
{
    display.setControlValue(list1Control, selectedEnvelope);
    display.refreshControl(list1Control);
}

void cInstrumentEditor::showEnvState()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    uint8_t val = 0;
    if (!editorInstrument->isAutomationDestEnabled(selectedEnvelope))
        val = 0;
    else if (editorInstrument->envelope[envSlot].loop == 0)
        val = 1;
    else if (editorInstrument->envelope[envSlot].loop == 1)
        val = 2;
    else if (editorInstrument->envelope[envSlot].loop == 2)
        val = 3;
    else if (editorInstrument->envelope[envSlot].loop == 3)
        val = 4;

    display.setControlText2(label[2], envStateLabels[val]);
    display.refreshControl(label[2]);

    display.setControlValue(list2Control, val);
    display.refreshControl(list2Control);
}

void cInstrumentEditor::showWtEnvWave()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    uint8_t shape = editorInstrument->lfo[envSlot].shape;
    if (shape >= 128)
    {
        static bool s_userWavetablesLoaded = false;
        if (!s_userWavetablesLoaded)
        {
            s_userWavetablesLoaded = true;
            loadUserWavetablesFromSD(newFileManager.getFileSystem());
        }
    }

    if (shape < 128)
        sprintf(labelText[3], "F %03d", shape + 1);
    else
        sprintf(labelText[3], "U %03d", (shape - 128) + 1);

    display.setControlText2(label[3], labelText[3]);
    display.refreshControl(label[3]);
    display.setControlValue(barControl[3], (shape * 100) / 255);
    display.refreshControl(barControl[3]);
}

void cInstrumentEditor::showWtWarpXY()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    uint8_t bend = editorInstrument->lfo[envSlot].wtBend;
    int8_t wx = unpackWarpX(bend);
    int8_t wy = unpackWarpY(bend);
    sprintf(labelText[5], "%+d / %+d", wx, wy);

    display.setControlText2(label[5], labelText[5]);
    display.refreshControl(label[5]);
    display.setControlValue(barControl[5], 1000 + (int)bend);
    display.refreshControl(barControl[5]);
}

void cInstrumentEditor::showWtWarpX()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    int8_t wx = unpackWarpX(editorInstrument->lfo[envSlot].wtBend);
    sprintf(labelText[5], "%+d", wx);

    display.setControlText2(label[5], labelText[5]);
    display.refreshControl(label[5]);
    display.setControlValue(barControl[5], ((int)wx * 100) / 7);
    display.refreshControl(barControl[5]);
}

void cInstrumentEditor::showWtWarpY()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    int8_t wy = unpackWarpY(editorInstrument->lfo[envSlot].wtBend);
    sprintf(labelText[6], "%+d", wy);

    display.setControlText2(label[6], labelText[6]);
    display.refreshControl(label[6]);
    display.setControlValue(barControl[6], ((int)wy * 100) / 7);
    display.refreshControl(barControl[6]);
}

void cInstrumentEditor::showWtSustain()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    uint8_t sust = editorInstrument->lfo[envSlot].wtSust;
    if (sust == 0)
        sprintf(labelText[6], "Off");
    else
        sprintf(labelText[6], "%d%%", sust);

    display.setControlText2(label[6], labelText[6]);
    display.refreshControl(label[6]);
    display.setControlValue(barControl[6], sust);
    display.refreshControl(barControl[6]);
}

void cInstrumentEditor::showEnvAttack()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    sprintf(labelText[3],
            "%.3fs",
            (float)(editorInstrument->envelope[envSlot].attack / 1000.0f));

    if (editorInstrument->isAutomationDestEnabled(selectedEnvelope))
    {
        display.setControlColors(label[3], display.getTheme().activeLabelsColors);
        display.setControlColors(barControl[3], localActiveBarColors);
    }
    else
    {
        display.setControlColors(label[3], display.getTheme().inactiveLabelsColors);
        display.setControlColors(barControl[3], localInactiveBarColors);
    }

    display.setControlText2(label[3], labelText[3]);
    display.refreshControl(label[3]);

    display.setControlValue(
      barControl[3],
      (editorInstrument->envelope[envSlot].attack > ENVELOPE_MICRO_RANGE)
        ? map<int>(editorInstrument->envelope[envSlot].attack,
                   ENVELOPE_MICRO_RANGE,
                   ATTACK_MAX,
                   50,
                   100)
        : map<int>(
            editorInstrument->envelope[envSlot].attack, 0, ENVELOPE_MICRO_RANGE, 0, 50));
    display.refreshControl(barControl[3]);
}

void cInstrumentEditor::showEnvDecay()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    sprintf(
      labelText[4], "%.3fs", (float)(editorInstrument->envelope[envSlot].decay / 1000.0f));

    if (editorInstrument->isAutomationDestEnabled(selectedEnvelope))
    {
        display.setControlColors(label[4], display.getTheme().activeLabelsColors);
        display.setControlColors(barControl[4], localActiveBarColors);
    }
    else
    {
        display.setControlColors(label[4], display.getTheme().inactiveLabelsColors);
        display.setControlColors(barControl[4], localInactiveBarColors);
    }

    display.setControlText2(label[4], labelText[4]);
    display.refreshControl(label[4]);

    display.setControlValue(
      barControl[4],
      (editorInstrument->envelope[envSlot].decay > ENVELOPE_MICRO_RANGE)
        ? map<int>(editorInstrument->envelope[envSlot].decay,
                   ENVELOPE_MICRO_RANGE,
                   DECAY_MAX,
                   50,
                   100)
        : map<int>(
            editorInstrument->envelope[envSlot].decay, 0, ENVELOPE_MICRO_RANGE, 0, 50));
    display.refreshControl(barControl[4]);
}

void cInstrumentEditor::showEnvSustain()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    snprintf(labelText[5],
             sizeof(labelText[5]),
             "%.0f",
             (float)(editorInstrument->envelope[envSlot].sustain * 100));

    if (editorInstrument->isAutomationDestEnabled(selectedEnvelope))
    {
        display.setControlColors(label[5], display.getTheme().activeLabelsColors);
        display.setControlColors(barControl[5], localActiveBarColors);
    }
    else
    {
        display.setControlColors(label[5], display.getTheme().inactiveLabelsColors);
        display.setControlColors(barControl[5], localInactiveBarColors);
    }

    display.setControlText2(label[5], labelText[5]);
    display.refreshControl(label[5]);

    display.setControlValue(barControl[5],
                            (editorInstrument->envelope[envSlot].sustain * 100));
    display.refreshControl(barControl[5]);
}

void cInstrumentEditor::showEnvRelease()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    sprintf(labelText[6],
            "%.3fs",
            (float)(editorInstrument->envelope[envSlot].release / 1000.0f));

    if (editorInstrument->isAutomationDestEnabled(selectedEnvelope))
    {
        display.setControlColors(label[6], display.getTheme().activeLabelsColors);
        display.setControlColors(barControl[6], localActiveBarColors);
    }
    else
    {
        display.setControlColors(label[6], display.getTheme().inactiveLabelsColors);
        display.setControlColors(barControl[6], localInactiveBarColors);
    }

    display.setControlText2(label[6], labelText[6]);
    display.refreshControl(label[6]);

    display.setControlValue(
      barControl[6],
      (editorInstrument->envelope[envSlot].release > ENVELOPE_MICRO_RANGE)
        ? map<int>(editorInstrument->envelope[envSlot].release,
                   ENVELOPE_MICRO_RANGE,
                   RELEASE_MAX,
                   50,
                   100)
        : map<int>(
            editorInstrument->envelope[envSlot].release, 0, ENVELOPE_MICRO_RANGE, 0, 50));
    display.refreshControl(barControl[6]);
}

void cInstrumentEditor::showEnvAmount()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    snprintf(labelText[7],
             sizeof(labelText[7]),
             "%.0f",
             (float)(editorInstrument->envelope[envSlot].amount * 100));

    if (editorInstrument->isAutomationDestEnabled(selectedEnvelope))
    {
        display.setControlColors(label[7], display.getTheme().activeLabelsColors);
        display.setControlColors(barControl[7], localActiveBarColors);
    }
    else
    {
        display.setControlColors(label[7], display.getTheme().inactiveLabelsColors);
        display.setControlColors(barControl[7], localInactiveBarColors);
    }

    display.setControlText2(label[7], labelText[7]);
    display.refreshControl(label[7]);

    display.setControlValue(barControl[7],
                            (editorInstrument->envelope[envSlot].amount * 100));
    display.refreshControl(barControl[7]);
}

void cInstrumentEditor::showLfoShape()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    uint8_t shape = editorInstrument->lfo[envSlot].shape;
    if (shape >= lfoShapeCount)
    {
        shape = 0;
        editorInstrument->lfo[envSlot].shape = 0;
    }
    display.setControlText2(label[3], lfoShapeLabels[shape]);
    display.refreshControl(label[3]);

    display.setControlValue(list3Control, shape);
    display.refreshControl(list3Control);
}
void cInstrumentEditor::showLfoSpeed()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    uint8_t speed = editorInstrument->lfo[envSlot].speed;
    uint8_t maxSpeed = (envSlot == envAmp) ? CONSTRAIN_SPEED_AMP_ENVELOPE : CONSTRAIN_SPEED_OTHER_ENVELOPE;
    if (speed > maxSpeed)
    {
        speed = 0;
        editorInstrument->lfo[envSlot].speed = 0;
    }
    uint8_t selectedLabel =
      (envSlot == envAmp)
        ? speed + ENVELOPE_SPEED_CONSTRAIN_OFFSET
        : speed;
    if (selectedLabel >= ENVELOPE_SPEED_OTHER_COUNT)
    {
        selectedLabel = 0;
    }
    display.setControlText2(label[4], lfoSpeedLabels[selectedLabel]);
    display.refreshControl(label[4]);

    display.setControlValue(list4Control, speed);
    display.refreshControl(list4Control);
}
void cInstrumentEditor::showLfoAmount()
{
    const uint8_t envSlot = automationDestToEnvSlot(selectedEnvelope);
    uint8_t col = (editorInstrument->envelope[envSlot].loop == 3) ? 7 : 5;
    snprintf(labelText[col],
             sizeof(labelText[col]),
             "%.0f",
             (float)(editorInstrument->lfo[envSlot].amount * 100));
    display.setControlText2(label[col], labelText[col]);
    display.refreshControl(label[col]);

    display.setControlValue(barControl[col], (editorInstrument->lfo[envSlot].amount * 100));
    display.refreshControl(barControl[col]);
}

void cInstrumentEditor::showParamsVelocity()
{
    if (mtProject.values.lastUsedInstrument < kSampleInstrumentsCount)
    {
        sprintf(labelText[0], "%d", editorInstrument->volume);

        display.setControlValue(barControl[0], editorInstrument->volume);
    }
    else
    {
        uint8_t temp_velocity =
          mtProject.values
            .midiInstrument[mtProject.values.lastUsedInstrument - kSampleInstrumentsCount]
            .velocity;
        sprintf(labelText[0], "%d", temp_velocity);

        display.setControlValue(barControl[0], (temp_velocity * 100) / 127);
    }

    display.setControlText2(label[0], labelText[0]);

    display.refreshControl(label[0]);
    display.refreshControl(barControl[0]);
}

void cInstrumentEditor::showParamsOffset()
{
    if (mtProject.values.lastUsedInstrument < kSampleInstrumentsCount)
    {
        return;
    }
    else
    {
        uint8_t temp_offset =
          mtProject.values
            .midiInstrument[mtProject.values.lastUsedInstrument - kSampleInstrumentsCount]
            .offset;
        sprintf(labelText[1], "%d", temp_offset);

        display.setControlValue(barControl[1], (temp_offset));
    }

    display.setControlText2(label[1], labelText[1]);

    display.refreshControl(label[1]);
    display.refreshControl(barControl[1]);
}

void cInstrumentEditor::showParamsCC(uint8_t index)
{
    if (mtProject.values.lastUsedInstrument < MIDI_INSTRUMENTS_START_IDX ||
        mtProject.values.lastUsedInstrument >= MIDI_INSTRUMENTS_START_IDX + kMidiInstrumentsCount ||
        index >= 6)
    {
        return;
    }

    uint8_t value = getActualMidiInstrumentCCValue(index);

    int labelIdx = index + 2;
    snprintf(labelText[labelIdx], 8, "%d", value);

    display.setControlValue(ccNameListControl[index], value);
    display.setControlText2(label[labelIdx], labelText[labelIdx]);
    display.refreshControl(label[labelIdx]);
    display.refreshControl(ccNameListControl[index]);
}

void cInstrumentEditor::showParamsVolume()
{
    float volumeInDecybels = calcInstrumentVolumeDecybelValue(editorInstrument->volume);

    sprintf(labelText[0], "%0.02f dB", volumeInDecybels);

    display.setControlText2(label[0], labelText[0]);
    display.refreshControl(label[0]);

    display.setControlValue(barControl[0], editorInstrument->volume);
    display.refreshControl(barControl[0]);
}

void cInstrumentEditor::showParamsPanning()
{
    sprintf(labelText[1], "%d", editorInstrument->panning - 50);

    display.setControlText2(label[1], labelText[1]);
    display.refreshControl(label[1]);

    display.setControlValue(barControl[1], editorInstrument->panning * 2 - 100);
    display.refreshControl(barControl[1]);
}

void cInstrumentEditor::showParamsTune()
{
    sprintf(labelText[2], "%d", editorInstrument->tune);

    display.setControlText2(label[2], labelText[2]);
    display.refreshControl(label[2]);

    display.setControlValue(barControl[2], (editorInstrument->tune * 100) / 24);
    display.refreshControl(barControl[2]);
}

void cInstrumentEditor::showParamsFineTune()
{
    sprintf(labelText[3], "%d", editorInstrument->fineTune);
    display.setControlText2(label[3], labelText[3]);
    display.refreshControl(label[3]);

    display.setControlValue(barControl[3], editorInstrument->fineTune);
    display.refreshControl(barControl[3]);
}

void cInstrumentEditor::showFilterType()
{
    if (editorInstrument->filterEnable)
    {
        if (editorInstrument->filterType == lowPass)
            filterModeListPos = 1;
        else if (editorInstrument->filterType == highPass)
            filterModeListPos = 2;
        else
            filterModeListPos = 3;
    }
    else
    {
        filterModeListPos = 0;
    }

    filterModeList.start = filterModeListPos;
    //	filterModeList.length = filterModeCount;
    //	filterModeList.linesCount = 5;
    //	filterModeList.data = (char**)filterModeNames;

    display.setControlText2(label[4], filterModeFunctLabels[filterModeListPos]);
    display.refreshControl(label[4]);

    display.setControlData(filterModeListControl, &filterModeList);
    // display.setControlValue(IE->filterModeListControl, IE->filterModeListPos);
    display.setControlShow(filterModeListControl);
    display.refreshControl(filterModeListControl);

    showFilterCutOff();
    showFilterResonance();
}

void cInstrumentEditor::showFilterCutOff()
{
    uint8_t temp_cutoff = (editorInstrument->cutOff * 100);

    sprintf(labelText[5], "%d", temp_cutoff);

    if (editorInstrument->filterEnable)
    {
        display.setControlColors(label[5], display.getTheme().activeLabelsColors);
        display.setControlColors(barControl[5], localActiveBarColors);
    }
    else
    {
        display.setControlColors(label[5], display.getTheme().inactiveLabelsColors);
        display.setControlColors(barControl[5], localInactiveBarColors);
    }

    display.setControlText2(label[5], labelText[5]);
    display.refreshControl(label[5]);

    display.setControlValue(barControl[5], temp_cutoff);
    display.refreshControl(barControl[5]);
}

void cInstrumentEditor::showFilterResonance()
{
    uint8_t temp_resonance =
      ((editorInstrument->resonance - RESONANCE_MIN) / (RESONANCE_MAX - RESONANCE_MIN)) * 100;

    sprintf(labelText[6], "%d", temp_resonance);

    if (editorInstrument->filterEnable)
    {
        display.setControlColors(label[6], display.getTheme().activeLabelsColors);
        display.setControlColors(barControl[6], localActiveBarColors);
    }
    else
    {
        display.setControlColors(label[6], display.getTheme().inactiveLabelsColors);
        display.setControlColors(barControl[6], localInactiveBarColors);
    }

    display.setControlText2(label[6], labelText[6]);
    display.refreshControl(label[6]);

    display.setControlValue(barControl[6], temp_resonance);
    display.refreshControl(barControl[6]);
}

void cInstrumentEditor::showParamsDelaySend()
{
    float sendInDecybels = calcInstrumentSendsDecybelValue(editorInstrument->delaySend);

    sprintf(labelText[3], "%0.02f dB", sendInDecybels);

    display.setControlText2(label[3], labelText[3]);
    display.refreshControl(label[3]);

    display.setControlValue(barControl[3], editorInstrument->delaySend);
    display.refreshControl(barControl[3]);
}

void cInstrumentEditor::showParamsReverbSend()
{
    float sendInDecybels = calcInstrumentSendsDecybelValue(editorInstrument->reverbSend);

    sprintf(labelText[2], "%0.02f dB", sendInDecybels);

    display.setControlText2(label[2], labelText[2]);
    display.refreshControl(label[2]);

    display.setControlValue(barControl[2], editorInstrument->reverbSend);
    display.refreshControl(barControl[2]);
}

void cInstrumentEditor::showParamsOverdrive()
{
    sprintf(labelText[0], "%d", editorInstrument->overdrive);
    display.setControlText2(label[0], labelText[0]);
    display.refreshControl(label[0]);

    display.setControlValue(barControl[0], editorInstrument->overdrive);
    display.refreshControl(barControl[0]);
}
void cInstrumentEditor::showParamsBitDepth()
{
    sprintf(labelText[1], "%d", editorInstrument->bitDepth);
    display.setControlText2(label[1], labelText[1]);
    display.refreshControl(label[1]);

    uint8_t barDisplayedBitDepth =
      (uint8_t)map<int>(editorInstrument->bitDepth, BIT_DEPTH_MIN, BIT_DEPTH_MAX, 0, 100);

    display.setControlValue(barControl[1], barDisplayedBitDepth);
    display.refreshControl(barControl[1]);
}

void cInstrumentEditor::refreshFramePosition()
{
    switch (mode)
    {
        case mtInstEditModeParams:
        {
            frameData.placesCount = 7;
            frameData.startPlace = 0;
            frameData.places[0] = &framesPlaces[0][0];
            frameData.places[1] = &framesPlaces[1][0];
            frameData.places[2] = &framesPlaces[2][0];
            frameData.places[3] = &framesPlaces[3][0];
            frameData.places[4] = &framesPlaces[4][0];
            frameData.places[5] = &framesPlaces[5][0];
            frameData.places[6] = &framesPlaces[6][0];
            break;
        }
        case mtInstEditModeMidi:
        {
            frameData.placesCount = 8;
            frameData.startPlace = 0;
            frameData.places[0] = &framesPlaces[0][0];
            frameData.places[1] = &framesPlaces[1][0];
            frameData.places[2] = &framesPlaces[2][0];
            frameData.places[3] = &framesPlaces[3][0];
            frameData.places[4] = &framesPlaces[4][0];
            frameData.places[5] = &framesPlaces[5][0];
            frameData.places[6] = &framesPlaces[6][0];
            frameData.places[7] = &framesPlaces[7][0];
            break;
        }
        case mtInstEditModeEnv:
        {
            frameData.placesCount = 7;
            frameData.startPlace = 0;
            frameData.places[0] = &framesPlacesAutomation[0][0];
            frameData.places[1] = &framesPlacesAutomation[1][0];
            frameData.places[2] = &framesPlacesAutomation[2][0];
            frameData.places[3] = &framesPlacesAutomation[3][0];
            frameData.places[4] = &framesPlacesAutomation[4][0];
            frameData.places[5] = &framesPlacesAutomation[5][0];
            frameData.places[6] = &framesPlacesAutomation[6][0];
            break;
        }
        case mtInstEditModeInstrSynth:
        {
            frameData.placesCount = 2;
            frameData.startPlace = 0;
            frameData.places[0] = &framesPlacesCC[0][0];
            frameData.places[1] = &framesPlacesCC[1][0];
            break;
        }
#if GUI_HANDLE_SYNTH
        case mtInstEditModeSynthMacros:
        {
            frameData.placesCount = 6;
            frameData.startPlace = 0;
            frameData.places[0] = &framesPlaces[0][0];
            frameData.places[1] = &framesPlaces[1][0];
            frameData.places[2] = &framesPlaces[2][0];
            frameData.places[3] = &framesPlaces[3][0];
            frameData.places[4] = &framesPlaces[4][0];
            frameData.places[5] = &framesPlaces[5][0];
            break;
        }
        case mtInstEditModeSynth:
        {
            frameData.placesCount = 6;
            frameData.startPlace = 0;
            frameData.places[0] = &framesPlaces[0][0];
            frameData.places[1] = &framesPlaces[1][0];
            frameData.places[2] = &framesPlaces[2][0];
            frameData.places[3] = &framesPlaces[3][0];
            frameData.places[4] = &framesPlaces[4][0];
            frameData.places[5] = &framesPlaces[5][0];
            break;
        }
        case mtInstEditModeSynthPatchEdit:
        {
            frameData.placesCount = 2;
            frameData.startPlace = 0;
            frameData.places[0] = &framesPlaces[0][0];
            frameData.places[1] = &framesPlacesSynthEdit[0][0];
            break;
        }
#endif
        default: break;
    }
    activateLabelsBorder();
}

void cInstrumentEditor::showActualInstrument()
{
}

void cInstrumentEditor::hideControlUnderMenu()
{
    display.setControlHide(filterModeListControl);
    display.setControlHide(list2Control);

    display.setControlHide(barControl[2]);
    display.setControlHide(barControl[3]);
    display.setControlHide(barControl[4]);
    display.setControlHide(barControl[5]);

    display.setControlHide(ccNameListControl[0]);
    display.setControlHide(ccNameListControl[1]);
    display.setControlHide(ccNameListControl[2]);
    display.setControlHide(ccNameListControl[3]);

    display.refreshControl(label[5]);
}

const char* const InstrumentCCNames[] = {
    "Tune = 5",
    "Finetune = 3",
    "Volume = 7",
    "Panning = 10",
    "Filter type = 9",
    "Cutoff = 1",
    "Resonance = 11",
    "Overdrive = 12",
    "Bit Depth = 13",
    "Reverb Send = 14",
    "Delay Send = 15",
    "Volume Automation Type = 20",
    "Volume Attack - Shape LFO = 21",
    "Volume Decay - Speed LFO = 22",
    "Volume Sustain = 23",
    "Volume Release = 24",
    "Volume Amount = 25",
    "Panning Automation Type = 26",
    "Panning Attack - Shape LFO = 27",
    "Panning Decay - Speed LFO = 28",
    "Panning Sustain = 29",
    "Panning Release = 30",
    "Panning Amount = 31",
    "Cutoff Automation Type = 83",
    "Cutoff Attack - Shape LFO = 84",
    "Cutoff Decay - Speed LFO = 85",
    "Cutoff Sustain = 86",
    "Cutoff Release = 87",
    "Cutoff Amount = 88",
    "Wavetable Position Automation Type = 102",
    "Wavetable Position Attack - Shape LFO = 103",
    "Wavetable Position Decay - Speed LFO = 104",
    "Wavetable Position Sustain = 105",
    "Wavetable Position Release = 106",
    "Wavetable Position Amount = 107",
    "Granular Position Automation Type = 108",
    "Granular Position Attack - Shape LFO = 109",
    "Granular Position Decay - Speed LFO = 110",
    "Granular Position Sustain = 111",
    "Granular Position Release = 112",
    "Granular Position Amount = 113",
    "Finetune Automation Type = 114",
    "Finetune Attack - Shape LFO = 115",
    "Finetune Decay - Speed LFO = 116",
    "Finetune Sustain = 117",
    "Finetune Release = 118",
    "Finetune Amount = 119",
};

// --------------------------------------------  synth  -------------------------------------------
// --------------------------------------------  synth  -------------------------------------------
// --------------------------------------------  synth  -------------------------------------------
