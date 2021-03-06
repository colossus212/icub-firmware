/*
 * Copyright (C) 2011 Department of Robotics Brain and Cognitive Sciences - Istituto Italiano di Tecnologia
 * Author:  Valentina Gaggero
 * email:   valentina.gaggero@iit.it
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
#ifndef _EOTHEEMASAPPLBODY_HID_H_
#define _EOTHEEMASAPPLBODY_HID_H_


// - doxy -------------------------------------------------------------------------------------------------------------
/* @file       EOtheEMSApplBody_hid.h
    @brief     This file provides hidden interface to TheEmsAppBody singleton.
    @author    valentina.gaggero@iit.it
    @date      09/19/2012
**/


// - external dependencies --------------------------------------------------------------------------------------------
#include "EoManagement.h"
#include "EOtimer.h"




// - declaration of extern public interface ---------------------------------------------------------------------------
#include "EOtheEMSapplBody.h"


// - definition of the hidden struct implementing the object ----------------------------------------------------------


typedef enum
{
    eo_emsApplBody_st__NOTinitted   = 0,
    eo_emsApplBody_st__initted      = 1,
} eOtheEMSapplBody_hid_states_t;


typedef struct
{
    EOappTheDB                      *appDB;
    EOappCanSP                      *appCanSP;
    EOappEncReader                  *appEncReader;
    EOemsController                 *emsController;
    EOappMeasConv                   *appMeasConv;
} eOtheEMSapplBody_hid_bodyobjs_t;

struct EOtheEMSapplBody_hid
{
    eOemsapplbody_cfg_t                 config; 
    eOtheEMSapplBody_hid_states_t       st;
    eOmn_appl_runMode_t                 appRunMode;
    EOtimer                             *checkCanBoards_timer;
    eOtheEMSapplBody_hid_bodyobjs_t     bodyobjs;
}; 


// - declaration of extern hidden functions ---------------------------------------------------------------------------

extern void eo_emsapplBody_hid_canSP_cbkonrx(void *arg);


#endif  // include guard

// - end-of-file (leave a blank line after)----------------------------------------------------------------------------



