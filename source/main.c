#include "affine_background.h"
#include "blind.h"
#include "card.h"
#include "font.h"
#include "game.h"
#include "gbalatro_sys8.h"
#include "graphic_utils.h"
#include "joker.h"
#include "sprite.h"

#include <maxmod9.h>
#include <nds.h>
#include <string.h>

// Graphics
#include "affine_background_gfx.h"
#include "background_gfx.h"
#include "gbalatro_sys8.h"

// Audio
#include "soundbank.h"
#include "soundbank_bin.h"

PrintConsole topScreen, bottomScreen;
const size_t size_char_4bpp = (8 * 8) / 2; // 4bpp = 2 pixels per byte
ConsoleFont font = {
    .gfx = gbalatro_sys8Tiles,
    .pal = gbalatro_sys8Pal,
    .numColors = gbalatro_sys8PalLen / 2,
    .bpp = 4,
    .asciiOffset = 32,
    .numChars = gbalatro_sys8TilesLen / size_char_4bpp
};

int bg_0, bg_1, bg_2;

void init()
{
    powerOn(POWER_ALL_2D);

    videoSetMode(MODE_2_2D);
    videoSetModeSub(MODE_2_2D);

    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankB(VRAM_B_MAIN_SPRITE);
    vramSetBankC(VRAM_C_SUB_BG);
    vramSetBankD(VRAM_D_SUB_SPRITE);

    oamInit(&oamMain, SpriteMapping_1D_128, false);
    oamInit(&oamSub, SpriteMapping_1D_128, false);

    oamClear(&oamMain, 0, MAX_SPRITES);
    oamClear(&oamSub, 0, MAX_SPRITES);

    bg_0 = bgInit(0, BgType_Text4bpp, BgSize_T_256x256, 0, 1);
    bg_1 = bgInit(1, BgType_Text8bpp, BgSize_T_512x512, 2, 3);
    bg_2 = bgInit(2, BgType_Rotation, BgSize_R_128x128, 1, 7);

    consoleInit(
        &topScreen,
        0,                // Background layer
        BgType_Text4bpp,  // 4 BPP mode
        BgSize_T_256x256, // Size of the background layer
        0,                // Map base
        1,                // Tile base,
        true,             // Main screen
        false             // Don't load graphics
    );
    consoleSetFont(&topScreen, &font);

    consoleInit(&bottomScreen, 0, BgType_Text4bpp, BgSize_T_256x256, 0, 1, false, true);

    BG_PALETTE[(TTE_YELLOW_PB * 16) + 1] = TEXT_CLR_YELLOW;
    BG_PALETTE[(TTE_BLUE_PB * 16) + 1] = TEXT_CLR_BLUE;
    BG_PALETTE[(TTE_RED_PB * 16) + 1] = TEXT_CLR_RED;
    BG_PALETTE[(TTE_WHITE_PB * 16) + 1] = TEXT_CLR_WHITE;

    windowSetBounds(WINDOW_0, 72, 44, 200, 128);
    windowSetBounds(WINDOW_1, 72, 0, 232, 44);

    int eva = 8, evb = 8, evy = 8;

    REG_BLDCNT = BLEND_ALPHA | BLEND_SRC_BG1 | BLEND_DST_BG2;
    REG_BLDALPHA = BLDALPHA_EVA(8) | BLDALPHA_EVB(8);
    REG_BLDY = evy;

    // Enable blending in both WINDOW_0 and WINDOW_1
    REG_WININ =
        (0x0F | (1 << 5)) | ((0x0F | (1 << 5)) << 8); // BG0-3 + blend for both win0 and win1
    REG_WINOUT = 0x0F;                                // BG0-3 outside windows (no blending)

    bgWindowEnable(bg_0, WINDOW_OUT | WINDOW_0 | WINDOW_1);
    bgWindowEnable(bg_1, WINDOW_OUT | WINDOW_0 | WINDOW_1);
    bgWindowEnable(bg_2, WINDOW_OUT | WINDOW_0 | WINDOW_1);
    oamWindowEnable(&oamMain, WINDOW_OUT | WINDOW_0 | WINDOW_1);
    windowEnable(WINDOW_OUT | WINDOW_0 | WINDOW_1);

    // Initialize subsystems
    mmInitDefault((mm_addr)soundbank_bin);
    affine_background_init();
    sprite_init(&oamMain);
    card_init();
    blind_init();
    joker_init();
    game_init();
    game_change_state(GAME_STATE_SPLASH_SCREEN);
}

void update()
{
    affine_background_update();
    game_update();
}

void draw()
{
    sprite_draw(&oamMain);
    oamUpdate(&oamMain);
    oamUpdate(&oamSub);
}

int main()
{
    init();

    while (true)
    {
        scanKeys();

        update();
        draw();

        swiWaitForVBlank();
    }

    return 0;
}
