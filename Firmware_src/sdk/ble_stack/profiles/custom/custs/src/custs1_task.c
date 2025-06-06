/**
 ****************************************************************************************
 *
 * @file custs1_task.c
 *
 * @brief Custom Server 1 (CUSTS1) profile task source file.
 *
 * Copyright (C) 2014-2023 Renesas Electronics Corporation and/or its affiliates.
 * All rights reserved. Confidential Information.
 *
 * This software ("Software") is supplied by Renesas Electronics Corporation and/or its
 * affiliates ("Renesas"). Renesas grants you a personal, non-exclusive, non-transferable,
 * revocable, non-sub-licensable right and license to use the Software, solely if used in
 * or together with Renesas products. You may make copies of this Software, provided this
 * copyright notice and disclaimer ("Notice") is included in all such copies. Renesas
 * reserves the right to change or discontinue the Software at any time without notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS". RENESAS DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. TO THE
 * MAXIMUM EXTENT PERMITTED UNDER LAW, IN NO EVENT SHALL RENESAS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE, EVEN IF RENESAS HAS BEEN ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGES. USE OF THIS SOFTWARE MAY BE SUBJECT TO TERMS AND CONDITIONS CONTAINED IN
 * AN ADDITIONAL AGREEMENT BETWEEN YOU AND RENESAS. IN CASE OF CONFLICT BETWEEN THE TERMS
 * OF THIS NOTICE AND ANY SUCH ADDITIONAL LICENSE AGREEMENT, THE TERMS OF THE AGREEMENT
 * SHALL TAKE PRECEDENCE. BY CONTINUING TO USE THIS SOFTWARE, YOU AGREE TO THE TERMS OF
 * THIS NOTICE.IF YOU DO NOT AGREE TO THESE TERMS, YOU ARE NOT PERMITTED TO USE THIS
 * SOFTWARE.
 *
 ****************************************************************************************
 */

#include "rwble_config.h"              // SW configuration

#if (BLE_CUSTOM1_SERVER)

#include <string.h>

#include "custs1_task.h"
#include "custs1.h"
#include "custom_common.h"
#include "app_customs.h"
#include "attm_db_128.h"
#include "ke_task.h"
#include "gapc.h"
#include "gapc_task.h"
#include "gattc_task.h"
#include "attm_db.h"
#include "prf_utils.h"
#include "app_prf_types.h"
#include "arch.h"

#if !defined (__DA14531__) || defined (__EXCLUDE_ROM_CUSTS1__)

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Stores characteristic value.
 * @param[in] att_idx  Custom attribute index.
 * @param[in] length   Value length.
 * @param[in] data     Pointer to value data.
 * @return 0 on success.
 ****************************************************************************************
 */
static int custs1_att_set_value(uint8_t att_idx, uint16_t length, const uint8_t *data)
{
    struct custs1_env_tag *custs1_env = PRF_ENV_GET(CUSTS1, custs1);
    ASSERT_ERROR(custs1_env);
    // Check value if already present in service
    struct custs1_val_elmt *val = (struct custs1_val_elmt *)co_list_pick(&(custs1_env->values));
    // loop until value found
    while (val != NULL)
    {
        // value is present in service
        if (val->att_idx == att_idx)
        {
            // Value present but size changed, free old value
            if (length != val->length)
            {
                co_list_extract(&custs1_env->values, &val->hdr, 0);
                ke_free(val);
                val = NULL;
            }
            break;
        }

        val = (struct custs1_val_elmt *)val->hdr.next;
    }

    if (val == NULL)
    {
        // allocate value data
        val = (struct custs1_val_elmt *) ke_malloc(sizeof(struct custs1_val_elmt) + length, KE_MEM_ATT_DB);
        memset(val, 0, sizeof(struct custs1_val_elmt) + length);
        // insert value into the list
        co_list_push_back(&custs1_env->values, &val->hdr);
    }
    val->att_idx = att_idx;
    val->length = length;
    memcpy(val->data, data, length);

    return 0;
}

