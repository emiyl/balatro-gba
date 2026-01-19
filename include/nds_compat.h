#ifndef NDS_COMPAT_H
#define NDS_COMPAT_H

#include <nds.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define MAP_INDEX(x, y) ((x) + (y) * 32)

typedef struct
{
    int x;
    int y;
} POINT;

typedef POINT BG_POINT;

typedef struct
{
    int left;
    int top;
    int right;
    int bottom;
} RECT;

typedef s32 FIXED;
typedef u16 COLOR;
typedef u16 SE;

#define INLINE static inline
#define ALIGN4 __attribute__((aligned(4)))

typedef struct OBJ_ATTR
{
    u16 attr0;
    u16 attr1;
    u16 attr2;
    s16 fill;
} ALIGN4 OBJ_ATTR;

typedef struct OBJ_AFFINE
{
    u16 fill0[3];
    s16 pa;
    u16 fill1[3];
    s16 pb;
    u16 fill2[3];
    s16 pc;
    u16 fill3[3];
    s16 pd;
} ALIGN4 OBJ_AFFINE;

typedef struct AFF_SRC_EX
{
    s32 tex_x; //!< Texture-space anchor, x coordinate	(.8f)
    s32 tex_y; //!< Texture-space anchor, y coordinate	(.8f)
    s16 scr_x; //!< Screen-space anchor, x coordinate	(.0f)
    s16 scr_y; //!< Screen-space anchor, y coordinate	(.0f)
    s16 sx;    //!< Horizontal zoom	(8.8f)
    s16 sy;    //!< Vertical zoom		(8.8f)
    u16 alpha; //!< Counter-clockwise angle ( range [0, 0xFFFF] )
} ALIGN4 AFF_SRC_EX, BgAffineSource;

typedef struct AFF_DST_EX
{
    s16 pa, pb;
    s16 pc, pd;
    s32 dx, dy;
} ALIGN4 AFF_DST_EX, BgAffineDest;

typedef struct AFF_DST_EX BG_AFFINE;

#define FIX_SHIFT  8
#define FIX_SCALE  (1 << FIX_SHIFT)
#define FIX_SCALEF ((float)FIX_SCALE)
#define FIX_ONE    FIX_SCALE

static const BG_AFFINE bg_aff_default = {256, 0, 0, 256, 0, 0};

void bg_rotscale_ex(BG_AFFINE* bgaff, const AFF_SRC_EX* asx);

int max(int a, int b);
int min(int a, int b);
int wrap(int x, int min, int max);

FIXED int2fx(int d);

int fx2int(FIXED fx);

u32 fx2uint(FIXED fx);

FIXED float2fx(float f);

void clr_grayscale(COLOR* dst, const COLOR* src, uint nclrs);

void clr_rgbscale(COLOR* dst, const COLOR* src, uint nclrs, COLOR clr);

void memcpy16(void* dest, const void* src, size_t size);

void memcpy32(void* dest, const void* src, size_t size);

void memset16(void* dest, uint16_t value, size_t size);

void memset32(void* dest, uint32_t value, size_t size);

extern PrintConsole topScreen;

void apply_control_block(const char* block, va_list* ap);

void tte_printf(const char* fmt, ...);

void tte_set_pos(int x, int y);

void tte_set_special(int special);

void tte_write(const char* str);

void tte_erase_rect(int left, int top, int right, int bottom);

void tte_erase_screen(void);

int bit_tribool(u32 flags, uint plus, uint minus);

// NDS compatibility macros for GBA key functions
#define key_hit(keys)      (keysDown() & (keys))
#define key_is_down(keys)  (keysHeld() & (keys))
#define key_released(keys) (keysUp() & (keys))
#define key_transit(keys)  (keysDown() & (keys))
#define key_curr_state()   keysHeld()
#define key_prev_state()   keysDownRepeat()

#define KI_DOWN  KEY_DOWN
#define KI_LEFT  KEY_LEFT
#define KI_RIGHT KEY_RIGHT
#define KI_UP    KEY_UP

#define KEY_ANY                                                                                  \
    (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_A | KEY_B | KEY_SELECT | KEY_START | KEY_L | \
     KEY_R)
#define KEY_DIR (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)

// Memory and register compatibility
// Note: Charblocks are 16KB (0x4000 bytes), so tile_mem[n] accesses 4096 u32s per block
#define tile_mem    ((u32(*)[0x1000])BG_TILE_RAM(0)) // 32-bit access, 4096 u32s per charblock
#define pal_bg_mem  BG_PALETTE_SUB
#define pal_bg_bank ((u16(*)[16])BG_PALETTE_SUB)
#define tile8_mem   ((u8(*)[0x4000])BG_TILE_RAM(0)) // Array of 16KB charblocks
#define se_mem      ((u16(*)[1024])BG_MAP_RAM(0))   // Array of 2KB screenblocks
#define se_mat      ((u16(*)[32][32])BG_MAP_RAM(0))

#endif // NDS_COMPAT_H