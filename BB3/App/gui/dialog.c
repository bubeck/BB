/*
 * dialog.c
 *
 *  Created on: Jan 13, 2021
 *      Author: horinek
 */

#include "dialog.h"

#include "gui.h"
#include "keyboard.h"

#include "drivers/esp/download/slot.h"

static void * dialog_opt_data  = NULL;
static void * dialog_opt_param  = NULL;

void dialog_downloads_error(uint8_t res)
{
    if (res == DOWNLOAD_SLOT_NOT_FOUND)
    {
        dialog_show(_("Error"), _("No slot available"), dialog_confirm, NULL);
    }

    if (res == DOWNLOAD_SLOT_NO_CONNECTION)
    {
        dialog_show(_("Error"), _("Not connected!"), dialog_confirm, NULL);
    }

    if (res == DOWNLOAD_SLOT_NOT_FOUND)
    {
        dialog_show(_("Error"), _("Resource not found!"), dialog_confirm, NULL);
    }

    if (res == DOWNLOAD_SLOT_TIMEOUT)
    {
        dialog_show(_("Error"), _("Connection timeout!"), dialog_confirm, NULL);
    }
}

static void dialog_opa_anim(lv_obj_t * obj, lv_anim_value_t v)
{
    lv_obj_set_style_local_opa_scale(obj, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, v);
}

void dialog_stop(dialog_result_t res, void * data)
{
    gui.dialog.active = false;
    //restore groups
    lv_group_set_editing(gui.dialog.group, false);
    lv_indev_reset(gui.input.indev, NULL);
    lv_indev_wait_release(gui.input.indev);
    lv_indev_set_group(gui.input.indev, gui.input.group);

    lv_group_remove_all_objs(gui.dialog.group);
    lv_obj_del_async(gui.dialog.window);

    if (gui.dialog.cb != NULL)
        gui.dialog.cb(res, data);
}

static void dialog_event_cb(lv_obj_t * obj, lv_event_t event)
{
    uint32_t key = 0;

    if (event == LV_EVENT_KEY || event == LV_EVENT_CLICKED)
    {
        if (event == LV_EVENT_CLICKED)
            key = LV_KEY_ENTER;
        else
            key = *((uint32_t*) lv_event_get_data());
    }

    if (gui.dialog.type == dialog_yes_no)
    {
        if (key == LV_KEY_ESC)
            dialog_stop(dialog_res_no, dialog_opt_data);
        if (key == LV_KEY_ENTER)
            dialog_stop(dialog_res_yes, dialog_opt_data);
    }

    if (gui.dialog.type == dialog_confirm)
    {
        if (key == LV_KEY_ESC || key == LV_KEY_ENTER)
            dialog_stop(dialog_res_none, dialog_opt_data);
    }

    if (gui.dialog.type == dialog_release_note)
    {
        int16_t scroll = 0;
        if (key == LV_KEY_LEFT)
            scroll = +1;
        if (key == LV_KEY_RIGHT)
            scroll = -1;

        if (scroll != 0)
        {
            lv_obj_t * c = lv_obj_get_child(gui.dialog.window, NULL);
            c = lv_obj_get_child(gui.dialog.window, c);
            lv_obj_t * s = lv_obj_get_child(c, NULL);
            lv_page_scroll_ver(s, scroll * 16);
        }

        if (key == LV_KEY_ESC || key == LV_KEY_ENTER)
            dialog_stop(dialog_res_none, dialog_opt_data);
    }

    if (gui.dialog.type == dialog_progress)
    {
        if (key == LV_KEY_ESC && gui.dialog.cb != NULL)
            dialog_stop(dialog_res_cancel, dialog_opt_data);
    }

    if (gui.dialog.type == dialog_textarea)
    {

        if(event == LV_EVENT_APPLY || event == LV_EVENT_CANCEL)
        {
            keyboard_hide();
            if (event == LV_EVENT_APPLY)
                dialog_stop(dialog_res_yes, (void *)lv_textarea_get_text(obj));
            else
                dialog_stop(dialog_res_no, dialog_opt_data);
        }

        if (event == LV_EVENT_LEAVE)
            dialog_stop(dialog_res_no, dialog_opt_data);
    }
}

#define SPINNER_SIZE            80
#define SPINNER_STEP            10

static void dialog_progress_anim_cb(lv_obj_t * obj, lv_anim_value_t v)
{
    uint16_t start = 360 + lv_arc_get_angle_end(obj) - SPINNER_SIZE;
    start += SPINNER_STEP;
    start %= 360;
    lv_arc_set_angles(obj, start, start + SPINNER_SIZE);
}

