#ifndef SPRITE_H
#define SPRITE_H

#include "nds_compat.h"

#include <maxmod9.h>
#include <nds.h>

#define CARD_SPRITE_SIZE                  32
#define MAX_AFFINES                       32
#define MAX_SPRITES                       128
#define MAX_SPRITE_OBJECTS                16
#define SPRITE_FOCUS_RAISE_PX             10
#define CARD_FOCUS_SFX_PITCH_OFFSET_RANGE 512

typedef struct
{
    SpriteEntry entry;
    OamState* oam;
    int index;
    u16* gfx;
    SpriteSize size;
    int affine_index;
    s16 rotation;
    s16 scale_x;
    s16 scale_y;
    bool isDoubleSize;
    bool active;
} Sprite;

// A sprite object is a sprite that is focusable and movable in animation
typedef struct
{
    Sprite* sprite;
    FIXED tx, ty; // target position
    FIXED x, y;   // position
    FIXED vx, vy; // velocity
    FIXED tscale;
    FIXED scale;
    FIXED vscale;
    FIXED trotation; // this never gets used so i might remove it later
    FIXED rotation;
    FIXED vrotation;
    bool focused;

} SpriteObject;

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
);
Sprite* affine_sprite_new(
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
);
void sprite_destroy(Sprite** sprite);
int sprite_get_layer(Sprite* sprite);
bool sprite_get_dimensions(Sprite* sprite, int* width, int* height);
bool sprite_get_height(Sprite* sprite, int* height);
bool sprite_get_width(Sprite* sprite, int* width);

// Sprite functions
void sprite_init(OamState* oam);
void sprite_draw(OamState* oam);
int sprite_get_pb(const Sprite* sprite);

// SpriteObject methods
SpriteObject* sprite_object_new();
void sprite_object_destroy(SpriteObject** sprite_object);
void sprite_object_set_sprite(SpriteObject* sprite_object, Sprite* sprite);
void sprite_object_reset_transform(SpriteObject* sprite_object);
void sprite_object_update(SpriteObject* sprite_object);
void sprite_object_shake(SpriteObject* sprite_object, mm_word sound_id);

Sprite* sprite_object_get_sprite(SpriteObject* sprite_object);
void sprite_object_set_focus(SpriteObject* sprite_object, bool focus);
bool sprite_object_get_dimensions(SpriteObject* sprite_object, int* width, int* height);
bool sprite_object_get_height(SpriteObject* sprite_object, int* height);
bool sprite_object_get_width(SpriteObject* sprite_object, int* width);
bool sprite_object_is_focused(SpriteObject* sprite_object);

void sprite_set_rotation(Sprite* sprite, s16 angle);
void sprite_set_scale(Sprite* sprite, s16 scale_x, s16 scale_y);
void sprite_enable_affine(Sprite* sprite, bool double_size);
void sprite_disable_affine(Sprite* sprite);

void sprite_position(Sprite* sprite, int x, int y);

void sprite_entry_hide(SpriteEntry* sprite);

void sprite_hide(Sprite* sprite);

void sprite_entry_unhide(SpriteEntry* sprite);

void sprite_unhide(Sprite* sprite);

#endif // SPRITE_H
