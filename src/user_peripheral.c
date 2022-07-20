/**
****************************************************************************************
* @file user_peripheral.c
* @brief Peripheral project source code.
****************************************************************************************
*/
/**
****************************************************************************************
* @addtogroup APP
* @{
****************************************************************************************
*/
#include "rwip_config.h" // SW configuration
#include "gap.h"
#include "app_easy_timer.h"
#include "user_peripheral.h"
#include "user_custs1_impl.h"
#include "user_custs1_def.h"
#include "co_bt.h"

// Include for UART TX and GPIO, for Initial Project Configuration
#include "arch_console.h"

// Include for General Purpose ADC
#include "adc.h"
#include "adc_58x.h"


// Manufacturer Specific Data ADV structure type
struct mnf_specific_data_ad_structure {
    uint8_t ad_structure_size;
    uint8_t ad_structure_type;
    uint8_t company_id[APP_AD_MSD_COMPANY_ID_LEN];
    uint8_t proprietary_data[APP_AD_MSD_DATA_LEN];
};

/*
* GLOBAL VARIABLE DEFINITIONS
****************************************************************************************
*/
uint8_t app_connection_idx                      __SECTION_ZERO("retention_mem_area0");
timer_hnd app_adv_data_update_timer_used        __SECTION_ZERO("retention_mem_area0");
timer_hnd app_param_update_request_timer_used   __SECTION_ZERO("retention_mem_area0");

// Addition for timing of ADC Readings
ke_msg_id_t timer_used                           __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY


// Retained variables
struct mnf_specific_data_ad_structure mnf_data  __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY
// Index of manufacturer data in advertising data or scan response data (when MSB is 1)
uint8_t mnf_data_index                          __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY
uint8_t stored_adv_data_len                     __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY
uint8_t stored_scan_rsp_data_len                __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY
uint8_t stored_adv_data[ADV_DATA_LEN]           __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY
uint8_t stored_scan_rsp_data[SCAN_RSP_DATA_LEN] __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY

/*
* FUNCTION DEFINITIONS
****************************************************************************************
*/
// static uint16_t gpadc_read(void);
// static uint16_t gpadc_sample_to_mv(uint16_t sample);
// static timer_hnd timer_id __attribute__((section("retention_mem_area0"),zero_init));


void user_on_init(void) {
   default_app_on_init();
}


void user_on_set_dev_config_complete(void) {
    default_app_on_set_dev_config_complete();
}


static uint16_t gpadc_read(void) {
    adc_config_t adc_cfg = {                   // Initialize the ADC
        .mode = ADC_INPUT_MODE_SINGLE_ENDED,
        .input = ADC_INPUT_SE_P0_2,            // Can use P0_0 to P0_3
        .attn = true,
        .sign = false,
        // .oversampling = 0,
        // .chopping = false,
        // .input_attenuator = ADC_INPUT_ATTN_4X,
    };
    adc_init(&adc_cfg);

    /* Perform offset calibration of the ADC */
    adc_offset_calibrate(ADC_INPUT_MODE_SINGLE_ENDED);

    adc_enable();

    // adc_start();
    // uint16_t result = adc_correct_sample(adc_get_sample());

    uint16_t result = adc_get_sample();
    adc_disable();

    return (result);
}


/**
 ****************************************************************************************
 * @brief Initialize Manufacturer Specific Data
 ****************************************************************************************
 */
static void mnf_data_init() {
    mnf_data.ad_structure_size = sizeof(struct mnf_specific_data_ad_structure ) - sizeof(uint8_t); // minus the size of the ad_structure_size field
    mnf_data.ad_structure_type = GAP_AD_TYPE_MANU_SPECIFIC_DATA;
    mnf_data.company_id[0] = APP_AD_MSD_COMPANY_ID & 0xFF; // LSB
    mnf_data.company_id[1] = (APP_AD_MSD_COMPANY_ID >> 8 )& 0xFF; // MSB
    mnf_data.proprietary_data[0] = 0;
    mnf_data.proprietary_data[1] = 0;
}

/**
 ****************************************************************************************
 * @brief Update Manufacturer Specific Data
 ****************************************************************************************
 */
