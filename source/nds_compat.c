#include "nds_compat.h"

#include <nds.h>

void bg_rotscale_ex(BG_AFFINE* bgaff, const AFF_SRC_EX* asx)
{
    int sx = asx->sx, sy = asx->sy;
    int sina = sinLerp(asx->alpha), cosa = cosLerp(asx->alpha);

    FIXED pa, pb, pc, pd;
    pa = sx * cosa >> 12;
    pb = -sx * sina >> 12; // .8f
    pc = sy * sina >> 12;
    pd = sy * cosa >> 12; // .8f

    bgaff->pa = pa;
    bgaff->pb = pb;
    bgaff->pc = pc;
    bgaff->pd = pd;

    bgaff->dx = asx->tex_x - (pa * asx->scr_x + pb * asx->scr_y);
    bgaff->dy = asx->tex_y - (pc * asx->scr_x + pd * asx->scr_y);
}

int max(int a, int b)
{
    return (a > b) ? (a) : (b);
}

int min(int a, int b)
{
    return (a < b) ? (a) : (b);
}

int wrap(int x, int min, int max)
{
    return (x >= max) ? (x + min - max) : ((x < min) ? (x + max - min) : x);
}

FIXED int2fx(int d)
{
    return d << FIX_SHIFT;
}

int fx2int(FIXED fx)
{
    return fx / FIX_SCALE;
}

u32 fx2uint(FIXED fx)
{
    return fx >> FIX_SHIFT;
}

FIXED float2fx(float f)
{
    return (FIXED)(f * FIX_SCALEF);
}

void clr_grayscale(COLOR* dst, const COLOR* src, uint nclrs)
{
    u32 ii;
    u32 clr, gray, rr, gg, bb;

    for (ii = 0; ii < nclrs; ii++)
    {
        clr = *src++;

        // Do RGB conversion in .8 fixed point
        rr = ((clr) & 31) * 0x4C;       // 29.7%
        gg = ((clr >> 5) & 31) * 0x96;  // 58.6%
        bb = ((clr >> 10) & 31) * 0x1E; // 11.7%
        gray = (rr + gg + bb + 0x80) >> 8;

        *dst++ = RGB15(gray, gray, gray);
    }
}

void clr_rgbscale(COLOR* dst, const COLOR* src, uint nclrs, COLOR clr)
{
    int ii;
    u32 rr, gg, bb, scale, gray;

    // Normalize target color
    rr = (clr) & 31;
    gg = (clr >> 5) & 31;
    bb = (clr >> 10) & 31;

    scale = max(max(rr, gg), bb);
    if (scale == 0) // Can't scale to black : use gray instead
    {
        clr_grayscale(dst, src, nclrs);
        return;
    }

    scale = 1 / scale;
    rr *= scale; // rr, gg, bb -> 0.16f
    gg *= scale;
    bb *= scale;

    for (ii = 0; ii < nclrs; ii++)
    {
        clr = *src++;

        // Grayscaly in 5.8f
        gray = ((clr) & 31) * 0x4C;        // 29.7%
        gray += ((clr >> 5) & 31) * 0x96;  // 58.6%
        gray += ((clr >> 10) & 31) * 0x1E; // 11.7%

        // Match onto color vector
        dst[ii] = RGB15(rr * gray >> 24, gg * gray >> 24, bb * gray >> 24);
    }
}

void memcpy16(void* dest, const void* src, size_t size)
{
    for (size_t i = 0; i < size * 2; i += 2)
    {
        *((uint16_t*)((uintptr_t)dest + i)) = *((uint16_t*)((uintptr_t)src + i));
    }
}

void memcpy32(void* dest, const void* src, size_t size)
{
    for (size_t i = 0; i < size * 4; i += 4)
    {
        *((uint32_t*)((uintptr_t)dest + i)) = *((uint32_t*)((uintptr_t)src + i));
    }
}

void memset16(void* dest, uint16_t value, size_t size)
{
    for (size_t i = 0; i < size * 2; i += 2)
    {
        *((uint16_t*)((uintptr_t)dest + i)) = value;
    }
}

void memset32(void* dest, uint32_t value, size_t size)
{
    for (size_t i = 0; i < size * 4; i += 4)
    {
        *((uint32_t*)((uintptr_t)dest + i)) = value;
    }
}

void apply_control_block(const char* block, va_list* ap)
{
    const char* p = block;

    while (*p)
    {
        // Skip whitespace and separators
        while (*p == ' ' || *p == ';')
            p++;

        // Cursor position: P:%d,%d
        if (strncmp(p, "P:", 2) == 0)
        {
            p += 2;

            int x = va_arg(*ap, int);
            int y = va_arg(*ap, int);

            consoleSetCursor(&topScreen, x / 8, y / 8);

            // Skip until next separator
            while (*p && *p != ';')
                p++;
        }
        // Color: cx:0x%X000
        else if (strncmp(p, "cx:", 3) == 0)
        {
            p += 3;

            int color = va_arg(*ap, int);

            consoleSetColor(&topScreen, color);

            while (*p && *p != ';')
                p++;
        }
        else
        {
            // Unknown directive → skip safely
            while (*p && *p != ';')
                p++;
        }
    }

    consoleSelect(&topScreen);
}

void tte_printf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    const char* p = fmt;
    char text_buf[256];

    while (*p)
    {
        // Control block
        if (p[0] == '#' && p[1] == '{')
        {
            p += 2; // skip #{

            const char* block_start = p;
            while (*p && *p != '}')
                p++;

            if (*p == '}')
            {
                char block[128];
                size_t len = p - block_start;
                if (len >= sizeof(block))
                    len = sizeof(block) - 1;

                memcpy(block, block_start, len);
                block[len] = '\0';

                apply_control_block(block, &ap);
                p++; // skip }
            }
        }
        // Normal text
        else
        {
            const char* text_start = p;

            while (*p && !(p[0] == '#' && p[1] == '{'))
                p++;

            size_t len = p - text_start;
            if (len >= sizeof(text_buf))
                len = sizeof(text_buf) - 1;

            memcpy(text_buf, text_start, len);
            text_buf[len] = '\0';

            vprintf(text_buf, ap);
        }
    }

    va_end(ap);
}

void tte_set_pos(int x, int y)
{
    consoleSetCursor(&topScreen, x, y);
}

void tte_set_special(int special)
{
    special = special / 0x1000; // TTE_SPECIAL_PB_MULT_OFFSET
    consoleSetColor(&topScreen, special);
}

void tte_write(const char* str)
{
    consoleSelect(&topScreen);
    printf("%s", str);
}

void tte_erase_rect(int left, int top, int right, int bottom)
{
    consoleSelect(&topScreen);
    for (int row = top; row <= bottom; row++)
    {
        consoleSetCursor(&topScreen, left / 8, row / 8);
        for (int col = left; col <= right; col++)
        {
            printf(" ");
        }
    }
}

void tte_erase_screen(void)
{
    consoleSelect(&topScreen);
    for (int row = 0; row < 192 / 8; row++)
    {
        consoleSetCursor(&topScreen, 0, row);
        for (int col = 0; col < 256 / 8; col++)
        {
            printf(" ");
        }
    }
}

int bit_tribool(u32 flags, uint plus, uint minus)
{
    return ((flags >> plus) & 1) - ((flags >> minus) & 1);
}