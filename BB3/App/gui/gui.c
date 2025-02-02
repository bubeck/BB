/*
 * gui.cc
 *
 *  Created on: 14. 5. 2020
 *      Author: horinek
 */


#include "gui.h"

#include "gui_list.h"
#include "statusbar.h"
#include "keyboard.h"
#include "ctx.h"

#include "tasks/page/pages.h"
#include "lib/lvgl/src/lv_misc/lv_gc.h"

#include "drivers/tft/tft.h"

#include "gui/dialog.h"

#include "drivers/rev.h"

#include "gui/dbg_overlay.h"
#include "fc/fc.h"

gui_t gui;

//LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_16_ext);
LV_FONT_DECLARE(lv_font_montserrat_50);
LV_FONT_DECLARE(lv_font_montserrat_60);
LV_FONT_DECLARE(lv_font_montserrat_70);
LV_FONT_DECLARE(lv_font_montserrat_85);
LV_FONT_DECLARE(lv_font_montserrat_100);
LV_FONT_DECLARE(lv_font_montserrat_120);
LV_FONT_DECLARE(lv_font_montserrat_140);

gui_task_t * gui_startup_task = &gui_pages;

/**
 * Set the speed of the GUI loop in HAL ticks.
 *
 * @param period the next period in HAL ticks
 */
void gui_set_loop_period(uint16_t period)
{
    gui.task.loop_period = period;
}

void gui_set_dummy_event_cb(lv_obj_t * par, lv_event_cb_t event_cb)
{
    lv_obj_t * dummy = lv_obj_create(par, NULL);
    lv_obj_set_size(dummy, 0, 0);
    lv_group_add_obj(gui.input.group, dummy);
    lv_group_set_editing(gui.input.group, true);
    lv_obj_set_event_cb(dummy, event_cb);
}

void gui_load_language()
{
    uint8_t id = config_get_select(&config.display.language);

    lv_i18n_init(lv_i18n_language_pack);

    switch (id)
    {
        case(LANG_DE):
            lv_i18n_set_locale("de-DE");
        break;

        case(LANG_SK):
            lv_i18n_set_locale("sk-SK");
        break;

        case(LANG_FR):
            lv_i18n_set_locale("fr-FR");
        break;

        case(LANG_PL):
            lv_i18n_set_locale("pl-PL");
        break;

        default:
            lv_i18n_set_locale("en-US");
        break;
    }
}

void gui_set_group_focus(lv_obj_t * obj)
{
	gui.input.focus = obj;
}

static void gui_group_focus_cb(lv_group_t * group)
{
	if (!gui.input.focus)
		return;

    lv_obj_t * focused = lv_group_get_focused(group);

    if (gui.input.focus != NULL)
    	lv_page_focus(gui.input.focus, focused, LV_ANIM_ON);
}

void end_last_task()
{
	if (gui.task.last != NULL)
	{
		if (gui.task.last->stop != NULL)
		{
			if (gui.task.last == gui.task.actual)
			{
			    //handles problem when the last and actual task are the same (eg: Filemanager)
				void * actual_memory = *gui.task.actual->local_vars;
				*gui.task.last->local_vars = gui.task.last_memory;
				gui.task.last->stop();
				*gui.task.actual->local_vars = actual_memory;
			}
			else
			{
				gui.task.last->stop();
			}
		}

		if (gui.task.last_memory != NULL)
		{
			tfree(gui.task.last_memory);
			gui.task.last_memory = NULL;
		}
	}

}

//when task screen is deleted, trigger task stop, clear memory
void screen_event_cb(lv_obj_t * obj, lv_event_t event)
{
	if (event == LV_EVENT_DELETE)
	{
		end_last_task();
	}
}

lv_obj_t * gui_task_create(gui_task_t * task)
{
	lv_obj_t * screen = lv_obj_create(NULL, NULL);
	lv_obj_t * base = lv_obj_create(screen, NULL);

    lv_obj_set_pos(base, 0, GUI_STATUSBAR_HEIGHT);
    lv_obj_set_size(base, LV_HOR_RES, LV_VER_RES - GUI_STATUSBAR_HEIGHT);

    //allocate memory for local task
    if (task->local_vars_size > 0)
    {
    	*task->local_vars = tmalloc(task->local_vars_size);
    	ASSERT(*task->local_vars != NULL);
    }
    else
    {
    	*task->local_vars = NULL;
    }

	//init
	task->init(base);
	if (task->loop != NULL)
	    task->loop();

	gui_list_retrive_pos(task);

	lv_obj_set_event_cb(screen, screen_event_cb);

	return screen;
}

