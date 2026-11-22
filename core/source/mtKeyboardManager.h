#ifndef SOURCE_MTKEYBOARDMANAGER_H_
#define SOURCE_MTKEYBOARDMANAGER_H_

#include "display.h"
#include "keyboardControl.h"
#include "mtRandomNameGenerator.h"
#include "mtStructs.h"

enum valueMapDirecion
{
    valueMapDirectionLeft,
    valueMapDirectionRight,
    valueMapDirectionUp,
    valueMapDirectionDown
};

constexpr char smallKeyboard[KEYBOARD_SIZE] = { '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                                '0', 0,   'q', 'w', 'e', 'r', 't', 'y', 'u',
                                                'i', 'o', 'p', '-', '+', 'a', 's', 'd', 'f',
                                                'g', 'h', 'j', 'k', 'l', '@', 1,   'z', 'x',
                                                'c', 'v', 'b', 'n', 'm', ' ' };

constexpr char bigKeyboard[KEYBOARD_SIZE] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0,
                                              'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '-',
                                              '+', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '@',
                                              1,   'Z', 'X', 'C', 'V', 'B', 'N', 'M', ' ' };

constexpr uint8_t valueMap[4][42] = {
    { 10, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  22, 11, 12, 13, 14, 15, 16, 17, 18, 19,
      20, 21, 33, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 41, 34, 35, 36, 37, 38, 39, 40 },

    { 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 0,  12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
      22, 11, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 23, 35, 36, 37, 38, 39, 40, 41, 34 },

    {
      34, 35, 36, 37, 38, 39, 40, 41, 41, 41, 41, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
      10, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24, 25, 26, 27, 28, 29, 30,
    },

    { 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
      33, 33, 34, 35, 36, 37, 38, 39, 40, 41, 41, 41, 41, 0,  1,  2,  3,  4,  5,  6,  7 },
};

constexpr uint8_t valueMapPads[48] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 10,
                                       11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
                                       23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 33,
                                       34, 35, 36, 37, 38, 39, 40, 41, 41, 41, 41, 41 };

constexpr uint8_t keyPositionToPads[42] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 36, 37, 38, 39, 40, 41, 42, 43
};

constexpr uint8_t BACKSPACE_PAD_1 = 10;
constexpr uint8_t BACKSPACE_PAD_2 = 11;

constexpr uint8_t CAPS_LOCK_PAD_1 = 34;
constexpr uint8_t CAPS_LOCK_PAD_2 = 35;

constexpr uint8_t SPACE_PAD_1 = 43;
constexpr uint8_t SPACE_PAD_2 = 44;
constexpr uint8_t SPACE_PAD_3 = 45;
constexpr uint8_t SPACE_PAD_4 = 46;
constexpr uint8_t SPACE_PAD_5 = 47;

constexpr uint8_t F_PAD = 27;

constexpr uint8_t J_PAD = 30;

constexpr uint8_t MAX_NAME_LENGTH = 65;

class Leds;
class Timer;
class mtKeyboardManager
{
  public:
    void initControls();
    void refreshColors();
    void init(Leds* leds, Timer* timer);

    void onPadChange(uint8_t pad, uint8_t state);
    char* getName();
    void fillName(const char* txt);

    void activateKeyboard(int nameLengthLimit = -1);
    void deactivateKeyboard();

    void setRandomName();
    void confirmKey();
    void makeBackspace();
    void makeMove(char c); // steruje się - w s a d
    void blockMove();
    uint8_t getState();
    void setNameLengthLimit(uint8_t limit);

  private:
    void showKeyboard();
    void hideKeyboard();
    void showKeyboardEditName();
    void hideEditName();
    void editSelectedText();

    hControl keyboardControl = -1;
    hControl editName = -1;

    strEditData editNameData;

    uint8_t keyboardActiveFlag;
    uint8_t lastPressedPad;
    uint8_t keyboardShiftFlag;
    uint8_t keyboardPosition;
    uint8_t editPosition;

    uint8_t nameLengthLimit;

    char name[MAX_NAME_LENGTH];
    Leds* leds;
    Timer* timer;
};

#endif /* SOURCE_MTKEYBOARDMANAGER_H_ */