/**
 ****************************************************************************************
 * @brief Read characteristic value from.
 * Function checks if attribute exists, and if so returns its length and pointer to data.
 * @param[in]  att_idx  Custom attribute index.
 * @param[out] length   Pointer to variable that receives length of the attribute.
 * @param[out] data     Pointer to variable that receives pointer characteristic value.
 * @return 0 on success, ATT_ERR_ATTRIBUTE_NOT_FOUND if there is no value for such attribute.
 ****************************************************************************************
 */
static int custs1_att_get_value(uint8_t att_idx, uint16_t *length, const uint8_t **data)
{
    struct custs1_env_tag *custs1_env = PRF_ENV_GET(CUSTS1, custs1);
    ASSERT_ERROR(custs1_env);
    // Check value if already present in service
    struct custs1_val_elmt *val = (struct custs1_val_elmt *)co_list_pick(&custs1_env->values);
    ASSERT_ERROR(data);
    ASSERT_ERROR(length);

    // loop until value found
    while (val != NULL)
    {
        // value is present in service
        if (val->att_idx == att_idx)
        {
            *length = val->length;
            *data = val->data;
            break;
        }

        val = (struct custs1_val_elmt *)val->hdr.next;
    }

    if (val == NULL)
    {
        *length = 0;
        *data = NULL;
    }
    return val ? 0 : ATT_ERR_ATTRIBUTE_NOT_FOUND;
}

/**
 ****************************************************************************************
 * @brief Sets initial values for all Client Characteristic Configurations.
 * @param[in]  att_db     Custom service attribute definition table.
 * @param[in]  max_nb_att Number of elements in att_db.
 ****************************************************************************************
 */
void custs1_init_ccc_values(const struct attm_desc_128 *att_db, int max_nb_att)
{
    // Default values 0 means no notification
    uint8_t ccc_values[BLE_CONNECTION_MAX] = {0};
    int i;

    // Start form 1, skip service description
    for (i = 1; i < max_nb_att; i++)
    {
        // Find only CCC characteristics
        if (att_db[i].uuid_size == ATT_UUID_16_LEN &&
            *(uint16_t *)att_db[i].uuid == ATT_DESC_CLIENT_CHAR_CFG)
        {
            // Set default values for all possible connections
            custs1_att_set_value(i, sizeof(ccc_values), ccc_values);
        }
    }
}

/**
 ****************************************************************************************
 * @brief Set value of CCC for given attribute and connection index.
 * @param[in] conidx   Connection index.
 * @param[in] att_idx  CCC attribute index.
 * @param[in] cc       Value to store.
 ****************************************************************************************
 */
void custs1_set_ccc_value(uint8_t conidx, uint8_t att_idx, uint16_t ccc)
{
    uint16_t length;
    const uint8_t *value;
    uint8_t new_value[BLE_CONNECTION_MAX];
    ASSERT_ERROR(conidx < BLE_CONNECTION_MAX);

    custs1_att_get_value(att_idx, &length, &value);
    ASSERT_ERROR(length);
    ASSERT_ERROR(value);
    memcpy(new_value, value, length);
    // For now there are only two valid values for ccc, store just one byte other is 0 anyway
    new_value[conidx] = (uint8_t)ccc;
    custs1_att_set_value(att_idx, length, new_value);
}

/**
 ****************************************************************************************
 * @brief Read value of CCC for given attribute and connection index.
 * @param[in]  conidx   Connection index.
 * @param[in]  att_idx  Custom attribute index.
 * @return Value of CCC.
 ****************************************************************************************
 */
static uint16_t custs1_get_ccc_value(uint8_t conidx, uint8_t att_idx)
{
    uint16_t length;
    const uint8_t *value;
    uint16_t ccc_value;

    ASSERT_ERROR(conidx < BLE_CONNECTION_MAX);

    custs1_att_get_value(att_idx, &length, &value);
    ASSERT_ERROR(length);
    ASSERT_ERROR(value);

    ccc_value = value[conidx];

    return ccc_value;
}

