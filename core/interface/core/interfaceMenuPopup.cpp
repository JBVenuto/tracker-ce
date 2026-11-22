#include "interfaceMenuPopup.h"
#include "core/interface.h"
#include "mtStructs.h"

InterfaceMenuPopup mtMenuPopup;

const uint8_t kMenuModulesCount = 11;
const char* const kModuleNamesList[kMenuModulesCount] = { "Pattern",
                                                          "Instrument Parameters",
                                                          "Sample Playback",
                                                          "Master",
                                                          "Song",
                                                          "Sample Loader",
                                                          "File",
                                                          "Config",
                                                          "Perform",
                                                          "Sample Editor",
                                                          "Sample Recorder" };

#ifdef TRACKER_MINI

uint32_t instrumentParamOptions[2] = { 1, 2 };
InterfaceMenuPopup::ModuleModes instrumentParamModes = { 2, instrumentParamOptions, { 2, 0 } };
uint32_t masterOptions[3] = { (1 << 8), (2 << 8), (3 << 8) };
InterfaceMenuPopup::ModuleModes masterModes = { 3, masterOptions, { 3, 0 } };
uint32_t performOptions[2] = { 1, 2 };
InterfaceMenuPopup::ModuleModes performModes = { 2, performOptions, { 2, 0 } };

const InterfaceMenuPopup::ModuleRef InterfaceMenuPopup::modules[kMenuModulesCount] = {
    { mtInterface.modules[2], interfaceButtonPattern, 0, nullptr },
    { mtInterface.modules[5], interfaceButtonParams, 0, &instrumentParamModes },
    { mtInterface.modules[3], interfaceButtonSamplePlay, 0, nullptr },
    { mtInterface.modules[12], interfaceButtonMaster, 0, &masterModes },
    { mtInterface.modules[4], interfaceButtonSong, 0, nullptr },
    { mtInterface.modules[1], interfaceButtonSampleLoad, 0, nullptr },
    { mtInterface.modules[0], interfaceButtonFile, 0, nullptr },
    { mtInterface.modules[7], interfaceButtonConfig, 0, nullptr },
    { mtInterface.modules[9], interfaceButtonPerformance, 0, &performModes },
    { mtInterface.modules[6], interfaceButtonSampleEdit, 0, nullptr },
    { mtInterface.modules[8], interfaceButtonSampleRec, 0, nullptr },

};

static void functLeftPopup();
static void functRightPopup();
static void functUpPopup();
static void functDownPopup();
static void functMiddlePopup();

static void functEncoderPopup(int16_t value);

void InterfaceMenuPopup::initPopupsDisplayControls()
{
    boxMenuData.columnsCount = kMenuColumnsCount;
    boxMenuData.rowsCount = kMenuRowsCount;
    boxMenuData.textList = (char**)modulesMenuTexts;
    boxMenuData.modes = modulesMenuModes;

    strControlProperties prop;
    prop.x = (800 - kMenuWidth) / 2;
    prop.y = (480 - kMenuHeight) / 2;
    prop.w = kMenuWidth;
    prop.h = kMenuHeight;
    prop.style = controlStyleCenterX | controlStyleCenterY | controlStyleFont2;
    prop.data = &boxMenuData;
    prop.text = nullptr;
    prop.colors = display.getTheme().miniBoxMenuColors;
    boxMenu = display.createControl<cBoxMenu>(&prop);
}

void InterfaceMenuPopup::refreshColors()
{
    display.setControlColors(boxMenu, display.getTheme().miniBoxMenuColors);
}

