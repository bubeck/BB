/*
 * protocol.c
 *
 *  Created on: 4. 12. 2020
 *      Author: horinek
 */

#define DEBUG_LEVEL DGB_DEBUG

#include "protocol.h"
#include "wifi.h"
#include "drivers/uart.h"
#include "etc/stream.h"
#include "drivers/tas5720.h"
#include "drivers/spi.h"
#include "pipeline/sound.h"
#include "pipeline/vario.h"
#include "download.h"

static bool protocol_enable_processing = false;

void protocol_enable()
{
	protocol_enable_processing = true;
}

void protocol_send_info()
{
    DBG("sending info");
    proto_device_info_t data;
    esp_read_mac((uint8_t *)&data.wifi_sta_mac, ESP_MAC_WIFI_STA);
    esp_read_mac((uint8_t *)&data.wifi_ap_mac, ESP_MAC_WIFI_SOFTAP);
    esp_read_mac((uint8_t *)&data.bluetooth_mac, ESP_MAC_BT);

    data.server_ok = system_status.server_ok;
    data.amp_ok = system_status.amp_ok;

    protocol_send(PROTO_DEVICE_INFO, (uint8_t *)&data, sizeof(data));
}

void protocol_send_spi_ready(uint32_t len)
{
    proto_spi_ready_t data;
    data.data_lenght = len;
    protocol_send(PROTO_SPI_READY, (uint8_t*) &data, sizeof(data));
}

void protocol_send_sound_reg_more(uint8_t id, uint32_t len)
{
	proto_sound_req_more_t data;
	data.id = id;
    data.data_lenght = len;
    protocol_send(PROTO_SOUND_REQ_MORE, (uint8_t*) &data, sizeof(data));
}

void esp_log_impl_lock(void);
void esp_log_impl_unlock(void);

void protocol_send(uint8_t type, uint8_t *data, uint16_t data_len)
{
    uint8_t buf_out[data_len + STREAM_OVERHEAD];

    stream_packet(type, buf_out, data, data_len);

    uart_send(buf_out, sizeof(buf_out));
}

void protocol_handle(uint8_t type, uint8_t *data, uint16_t len)
{
    DBG("protocol_handle %u", type);
    
    if (!protocol_enable_processing)
    {
    	return;
    }

	switch (type)
    {
        case (PROTO_PING):
            protocol_send(PROTO_PONG, NULL, 0);
        break;

        case (PROTO_GET_INFO):
        {
            protocol_send_info();
        }
        break;

        case (PROTO_SET_VOLUME):
        {
            proto_volume_t * packet = (proto_volume_t *)data;

            if (packet->type == PROTO_VOLUME_MASTER)
                tas_volume(packet->val);
        }
        break;

        case (PROTO_SPI_PREPARE):
        {
            spi_prepare_buffer();
        }
        break;

        case (PROTO_SOUND_START):
		{
        	proto_sound_start_t * packet = (proto_sound_start_t *)data;
        	pipe_sound_start(packet->file_id, packet->file_type, packet->file_lenght);
		}
        break;

        case (PROTO_SOUND_STOP):
        	pipe_sound_stop();
        break;

        case (PROTO_TONE_PLAY):
		{
        	proto_tone_play_t * packet = (proto_tone_play_t *)data;

    		if (packet->size == 0)
    		{
    			tone_part_t ** new_tones = ps_malloc(sizeof(tone_part_t *) * 1);
				new_tones[0] = vario_create_part(0, 10);
				pipe_vario_replace(new_tones, packet->size);
    		}
    		else
    		{
    			tone_part_t ** new_tones = ps_malloc(sizeof(tone_part_t *) * packet->size);
				for (uint8_t i = 0; i < packet->size; i++)
				{
					new_tones[i] = vario_create_part(packet->freq[i], packet->dura[i]);
				}

				pipe_vario_replace(new_tones, packet->size);
    		}

		}
        break;

        case (PROTO_WIFI_SET_MODE):
		{
        	proto_wifi_mode_t * packet = (proto_wifi_mode_t *) ps_malloc(sizeof(proto_wifi_mode_t));
        	memcpy(packet, data, sizeof(proto_wifi_mode_t));

        	xTaskCreate((TaskFunction_t)wifi_enable, "wifi_enable", 1024 * 3, (void *)packet, 24, NULL);
		}
        break;

        case (PROTO_WIFI_SCAN_START):
        	xTaskCreate((TaskFunction_t)wifi_start_scan, "wifi_start_scan", 1024 * 3, NULL, 24, NULL);
        break;

        case (PROTO_WIFI_CONNECT):
		{
        	proto_wifi_connect_t * packet = (proto_wifi_connect_t *) ps_malloc(sizeof(proto_wifi_connect_t));
        	memcpy(packet, data, sizeof(proto_wifi_connect_t));

			xTaskCreate((TaskFunction_t)wifi_connect, "wifi_connect", 1024 * 3, (void *)packet, 24, NULL);
		}
        break;


        case (PROTO_DOWNLOAD_URL):
		{
        	proto_download_url_t * packet = (proto_download_url_t *) ps_malloc(sizeof(proto_download_url_t));
        	memcpy(packet, data, sizeof(proto_download_url_t));

			xTaskCreate((TaskFunction_t)download_url, "download_url", 1024 * 4, (void *)packet, 24, NULL);
		}
        break;

        case (PROTO_DOWNLOAD_STOP):
		{
        	download_stop(((proto_download_stop_t *)data)->data_id);
		}
        break;

        case (PROTO_FANET_BOOT0_CTRL):
		{
        	proto_fanet_boot0_ctrl_t * packet = (proto_fanet_boot0_ctrl_t *)data;
        	fanet_boot0_ctrl(packet);
		}
        break;

        default:
            DBG("Unknown packet");
            DUMP(data, len);
        break;
    }
}