static void mnf_data_update() {
    uint16_t data;
    data = mnf_data.proprietary_data[0] | (mnf_data.proprietary_data[1] << 8);
    data += 1;
    mnf_data.proprietary_data[0] = data & 0xFF;
    mnf_data.proprietary_data[1] = (data >> 8) & 0xFF;

    if (data == 0xFFFF) {
         mnf_data.proprietary_data[0] = 0;
         mnf_data.proprietary_data[1] = 0;
    }
}

/**
 ****************************************************************************************
 * @brief Add an AD structure in the Advertising or Scan Response Data of the
 *        GAPM_START_ADVERTISE_CMD parameter struct.
 * @param[in] cmd               GAPM_START_ADVERTISE_CMD parameter struct
 * @param[in] ad_struct_data    AD structure buffer
 * @param[in] ad_struct_len     AD structure length
 * @param[in] adv_connectable   Connectable advertising event or not. It controls whether
 *                              the advertising data use the full 31 bytes length or only
 *                              28 bytes (Document CCSv6 - Part 1.3 Flags).
 ****************************************************************************************
 */
static void app_add_ad_struct(struct gapm_start_advertise_cmd *cmd, void *ad_struct_data, uint8_t ad_struct_len, uint8_t adv_connectable) {
    uint8_t adv_data_max_size = (adv_connectable) ? (ADV_DATA_LEN - 3) : (ADV_DATA_LEN);

    if ((adv_data_max_size - cmd->info.host.adv_data_len) >= ad_struct_len) {
        // Append manufacturer data to advertising data
        memcpy(&cmd->info.host.adv_data[cmd->info.host.adv_data_len], ad_struct_data, ad_struct_len);

        // Update Advertising Data Length
        cmd->info.host.adv_data_len += ad_struct_len;

        // Store index of manufacturer data which are included in the advertising data
        mnf_data_index = cmd->info.host.adv_data_len - sizeof(struct mnf_specific_data_ad_structure);
    }
    else if ((SCAN_RSP_DATA_LEN - cmd->info.host.scan_rsp_data_len) >= ad_struct_len) {
        // Append manufacturer data to scan response data
        memcpy(&cmd->info.host.scan_rsp_data[cmd->info.host.scan_rsp_data_len], ad_struct_data, ad_struct_len);

        // Update Scan Response Data Length
        cmd->info.host.scan_rsp_data_len += ad_struct_len;

        // Store index of manufacturer data which are included in the scan response data
        mnf_data_index = cmd->info.host.scan_rsp_data_len - sizeof(struct mnf_specific_data_ad_structure);
        // Mark that manufacturer data is in scan response and not advertising data
        mnf_data_index |= 0x80;
    }
    else {
        // Manufacturer Specific Data do not fit in either Advertising Data or Scan Response Data
        ASSERT_WARNING(0);
    }
    // Store advertising data length
    stored_adv_data_len = cmd->info.host.adv_data_len;
    // Store advertising data
    memcpy(stored_adv_data, cmd->info.host.adv_data, stored_adv_data_len);
    // Store scan response data length
    stored_scan_rsp_data_len = cmd->info.host.scan_rsp_data_len;
    // Store scan_response data
    memcpy(stored_scan_rsp_data, cmd->info.host.scan_rsp_data, stored_scan_rsp_data_len);
}

/**
 ****************************************************************************************
 * @brief Advertisement data update timer callback function.
 ****************************************************************************************
*/
static void adv_data_update_timer_cb() {
    // If mnd_data_index has MSB set, manufacturer data is stored in scan response
    uint8_t *mnf_data_storage = (mnf_data_index & 0x80) ? stored_scan_rsp_data : stored_adv_data;

    // Update manufacturer data
    mnf_data_update();

    // Update the selected fields of the advertising data (manufacturer data)
    memcpy(mnf_data_storage + (mnf_data_index & 0x7F), &mnf_data, sizeof(struct mnf_specific_data_ad_structure));

    // Update advertising data on the fly
    app_easy_gap_update_adv_data(stored_adv_data, stored_adv_data_len, stored_scan_rsp_data, stored_scan_rsp_data_len);

    // Restart timer for the next advertising update
    app_adv_data_update_timer_used = app_easy_timer(APP_ADV_DATA_UPDATE_TO, adv_data_update_timer_cb);
}

/**
 ****************************************************************************************
 * @brief Parameter update request timer callback function.
 ****************************************************************************************
*/
static void param_update_request_timer_cb() {
    app_easy_gap_param_update_start(app_connection_idx);
    app_param_update_request_timer_used = EASY_TIMER_INVALID_TIMER;
}

