#include "selection_grid.h"

#include "game.h"

static void selection_grid_process_directional_input(SelectionGrid* selection_grid, void* ctx)
{
    u32 keys_down = keysDown();

    // Calculate horizontal input: +1 for right, -1 for left, 0 for neither/both
    int has_right = (keys_down & KI_RIGHT) ? 1 : 0;
    int has_left = (keys_down & KI_LEFT) ? 1 : 0;
    int horz_tri_input = has_right - has_left;

    if (horz_tri_input != 0)
    {
        selection_grid_move_selection_horz(selection_grid, horz_tri_input, ctx);
        /* Avoid handling both vertical and horizontal input at the same time,
         * it creates all sorts of difficult edge cases.
         */
        return;
    }

    // Calculate vertical input: +1 for down, -1 for up, 0 for neither/both
    int has_down = (keys_down & KI_DOWN) ? 1 : 0;
    int has_up = (keys_down & KI_UP) ? 1 : 0;
    int vert_tri_input = has_down - has_up;

    if (vert_tri_input != 0)
    {
        selection_grid_move_selection_vert(selection_grid, vert_tri_input, ctx);
    }
}

void selection_grid_move_selection_horz(
    SelectionGrid* selection_grid,
    int direction_tribool,
    void* ctx
)
{
    if (selection_grid == NULL || selection_grid->selection.y < 0 ||
        selection_grid->selection.y >= selection_grid->num_rows)
    {
        return;
    }

    Selection new_selection = selection_grid->selection;
    new_selection.x += direction_tribool;
    int row_size = selection_grid->rows[new_selection.y].get_size(ctx);
    bool wrap_enabled = selection_grid->rows[new_selection.y].attributes.wrap;

    if (wrap_enabled)
    {
        new_selection.x = wrap(new_selection.x, 0, row_size);
    }

    if (wrap_enabled || (new_selection.x >= 0 && new_selection.x < row_size))
    {
        bool proceed_selection = selection_grid->rows[new_selection.y].on_selection_changed(
            selection_grid,
            new_selection.y,
            &selection_grid->selection,
            &new_selection,
            ctx
        );

        if (proceed_selection)
        {
            selection_grid->selection = new_selection;
        }
    }
}

void selection_grid_move_selection_vert(
    SelectionGrid* selection_grid,
    int direction_tribool,
    void* ctx
)
{
    if (selection_grid == NULL)
        return;

    Selection selection = selection_grid->selection;
    Selection new_selection = selection;
    new_selection.y += direction_tribool;

    if (new_selection.y >= 0 && new_selection.y < selection_grid->num_rows)
    {
        int new_row_size = selection_grid->rows[new_selection.y].get_size(ctx);
        if (new_row_size <= 0)
            return;

        int old_row_size = selection_grid->rows[selection.y].get_size(ctx);
        // Branchless set to 1 if 0 to avoid division by 0
        old_row_size += (old_row_size == 0);

        // Maintain relative horizontal position
        // The operations are equivalent to fixed point if all the numbers were converted
        new_selection.x = fx2int(selection.x * ((int2fx(new_row_size) / old_row_size)));

        bool proceed_selection = true;

        if (selection.y >= 0 && selection.y < selection_grid->num_rows)
        {
            proceed_selection = selection_grid->rows[selection.y].on_selection_changed(
                selection_grid,
                selection.y,
                &selection,
                &new_selection,
                ctx
            );
        }

        if (proceed_selection)
        {
            proceed_selection = selection_grid->rows[new_selection.y].on_selection_changed(
                selection_grid,
                new_selection.y,
                &selection,
                &new_selection,
                ctx
            );
        }

        if (proceed_selection)
        {
            selection_grid->selection = new_selection;
        }
    }
}

void selection_grid_process_input(SelectionGrid* selection_grid, void* ctx)
{
    if (selection_grid == NULL || selection_grid->rows == NULL)
        return;

    selection_grid_process_directional_input(selection_grid, ctx);

    u32 non_directional_key = KEY_ANY & ~KEY_DIR;
    if (key_transit(non_directional_key))
    {
        // To make the next line shorter and more readable
        Selection* selection = &selection_grid->selection;
        if (selection->y >= 0 && selection->y < selection_grid->num_rows)
        {
            selection_grid->rows[selection->y].on_key_transit(selection_grid, selection, ctx);
        }
    }
}