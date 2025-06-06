/**
 ****************************************************************************************
 *
 * @file atts.h
 *
 * @brief Header file - ATTS.
 *
 * Copyright (C) RivieraWaves 2009-2014
 *
 *
 ****************************************************************************************
 */

#ifndef ATTS_H_
#define ATTS_H_

/**
 ****************************************************************************************
 * @addtogroup ATTS Attribute Server
 * @ingroup ATT
 * @brief Attribute Protocol Server
 *
 * The ATTS module is responsible for handling messages intended for the attribute
 * profile server. It has defined interfaces with @ref ATTM "ATTM".
 *
 *
 * @{
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"
#include "l2cc_pdu.h"
#if (BLE_ATTS)
#include "att.h"
#include "compiler.h"
#include <stdbool.h>
#include "co_list.h"
#include "gattc_task.h"

/*
 * DEFINES
 ****************************************************************************************
 */



/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Attribute server environment variables
struct atts_env
{
    /// This is used to merge save all the prepare write request received ,
    /// before receiving the execute or cancel or disconnection.
    struct co_list             prep_wr_req_list;
    /// This list is used to put any data in order to send a response to peer device
    struct co_list             rsp;
#if defined (__DA14531__)
    /// Stores all sent GATTC_WRITE_REQ_IND for which GATTC_WRITE_CFM is pending
    struct co_list             pending_wr_ind_list;
#endif /* __DA14531__ */
    /// This structure is used to store in cache latest attribute read value
    struct gattc_read_cfm*     read_cache;
    /// pointer to the PDU which is currently handled by ATTS
    struct l2cc_pdu_recv_ind*  pdu;
};

#if defined (__DA14531__)
struct atts_pending_wr_ind
{
    /// List header for chaining
    struct co_list_hdr hdr;
    /// ATT operation code
    uint8_t att_code;
    /// Attribute handle
    uint16_t handle;
};
#endif /* __DA14531__ */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */


#if (RW_BLE_USE_CRYPT)
/**
 * Confirmation of write signed (signature verified)
 *
 * @param[in] conidx    Connection Index
 * @param[in] length    Signed Data Length
 * @param[in] sign_data Signed Data
 */
void atts_write_signed_cfm(uint8_t conidx, uint16_t length, uint8_t* sign_data);
#endif // (RW_BLE_USE_CRYPT)


/**
 ****************************************************************************************
 * @brief Send an attribute error response to peer.
 *
 * @param[in] conidx            Index of the connection with the peer device
 * @param[in] opcode            failing operation code
 * @param[in] uuid              attribute UUID
 * @param[in] error             error code
 ****************************************************************************************
 */
void atts_send_error(uint8_t conidx, uint8_t opcode, uint16_t uuid, uint8_t error);


/**
 ****************************************************************************************
 * @brief Format the Write Response PDU and send it after receiving a Write Request PDU
 * @param[in] conidx Index of the connection with the peer device
 * @param[in] atthdl  Attribute handle for which to send the response
 * @param[in] status  ATT error code
 ****************************************************************************************
 */
void atts_write_rsp_send(uint8_t conidx, uint16_t atthdl, uint8_t status);


/**
 ****************************************************************************************
 * @brief Sends a value notification/indication command
 *
 * @param[in] conidx    Connection Index
 * @param[in] event     Event parameters to send
 *
 * @return If notification can be sent or not
 ****************************************************************************************
 */
uint8_t atts_send_event(uint8_t conidx, struct gattc_send_evt_cmd *event);


/**
 * @brief  Clear allocated prepare write temporary data.
 *
 * @param[in] conidx            connection index
 */
void atts_clear_prep_data(uint8_t conidx);


/**
 * @brief  Clear allocated temporary data used for ATTS response.
 *
 * @param[in] conidx            connection index
 */
void atts_clear_rsp_data(uint8_t conidx);

/**
 * @brief  Clear temporary attribute read data present in cache.
 *
 * @param[in] conidx            connection index
 */
void atts_clear_read_cache(uint8_t conidx);

#if defined (__DA14531__)
void atts_clear_pending_write_ind_data(uint8_t conidx);
#endif /* __DA14531__ */

/// @} ATTS
#endif /* (BLE_ATTS) */

#if (BLE_CENTRAL || BLE_PERIPHERAL)
/**
 ****************************************************************************************
 * @brief Handles reception of PDU Packet
 *
 * @param[in] conidx  Index of the connection with the peer device
 * @param[in] param   Received PDU Packet
 *
 * @return If message has been proceed or consumed
 ****************************************************************************************
 */
int atts_l2cc_pdu_recv_handler(uint8_t conidx, struct l2cc_pdu_recv_ind *param);
#endif /* #if (BLE_CENTRAL || BLE_PERIPHERAL) */

#endif // ATTS_H_
