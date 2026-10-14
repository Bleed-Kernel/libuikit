#include <stdio.h>
#include <crime/crime.h>
#include <bleed_uikit.h>

#define WIN_W   360
#define WIN_H   300
#define PAD     16

static ui_button_t more  = { .rect = {PAD, WIN_H - PAD - 26, 90, 26}, .label = "More" };
static ui_button_t ask   = { .rect = {(WIN_W - 90) / 2, WIN_H - PAD - 26, 90, 26}, .label = "Ask" };
static ui_button_t close = { .rect = {WIN_W - PAD - 90, WIN_H - PAD - 26, 90, 26}, .label = "Close" };

static ui_image_t  logo;
static int         permille = 350;
static const char *answer   = "a sunken panel with text in the middle";

// a box with everything set, and what to make of what it gives back
static void ask_something(void) {
    ui_msgbox_t box = {
        .title          = "Unsaved changes",
        .text           = "This is a message box from the kit.\nA long line is wrapped where it runs out of room, "
                          "and the buttons are whatever the program asks for.",
        .icon           = UI_ICON_WARNING,
        .buttons        = { "Save", "Dont Save", "Cancel" },
        .default_button = 0,
        .escape_button  = 3,
    };

    switch (ui_msgbox(&box)) {
    case 0:  answer = "you pressed Save"; break;
    case 1:  answer = "you pressed Dont Save"; break;
    case 2:  answer = "you pressed Cancel, or escaped"; break;
    default: answer = "the box couldnt be shown"; break;
    }
}

static void draw(cs_window_t *win) {
    ui_canvas_t c = ui_canvas(win->pixels, win->width, win->height, win->pitch);
    char        text[32];

    ui_fill(&c, c.clip, UI_COLOUR_FACE);

    ui_image_draw(&c, &logo, PAD, PAD);
    ui_text(&c, &ui_font_heading, PAD + 76, PAD + 12, "UI Kit", UI_COLOUR_TEXT);
    ui_text(&c, &ui_font_light, PAD + 76, PAD + 12 + ui_font_height(&ui_font_heading),
            "one of everything", UI_COLOUR_BACK_TEXT);

    ui_separator(&c, PAD, 92, win->width - PAD * 2);

    ui_rect_t well = {PAD, 106, win->width - PAD * 2, 60};
    ui_panel(&c, well, UI_SUNKEN);
    ui_text_in(&c, &ui_font_text, ui_rect_shrink(well, 8), answer,
               UI_COLOUR_TEXT, UI_ALIGN_CENTRE);

    snprintf(text, sizeof(text), "%d%%", permille / 10);
    ui_rect_t bar = {PAD, 184, win->width - PAD * 2 - 50, 18};
    ui_progress(&c, bar, permille);
    ui_text_in(&c, &ui_font_text, (ui_rect_t){bar.x + bar.w, bar.y, 50, bar.h}, text,
               UI_COLOUR_TEXT, UI_ALIGN_RIGHT);

    ui_button_draw(&c, &more);
    ui_button_draw(&c, &ask);
    ui_button_draw(&c, &close);

    cs_window_commit(win, 0, 0, win->width, win->height);
}

int main(void) {
    cs_window_t *win = cs_window_open(WIN_W, WIN_H, "UI Kit");
    if (!win) {
        printf("uikit-demo: needs the desktop\n");
        return 1;
    }

    if (ui_init() < 0) {
        printf("uikit-demo: no font ending in " UI_FONT_REGULAR " in " UI_FONT_DIR "\n");
        return 1;
    }
    if (ui_image_load_raw(&logo, "/initrd/boot/splash.bgra", 256, 256) == 0) {
        ui_image_scale(&logo, 64, 64);
        ui_image_key_black(&logo);
    }
    draw(win);

    cs_event_t ev;
    for (;;) {
        cs_event_wait(&ev, -1);
        if (ev.type == CS_EV_CLOSE_REQUEST)
            break;
        if (ev.type == CS_EV_RESIZE)
            draw(win);
        if (ev.type != CS_EV_MOUSE)
            continue;

        int left   = ev.buttons & CS_BUTTON_LEFT;
        int a      = ui_button_mouse(&more, ev.x, ev.y, left);
        int b      = ui_button_mouse(&close, ev.x, ev.y, left);
        int q      = ui_button_mouse(&ask, ev.x, ev.y, left);

        if (a & UI_CLICKED)
            permille = permille >= 1000 ? 0 : permille + 130;
        if (b & UI_CLICKED)
            break;
        if (q & UI_CLICKED) {
            ask_something();
            if (ui_ask("One more", "Was that the answer you meant to give?"))
                ui_message("Good", "Then thats that.", UI_ICON_INFO);
            else
                ui_message("Oh", "Press Ask and have another go.", UI_ICON_ERROR);
        }
        if ((a | b | q) & (UI_CHANGED | UI_CLICKED))
            draw(win);
    }

    cs_window_close(win);
    return 0;
}
