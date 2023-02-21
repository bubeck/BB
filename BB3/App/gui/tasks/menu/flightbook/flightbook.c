/*
 * flightbook.c
 *
 * Show all flight logs recorded so far and let user select a flight to show.
 *
 *  Created on: Feb 27, 2022
 *      Author: tilmann@bubecks.de
 */

#include "flightbook.h"
#include "flightbook_flight.h"
#include "flightbook_statistics.h"

#include "gui/tasks/menu/settings.h"
#include "gui/tasks/filemanager.h"

#include "gui/statusbar.h"
#include "gui/gui_list.h"
#include "gui/ctx.h"
#include "gui/dialog.h"

#include "etc/format.h"

static bool open_flightbook_stat(lv_obj_t * obj, lv_event_t event)
{
	if (event == LV_EVENT_CLICKED)
	{
		//this is standard method how to pass extra parameter to task
		//1. Switch to task, so the local memory is allocated for the new task
		gui_switch_task(&gui_flightbook_statistics, LV_SCR_LOAD_ANIM_MOVE_LEFT);
		//2. Call custom function specific to target task to pass parameters
		flightbook_statistics_load(NULL, NULL);
	}

	//supress default handler
	return false;
}

static void flightbook_fm_remove_cb(uint8_t res, void * opt_data)
{
    char * path = opt_data;

    if (res == dialog_res_yes)
    {                                                                                                                                       
        red_unlink(path);                                                                                                                   

        char name[PATH_LEN];
        filemanager_get_filename(name, path);

        char text[64];
        snprintf(text, sizeof(text), _("Flight '%s' deleted"), name);
        statusbar_msg_add(STATUSBAR_MSG_INFO, text);                                                                                        

        filemanager_refresh();
    }
    tfree(opt_data);
}

bool flightbook_flights_fm_cb(uint8_t event, char * path)
{
    switch (event)
    {
        //called after the list is populated with files
        case FM_CB_INIT:
            help_set_base("Flightbook");
        break;

        //called after the list is populated with files
    	case FM_CB_APPEND:
    	    //only in root dir
    		if (filemanager_get_current_level() == 0)
    			gui_list_auto_entry(gui.list.list, _h("Flight statistics"), CUSTOM_CB, open_flightbook_stat);
		break;

        case FM_CB_SELECT:
        case (0):
        {
        	uint8_t fm_level = filemanager_get_current_level();
            gui_switch_task(&gui_flightbook_flight, LV_SCR_LOAD_ANIM_MOVE_LEFT);
            flightbook_flight_open(path, fm_level);
            return false;
        }

        case FM_CB_FOCUS_FILE:
        {
            ctx_clear();
            ctx_add_option(LV_SYMBOL_EYE_OPEN " View");
            ctx_add_option(LV_SYMBOL_TRASH " Delete");
            ctx_show();
            break;
        }

        case (1): //delete
        {
            char name[strlen(path) + 1];
            filemanager_get_filename(name, path);

            char text[64];
            sniprintf(text, sizeof(text), _("Do you want to remove flight '%s'"), name);
            dialog_show(_("Confirm"), text, dialog_yes_no, flightbook_fm_remove_cb);
            char * opt_data = tmalloc(strlen(path) + 1);
            strcpy(opt_data, path);
            dialog_add_opt_data((void *)opt_data);
            break;
        }
    }
    return true;
}

#include "lib/miniz/miniz.h"

void unzip_map(void * param)
{
    char path[PATH_LEN];
    char path2[PATH_LEN];
	mz_zip_archive_file_stat *file_stat = malloc(sizeof(mz_zip_archive_file_stat));

	//char *path = malloc(PATH_LEN);
	//char *path2 = malloc(PATH_LEN);

    mz_zip_archive * zip = malloc(sizeof(mz_zip_archive));
    mz_zip_zero_struct(zip);
#if 0
    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);
#endif
    char mess[129];

    strcpy(path, PATH_MAP_DIR "/SK_Slovakia_map.zip");
    if (file_exists(path))
    {
        bool res = mz_zip_reader_init_file(zip, path, 0);
        if (res)
        {
            int fileCount = (int)mz_zip_reader_get_num_files(zip);
            for ( int i = 0; i < fileCount; i++ )
            {
        	    if (!mz_zip_reader_file_stat(zip, i, file_stat)) continue;
        	    if (mz_zip_reader_is_file_a_directory(zip, i)) continue; // skip directories for now
        	    sprintf(path2, PATH_MAP_DIR "/%s", file_stat->m_filename);
        	    if ( file_exists(path2)) red_unlink(path2);
        	    if ( !file_exists(path2) || file_size_from_name(path2) != file_stat->m_uncomp_size)
        	    {
        	    	uint8_t h;
        	    		uint8_t m;
        	    		uint8_t s;
        	    		char value[20];

        	    		rtc_get_time(&h, &m, &s);
        	    		format_time(value, h, m);

        	    	INFO("%s Unzip: %s (%d)", value, path2, (int)file_stat->m_uncomp_size);
        	    	//statusbar_msg_add(STATUSBAR_MSG_WARN, mess);
        	    	mz_zip_reader_extract_to_file(zip, i, path2, 0);
        	    	//break;
        	    }
            }
        }
	    mz_zip_reader_end(zip);
    }

    vTaskDelete(NULL);
}


void flightbook_open(bool from_left)
{
	//airspace_find_vertical();

    xTaskCreate((TaskFunction_t)unzip_map, "unzip_map", 1024 * 4, NULL, 24, NULL);


    gui_switch_task(&gui_filemanager, from_left ? LV_SCR_LOAD_ANIM_MOVE_LEFT : LV_SCR_LOAD_ANIM_MOVE_RIGHT);
    filemanager_open(PATH_LOGS_DIR, 0, &gui_settings, FM_FLAG_FOCUS | FM_FLAG_SORT_NAME | FM_FLAG_SHOW_EXT, flightbook_flights_fm_cb);
}