void dialog_progress_spin()
{
    lv_obj_t * cont = lv_obj_get_child_back(gui.dialog.window, NULL);
    lv_obj_t * spinner = lv_obj_get_child(cont, lv_obj_get_child(cont, NULL));

    lv_anim_del(spinner, NULL);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, spinner);
    lv_anim_set_values(&a, 0, 360);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)dialog_progress_anim_cb);
    lv_anim_start(&a);

    lv_arc_set_angles(spinner, 270, 270);
}

void dialog_progress_set_progress(uint8_t progress)
{
    lv_obj_t * cont = lv_obj_get_child_back(gui.dialog.window, NULL);

    if (cont == NULL)
        return;

    lv_obj_t * spinner = lv_obj_get_child(cont, lv_obj_get_child(cont, NULL));
    lv_obj_t * label = lv_obj_get_child(spinner, NULL);

    lv_anim_del(spinner, NULL);

    lv_arc_set_angles(spinner, 270, 270 + 360 * (progress / 100.0));
    lv_obj_invalidate(spinner);

    lv_label_set_text_fmt(label, "%u%%", progress);

}

void dialog_progress_set_subtitle(char * text)
{
    lv_obj_t * cont = lv_obj_get_child_back(gui.dialog.window, NULL);
    if (cont == NULL)
        return;

    lv_obj_t * label = lv_obj_get_child(cont, NULL);

    lv_label_set_text(label, text);
}

void dialog_set_text(char * text)
{
    lv_obj_t * cont = lv_obj_get_child_back(gui.dialog.window, NULL);
    if (cont == NULL)
        return;

    lv_obj_t * title = lv_obj_get_child_back(cont, NULL);
    lv_obj_t * obj = lv_obj_get_child_back(cont, title);

    lv_label_set_text(obj, text);
}

void dialog_close()
{
	gui_lock_acquire();
	if (gui.dialog.active)
	{
	    dialog_stop(dialog_res_none, NULL);
	}
	gui_lock_release();
}


//set opt data after you called dialog show
void dialog_add_opt_data(void * opt_data)
{
    dialog_opt_data = opt_data;
}

void * dialog_get_opt_data()
{
    return dialog_opt_data;
}

void dialog_add_opt_param(void * opt_param)
{
    dialog_opt_param = opt_param;
}