static void custs1_exe_operation(void)
{
    struct custs1_env_tag *custs1_env = PRF_ENV_GET(CUSTS1, custs1);
    ASSERT_ERROR(custs1_env);
    bool notification_sent = false;
    const uint8_t *ccc_values;
    uint16_t length;

    struct custs1_val_ntf_ind_req *app_req = (struct custs1_val_ntf_ind_req *)ke_msg2param(custs1_env->operation);

    custs1_att_get_value(custs1_env->ccc_idx, &length, &ccc_values);
    ASSERT_ERROR(length == BLE_CONNECTION_MAX);

    // loop on all connections
    while (!notification_sent && custs1_env->cursor < BLE_CONNECTION_MAX)
    {
        struct gattc_send_evt_cmd *req;
        uint8_t cursor = custs1_env->cursor++;

        // Check if notification or indication is set for connection
        if ((app_req->notification && ((ccc_values[cursor] & PRF_CLI_START_NTF) == 0)) ||
            (!app_req->notification && ((ccc_values[cursor] & PRF_CLI_START_IND) == 0)))
            continue;

        notification_sent = true;

        // Allocate the GATT notification message
        req = KE_MSG_ALLOC_DYN(GATTC_SEND_EVT_CMD,
            KE_BUILD_ID(TASK_GATTC, cursor), custs1_env->operation->dest_id, gattc_send_evt_cmd, app_req->length);

        // Fill in the parameter structure
        req->operation = app_req->notification ? GATTC_NOTIFY : GATTC_INDICATE;
        req->handle = custs1_env->shdl + app_req->handle;
        req->length = app_req->length;
        memcpy(req->value, app_req->value, app_req->length);

        // Send the event
        KE_MSG_SEND(req);
    }

    // check if operation finished
    if (!notification_sent)
    {
        if (app_req->notification)
        {
            // Inform the application that the notification PDU has been sent over the air.
            struct custs1_val_ntf_cfm *cfm = KE_MSG_ALLOC(CUSTS1_VAL_NTF_CFM,
                                                          custs1_env->operation->src_id, custs1_env->operation->dest_id,
                                                          custs1_val_ntf_cfm);
            cfm->handle = app_req->handle;
            cfm->status = GAP_ERR_NO_ERROR;
            KE_MSG_SEND(cfm);
        }
        else
        {
            // Inform the application that the indication has been confirmed by the peer device.
            struct custs1_val_ind_cfm *cfm = KE_MSG_ALLOC(CUSTS1_VAL_IND_CFM,
                                                          custs1_env->operation->src_id, custs1_env->operation->dest_id,
                                                          custs1_val_ind_cfm);
            cfm->handle = app_req->handle;
            cfm->status = GAP_ERR_NO_ERROR;
            KE_MSG_SEND(cfm);
        }
        ke_free(custs1_env->operation);
        custs1_env->operation = NULL;
        ke_state_set(prf_src_task_get(&(custs1_env->prf_env), 0), CUSTS1_IDLE);
    }
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_CMP_EVT message.
 * @details The GATTC_CMP_EVT message that signals the completion of a GATTC_NOTIFY
 *          operation is sent back as soon as the notification PDU has been sent over
 *          the air.
 *          The GATTC_CMP_EVT message that signals the completion of a GATTC_INDICATE
 *          operation is sent back as soon as the ATT_HANDLE_VALUE_CONFIRMATION PDU is
 *          received confirming that the indication has been correctly received by the
 *          peer device.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                 struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    if (param->operation == GATTC_NOTIFY || param->operation == GATTC_INDICATE)
    {
        custs1_exe_operation();
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CUSTS1_VAL_SET_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int custs1_val_set_req_handler(ke_msg_id_t const msgid,
                                      struct custs1_val_set_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    struct custs1_env_tag *custs1_env = PRF_ENV_GET(CUSTS1, custs1);
    ASSERT_ERROR(custs1_env);
    // Update value in DB
    attmdb_att_set_value(custs1_env->shdl + param->handle, param->length, 0, (uint8_t *)&param->value);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CUSTS1_VAL_NTF_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int custs1_val_ntf_req_handler(ke_msg_id_t const msgid,
                                      struct custs1_val_ntf_ind_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    struct custs1_env_tag *custs1_env = PRF_ENV_GET(CUSTS1, custs1);
    ASSERT_ERROR(custs1_env);
    uint16_t ccc_hdl;
    uint16_t handle = custs1_env->shdl + param->handle;
    uint8_t ccc_idx;
    uint8_t status;

    uint8_t state = ke_state_get(dest_id);
    if (state == CUSTS1_BUSY)
    {
        return KE_MSG_SAVED;
    }

    // Find the handle of the Characteristic Client Configuration
    ccc_hdl = get_cfg_handle(handle);
    ASSERT_ERROR(ccc_hdl);

    // Convert handle to index
    status = custs1_get_att_idx(ccc_hdl, &ccc_idx);
    ASSERT_ERROR(status == ATT_ERR_NO_ERROR);

    ke_state_set(dest_id, CUSTS1_BUSY);
    custs1_env->operation = ke_param2msg(param);
    custs1_env->cursor = 0;
    custs1_env->ccc_idx = ccc_idx;

    // Trigger notification
    custs1_exe_operation();

    return KE_MSG_NO_FREE;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CUSTS1_VAL_IND_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int custs1_val_ind_req_handler(ke_msg_id_t const msgid,
                                      struct custs1_val_ind_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    struct custs1_env_tag *custs1_env = PRF_ENV_GET(CUSTS1, custs1);
    ASSERT_ERROR(custs1_env);
    uint16_t ccc_hdl;
    uint16_t handle = custs1_env->shdl + param->handle;
    uint8_t ccc_idx;
    uint8_t status;

    uint8_t state = ke_state_get(dest_id);
    if (state == CUSTS1_BUSY)
    {
        return KE_MSG_SAVED;
    }

    // Find the handle of the Characteristic Client Configuration
    ccc_hdl = get_cfg_handle(handle);
    ASSERT_ERROR(ccc_hdl);

    // Convert handle to index
    status = custs1_get_att_idx(ccc_hdl, &ccc_idx);
    ASSERT_ERROR(status == ATT_ERR_NO_ERROR);

    ke_state_set(dest_id, CUSTS1_BUSY);
    custs1_env->operation = ke_param2msg(param);
    custs1_env->cursor = 0;
    custs1_env->ccc_idx = ccc_idx;

    // Trigger indication
    custs1_exe_operation();

    return KE_MSG_NO_FREE;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CUSTS1_ATT_INFO_RSP message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance (probably unused).
 * @return If the message shall be consumed or not.
 ****************************************************************************************
 */
static int custs1_att_info_rsp_handler(ke_msg_id_t const msgid,
                                       struct custs1_att_info_rsp const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);

    if (state == CUSTS1_IDLE)
    {
        // Extract the service start handle.
        struct custs1_env_tag *custs1_env = PRF_ENV_GET(CUSTS1, custs1);
        ASSERT_ERROR(custs1_env);
        // Allocate the attribute information confirmation message.
        struct gattc_att_info_cfm *cfm;
        cfm = KE_MSG_ALLOC(GATTC_ATT_INFO_CFM, TASK_GATTC, dest_id, gattc_att_info_cfm);
        // Fill the message.
        cfm->handle = custs1_env->shdl + param->att_idx;
        cfm->length = param->length;
        cfm->status = param->status;
        // Send the confirmation message to GATTC.
        KE_MSG_SEND(cfm);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_READ_REQ_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_read_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_read_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    ke_state_t state = ke_state_get(dest_id);

    if(state == CUSTS1_IDLE)
    {
        struct custs1_env_tag *custs1_env = PRF_ENV_GET(CUSTS1, custs1);
        ASSERT_ERROR(custs1_env);
        struct gattc_read_cfm * cfm;
        uint8_t att_idx = 0;
        uint8_t conidx = KE_IDX_GET(src_id);
        // retrieve handle information
        uint8_t status = custs1_get_att_idx(param->handle, &att_idx);
        uint16_t length = 0;
        uint16_t ccc_val = 0;

        // If the attribute has been found, status is GAP_ERR_NO_ERROR
        if (status == GAP_ERR_NO_ERROR)
        {
            const struct cust_prf_func_callbacks *callbacks = custs_get_func_callbacks(TASK_ID_CUSTS1);
            ASSERT_ERROR(callbacks);

            if (callbacks->att_db[att_idx].uuid_size == ATT_UUID_16_LEN &&
                *(uint16_t *)callbacks->att_db[att_idx].uuid == ATT_DESC_CLIENT_CHAR_CFG)
            {
                ccc_val = custs1_get_ccc_value(conidx, att_idx);
                length = 2;
            }
            else
            {
                // Request value from application
                struct custs1_value_req_ind* req_ind = KE_MSG_ALLOC(CUSTS1_VALUE_REQ_IND,
                                                        prf_dst_task_get(&(custs1_env->prf_env), KE_IDX_GET(src_id)),
                                                        dest_id,
                                                        custs1_value_req_ind);

                req_ind->conidx  = KE_IDX_GET(src_id);
                req_ind->att_idx = att_idx;

                // Send message to application
                KE_MSG_SEND(req_ind);

                // Put Service in a busy state
                ke_state_set(dest_id, CUSTS1_BUSY);

                return (KE_MSG_CONSUMED);
            }
        }

        // Send read response
        cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM,
                               src_id,
                               dest_id,
                               gattc_read_cfm,
                               length);

        cfm->handle = param->handle;
        cfm->status = status;
        cfm->length = length;

        if (status == GAP_ERR_NO_ERROR)
        {
            memcpy(cfm->value, &ccc_val, length);
        }

        KE_MSG_SEND(cfm);

        return (KE_MSG_CONSUMED);
    }
    // Postpone request if profile is in a busy state
    else
    {
        return (KE_MSG_SAVED);
    }
}

#endif

// Function is wrong only in DA14531 ROM
#if (defined (__DA14531__) && (!defined (__DA14531_01__) && !defined (__DA14535__))) ||     \
    (!defined (__DA14531__)) ||                                                             \
    ((defined (__DA14531_01__) || defined (__DA14535__)) && defined (__EXCLUDE_ROM_CUSTS1__))
/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_WRITE_REQ_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_write_req_ind_handler(ke_msg_id_t const msgid,
                                       const struct gattc_write_req_ind *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    struct custs1_env_tag *custs1_env = PRF_ENV_GET(CUSTS1, custs1);
    ASSERT_ERROR(custs1_env);
    struct gattc_write_cfm * cfm;
    uint8_t att_idx = 0;
    uint8_t conidx = KE_IDX_GET(src_id);
    // retrieve handle information
    uint8_t status = custs1_get_att_idx(param->handle, &att_idx);
    att_perm_type perm;

    ASSERT_ERROR(param->offset == 0);
    // If the attribute has been found, status is ATT_ERR_NO_ERROR
    if (status == ATT_ERR_NO_ERROR)
    {
        const struct cust_prf_func_callbacks *callbacks = custs_get_func_callbacks(TASK_ID_CUSTS1);
        ASSERT_ERROR(callbacks);

        if (callbacks->att_db[att_idx].uuid_size == ATT_UUID_16_LEN &&
            *(uint16_t *)callbacks->att_db[att_idx].uuid == ATT_DESC_CLIENT_CHAR_CFG)
        {
            struct attm_elmt elem = {0};

            // Find the handle of the Characteristic Value
            uint16_t value_hdl = get_value_handle(param->handle);
            ASSERT_ERROR(value_hdl);

            // Get permissions to identify if it is NTF or IND.
            attmdb_att_get_permission(value_hdl, &perm, 0, &elem);
            status = check_client_char_cfg(PERM_IS_SET(perm, NTF, ENABLE), param);

            if (status == ATT_ERR_NO_ERROR)
            {
                custs1_set_ccc_value(conidx, att_idx, *(uint16_t *)param->value);
            }
        }
        else
        {
            if (callbacks != NULL && callbacks->value_wr_validation_func != NULL)
            {
                status = callbacks->value_wr_validation_func(att_idx, param->offset, param->length, (uint8_t *)&param->value[0]);
            }

            struct attm_elmt elmt = ATT_ELEMT_INIT;
            attmdb_get_attribute(param->handle, &elmt);

            // Check if data present in database
            if ((status == ATT_ERR_NO_ERROR) && (PERM_GET(elmt.info.att->info.max_lengh, RI) == 0))
            {
                // Set value in the database
                status = attmdb_att_set_value(param->handle, param->length, param->offset, (uint8_t *)&param->value[0]);
            }
        }

        if (status == ATT_ERR_NO_ERROR)
        {
            // Inform APP
            struct custs1_val_write_ind *req_id = KE_MSG_ALLOC_DYN(CUSTS1_VAL_WRITE_IND,
                                                                   prf_dst_task_get(&(custs1_env->prf_env), KE_IDX_GET(src_id)),
                                                                   dest_id,
                                                                   custs1_val_write_ind,
                                                                   param->length);
            memcpy(req_id->value, param->value, param->length);
            req_id->handle = att_idx;
            req_id->length = param->length;
            req_id->conidx = conidx;

            KE_MSG_SEND(req_id);
        }
    }

    //Send write response
    cfm = KE_MSG_ALLOC(GATTC_WRITE_CFM, src_id, dest_id, gattc_write_cfm);
    cfm->handle = param->handle;
    cfm->status = status;
    KE_MSG_SEND(cfm);

    return (KE_MSG_CONSUMED);
}
#endif

#if !defined (__DA14531__) || defined (__EXCLUDE_ROM_CUSTS1__)

/**
 ****************************************************************************************
 * @brief Handles reception of the attribute info request message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance (probably unused).
 * @return If the message shall be consumed or not.
 ****************************************************************************************
 */
static int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid,
                                          struct gattc_att_info_req_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);

    if (state == CUSTS1_IDLE)
    {
        // Extract the service start handle.
        struct custs1_env_tag *custs1_env = PRF_ENV_GET(CUSTS1, custs1);
        ASSERT_ERROR(custs1_env);
        // Allocate the attribute information request message to be processed by
        // the application.
        struct custs1_att_info_req *req = KE_MSG_ALLOC(CUSTS1_ATT_INFO_REQ,
                                                       TASK_APP,
                                                       dest_id,
                                                       custs1_att_info_req);
        // Fill the message.
        req->conidx  = KE_IDX_GET(src_id);
        ASSERT_ERROR((req->conidx) < BLE_CONNECTION_MAX);
        req->att_idx = param->handle - custs1_env->shdl;
        // Send the message to the application.
        KE_MSG_SEND(req);
    }

    return (KE_MSG_CONSUMED);
}
#endif

// Function is wrong only in DA14531 and DA14531-01 ROM
#if (defined (__DA14531__) && !defined (__DA14535__)) ||    \
    (!defined (__DA14531__)) ||                             \
    (defined (__DA14535__) && defined (__EXCLUDE_ROM_CUSTS1__))
/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CUSTS1_VALUE_REQ_RSP message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance (probably unused).
 * @return If the message shall be consumed or not.
 ****************************************************************************************
 */
static int custs1_value_req_rsp_handler(ke_msg_id_t const msgid,
                                        struct custs1_value_req_rsp *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{

    ke_state_t state = ke_state_get(dest_id);

    if(state == CUSTS1_BUSY)
    {
        if (param->status == ATT_ERR_NO_ERROR)
        {
            // Send value to peer device.
            struct gattc_read_cfm* cfm = KE_MSG_ALLOC_DYN(GATTC_READ_CFM,
                                                          KE_BUILD_ID(TASK_GATTC, param->conidx),
                                                          dest_id,
                                                          gattc_read_cfm,
                                                          param->length);
            // Fill parameters
            cfm->handle = custs1_get_att_handle(param->att_idx);
            cfm->status = ATT_ERR_NO_ERROR;
            cfm->length = param->length;
            memcpy(cfm->value, param->value, param->length);

            // Send message to GATTC
            KE_MSG_SEND(cfm);
        }
        else
        {
            // Application error, value provided by application is not the expected one
            struct gattc_read_cfm* cfm = KE_MSG_ALLOC(GATTC_READ_CFM,
                                                      KE_BUILD_ID(TASK_GATTC, param->conidx),
                                                      dest_id,
                                                      gattc_read_cfm);

            // Fill parameters
            cfm->handle = custs1_get_att_handle(param->att_idx);
            cfm->status = param->status;

            // Send message to GATTC
            KE_MSG_SEND(cfm);
        }

        // Return to idle state
        ke_state_set(dest_id, CUSTS1_IDLE);
    }
    return (KE_MSG_CONSUMED);
}

#endif

// DA14531 using ROM SDK symbols (for DA14531 and DA14531_01)
#if (defined (__DA14531__) && (!defined (__DA14535__)))
#if !defined(__EXCLUDE_ROM_CUSTS1__)
extern int gattc_read_req_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_read_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id);

extern int gattc_att_info_req_ind_handler(ke_msg_id_t const msgid,
                                          struct gattc_att_info_req_ind *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id);

extern int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                 struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id);

extern int custs1_val_ntf_req_handler(ke_msg_id_t const msgid,
                                      struct custs1_val_ntf_ind_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id);

extern int custs1_val_set_req_handler(ke_msg_id_t const msgid,
                                      struct custs1_val_set_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id);

extern int custs1_val_ind_req_handler(ke_msg_id_t const msgid,
                                      struct custs1_val_ind_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id);

extern int custs1_att_info_rsp_handler(ke_msg_id_t const msgid,
                                       struct custs1_att_info_rsp const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id);

#endif
#endif

#if defined (__DA14531_01__) && !defined(__EXCLUDE_ROM_CUSTS1__)
extern int gattc_write_req_ind_handler(ke_msg_id_t const msgid,
                                       const struct gattc_write_req_ind *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id);
#endif

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

#if (defined (__DA14531__) && !defined (__DA14535__)) ||    \
    (!defined (__DA14531__)) ||                             \
    (defined (__DA14535__) && defined (__EXCLUDE_ROM_CUSTS1__))
/// Default State handlers definition
const struct ke_msg_handler custs1_default_state[] =
{
    {GATTC_READ_REQ_IND,            (ke_msg_func_t)gattc_read_req_ind_handler},
    {GATTC_WRITE_REQ_IND,           (ke_msg_func_t)gattc_write_req_ind_handler},
    {GATTC_ATT_INFO_REQ_IND,        (ke_msg_func_t)gattc_att_info_req_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},
    {CUSTS1_VAL_NTF_REQ,            (ke_msg_func_t)custs1_val_ntf_req_handler},
    {CUSTS1_VAL_SET_REQ,            (ke_msg_func_t)custs1_val_set_req_handler},
    {CUSTS1_VAL_IND_REQ,            (ke_msg_func_t)custs1_val_ind_req_handler},
    {CUSTS1_ATT_INFO_RSP,           (ke_msg_func_t)custs1_att_info_rsp_handler},
    {CUSTS1_VALUE_REQ_RSP,          (ke_msg_func_t)custs1_value_req_rsp_handler},
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler custs1_default_handler = KE_STATE_HANDLER(custs1_default_state);
#endif

#endif // BLE_CUSTOM1_SERVER
