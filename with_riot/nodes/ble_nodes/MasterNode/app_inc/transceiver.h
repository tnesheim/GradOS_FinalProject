/*
 * Copyright (C) 2010 - 2014 Oliver Hahm
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_transceiver Transceiver
 * @ingroup     sys
 *
 * @brief       The transceiver module implements a generic link abstraction to
 *              any radio interface.
 *
 * @{
 *
 * @file
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 */

#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

/**
 * @brief Message types for transceiver interface
 */
enum transceiver_msg_type_t {
    /* Message types for driver <-> transceiver communication */
    RCV_PKT_CC1020,        /**< packet was received by CC1020 transceiver */
    RCV_PKT_CC1100,        /**< packet was received by CC1100 transceiver */
    RCV_PKT_CC2420,        /**< packet was received by CC2420 transceiver */
    RCV_PKT_MC1322X,       /**< packet was received by mc1322x transceiver */
    RCV_PKT_NATIVE,        /**< packet was received by native transceiver */
    RCV_PKT_AT86RF231,     /**< packet was received by AT86RF231 transceiver */
    RCV_PKT_NRF51822BLE,

    /* Message types for transceiver <-> upper layer communication */
    PKT_PENDING,    /**< packet pending in transceiver buffer */
    SND_PKT,        /**< request for sending a packet */
    SND_ACK,        /**< request for sending an acknowledgement */
    SWITCH_RX,      /**< switch transceiver to RX sate */
    POWERDOWN,      /**< power down transceiver */
    GET_CHANNEL,    /**< Get current channel */
    SET_CHANNEL,    /**< Set a new channel */
    GET_ADDRESS,    /**< Get the radio address */
    SET_ADDRESS,    /**< Set the radio address */
    GET_LONG_ADDR,  /**< Get the long radio address, if existing */
    SET_LONG_ADDR,  /**< Set the long radio address, if supported by hardware */
    SET_MONITOR,    /**< Set transceiver to monitor mode (disable address
                         checking) */
    GET_PAN,        /**< Get current pan */
    SET_PAN,        /**< Set a new pan */

    /* debug message types */
    DBG_IGN,        /**< add a physical address to the ignore list */

    /* Error messages */
    ENOBUFFER,      /**< No buffer left */

    /* reserve message types for higher layer notifications */
    UPPER_LAYER_1,  /**< reserved */
    UPPER_LAYER_2,  /**< reserved */
    UPPER_LAYER_3,  /**< reserved */
    UPPER_LAYER_4,  /**< reserved */
    UPPER_LAYER_5,  /**< reserved */
};

#endif /* TRANSCEIVER_H */
/** @} */
