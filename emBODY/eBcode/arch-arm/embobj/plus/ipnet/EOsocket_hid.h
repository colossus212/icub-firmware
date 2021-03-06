/*
 * Copyright (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
 * Author:  Marco Accame
 * email:   marco.accame@iit.it
 * website: www.robotcub.org
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

// - include guard ----------------------------------------------------------------------------------------------------
#ifndef _EOSOCKET_HID_H_
#define _EOSOCKET_HID_H_

#ifdef __cplusplus
extern "C" {
#endif

/* @file       EOsocket_hid.h
    @brief      This header file implements hidden interface to the base socket object.
    @author     marco.accame@iit.it
    @date       09/01/2011
 **/


// - external dependencies --------------------------------------------------------------------------------------------

#include "EoCommon.h"
#include "EOaction.h" 


// - declaration of extern public interface ---------------------------------------------------------------------------
 
#include "EOsocket.h"


// - #define used with hidden struct ----------------------------------------------------------------------------------

#define STATUS_SOCK_NONE   0x00
#define STATUS_SOCK_OPENED 0x03


// - definition of the hidden struct implementing the object ----------------------------------------------------------

/* @struct     EOsocket_hid
    @brief      Hidden definition. Implements private data used only internally by the 
                public or private (static) functions of the object and protected data
                used also by its derived objects.
 **/  
 
struct EOsocket_hid 
{
    // other-stuff
    eOsocketType_t          type;               /*< the type of socket */
    eOipv4port_t            localport;          /*< the local port   */
    void                    *skthandle;         /*< used by the net... use a void pointer                  */
    EOaction                *onreception;       /*< action executed on reception of a packet */
    EOaction                *ontransmission;    /*< action executed on transmission of a packet */
    uint8_t                 status;             /*< used by the net                          */
    eOsocketDirection_t     dir;                /*< the direction of teh socket */
    eObool_t                block2wait4packet;  /*< block on get() if no packet i */
    void                    *blkgethandle;      /*< contains a semaphore used to block the socket... use a void pointer */
}; 


// - declaration of extern hidden functions ---------------------------------------------------------------------------


extern eOresult_t eo_socket_hid_derived_Prepare(EOsocketDerived *s, eOsocketType_t type, eOipv4port_t port, EOaction *onrec, EOaction *ontra, eOsocketDirection_t dir, eObool_t block2wait4packet);

extern eOresult_t eo_socket_hid_derived_SetActions(EOsocketDerived *s, EOaction *onrec, EOaction *ontra);

extern void * eo_socket_hid_derived_Get_Handle(EOsocketDerived *s);

extern eOsocketType_t eo_socket_hid_derived_Get_Type(EOsocketDerived *s);


#ifdef __cplusplus
}       // closing brace for extern "C"
#endif 
 
#endif  // include-guard

// - end-of-file (leave a blank line after)----------------------------------------------------------------------------