//request gui thread to run function from its context
void gui_inject_function(gui_injected_function_t f)
{
    gui_lock_acquire();
    while (gui.injected_function != NULL)
    {
        osDelay(10);
    }
    gui.injected_function = f;
    gui_lock_release();
}

void * gui_switch_task(gui_task_t * next, lv_scr_load_anim_t anim)
{
	gui_list_store_pos(gui.task.actual);
	help_unset();

	gui.input.focus = NULL;

	//hide ctx, dialog or keyboard if active
	ctx_hide();
	keyboard_hide();

    //reset input dev
    lv_indev_reset(gui.input.indev, NULL);
    lv_indev_wait_release(gui.input.indev);

	//remove input groups from old screen
	lv_group_remove_all_objs(gui.input.group);
	lv_group_set_editing(gui.input.group, false);

	//switch task
	lv_disp_t * d = lv_disp_get_default();
	if (d->prev_scr != NULL)
	{
		end_last_task();
		//remove delete callback from last task
		lv_obj_set_event_cb(d->prev_scr, NULL);
	}

	gui.task.last = gui.task.actual;
	gui.task.last_memory = *gui.task.last->local_vars;

	gui_set_loop_period(GUI_DEFAULT_LOOP_SPEED);

	//init new screen
    gui.task.actual = next;
	lv_obj_t * screen = gui_task_create(next);

	//switch screens
	lv_scr_load_anim(screen, anim, GUI_TASK_SW_ANIMATION, 0, true);

	return gui.task.last_memory;
}


void gui_init_styles()
{
    lv_style_init(&gui.styles.widget_label);
    lv_style_set_text_font(&gui.styles.widget_label, LV_STATE_DEFAULT, &lv_font_montserrat_12);

    lv_style_init(&gui.styles.widget_unit);
    lv_style_set_text_font(&gui.styles.widget_unit, LV_STATE_DEFAULT, &lv_font_montserrat_12);

	lv_style_init(&gui.styles.widget_box);
	lv_style_set_pad_all(&gui.styles.widget_label, LV_STATE_DEFAULT, 2);

	lv_style_init(&gui.styles.list_select);
	lv_style_set_radius(&gui.styles.list_select, LV_STATE_FOCUSED, 5);
	lv_style_set_radius(&gui.styles.list_select, LV_STATE_EDITED, 5);

	lv_style_init(&gui.styles.note);
	lv_style_set_radius(&gui.styles.note, LV_STATE_DEFAULT, 5);
	lv_style_set_margin_bottom(&gui.styles.note, LV_STATE_DEFAULT, 5);

	//numbers only
    gui.styles.widget_fonts[FONT_8XL] = &lv_font_montserrat_140;
    gui.styles.widget_fonts[FONT_7XL] = &lv_font_montserrat_120;
    gui.styles.widget_fonts[FONT_6XL] = &lv_font_montserrat_100;
    gui.styles.widget_fonts[FONT_5XL] = &lv_font_montserrat_85;
    gui.styles.widget_fonts[FONT_4XL] = &lv_font_montserrat_70;
    gui.styles.widget_fonts[FONT_3XL] = &lv_font_montserrat_60;
    gui.styles.widget_fonts[FONT_2XL] = &lv_font_montserrat_50;

    //full fonts
	gui.styles.widget_fonts[FONT_XL] = &lv_font_montserrat_44;
	gui.styles.widget_fonts[FONT_L] = &lv_font_montserrat_34; //10
	gui.styles.widget_fonts[FONT_M] = &lv_font_montserrat_28; //6
	gui.styles.widget_fonts[FONT_S] = &lv_font_montserrat_22; //6
    gui.styles.widget_fonts[FONT_XS] = &lv_font_montserrat_16_ext; //6
    gui.styles.widget_fonts[FONT_2XS] = &lv_font_montserrat_12; //6
}

