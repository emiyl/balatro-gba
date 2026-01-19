#include "card.h"

#include "deck_gfx.h"
#include "graphic_utils.h"

#include <maxmod9.h>
#include <stdlib.h>

// Audio
#include "pool.h"
#include "soundbank.h"

// Card sprites lookup table. First index is the suit, second index is the rank. The value is the
// tile index.
const static u16 _card_sprite_lut[NUM_SUITS][NUM_RANKS] = {
    {0,   16,  32,  48,  64,  80,  96,  112, 128, 144, 160, 176, 192},
    {208, 224, 240, 256, 272, 288, 304, 320, 336, 352, 368, 384, 400},
    {416, 432, 448, 464, 480, 496, 512, 528, 544, 560, 576, 592, 608},
    {624, 640, 656, 672, 688, 704, 720, 736, 752, 768, 784, 800, 816}
};

u16* gfx_cards[52];

void card_init()
{
    // Copy shared palette once for all cards
    dmaCopy(deck_gfxPal, &SPRITE_PALETTE[CARD_PB * 16], deck_gfxPalLen);

    // Allocate separate graphics buffers for each card on screen
    for (int i = 0; i < 52; i++)
    {
        gfx_cards[i] = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_16Color);
        dmaCopy(
            &deck_gfxTiles[i * TILE_SIZE * TILE_SIZE * sizeof(u16)],
            gfx_cards[i],
            deck_gfxTilesLen / 52
        );
    }
}

// Card methods
Card* card_new(u8 suit, u8 rank)
{
    Card* card = POOL_GET(Card);

    card->suit = suit;
    card->rank = rank;

    return card;
}

void card_destroy(Card** card)
{
    POOL_FREE(Card, *card);
    *card = NULL;
}

u8 card_get_value(Card* card)
{
    if (card->rank == JACK || card->rank == QUEEN || card->rank == KING)
    {
        return 10; // Face cards are worth 10
    }
    else if (card->rank == ACE)
    {
        return 11; // Ace is worth 11
    }
    else
    {
        return card->rank + RANK_OFFSET; // 2-10 are worth their rank + RANK_OFFSET
    }

    return 0; // Should never reach here, but just in case
}

// CardObject methods
CardObject* card_object_new(Card* card)
{
    CardObject* card_object = POOL_GET(CardObject);

    card_object->card = card;
    card_object->sprite_object = sprite_object_new();
    card_object->selected = false;

    return card_object;
}

void card_object_destroy(CardObject** card_object)
{
    if (*card_object == NULL)
        return;
    sprite_object_destroy(&((*card_object)->sprite_object));
    POOL_FREE(CardObject, *card_object);
    *card_object = NULL;
}

void card_object_update(CardObject* card_object)
{
    if (card_object == NULL)
        return;
    sprite_object_update(card_object->sprite_object);
}

void card_object_set_sprite(CardObject* card_object, int layer)
{
    int card_index = card_object->card->rank + card_object->card->suit * NUM_RANKS;

    // Create sprite at the specified OAM index using the allocated VRAM
    Sprite* sprite = sprite_new(
        layer + CARD_STARTING_LAYER,
        &oamMain,
        0,
        0,
        SpriteSize_32x32,
        SpriteColorFormat_16Color,
        layer,
        true,
        CARD_PB,
        gfx_cards[card_index]
    );

    sprite_object_set_sprite(card_object->sprite_object, sprite);
}

void card_object_shake(CardObject* card_object, mm_word sound_id)
{
    sprite_object_shake(card_object->sprite_object, sound_id);
}

void card_object_set_selected(CardObject* card_object, bool selected)
{
    if (card_object == NULL)
        return;
    card_object->selected = selected;
}

bool card_object_is_selected(CardObject* card_object)
{
    if (card_object == NULL)
        return false;
    return card_object->selected;
}

Sprite* card_object_get_sprite(CardObject* card_object)
{
    if (card_object == NULL)
        return NULL;
    return sprite_object_get_sprite(card_object->sprite_object);
}
