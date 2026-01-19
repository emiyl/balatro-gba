#include "sprite.h"

#include "audio_utils.h"
#include "game.h"
#include "pool.h"
#include "soundbank.h"
#include "util.h"

#include <maxmod9.h>
#include <nds.h>
#include <stdlib.h>

static Sprite main_sprites[MAX_SPRITES];
static Sprite sub_sprites[MAX_SPRITES];

// Track which affine slots are in use (true = in use, false = free)
static bool main_affine_used[MAX_AFFINES];
static bool sub_affine_used[MAX_AFFINES];

// Sprite methods
Sprite* sprite_new(
    int index,
    OamState* oam,
    int x,
    int y,
    SpriteSize size,
    SpriteColorFormat color,
    int priority,
    bool affine,
    int palette,
    u16* gfx
)
{
    Sprite* sprites = (oam == &oamMain) ? main_sprites : sub_sprites;
    Sprite* s = &sprites[index];

    s->index = index;
    s->active = true;
    s->oam = oam;
    s->entry.x = x;
    s->entry.y = y;
    s->entry.priority = 0;
    s->entry.palette = palette;
    s->entry.colorMode = color;
    s->entry.isHidden = false;

    switch (size)
    {
        case SpriteSize_8x16:
        case SpriteSize_16x32:
        case SpriteSize_32x64:
            s->entry.shape = OBJSHAPE_TALL;
            break;
        case SpriteSize_16x8:
        case SpriteSize_32x16:
        case SpriteSize_64x32:
            s->entry.shape = OBJSHAPE_WIDE;
            break;
        default:
            s->entry.shape = OBJSHAPE_SQUARE;
            break;
    }

    s->entry.size = size & 0x3;
    s->size = size;
    s->gfx = gfx;
    s->affine_index = -1; // No affine by default
    s->rotation = 0;
    s->scale_x = 256; // 1.0x scale
    s->scale_y = 256;
    s->priority = priority;
    s->isDoubleSize = false;

    if (affine)
    {
        sprite_enable_affine(s, false);
    }

    return s;
}

void sprite_destroy(Sprite** sprite)
{
    if (*sprite == NULL)
        return;

    (*sprite)->active = false;
    sprite_hide(*sprite);

    SpriteEntry* entry = &(*sprite)->entry;
    oamSet(
        (*sprite)->oam,
        (*sprite)->index,
        entry->x,
        entry->y,
        entry->priority,
        entry->palette,
        (*sprite)->size,
        entry->colorMode,
        (*sprite)->gfx,
        (*sprite)->affine_index, // Use affine if >= 0
        (*sprite)->isDoubleSize,
        entry->isHidden,
        false,
        false,
        false // Not mosaic
    );

    // Free affine slot if assigned
    if ((*sprite)->affine_index >= 0)
    {
        bool* affine_used = ((*sprite)->oam == &oamMain) ? main_affine_used : sub_affine_used;
        affine_used[(*sprite)->affine_index] = false;
        (*sprite)->affine_index = -1;
    }

    *sprite = NULL;
}

void sprite_inactive(Sprite* sprite)
{
    if (sprite == NULL)
        return;

    sprite->active = false;
}

int sprite_get_layer(Sprite* sprite)
{
    if (!sprite)
        return UNDEFINED;
    return sprite->priority;
}

// clang-format off
static const u8 sprite_width_lut[3][4] = {
    // size: 0     1      2      3
    /* Square */ { 8,   16,    32,    64 },
    /* Wide   */ { 16,  32,    64,     0 },
    /* Tall   */ { 8,    8,    16,    32 },
};

static const u8 sprite_height_lut[3][4] = {
    /* Square */ { 8,   16,    32,    64 },
    /* Wide   */ { 8,    8,    16,    32 },
    /* Tall   */ { 16,  32,    64,     0 },
};
// clang-format on

bool sprite_get_width(Sprite* sprite, int* width)
{
    if (!sprite || width == NULL)
    {
        return false;
    }

    *width = sprite_width_lut[sprite->entry.shape][sprite->entry.size];
    return true;
}

bool sprite_get_height(Sprite* sprite, int* height)
{
    if (!sprite || height == NULL)
    {
        return false;
    }

    *height = sprite_height_lut[sprite->entry.shape][sprite->entry.size];
    return true;
}

bool sprite_get_dimensions(Sprite* sprite, int* width, int* height)
{
    if (!sprite || width == NULL || height == NULL)
    {
        return false;
    }

    *width = sprite_width_lut[sprite->entry.shape][sprite->entry.size];
    *height = sprite_height_lut[sprite->entry.shape][sprite->entry.size];
    return true;
}

// Sprite functions
void sprite_init(OamState* oam)
{
    for (int i = 0; i < MAX_SPRITES; i++)
    {
        Sprite* sprites = (oam == &oamMain) ? main_sprites : sub_sprites;
        sprites[i].active = false;
        sprites[i].index = i;
    }

    // Initialize affine slot tracking
    bool* affine_used = (oam == &oamMain) ? main_affine_used : sub_affine_used;
    for (int i = 0; i < MAX_AFFINES; i++)
    {
        affine_used[i] = false;
    }
}