void app_adcval1_timer_cb_handler() {
    struct custs1_val_ntf_ind_req *req = KE_MSG_ALLOC_DYN(CUSTS1_VAL_NTF_REQ,
                                                          prf_get_task_from_id(TASK_ID_CUSTS1),
                                                          TASK_APP,
                                                          custs1_val_ntf_ind_req,
                                                          DEF_SVC1_ADC_VAL_1_CHAR_LEN);
    uint16_t out[100];
    for (int i = 0; i < 100; i++) {
        // uint16_t output = gpadc_sample_to_mv(gpadc_read()); // Get uint16_t ADC reading
        uint16_t output = gpadc_read();
        out[i] = output;
    }

    req->handle = SVC1_IDX_ADC_VAL_1_VAL;                   // Location to send it to
    req->length = DEF_SVC1_ADC_VAL_1_CHAR_LEN;
    req->notification = true;
    memcpy(req->value, &out, DEF_SVC1_ADC_VAL_1_CHAR_LEN);
    ke_msg_send(req);

    if (ke_state_get(TASK_APP) == APP_CONNECTED) {
        timer_used = app_easy_timer(100, app_adcval1_timer_cb_handler);
    };
}


void user_app_init(void) {
    app_param_update_request_timer_used = EASY_TIMER_INVALID_TIMER;
    mnf_data_init();                                                       // Initialize Manufacturer Specific Data
    memcpy(stored_adv_data, USER_ADVERTISE_DATA, USER_ADVERTISE_DATA_LEN); // Init. Advertising and Scan Response Data
    stored_adv_data_len = USER_ADVERTISE_DATA_LEN;
    memcpy(stored_scan_rsp_data, USER_ADVERTISE_SCAN_RESPONSE_DATA, USER_ADVERTISE_SCAN_RESPONSE_DATA_LEN);
    stored_scan_rsp_data_len = USER_ADVERTISE_SCAN_RESPONSE_DATA_LEN;
    default_app_on_init();
}


void user_app_adv_start(void) {
    app_adv_data_update_timer_used = app_easy_timer(APP_ADV_DATA_UPDATE_TO, adv_data_update_timer_cb);
    struct gapm_start_advertise_cmd* cmd;
    cmd = app_easy_gap_undirected_advertise_get_active();
    app_add_ad_struct(cmd, &mnf_data, sizeof(struct mnf_specific_data_ad_structure), 1);
    app_easy_gap_undirected_advertise_start();
}


void user_app_connection(uint8_t connection_idx, struct gapc_connection_req_ind const *param) {
    if (app_env[connection_idx].conidx != GAP_INVALID_CONIDX) {
        app_connection_idx = connection_idx;
        app_easy_timer_cancel(app_adv_data_update_timer_used);             // Stop advertising data update timer
        if ((param->con_interval < user_connection_param_conf.intv_min) || // Check if parameters are preferred ones.
            (param->con_interval > user_connection_param_conf.intv_max) || // If not, schedule connect. param. update req.
            (param->con_latency != user_connection_param_conf.latency)  ||
            (param->sup_to != user_connection_param_conf.time_out)) {
            app_param_update_request_timer_used = app_easy_timer(APP_PARAM_UPDATE_REQUEST_TO, param_update_request_timer_cb);
        }
    }
    else {
        user_app_adv_start();
    } // No connection has been established, restart advertising

    default_app_on_connection(connection_idx, param);             // Default app callback on connection
    timer_used = app_easy_timer(100, app_adcval1_timer_cb_handler); // Begin collection of ADC readings
}


void user_app_adv_undirect_complete(uint8_t status) {
    // If advertising was canceled then update advertising data and start advertising again
    if (status == GAP_ERR_CANCELED) {
        user_app_adv_start();
    }
}


void user_app_disconnect(struct gapc_disconnect_ind const *param) {
    if (app_param_update_request_timer_used != EASY_TIMER_INVALID_TIMER) { // Cancel the parameter update request timer
        app_easy_timer_cancel(app_param_update_request_timer_used);
        app_param_update_request_timer_used = EASY_TIMER_INVALID_TIMER;
    }
    mnf_data_update();       // Update manufacturer data for the next advertsing event
    user_app_adv_start();    // Restart Advertising
}