void gui_stop()
{
	tft_wait_for_buffer();
	tft_color_fill(0xFFFF);
	tft_refresh_buffer(0, 0, 239, 399, tft_buffer);
	tft_wait_for_buffer();
	tft_stop();
}

void gui_init()
{
    gui.take_screenshot = false;
    gui.dialog.active = false;
    gui.page_queue = xQueueCreate(GUI_QUEUE_SIZE, sizeof(void *));
    dbg_overlay_init();
	gui_init_styles();

    help_init_gui();

	//create statusbar
	statusbar_create();

	//create keyboard
	keyboard_create();

	//init ctx menu
	ctx_init();

    config_process_cb(&config.debug.tasks);

	//set group focus callback
	gui.input.focus = NULL;
	lv_group_set_focus_cb(gui.input.group, gui_group_focus_cb);

	//first task
	gui.task.last = NULL;
	gui.task.actual = gui_startup_task;
	gui.task.last_memory = NULL;

	//list
	gui.list.back = NULL;
	gui.list.entry_list = NULL;

	//load the screen
	lv_obj_t * screen = gui_task_create(gui.task.actual);
    gui_page_set_mode(&profile.ui.autoset.power_on);


	if (gui.task.actual == &gui_pages)
		pages_splash_show();

	lv_scr_load(screen);
}

#define AUTO_POWEROFF_NOTICE_TIMEOUT (30 * 1000)
#define RETURN_TO_PAGES_TIMEOUT (30 * 1000)

void gui_autopoweroff_step()
{
    static lv_obj_t * auto_pwr_off_msg = NULL;
    static bool powering_off = false;

    if (powering_off)
        return;

    uint32_t delta = HAL_GetTick() - fc.inactivity_timer;

    if (fc.flight.mode != flight_flight && config_get_bool(&profile.ui.auto_power_off_enabled))
    {
        if (delta > (config_get_int(&profile.ui.auto_power_off_time) * 60 * 1000) - AUTO_POWEROFF_NOTICE_TIMEOUT)
        {
            if (auto_pwr_off_msg == NULL)
            {
                auto_pwr_off_msg = statusbar_msg_add(STATUSBAR_MSG_PROGRESS, _("Auto power off"));
                statusbar_msg_update_progress(auto_pwr_off_msg, 100);
            }
            else
            {
                int32_t val = delta - (config_get_int(&profile.ui.auto_power_off_time) * 60 * 1000) + AUTO_POWEROFF_NOTICE_TIMEOUT;
                val = max(0, val / (AUTO_POWEROFF_NOTICE_TIMEOUT / 100));
                statusbar_msg_update_progress(auto_pwr_off_msg, 100 - val);
            }
        }
        else
        {
            if (auto_pwr_off_msg != NULL)
            {
                statusbar_msg_close(auto_pwr_off_msg);
                auto_pwr_off_msg = NULL;
            }
        }

        if (delta > config_get_int(&profile.ui.auto_power_off_time) * 60 * 1000)
        {
            //off
            if (gui.task.actual == &gui_pages)
            {
                statusbar_msg_close(auto_pwr_off_msg);
                pages_power_off();
            }
            else
            {
                system_poweroff();
            }

            powering_off = true;
        }
    }
    else if (fc.flight.mode == flight_flight && config_get_bool(&profile.ui.return_to_pages))
    {
        if (delta > RETURN_TO_PAGES_TIMEOUT)
        {
            if (gui.task.actual != &gui_pages)
            {
                dialog_close();
                gui_switch_task(&gui_pages, LV_SCR_LOAD_ANIM_MOVE_LEFT);
                gui_inactivity_reset();
            }
        }
    }
}

void gui_inactivity_reset()
{
    fc.inactivity_timer = HAL_GetTick();
}


void gui_loop()
{
    static uint32_t next_update = 0;

    if (next_update < HAL_GetTick())
    {
        next_update = HAL_GetTick() + gui.task.loop_period;

        gui_autopoweroff_step();
        statusbar_step();
        dbg_overlay_step();

        //execute task
        if (gui.task.actual->loop != NULL)
        {
            gui.task.actual->loop();
        }
    }
}
