#include "splash_screen.h"

#include "font.h"
#include "game.h"
#include "game/rect.h"
#include "graphic_utils.h"
#include "maxmod9.h"
#include "soundbank.h"

#include <nds.h>

static const Rect COUNTDOWN_TIMER_RECT = {224, 168, 256, 176};
static uint timer = 0;

extern PrintConsole topScreen;
void splash_screen_on_init(void* _)
{
    timer = 0;

    BG_PALETTE[0] = 0;
    consoleSelect(&topScreen);
    consoleSetCursor(&topScreen, 80 / 8, 8 / 8);
    consoleSetColor(&topScreen, TTE_WHITE_PB);
    printf("DISCLAIMER");
    consoleSetCursor(&topScreen, 8 / 8, 24 / 8);
    printf(
        "This project is NOT endorsed \n by or affiliated with \n Playstack or "
        "LocalThunk.\n\n If you have paid for this, \n you have been scammed and \n should request "
        "a refund \n IMMEDIATELY. \n\n The only official place to \n obtain this is from: \n\n "
        "'github.com/\n  GBALATRO/balatro-gba'"
    );
    consoleSetCursor(&topScreen, 8 / 8, 168 / 8);
    printf("(Press any key to skip)");
}

void splash_screen_on_update(void* _)
{
    timer++;

    if (timer < SPLASH_DURATION_FRAMES)
    {
        tte_erase_rect_wrapper(COUNTDOWN_TIMER_RECT);
        consoleSetCursor(&topScreen, COUNTDOWN_TIMER_RECT.left / 8, COUNTDOWN_TIMER_RECT.top / 8);
        printf("%d", 1 + (SPLASH_DURATION_FRAMES - timer) / SPLASH_FPS);

        if (!key_hit(KEY_ANY))
        {
            return;
        }
    }

    game_change_state(GAME_STATE_MAIN_MENU);
    tte_erase_screen();
}

void splash_screen_on_exit(void* _)
{
    // mmStart(MOD_MAIN_THEME, MM_PLAY_LOOP);
}
