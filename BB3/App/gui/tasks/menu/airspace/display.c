#include <gui/tasks/menu/airspace/display.h>
#include <gui/tasks/menu/airspace/airspace.h>
#include "fc/airspaces/airspace.h"

#include "gui/gui_list.h"

#include "etc/format.h"

REGISTER_TASK_IS(airspace_display,
        bool change;
);

static gui_list_slider_options_t show_bellow_opt = {
	.disp_multi = 1,
	.step = 5,
	.format = format_FL_with_altitude_with_units,
};

bool airspace_display_cb(lv_obj_t * obj, lv_event_t event, uint16_t index)
{
    if (event == LV_EVENT_VALUE_CHANGED)
        local->change = true;

    return true;
}


static lv_obj_t * airspace_display_init(lv_obj_t * par)
{
    local->change = false;

	lv_obj_t * list = gui_list_create(par, _("Enabled airspaces"), &gui_airspace, airspace_display_cb);

	gui_list_auto_entry(list, _h("Class A"), &profile.airspace.display.class_A, NULL);
	gui_list_auto_entry(list, _h("Class B"), &profile.airspace.display.class_B, NULL);
	gui_list_auto_entry(list, _h("Class C"), &profile.airspace.display.class_C, NULL);
	gui_list_auto_entry(list, _h("Class D"), &profile.airspace.display.class_D, NULL);
	gui_list_auto_entry(list, _h("Class E"), &profile.airspace.display.class_E, NULL);
	gui_list_auto_entry(list, _h("Class F"), &profile.airspace.display.class_F, NULL);
	gui_list_auto_entry(list, _h("Class G"), &profile.airspace.display.class_G, NULL);
	gui_list_auto_entry(list, _h("CTR"), &profile.airspace.display.ctr, NULL);
	gui_list_auto_entry(list, _h("TMZ"), &profile.airspace.display.tmz, NULL);
	gui_list_auto_entry(list, _h("RMZ"), &profile.airspace.display.rmz, NULL);
	gui_list_auto_entry(list, _h("Danger"), &profile.airspace.display.danger, NULL);
	gui_list_auto_entry(list, _h("Prohibited"), &profile.airspace.display.prohibited, NULL);
	gui_list_auto_entry(list, _h("Restricted"), &profile.airspace.display.restricted, NULL);
	gui_list_auto_entry(list, _h("Glider prohibited"), &profile.airspace.display.glider_prohibited, NULL);
	gui_list_auto_entry(list, _h("Wave window"), &profile.airspace.display.wave_window, NULL);
	gui_list_auto_entry(list, _h("Undefined"), &profile.airspace.display.undefined, NULL);
	gui_list_auto_entry(list, _h("Ignore with floor above"), &profile.airspace.display.below, &show_bellow_opt);

	return list;
}

static void airspace_display_stop()
{
    if (local->change)
    {
        airspace_load_parallel();
    }
}

