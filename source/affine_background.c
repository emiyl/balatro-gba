#include "affine_background.h"

#include "affine_background_gfx.h"
#include "affine_main_menu_background_gfx.h"
#include "graphic_utils.h"
#include "nds_compat.h"

#define ANIMATION_SPEED_DIVISOR 16

// Prepare screen during VBLANK
// Pre-computes the affine matrices values for each scanline and stores in bgaff_arr. This is to be
// done in VBLANK so the HBLANK code can just fetch the values quickly.
static void s_affine_background_prep_bgaff_arr();

static BG_AFFINE _bgaff_arr[SCREEN_HEIGHT + 1];
static AFF_SRC_EX _asx = {0};
static enum AffineBackgroundID _background = AFFINE_BG_MAIN_MENU;
static uint _timer = 0;
static bool _hblank_enabled = false;

void set_affine_registers(BG_AFFINE bgaff)
{
    REG_BG2PA = bgaff.pa;
    REG_BG2PB = bgaff.pb;
    REG_BG2PC = bgaff.pc;
    REG_BG2PD = bgaff.pd;
    REG_BG2X = bgaff.dx;
    REG_BG2Y = bgaff.dy;
}

void affine_background_init()
{
    affine_background_update();

    set_affine_registers(bg_aff_default);
}

void affine_background_vblank()
{
    if (!_hblank_enabled)
        return;
    set_affine_registers(_bgaff_arr[0]);
}

void affine_background_hblank()
{
    vu16 vcount = REG_VCOUNT;

    if ((vcount >= SCREEN_HEIGHT)) // Exit the function if the scanline is outside the screen
    {
        return;
    }

    // See comment in affine_background_prep_bgaff_arr()
    BG_AFFINE bg = _bgaff_arr[vcount + 1];
    set_affine_registers(bg);
}

void affine_background_update()
{
    if (_hblank_enabled) // High quality mode with HBLANK interrupt
    {
        s_affine_background_prep_bgaff_arr();
    }
    else // Low quality mode without HBLANK interrupt
    {
        _asx.scr_x = 0;
        _asx.scr_y = 0;
        _asx.tex_x += 5;
        _asx.tex_y += 12;
        // Scale the sine value to fit in a s16
        _asx.sx = ((sinLerp(_timer * 100)) >> 8) + 256;
        // Scale the sine value to fit in a s16
        _asx.sy = ((sinLerp(_timer * 100 + 0x4000)) >> 8) + 256;
        _asx.alpha = 0;

        bg_rotscale_ex(&_bgaff_arr[0], &_asx);
        set_affine_registers(_bgaff_arr[0]);
    }

    _timer++;
}

void affine_background_set_color(COLOR color)
{
    // Reload the palette to reset any previous color scaling
    affine_background_change_background(_background);
    for (int i = 0; i < AFFINE_BG_PAL_LEN; i++)
    {
        clr_rgbscale(&BG_PALETTE[AFFINE_BG_PB] + i, &BG_PALETTE[AFFINE_BG_PB] + i, 1, color);
    }
}

void affine_background_load_palette(const u16* src)
{
    memcpy16(&BG_PALETTE[AFFINE_BG_PB], src, AFFINE_BG_PAL_LEN);
}

void affine_background_change_background(enum AffineBackgroundID new_bg)
{
    _background = new_bg;

    switch (_background)
    {
        case AFFINE_BG_MAIN_MENU:
            irqSet(IRQ_VBLANK, affine_background_vblank);
            irqEnable(IRQ_VBLANK);
            irqSet(IRQ_HBLANK, affine_background_hblank);
            irqEnable(IRQ_HBLANK);
            _hblank_enabled = true;

            memcpy32_tile8_with_palette_offset(
                (u32*)&tile8_mem[AFFINE_BG_CBB],
                (const u32*)affine_main_menu_background_gfxTiles,
                affine_main_menu_background_gfxTilesLen / 4,
                AFFINE_BG_PB
            );
            dmaCopy(
                affine_main_menu_background_gfxMap,
                &se_mem[AFFINE_BG_SBB],
                affine_main_menu_background_gfxMapLen
            );
            affine_background_load_palette(affine_main_menu_background_gfxPal);
            break;
        case AFFINE_BG_GAME:
            irqDisable(IRQ_HBLANK);
            irqClear(IRQ_HBLANK);
            _hblank_enabled = false;

            memcpy32_tile8_with_palette_offset(
                (u32*)&tile8_mem[AFFINE_BG_CBB],
                (const u32*)affine_background_gfxTiles,
                affine_background_gfxTilesLen / 4,
                AFFINE_BG_PB
            );
            dmaCopy(affine_background_gfxMap, &se_mem[AFFINE_BG_SBB], affine_background_gfxMapLen);
            affine_background_load_palette(affine_background_gfxPal);
            break;
    }
}

static void s_affine_background_prep_bgaff_arr()
{
    for (u16 vcount = 0; vcount < SCREEN_HEIGHT; vcount++)
    {
        const s32 timer_s32 = _timer << 8;
        const s32 vcount_s32 = vcount << 8;
        const s16 vcount_s16 = vcount;
        const s32 vcount_sine = sinLerp(vcount_s32 + timer_s32 / ANIMATION_SPEED_DIVISOR);

        _asx.scr_x = (SCREEN_WIDTH / 2);
        // scr_y must equal vcount otherwise the background will have no vertical difference
        _asx.scr_y = vcount_s16 - (SCREEN_HEIGHT / 2);
        _asx.tex_x = (1000 * 1000) + (vcount_sine);
        _asx.tex_y = (1000 * 1000);
        _asx.sx = 128;
        _asx.sy = 128;
        _asx.alpha = vcount_sine + (timer_s32 / ANIMATION_SPEED_DIVISOR);

        bg_rotscale_ex(&_bgaff_arr[vcount], &_asx);
    }

    /* HBLANK occurs after the scanline so REG_VCOUNT represents the
     * the scanline that just passed, so when it's SCREEN_HEIGHT we will
     * actually be updating the first line
     */
    _bgaff_arr[SCREEN_HEIGHT] = _bgaff_arr[0];
}