void InterfaceMenuPopup::showModulePopup(ModuleBase* sourceModule,
                                         void (*switchModuleCallback)(uint8_t))
{
    constexpr int boxesCount = kMenuRowsCount * kMenuColumnsCount;

    InterfaceMenuPopup::switchModuleCallback = switchModuleCallback;

    const uint8_t perfPages = (getProjectMaxTracks() > kSampleTracks) ? 2 : 1;
    performModes.count = perfPages;
    performModes.modes.count = perfPages;

    for (int box = 0; box < boxesCount; box++)
    {
        if (box >= kMenuModulesCount)
        {
            menuFunctions[box] = 0;
            modulesMenuTexts[box] = nullptr;
            modulesMenuModes[box] = nullptr;
            continue;
        }

        if (box < 8)
        {
            menuFunctions[box] = mtConfig.interface.functionKeysBinding[box];
            if (menuFunctions[box] >= kMenuModulesCount)
            {
                menuFunctions[box] = 0;
            }
        }
        else
        {
            bool assigned;
            for (uint8_t moduleIdx = 0; moduleIdx < kMenuModulesCount; moduleIdx++)
            {
                assigned = false;
                for (uint8_t assignedIdx = 0; assignedIdx < box; assignedIdx++)
                {
                    if (menuFunctions[assignedIdx] == moduleIdx)
                    {
                        assigned = true;
                    }
                }

                if (!assigned)
                {
                    menuFunctions[box] = moduleIdx;
                    break;
                }
            }
        }

        modulesMenuTexts[box] = kModuleNamesList[menuFunctions[box]];
        if (modules[menuFunctions[box]].modes != nullptr)
        {
            modules[menuFunctions[box]].modes->modes.current =
              modules[menuFunctions[box]].module->getCurrentModeIdx();
            modulesMenuModes[box] = (modules[menuFunctions[box]].modes->modes.count > 1)
                                      ? &modules[menuFunctions[box]].modes->modes
                                      : nullptr;
        }
        else
        {
            modulesMenuModes[box] = nullptr;
        }
    }

    selectedBox = 0;

    for (int moduleIdx = 0; moduleIdx < kMenuModulesCount; moduleIdx++)
    {
        if (modules[moduleIdx].module == sourceModule)
        {
            for (uint8_t box = 0; box < boxesCount; box++)
            {
                if (menuFunctions[box] == moduleIdx)
                {
                    selectedModuleOnShow = moduleIdx;
                    selectedBox = box;
                    break;
                }
            }
        }
    }

    this->sourceModule = sourceModule;
    isVisible = true;

    setModulePopupFunct();

    sourceModule->hideControlsUnderMiniMenu();

    display.setControlValue(boxMenu, selectedBox);
    display.setControlShow(boxMenu);
    display.refreshControl(boxMenu);

    display.synchronizeRefresh();
}

void InterfaceMenuPopup::hideModulePopup(ModuleBase* modulePtr)
{
    display.setControlHide(boxMenu);

    mtInterface.ip->clearButton(interfaceButtonLeft);
    mtInterface.ip->clearButton(interfaceButtonRight);
    mtInterface.ip->clearButton(interfaceButtonUp);
    mtInterface.ip->clearButton(interfaceButtonDown);
    mtInterface.ip->clearButton(interfaceButtonMiniMiddle);

    mtInterface.ip->clearAllPots();

    display.refreshControl(boxMenu);

    display.synchronizeRefresh();

    isVisible = false;

    uint8_t moduleToSwitch = modules[menuFunctions[selectedBox]].button;
    uint32_t startOptions = modules[menuFunctions[selectedBox]].defaultOption;

    if (modules[selectedModuleOnShow].button != modules[menuFunctions[selectedBox]].button)
    {
        if (switchModuleCallback != nullptr)
        {
            switchModuleCallback(moduleToSwitch);
            switchModuleCallback = nullptr;
        }
        else
        {
            modulePtr->eventFunct(eventSwitchModule, modulePtr, &moduleToSwitch, &startOptions);
        }
    }
    else
    {
        if (switchModuleCallback != nullptr)
        {
            switchModuleCallback(moduleToSwitch);
            switchModuleCallback = nullptr;
        }
        else
        {
            modulePtr->start(modules[menuFunctions[selectedBox]].defaultOption);
        }
    }
}

void InterfaceMenuPopup::setModulePopupFunct()
{
    mtInterface.ip->clearButtonsRange(interfaceButton0, interfaceButton32);

    mtInterface.ip->setPotObj(interfacePot0, functEncoderPopup);

    mtInterface.ip->setButtonObj(interfaceButtonLeft, buttonPress, functLeftPopup);
    mtInterface.ip->setButtonObj(interfaceButtonRight, buttonPress, functRightPopup);
    mtInterface.ip->setButtonObj(interfaceButtonUp, buttonPress, functUpPopup);
    mtInterface.ip->setButtonObj(interfaceButtonDown, buttonPress, functDownPopup);

    mtInterface.ip->setButtonObj(interfaceButtonMiniMiddle, buttonPress, functMiddlePopup);
}

void InterfaceMenuPopup::handleFailedSwitching()
{
    // source module handles functions assignment
    display.setControlHide(boxMenu);
    display.refreshControl(boxMenu);
    display.synchronizeRefresh();

    isVisible = false;
}

