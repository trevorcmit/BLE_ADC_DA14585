/**
 ****************************************************************************************
 * @file user_custs1_impl.c
 * @brief Peripheral project Custom1 Server implementation source code.
 ****************************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include "gpio.h"
#include "app_api.h"
#include "app.h"
#include "prf_utils.h"
#include "custs1.h"
#include "custs1_task.h"
#include "user_custs1_def.h"
#include "user_custs1_impl.h"
#include "user_peripheral.h"
#include "user_periph_setup.h"

// For ADC
#include "adc.h"
#include "adc_58x.h"

// ke_msg_id_t timer_used      __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY
uint16_t indication_counter __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY
uint16_t non_db_val_counter __SECTION_ZERO("retention_mem_area0"); //@RETENTION MEMORY

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void user_svc1_ctrl_wr_ind_handler(ke_msg_id_t const msgid,
                                   struct custs1_val_write_ind const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id) {
    uint8_t val = 0;
    memcpy(&val, &param->value[0], param->length);
    SetBits16(SYS_CTRL_REG, SW_RESET, 1);
}

void user_svc1_led_wr_ind_handler(ke_msg_id_t const msgid,
                                  struct custs1_val_write_ind const *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id) {
    uint8_t val = 0;
    memcpy(&val, &param->value[0], param->length);
    if      (val == CUSTS1_LED_ON)  {GPIO_SetActive(GPIO_LED_PORT, GPIO_LED_PIN);}
    else if (val == CUSTS1_LED_OFF) {GPIO_SetInactive(GPIO_LED_PORT, GPIO_LED_PIN);}
}

void user_svc1_long_val_cfg_ind_handler(ke_msg_id_t const msgid,
                                        struct custs1_val_write_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id) {
    if (param->value[0]) { // Generate indication when the central subscribes to it
        uint8_t conidx = KE_IDX_GET(src_id);

        struct custs1_val_ind_req* req = KE_MSG_ALLOC_DYN(CUSTS1_VAL_IND_REQ,
                                                          prf_get_task_from_id(TASK_ID_CUSTS1),
                                                          TASK_APP,
                                                          custs1_val_ind_req,
                                                          sizeof(indication_counter));
        req->conidx = app_env[conidx].conidx;
        req->handle = SVC1_IDX_INDICATEABLE_VAL;
        req->length = sizeof(indication_counter);
        req->value[0] = (indication_counter >> 8) & 0xFF;
        req->value[1] = indication_counter & 0xFF;
        indication_counter++;
        ke_msg_send(req);
    }
}

void user_svc1_long_val_wr_ind_handler(ke_msg_id_t const msgid, struct custs1_val_write_ind const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id) {}
void user_svc1_long_val_ntf_cfm_handler(ke_msg_id_t const msgid, struct custs1_val_write_ind const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id) {}
void user_svc1_adc_val_1_cfg_ind_handler(ke_msg_id_t const msgid, struct custs1_val_write_ind const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id) {}
void user_svc1_adc_val_1_ntf_cfm_handler(ke_msg_id_t const msgid, struct custs1_val_write_ind const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id) {}
void user_svc1_button_cfg_ind_handler(ke_msg_id_t const msgid, struct custs1_val_write_ind const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id) {}
void user_svc1_button_ntf_cfm_handler(ke_msg_id_t const msgid, struct custs1_val_write_ind const *param, ke_task_id_t const dest_id,ke_task_id_t const src_id) {}
void user_svc1_indicateable_cfg_ind_handler(ke_msg_id_t const msgid, struct custs1_val_write_ind const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id) {}
void user_svc1_indicateable_ind_cfm_handler(ke_msg_id_t const msgid, struct custs1_val_write_ind const *param, ke_task_id_t const dest_id, ke_task_id_t const src_id) {}

void user_svc1_long_val_att_info_req_handler(ke_msg_id_t const msgid,
                                             struct custs1_att_info_req const *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id) {
    struct custs1_att_info_rsp *rsp = KE_MSG_ALLOC(CUSTS1_ATT_INFO_RSP,
                                                   src_id,
                                                   dest_id,
                                                   custs1_att_info_rsp);
    rsp->conidx  = app_env[param->conidx].conidx; // Provide the connection index.
    rsp->att_idx = param->att_idx;                // Provide the attribute index.
    rsp->length  = 0;                             // Provide the current length of the attribute.
    rsp->status  = ATT_ERR_NO_ERROR;              // Provide the ATT error code.
    ke_msg_send(rsp);
}

void user_svc1_rest_att_info_req_handler(ke_msg_id_t const msgid,
                                         struct custs1_att_info_req const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id) {
    struct custs1_att_info_rsp *rsp = KE_MSG_ALLOC(CUSTS1_ATT_INFO_RSP,
                                                   src_id,
                                                   dest_id,
                                                   custs1_att_info_rsp);
    rsp->conidx  = app_env[param->conidx].conidx; // Provide the connection index.
    rsp->att_idx = param->att_idx;                // Provide the attribute index.
    rsp->length  = 0;                             // Force current length to zero.
    rsp->status  = ATT_ERR_WRITE_NOT_PERMITTED;   // Provide the ATT error code.
    ke_msg_send(rsp);
}

void user_svc3_read_non_db_val_handler(ke_msg_id_t const msgid,
                                       struct custs1_value_req_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id) {
    // Increase value by one
    non_db_val_counter++;

    struct custs1_value_req_rsp *rsp = KE_MSG_ALLOC_DYN(CUSTS1_VALUE_REQ_RSP,
                                                        prf_get_task_from_id(TASK_ID_CUSTS1),
                                                        TASK_APP,
                                                        custs1_value_req_rsp,
                                                        DEF_SVC3_READ_VAL_4_CHAR_LEN);
    rsp->conidx  = app_env[param->conidx].conidx;
    rsp->att_idx = param->att_idx;
    rsp->length  = sizeof(non_db_val_counter);
    rsp->status  = ATT_ERR_NO_ERROR;
    memcpy(&rsp->value, &non_db_val_counter, rsp->length);
    ke_msg_send(rsp);
}
