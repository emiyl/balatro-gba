#include "game/main_menu.h"

#include "affine_background.h"
#include "audio_utils.h"
#include "background_main_menu_gfx.h"
#include "button.h"
#include "card.h"
#include "game.h"
#include "game/common_ui.h"
#include "game/palette.h"
#include "graphic_utils.h"
#include "soundbank.h"
#include "sprite.h"
#include "util.h"

#include <nds.h>
#include <stdint.h>

#define MAIN_MENU_BUTTONS             2
#define MAIN_MENU_IMPLEMENTED_BUTTONS 1 // Remove this once all buttons are implemented
#define MAIN_MENU_PLAY_BTN_IDX        0

#define HIGHLIGHT_COLOR 0xFFFF

#define BUTTON_SFX_VOLUME 154 // 60% of MM_FULL_VOLUME

#define MENU_POP_OUT_ANIM_FRAMES 20

// Pixel sizes
#define MAIN_MENU_ACE_T_X 88
#define MAIN_MENU_ACE_T_Y 26

// Main menu sprite - the ace of spades
static CardObject* main_menu_ace = NULL;

// Current selected button index
static int selection_x = 0;

void game_main_menu_on_init(void* _)
{
    affine_background_change_background(AFFINE_BG_MAIN_MENU);
    change_background(BG_MAIN_MENU);
    main_menu_ace = card_object_new(card_new(SPADES, ACE));
    card_object_set_sprite(main_menu_ace, 0); // Set the sprite for the ace of spades
    main_menu_ace->sprite_object->sprite->isDoubleSize = true;
    main_menu_ace->sprite_object->tx = int2fx(MAIN_MENU_ACE_T_X);
    main_menu_ace->sprite_object->x = main_menu_ace->sprite_object->tx;
    main_menu_ace->sprite_object->ty = int2fx(MAIN_MENU_ACE_T_Y);
    main_menu_ace->sprite_object->y = main_menu_ace->sprite_object->ty;
    main_menu_ace->sprite_object->tscale = float2fx(0.8f);
    selection_x = 0;
}

extern int bg_1;
void game_main_menu_change_background(void)
{
    toggle_windows(false, false);

    tte_erase_screen();

    dmaCopy(background_main_menu_gfxPal, BG_PALETTE, background_main_menu_gfxPalLen);
    dmaCopy(background_main_menu_gfxTiles, bgGetGfxPtr(bg_1), background_main_menu_gfxTilesLen);
    dmaCopy(background_main_menu_gfxMap, bgGetMapPtr(bg_1), background_main_menu_gfxMapLen);

    // Disable the button highlight colors
    memcpy16(
        &BG_PALETTE[MAIN_MENU_PLAY_BUTTON_OUTLINE_PID],
        &BG_PALETTE[MAIN_MENU_PLAY_BUTTON_MAIN_COLOR_PID],
        1
    );
}

void game_main_menu_on_update(void* ctx)
{
    MainMenuProps* props = (MainMenuProps*)ctx;

    change_background(BG_MAIN_MENU);

    card_object_update(main_menu_ace);
    main_menu_ace->sprite_object->trotation = sinLerp((props->timer << 8) / 2) / 3;
    main_menu_ace->sprite_object->rotation = main_menu_ace->sprite_object->trotation;

    // Seed randomization
    props->rng_seed++;
    // If the keys have changed, make it more pseudo-random
    if (key_curr_state() != key_prev_state())
    {
        props->rng_seed *= 2;
    }

    if (key_hit(KEY_LEFT))
    {
        if (selection_x > 0)
        {
            selection_x--;
        }
    }
    else if (key_hit(KEY_RIGHT))
    {
        if (selection_x < MAIN_MENU_IMPLEMENTED_BUTTONS - 1)
        {
            selection_x++;
        }
    }

    if (selection_x == MAIN_MENU_PLAY_BTN_IDX)
    {
        memset16(&BG_PALETTE[MAIN_MENU_PLAY_BUTTON_OUTLINE_PID], BTN_HIGHLIGHT_COLOR, 1);

        if (key_hit(SELECT_CARD))
        {
            play_sfx(SFX_BUTTON, MM_BASE_PITCH_RATE, BUTTON_SFX_VOLUME);
            game_start();
        }
    }
    else
    {
        memcpy16(
            &BG_PALETTE[MAIN_MENU_PLAY_BUTTON_OUTLINE_PID],
            &BG_PALETTE[MAIN_MENU_PLAY_BUTTON_MAIN_COLOR_PID],
            1
        );
    }
}

void game_main_menu_cleanup(void)
{
    // Normally I would just cache these and hide/unhide but I didn't feel like dealing with
    // defining a layer for it
    card_destroy(&main_menu_ace->card);
    card_object_destroy(&main_menu_ace);
}