void InterfaceMenuPopup::changeModulePopupValue(int16_t value, uint8_t dir)
{
    if (dir > 0)
    {
        switch (dir)
        {
            case 1:
                selectedBox = selectedBox < kMenuColumnsCount
                                ? ((kMenuRowsCount - 1) * kMenuColumnsCount) + selectedBox
                                : selectedBox - kMenuColumnsCount;
                break;
            case 2:
                selectedBox = selectedBox % kMenuColumnsCount >= kMenuColumnsCount - 1
                                ? selectedBox - (kMenuColumnsCount - 1)
                                : selectedBox + 1;
                break;
            case 3:
                selectedBox = selectedBox >= (kMenuRowsCount - 1) * kMenuColumnsCount
                                ? selectedBox - ((kMenuRowsCount - 1) * kMenuColumnsCount)
                                : selectedBox + kMenuColumnsCount;
                break;
            case 4:
                selectedBox = selectedBox % kMenuColumnsCount == 0
                                ? selectedBox + (kMenuColumnsCount - 1)
                                : selectedBox - 1;
                break;
            default: selectedBox = 0; break;
        }
        if (selectedBox >= kMenuModulesCount)
        {
            selectedBox = kMenuModulesCount - 1;
        }
    }
    else
    {
        if (selectedBox + value < 0)
        {
            selectedBox = 0;
        }
        else if (selectedBox + value > (kMenuModulesCount - 1))
        {
            selectedBox = kMenuModulesCount - 1;
        }
        else
        {
            selectedBox += value;
        }
    }
    display.setControlValue(boxMenu, selectedBox);
    display.refreshControl(boxMenu);
}

void InterfaceMenuPopup::setModulePopupValue(int16_t value)
{
    selectedBox = value;

    display.setControlValue(boxMenu, value);
    display.refreshControl(boxMenu);
}

void InterfaceMenuPopup::changeModuleMode()
{
    ModuleBase* destModule = sourceModule;
    uint32_t options = 0;
    bool srcIsDst = modules[menuFunctions[selectedBox]].module == sourceModule;
    bool destHasModes = modules[menuFunctions[selectedBox]].modes != nullptr;

    if (srcIsDst)
    {
        if (destHasModes)
        {
            int optionsIdx = modules[menuFunctions[selectedBox]].modes->modes.current + 1;
            if (optionsIdx >= modules[menuFunctions[selectedBox]].modes->modes.count)
            {
                optionsIdx = 0;
            }
            options = modules[menuFunctions[selectedBox]].modes->options[optionsIdx];
        }
        else
        {
            if (switchModuleCallback != nullptr)
            {
                uint8_t moduleToSwitch = modules[menuFunctions[selectedBox]].button;
                switchModuleCallback(moduleToSwitch);
                switchModuleCallback = nullptr;
            }
            else
            {
                sourceModule->start(modules[menuFunctions[selectedBox]].defaultOption);
            }
        }

        if (switchModuleCallback != nullptr)
        {
            uint8_t moduleToSwitch = modules[menuFunctions[selectedBox]].button;
            switchModuleCallback(moduleToSwitch);
            switchModuleCallback = nullptr;
        }
        else
        {
            sourceModule->start(options);
        }
    }
    else
    {
        if (destHasModes)
        {
            int optionsIdx = modules[menuFunctions[selectedBox]].modes->modes.current;
            options = modules[menuFunctions[selectedBox]].modes->options[optionsIdx];
        }
        else
        {
            options = modules[menuFunctions[selectedBox]].defaultOption;
        }
        destModule = modules[menuFunctions[selectedBox]].module;
        uint8_t moduleToSwitchButton = modules[menuFunctions[selectedBox]].button;

        if (switchModuleCallback != nullptr)
        {
            switchModuleCallback(moduleToSwitchButton);
            switchModuleCallback = nullptr;
        }
        else
        {
            sourceModule->eventFunct(
              eventSwitchModule, sourceModule, &moduleToSwitchButton, &options);
        }
    }
    if (sourceModule->wasLastSwitchingSuccessfull())
    {
        showModulePopup(destModule);
    }
    else
    {
        handleFailedSwitching();
    }
}

//******************************************************************************************************
static void functLeftPopup()
{
    mtMenuPopup.changeModulePopupValue(0, 4);
}

static void functRightPopup()
{
    mtMenuPopup.changeModulePopupValue(0, 2);
}

static void functUpPopup()
{
    mtMenuPopup.changeModulePopupValue(0, 1);
}

static void functDownPopup()
{
    mtMenuPopup.changeModulePopupValue(0, 3);
}

static void functEncoderPopup(int16_t value)
{
    mtMenuPopup.changeModulePopupValue(value);
}

static void functMiddlePopup()
{
    mtMenuPopup.changeModuleMode();
}

#endif
