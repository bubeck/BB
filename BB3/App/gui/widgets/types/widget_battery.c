
#include "gui/widgets/widget.h"
#include "drivers/power/pwr_mng.h"

REGISTER_WIDGET_IU
(
    Battery,
    _i("Battery percent"),
    WIDGET_MIN_W,
    WIDGET_MIN_H,
	_b(wf_label_hide),

    lv_obj_t * value;
	lv_obj_t * sub_text;
);


static void Battery_init(lv_obj_t * base, widget_slot_t * slot)
{
    widget_create_base(base, slot);

    if (!widget_flag_is_set(slot, wf_label_hide))
    	widget_add_title(base, slot, NULL);

    local->value = widget_add_value(base, slot, "", &local->sub_text);
}

static void Battery_update(widget_slot_t * slot)
{
    char value[8];

    format_percent(value, pwr.fuel_gauge.battery_percentage);

    lv_label_set_text(local->value, value);
    widget_update_font_size(local->value);

    if (local->sub_text)
    {
        if (pwr.charger.charge_port == PWR_CHARGE_WEAK)
            lv_label_set_text(local->sub_text, _("weak chrg."));
        else if (pwr.charger.charge_port == PWR_CHARGE_SLOW)
            lv_label_set_text(local->sub_text, _("slow chrg."));
        else if (pwr.charger.charge_port == PWR_CHARGE_FAST)
            lv_label_set_text(local->sub_text, _("fast chrg."));
        else if (pwr.charger.charge_port == PWR_CHARGE_QUICK)
            lv_label_set_text(local->sub_text, _("quick chrg."));
        else if (pwr.data_port == PWR_DATA_CHARGE)
            lv_label_set_text(local->sub_text, _("slow chrg."));
        else
            lv_label_set_text(local->sub_text, "");
    }
}


