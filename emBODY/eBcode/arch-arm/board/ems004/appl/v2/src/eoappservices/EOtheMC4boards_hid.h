/*
 * Copyright (C) 2015 iCub Facility - Istituto Italiano di Tecnologia
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

#ifndef _EOTHEMC4BOARDS_HID_H_
#define _EOTHEMC4BOARDS_HID_H_




// - external dependencies --------------------------------------------------------------------------------------------

#include "EoCommon.h"
#include "EOtheCANprotocol.h"


// - declaration of extern public interface ---------------------------------------------------------------------------

#include "EOtheMC4boards.h"


// - definition of the hidden struct implementing the object ----------------------------------------------------------

typedef struct
{
    eOmc4boards_conv_encoder_factor_t   factor;
    eOmc4boards_conv_encoder_offset_t   offset;
    icubCanProto_position_t max_mc4_joint_pos;
    icubCanProto_position_t min_mc4_joint_pos;
    icubCanProto_position_t max_mc4_motor_pos;
    icubCanProto_position_t min_mc4_motor_pos;
} eOmc4boards_conv_encoder_t;

struct EOtheMC4boards_hid
{
    eObool_t                    initted;
    eObool_t                    therearemc4s;
    uint8_t                     numofjomos;
    eOmc4boards_config_t        config;
    eOmc4boards_config_t        config2use;
    eOcanprot_command_t         command;
    eObool_t                    configured;
    eOmc4boards_conv_encoder_t  convencoder[12];
}; 


// - declaration of extern hidden functions ---------------------------------------------------------------------------
// empty section


#endif  // include guard

// - end-of-file (leave a blank line after)----------------------------------------------------------------------------