void user_catch_rest_hndl(ke_msg_id_t const msgid,
                          void const *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id) {
    switch(msgid) {
        case CUSTS1_VAL_WRITE_IND:
        {
            struct custs1_val_write_ind const *msg_param = (struct custs1_val_write_ind const *)(param);

            switch (msg_param->handle) {
                case SVC1_IDX_CONTROL_POINT_VAL:
                    user_svc1_ctrl_wr_ind_handler(msgid, msg_param, dest_id, src_id);
                    break;
                case SVC1_IDX_LED_STATE_VAL:
                    user_svc1_led_wr_ind_handler(msgid, msg_param, dest_id, src_id);
                    break;
                case SVC1_IDX_ADC_VAL_1_NTF_CFG:
                    user_svc1_adc_val_1_cfg_ind_handler(msgid, msg_param, dest_id, src_id);
                    break;
                case SVC1_IDX_BUTTON_STATE_NTF_CFG:
                    user_svc1_button_cfg_ind_handler(msgid, msg_param, dest_id, src_id);
                    break;
                case SVC1_IDX_INDICATEABLE_IND_CFG:
                    user_svc1_long_val_cfg_ind_handler(msgid, msg_param, dest_id, src_id);
                    break;
                case SVC1_IDX_LONG_VALUE_NTF_CFG:
                    user_svc1_long_val_cfg_ind_handler(msgid, msg_param, dest_id, src_id);
                    break;
                case SVC1_IDX_LONG_VALUE_VAL:
                    user_svc1_long_val_wr_ind_handler(msgid, msg_param, dest_id, src_id);
                    break;
                default:
                    break;
            }
        } break;

        case CUSTS1_VAL_NTF_CFM:
        {
            struct custs1_val_ntf_cfm const *msg_param = (struct custs1_val_ntf_cfm const *)(param);
            switch (msg_param->handle) {
                case SVC1_IDX_ADC_VAL_1_VAL:
                    break;
                case SVC1_IDX_BUTTON_STATE_VAL:
                    break;
                case SVC1_IDX_LONG_VALUE_VAL:
                    break;
                default:
                    break;
            }
        } break;

        case CUSTS1_VAL_IND_CFM: {
            struct custs1_val_ind_cfm const *msg_param = (struct custs1_val_ind_cfm const *)(param);

            switch (msg_param->handle) {
                case SVC1_IDX_INDICATEABLE_VAL:
                    break;
                default:
                    break;
            }
        } break;

        case CUSTS1_ATT_INFO_REQ: {
            struct custs1_att_info_req const *msg_param = (struct custs1_att_info_req const *)param;
            switch (msg_param->att_idx) {
                case SVC1_IDX_LONG_VALUE_VAL:
                    user_svc1_long_val_att_info_req_handler(msgid, msg_param, dest_id, src_id);
                    break;
                default:
                    user_svc1_rest_att_info_req_handler(msgid, msg_param, dest_id, src_id);
                    break;
            }
        } break;

        case GAPC_PARAM_UPDATED_IND: {
            // Cast the "param" pointer to the appropriate message structure
            struct gapc_param_updated_ind const *msg_param = (struct gapc_param_updated_ind const *)(param);

            // Check if updated Conn Params filled to preferred ones
            if ((msg_param->con_interval >= user_connection_param_conf.intv_min) &&
                (msg_param->con_interval <= user_connection_param_conf.intv_max) &&
                (msg_param->con_latency == user_connection_param_conf.latency) &&
                (msg_param->sup_to == user_connection_param_conf.time_out)) {}
        } break;

        case CUSTS1_VALUE_REQ_IND: {
            struct custs1_value_req_ind const *msg_param = (struct custs1_value_req_ind const *) param;

            switch (msg_param->att_idx) {
                case SVC3_IDX_READ_4_VAL:
                    {user_svc3_read_non_db_val_handler(msgid, msg_param, dest_id, src_id);}
                    break;
                default:
                {
                    // Send Error message
                    struct custs1_value_req_rsp *rsp = KE_MSG_ALLOC(CUSTS1_VALUE_REQ_RSP,
                                                                    src_id,
                                                                    dest_id,
                                                                    custs1_value_req_rsp);
                    rsp->conidx  = app_env[msg_param->conidx].conidx;
                    rsp->att_idx = msg_param->att_idx;
                    rsp->length = 0;
                    rsp->status  = ATT_ERR_APP_ERROR;
                    ke_msg_send(rsp);
                } break;
             }
        } break;
        default:
            break;
    }
}

/// @} APP
