/**
 ****************************************************************************************
 *
 * @file ctsc.c
 *
 * @brief Current Time Service Client Implementation.
 *
 * Copyright (C) 2017-2023 Renesas Electronics Corporation and/or its affiliates.
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

/**
 ****************************************************************************************
 * @addtogroup ctsc
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_CTS_CLIENT)

#include <string.h>

#include "ctsc.h"
#include "ctsc_task.h"
#include "gap.h"
#include "prf_utils.h"
#include "arch.h"

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Initialization of the CTSC module.
 * This function performs all the initializations of the Profile module.
 *  - Creation of database (if it's a service)
 *  - Allocation of profile required memory
 *  - Initialization of task descriptor to register application
 *      - Task State array
 *      - Number of tasks
 *      - Default task handler
 *
 * @param[out]    env        Collector or Service allocated environment data.
 * @param[in|out] start_hdl  Service start handle (0 - dynamically allocated), only applies for services.
 * @param[in]     app_task   Application task number.
 * @param[in]     sec_lvl    Security level (AUTH, EKS and MI field of @enum attm_svc_perm_mask)
 * @param[in]     param      Configuration parameters of profile collector or service (32 bits aligned)
 *
 * @return status code to know if profile initialization succeed or not.
 ****************************************************************************************
 */
static uint8_t ctsc_init(struct prf_task_env *env, uint16_t *start_hdl, uint16_t app_task, uint8_t sec_lvl, void *params)
{
    uint8_t idx;
    //-------------------- allocate memory required for the profile  ---------------------

    struct ctsc_env_tag *ctsc_env =
        (struct ctsc_env_tag *) ke_malloc(sizeof(struct ctsc_env_tag), KE_MEM_ATT_DB);
    memset(ctsc_env, 0, sizeof(struct ctsc_env_tag));

    // allocate CTSC required environment variable
    env->env = (prf_env_t *) ctsc_env;

    ctsc_env->prf_env.app_task = app_task |
                                 (PERM_GET(sec_lvl, SVC_MI) ? PERM(PRF_MI, ENABLE) : PERM(PRF_MI, DISABLE));
    ctsc_env->prf_env.prf_task = env->task | PERM(PRF_MI, ENABLE);

    // initialize environment variable
    env->id                     = TASK_ID_CTSC;
    env->desc.idx_max           = CTSC_IDX_MAX;
    env->desc.state             = ctsc_env->state;
    env->desc.default_handler   = &ctsc_default_handler;

    for (idx = 0; idx < CTSC_IDX_MAX; idx++)
    {
        ctsc_env->env[idx] = NULL;
        // service is ready, go into an Idle state
        ke_state_set(KE_BUILD_ID(env->task, idx), CTSC_FREE);
    }

    return GAP_ERR_NO_ERROR;
}

/**
 ****************************************************************************************
 * @brief Destruction of the CTSC module - due to a reset for instance.
 * This function clean-up allocated memory (attribute database is destroyed by another
 * procedure)
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 ****************************************************************************************
 */
static void ctsc_destroy(struct prf_task_env *env)
{
    uint8_t idx;
    struct ctsc_env_tag *ctsc_env = (struct ctsc_env_tag *) env->env;

    // cleanup environment variable for each task instances
    for (idx = 0; idx < CTSC_IDX_MAX; idx++)
    {
        if (ctsc_env->env[idx] != NULL)
        {
            ke_free(ctsc_env->env[idx]);
        }
    }

    // free profile environment variables
    env->env = NULL;
    ke_free(ctsc_env);
}

/**
 ****************************************************************************************
 * @brief Handles Connection creation
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 ****************************************************************************************
 */