void sort_sprites_by_priority(OamState* oam)
{
    Sprite* sprites = (oam == &oamMain) ? main_sprites : sub_sprites;

    // Simple bubble sort based on priority (lower value = lower priority)
    for (int i = 0; i < MAX_SPRITES - 1; i++)
    {
        for (int j = 0; j < MAX_SPRITES - i - 1; j++)
        {
            if (sprites[j].active && sprites[j + 1].active &&
                sprites[j].priority < sprites[j + 1].priority)
            {
                // Swap sprites
                Sprite temp = sprites[j];
                sprites[j] = sprites[j + 1];
                sprites[j + 1] = temp;
            }
        }
    }
}

void set_sprite_priority(Sprite* sprite, int priority)
{
    if (sprite == NULL)
        return;
    sprite->priority = priority;
}

void sprite_draw(OamState* oam)
{
    Sprite* sprites = (oam == &oamMain) ? main_sprites : sub_sprites;
    sort_sprites_by_priority(oam);
    int lowest_priority = 0;
    for (int i = 0; i < MAX_SPRITES; i++)
    {
        Sprite* s = &sprites[i];

        // Skip inactive sprites
        if (!s->active)
            continue;

        SpriteEntry* entry = &s->entry;

        // Update affine matrix if used
        if (s->affine_index >= 0)
        {
            oamRotateScale(oam, s->affine_index, s->rotation, s->scale_x, s->scale_y);
        }

        oamSet(
            oam,
            s->index,
            entry->x,
            entry->y,
            entry->priority,
            entry->palette,
            s->size,
            entry->colorMode,
            s->gfx,
            s->affine_index, // Use affine if >= 0
            s->isDoubleSize,
            entry->isHidden,
            false,
            false,
            false // Not mosaic
        );
    }
}

int sprite_get_pb(const Sprite* sprite)
{
    if (sprite == NULL)
    {
        return UNDEFINED;
    }
    return sprite->entry.palette;
}

// SpriteObject methods
SpriteObject* sprite_object_new()
{
    SpriteObject* sprite_object = malloc(sizeof(SpriteObject));
    sprite_object->sprite = NULL;
    sprite_object_reset_transform(sprite_object);
    sprite_object->focused = false;

    return sprite_object;
}

void sprite_object_destroy(SpriteObject** sprite_object)
{
    if (*sprite_object == NULL)
        return;
    sprite_destroy(&(*sprite_object)->sprite);
    free(*sprite_object);
    *sprite_object = NULL;
}

void sprite_object_set_sprite(SpriteObject* sprite_object, Sprite* sprite)
{
    if (sprite_object == NULL)
        return;
    sprite_destroy(&sprite_object->sprite); // Destroy the old sprite if it exists
    sprite_object->sprite = sprite;
}

// Affine transformation helpers
void sprite_set_rotation(Sprite* sprite, s16 angle)
{
    if (sprite == NULL)
        return;
    sprite->rotation = angle;
}

void sprite_set_scale(Sprite* sprite, s16 scale_x, s16 scale_y)
{
    if (sprite == NULL)
        return;
    sprite->scale_x = scale_x;
    sprite->scale_y = scale_y;
}

void sprite_set_rotscale(Sprite* sprite, s16 scale_x, s16 scale_y, s16 angle)
{
    if (sprite == NULL)
        return;
    sprite_set_scale(sprite, scale_x, scale_y);
    sprite_set_rotation(sprite, angle);
}

void sprite_enable_affine(Sprite* sprite, bool double_size)
{
    if (sprite == NULL)
        return;

    // Allocate affine slot if not already assigned
    if (sprite->affine_index < 0)
    {
        bool* affine_used = (sprite->oam == &oamMain) ? main_affine_used : sub_affine_used;

        // Find first free affine slot
        for (int i = 0; i < MAX_AFFINES; i++)
        {
            if (!affine_used[i])
            {
                sprite->affine_index = i;
                affine_used[i] = true;
                break;
            }
        }

        // If no slot available, affine_index stays -1 (no affine)
        if (sprite->affine_index < 0)
        {
            // Could log warning: no affine slots available
            return;
        }
    }

    // Set double size attribute if requested
    if (double_size)
    {
        sprite->isDoubleSize = true;
    }
}

void sprite_disable_affine(Sprite* sprite)
{
    if (sprite == NULL)
        return;

    // Free affine slot if assigned
    if (sprite->affine_index >= 0)
    {
        bool* affine_used = (sprite->oam == &oamMain) ? main_affine_used : sub_affine_used;
        affine_used[sprite->affine_index] = false;
    }

    sprite->affine_index = -1;
    sprite->rotation = 0;
    sprite->scale_x = 256;
    sprite->scale_y = 256;
}

void sprite_object_reset_transform(SpriteObject* sprite_object)
{
    sprite_object->tx = 0; // Target position
    sprite_object->ty = 0;
    sprite_object->x = 0;
    sprite_object->y = 0;
    sprite_object->vx = 0;
    sprite_object->vy = 0;
    sprite_object->tscale = FIX_ONE; // Target scale
    sprite_object->scale = FIX_ONE;
    sprite_object->vscale = 0;
    sprite_object->trotation = 0; // Target rotation
    sprite_object->rotation = 0;
    sprite_object->vrotation = 0;
}