void dialog_show(char * title, char * message, dialog_type_t type, gui_dialog_cb_t cb)
{
	gui_lock_acquire();

    while (gui.dialog.active)
        dialog_stop(dialog_res_none, NULL);

    gui.dialog.active = true;
    dialog_opt_data = NULL;
    //switch group
    lv_indev_set_group(gui.input.indev, gui.dialog.group);

    gui.dialog.cb = cb;
    gui.dialog.type = type;
    gui.dialog.window = lv_obj_create(lv_layer_sys(), NULL);
    lv_obj_set_pos(gui.dialog.window, 0, 0);
    lv_obj_set_size(gui.dialog.window, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_local_bg_color(gui.dialog.window, LV_OBJ_PART_MAIN, LV_STATE_EDITED, LV_COLOR_BLACK);

    lv_group_add_obj(gui.dialog.group, gui.dialog.window);
    lv_obj_set_event_cb(gui.dialog.window, dialog_event_cb);
    lv_group_set_editing(gui.dialog.group, true);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, gui.dialog.window);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_90);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)dialog_opa_anim);
    lv_anim_start(&a);

    lv_obj_t * cont = lv_cont_create(gui.dialog.window, NULL);
    lv_cont_set_layout(cont, LV_LAYOUT_COLUMN_MID);
    lv_obj_set_auto_realign(cont, true);
    lv_obj_align_origo(cont, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_cont_set_fit(cont, LV_FIT_TIGHT);
    lv_obj_set_style_local_bg_opa(cont, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);

    lv_obj_t * title_label = lv_label_create(cont, NULL);
    lv_label_set_align(title_label, LV_LABEL_ALIGN_CENTER);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_BREAK);
    lv_obj_set_width(title_label, (LV_HOR_RES * 4) / 5);
    lv_obj_set_style_local_text_font(title_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_22);
    lv_label_set_text(title_label, title);

    lv_obj_t * text_label = lv_label_create(cont, NULL);
    lv_label_set_align(text_label, LV_LABEL_ALIGN_CENTER);
    lv_label_set_long_mode(text_label, LV_LABEL_LONG_BREAK);
    lv_obj_set_width(text_label, (LV_HOR_RES * 4) / 5);
    lv_obj_set_style_local_pad_top(text_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 10);
    lv_label_set_text(text_label, message);

    switch (type)
    {
        case (dialog_yes_no):
        {
            lv_obj_t * yes = lv_label_create(gui.dialog.window, NULL);
            lv_obj_set_style_local_text_font(yes, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_22);
            lv_obj_set_style_local_text_color(yes, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
            lv_obj_set_style_local_pad_all(yes, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 5);
            lv_label_set_text(yes, LV_SYMBOL_OK);
            lv_obj_align(yes, gui.dialog.window, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

            lv_obj_t * no = lv_label_create(gui.dialog.window, NULL);
            lv_obj_set_style_local_text_font(no, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_22);
            lv_obj_set_style_local_text_color(no, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
            lv_obj_set_style_local_pad_all(no, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 5);
            lv_label_set_text(no, LV_SYMBOL_CLOSE);
            lv_obj_align(no, gui.dialog.window, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
        }
        break;

        case (dialog_confirm):
        {
            lv_obj_t * yes = lv_label_create(gui.dialog.window, NULL);
            lv_obj_set_style_local_text_font(yes, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_22);
            lv_obj_set_style_local_text_color(yes, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
            lv_obj_set_style_local_pad_all(yes, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 5);
            lv_label_set_text(yes, LV_SYMBOL_OK);
            lv_obj_align(yes, gui.dialog.window, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
        }
        break;

        case (dialog_release_note):
        {
            lv_obj_t * yes = lv_label_create(gui.dialog.window, NULL);
            lv_obj_set_style_local_text_font(yes, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_22);
            lv_obj_set_style_local_text_color(yes, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
            lv_obj_set_style_local_pad_all(yes, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 5);
            lv_label_set_text(yes, LV_SYMBOL_OK);
            lv_obj_align(yes, gui.dialog.window, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

            lv_obj_t * win = lv_page_create(cont, NULL);
            lv_obj_set_style_local_bg_opa(win, LV_PAGE_PART_BG, LV_STATE_DEFAULT, LV_OPA_TRANSP);
            lv_obj_set_style_local_bg_opa(win, LV_PAGE_PART_SCROLLABLE, LV_STATE_DEFAULT, LV_OPA_TRANSP);
            lv_obj_set_size(win, LV_HOR_RES, (LV_VER_RES * 5) / 7);
            lv_page_set_scrl_layout(win, LV_LAYOUT_PRETTY_MID);
            lv_page_set_anim_time(win, 0);

            lv_obj_set_parent(text_label, win);
        	lv_label_set_align(text_label, LV_LABEL_ALIGN_LEFT);

            lv_obj_set_width(title_label, LV_HOR_RES);
        }
        break;

        case (dialog_progress):
        {
            lv_obj_t * spinner = lv_arc_create(cont, NULL);
            lv_obj_set_size(spinner, 120, 120);
            lv_obj_align(spinner, cont, LV_ALIGN_CENTER, 0, 0);
            lv_arc_set_angles(spinner, 270, 270);
            lv_obj_set_style_local_bg_opa(spinner, LV_ARC_PART_BG, LV_STATE_DEFAULT, LV_OPA_TRANSP);
            lv_obj_set_style_local_line_width(spinner, LV_ARC_PART_BG, LV_STATE_DEFAULT, 0);

            lv_obj_t * progress = lv_label_create(spinner, NULL);
            lv_label_set_align(progress, LV_LABEL_ALIGN_CENTER);
            lv_obj_align(progress, spinner, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_auto_realign(progress, true);
            lv_label_set_text(progress, "");

            lv_obj_t * subtitle = lv_label_create(cont, NULL);
            lv_label_set_align(subtitle, LV_LABEL_ALIGN_CENTER);
            lv_label_set_text(subtitle, "");
        }
        break;

        case (dialog_textarea):
        {
            lv_obj_t * textbox = lv_textarea_create(cont, NULL);

            if (dialog_opt_param != NULL)
                lv_textarea_set_text(textbox, dialog_opt_param);
            else
                lv_textarea_set_text(textbox, "");

//            lv_textarea_set_max_length(textbox, max_len);
            lv_textarea_set_one_line(textbox, true);
            lv_textarea_set_cursor_hidden(textbox, true);
            lv_obj_align(textbox, gui.dialog.window, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_auto_realign(textbox, true);
            lv_obj_set_style_local_margin_top(textbox, LV_TEXTAREA_PART_BG, LV_STATE_DEFAULT, 20);
            lv_obj_set_style_local_margin_bottom(textbox, LV_TEXTAREA_PART_BG, LV_STATE_DEFAULT, LV_VER_RES / 3);

            lv_group_remove_all_objs(gui.dialog.group);
            lv_group_add_obj(gui.dialog.group, textbox);

            lv_obj_set_event_cb(textbox, dialog_event_cb);

            keyboard_show(textbox);
        }

        break;
    }

    dialog_opt_param = NULL;

    gui_lock_release();
}