static void ctsc_create(struct prf_task_env *env, uint8_t conidx)
{
    /* Put CTS Client in Idle state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), CTSC_IDLE);
}

/**
 ****************************************************************************************
 * @brief Handles Disconnection
 *
 * @param[in|out]    env        Collector or Service allocated environment data.
 * @param[in]        conidx     Connection index
 * @param[in]        reason     Detach reason
 ****************************************************************************************
 */
static void ctsc_cleanup(struct prf_task_env *env, uint8_t conidx, uint8_t reason)
{
    struct ctsc_env_tag *ctsc_env = (struct ctsc_env_tag *) env->env;

    // clean-up environment variable allocated for task instance
    if (ctsc_env->env[conidx] != NULL)
    {
        ke_free(ctsc_env->env[conidx]);
        ctsc_env->env[conidx] = NULL;
    }

    /* Put CTS Client in Free state */
    ke_state_set(KE_BUILD_ID(env->task, conidx), CTSC_FREE);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// CTSC Task interface required by profile manager
const struct prf_task_cbs ctsc_itf =
{
    ctsc_init,
    ctsc_destroy,
    ctsc_create,
    ctsc_cleanup,
};

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

const struct prf_task_cbs *ctsc_prf_itf_get(void)
{
    return &ctsc_itf;
}

void ctsc_enable_rsp_send(struct ctsc_env_tag *ctsc_env, uint8_t conidx, uint8_t status)
{
    // Send APP the details of the discovered attributes on CTSC
    struct ctsc_enable_rsp *rsp = KE_MSG_ALLOC(CTSC_ENABLE_RSP,
                                               prf_dst_task_get(&ctsc_env->prf_env, conidx),
                                               prf_src_task_get(&ctsc_env->prf_env, conidx),
                                               ctsc_enable_rsp);

    rsp->status = status;

    if (status == GAP_ERR_NO_ERROR)
    {
        rsp->cts = ctsc_env->env[conidx]->cts;

        // Register CTSC task in gatt for indication/notifications
        prf_register_atthdl2gatt(&ctsc_env->prf_env, conidx, &(ctsc_env->env[conidx]->cts.svc));

        // Go to connected state
        ke_state_set(prf_src_task_get(&ctsc_env->prf_env, conidx), CTSC_IDLE);
    }
    else
    {
        // clean-up environment variable allocated for task instance
        ke_free(ctsc_env->env[conidx]);
        ctsc_env->env[conidx] = NULL;
    }

    KE_MSG_SEND(rsp);
}

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

uint8_t ctsc_get_char_info(uint8_t conidx, enum ctsc_char char_code,
                           struct prf_char_inf *char_inf)
{
    struct ctsc_env_tag *ctsc_env = PRF_ENV_GET(CTSC, ctsc);
    ASSERT_ERROR(ctsc_env);

    if (ctsc_env->env[conidx] != NULL &&
        ctsc_env->env[conidx]->cts.chars[char_code].val_hdl != 0 &&
        char_inf != NULL)
    {
        *char_inf = ctsc_env->env[conidx]->cts.chars[char_code];
        return GAP_ERR_NO_ERROR;
    }
    else
    {
        return ATT_ERR_APP_ERROR;
    }
}

void ctsc_pack_curr_time_value(const struct cts_curr_time *p_curr_time_val, uint8_t *p_pckd_time)
{
    // Date-Time
    prf_pack_date_time(p_pckd_time, &(p_curr_time_val->exact_time_256.day_date_time.date_time));

    // Day of Week
    *(p_pckd_time + 7) = p_curr_time_val->exact_time_256.day_date_time.day_of_week;

    // Fraction 256
    *(p_pckd_time + 8) = p_curr_time_val->exact_time_256.fraction_256;

    // Adjust Reason
    *(p_pckd_time + 9) = p_curr_time_val->adjust_reason;
}

void ctsc_unpack_curr_time_value(const uint8_t *p_pckd_time, struct cts_curr_time *p_curr_time_val)
{
    // Date-Time
    p_pckd_time += prf_unpack_date_time((uint8_t *) p_pckd_time, &p_curr_time_val->exact_time_256.day_date_time.date_time);

    // Day of Week
    p_curr_time_val->exact_time_256.day_date_time.day_of_week = *p_pckd_time++;

    // Fraction 256
    p_curr_time_val->exact_time_256.fraction_256 = *p_pckd_time++;

    // Adjust Reason
    p_curr_time_val->adjust_reason = *p_pckd_time++;
}

#endif // BLE_CTS_CLIENT

/// @} CTSC