void sprite_object_update(SpriteObject* sprite_object)
{
    sprite_object->vx += ((sprite_object->tx - sprite_object->x) * GAME_SPEED) / 8;
    sprite_object->vy += ((sprite_object->ty - sprite_object->y) * GAME_SPEED) / 8;

    // Scale up the card when it's played
    sprite_object->vscale += (sprite_object->tscale - sprite_object->scale) / 8;

    // Rotate the card when it's played
    sprite_object->vrotation += (sprite_object->trotation - sprite_object->rotation) / 8;

    // set velocity to 0 if it's close enough to the target
    const FIXED epsilon = float2fx(0.01f);
    if (sprite_object->vx < epsilon && sprite_object->vx > -epsilon &&
        sprite_object->vy < epsilon && sprite_object->vy > -epsilon)
    {
        sprite_object->vx = 0;
        sprite_object->vy = 0;

        sprite_object->x = sprite_object->tx;
        sprite_object->y = sprite_object->ty;
    }
    else
    {
        sprite_object->vx = (sprite_object->vx * 7) / 10;
        sprite_object->vy = (sprite_object->vy * 7) / 10;

        sprite_object->x += sprite_object->vx;
        sprite_object->y += sprite_object->vy;
    }

    // Set scale to 0 if it's close enough to the target
    if (sprite_object->vscale < epsilon && sprite_object->vscale > -epsilon)
    {
        sprite_object->vscale = 0;
        sprite_object->scale = sprite_object->tscale; // Set the scale to the target scale
    }
    else
    {
        sprite_object->vscale = (sprite_object->vscale * 7) / 10;
        sprite_object->scale += sprite_object->vscale;
    }

    // Set rotation to 0 if it's close enough to the target
    if (sprite_object->vrotation < epsilon && sprite_object->vrotation > -epsilon)
    {
        sprite_object->vrotation = 0;
        // Set the rotation to the target rotation
        sprite_object->rotation = sprite_object->trotation;
    }
    else
    {
        sprite_object->vrotation = (sprite_object->vrotation * 7) / 10;
        sprite_object->rotation += sprite_object->vrotation;
    }

    // Apply rotation and scale to the sprite

    sprite_set_rotscale(
        sprite_object->sprite,
        sprite_object->scale,
        sprite_object->scale,
        -sprite_object->vx + sprite_object->rotation
    );
    sprite_position(sprite_object->sprite, fx2int(sprite_object->x), fx2int(sprite_object->y));
}

void sprite_object_shake(SpriteObject* sprite_object, mm_word sound_id)
{
    sprite_object->vscale = float2fx(0.3f);
    sprite_object->vrotation = float2fx(8.0f); // Rotate the card when it's scored

    if (sound_id == UNDEFINED)
        return; // If no sound ID is provided, do nothing

    play_sfx(sound_id, MM_BASE_PITCH_RATE, SFX_DEFAULT_VOLUME);
}

Sprite* sprite_object_get_sprite(SpriteObject* sprite_object)
{
    if (sprite_object == NULL)
        return NULL;
    return sprite_object->sprite;
}

void sprite_object_set_focus(SpriteObject* sprite_object, bool focus)
{
    if (sprite_object->focused == focus)
    {
        return;
    }
    sprite_object->focused = focus;

    play_sfx(
        SFX_CARD_FOCUS,
        MM_BASE_PITCH_RATE + rand() % CARD_FOCUS_SFX_PITCH_OFFSET_RANGE,
        SFX_DEFAULT_VOLUME
    );
    sprite_object->ty = sprite_object->ty + int2fx((focus ? -1 : 1) * SPRITE_FOCUS_RAISE_PX);
}

bool sprite_object_get_width(SpriteObject* sprite_object, int* width)
{
    if (sprite_object == NULL)
    {
        return false;
    }

    return sprite_get_width(sprite_object->sprite, width);
}

bool sprite_object_get_height(SpriteObject* sprite_object, int* height)
{
    if (sprite_object == NULL)
    {
        return false;
    }

    return sprite_get_height(sprite_object->sprite, height);
}

bool sprite_object_get_dimensions(SpriteObject* sprite_object, int* width, int* height)
{
    if (sprite_object == NULL)
    {
        return false;
    }

    return sprite_get_dimensions(sprite_object->sprite, width, height);
}

bool sprite_object_is_focused(SpriteObject* sprite_object)
{
    return sprite_object->focused;
}

void sprite_position(Sprite* sprite, int x, int y)
{
    sprite->entry.x = x;
    sprite->entry.y = y;
}

void sprite_entry_hide(SpriteEntry* sprite)
{
    sprite->isHidden = true;
}

void sprite_hide(Sprite* sprite)
{
    sprite_entry_hide(&sprite->entry);
}

void sprite_entry_unhide(SpriteEntry* sprite)
{
    sprite->isHidden = false;
}

void sprite_unhide(Sprite* sprite)
{
    sprite_entry_unhide(&sprite->entry);
}
