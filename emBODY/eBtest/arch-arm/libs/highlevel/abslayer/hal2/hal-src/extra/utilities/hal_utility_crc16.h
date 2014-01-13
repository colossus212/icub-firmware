/*
 * Copyright (C) 2012 iCub Facility - Istituto Italiano di Tecnologia
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

#ifndef _HAL_UTILITY_CRC16_H_
#define _HAL_UTILITY_CRC16_H_


// - doxy begin -------------------------------------------------------------------------------------------------------

/** @file       hal_utility_crc16.h
    @brief      This header file implements public interface to hal_utility_crc16 using lut, as described by Sven Reifegerste
                at http://zorc.breitbandkatze.de/crc.html
    @author     marco.accame@iit.it
    @date       12/02/2011
**/

/** @defgroup hal_utility_crc16
 
    
    @todo put documentation proper to the entity.
    
    @{        
 **/


// - external dependencies --------------------------------------------------------------------------------------------

#include "stdint.h"
#include "hal_base.h"



// - public #define  --------------------------------------------------------------------------------------------------

// uncomment to save 512 bytes of rom
#define _USE_HAL_UTILITY_CRC16TABLE_IN_ROM_
  

// - declaration of public user-defined types ------------------------------------------------------------------------- 
// empoty-section

    
// - declaration of extern public variables, ...deprecated: better using use _get/_set instead ------------------------

extern const uint16_t hal_utility_crc16_table_0x1021[256];


// - declaration of extern public functions ---------------------------------------------------------------------------
 
/** @fn         extern void hal_utility_crc16_table_get(uint16_t polynomial, uint16_t *hal_utility_crc16table)
    @brief      initialise the crctable for a given quotient polynomial. 
    @param      quotient        the poly.
    @param      crctab          the crctab whcih must point to 256 words of 2 bytes each.
 **/
extern void hal_utility_crc16_table_get(uint16_t polynomial, uint16_t *hal_utility_crc16table);


/** @fn         extern uint16_t hal_utility_crc16_compute(const uint32_t polynom, const uint16_t *hal_utility_crc16table, uint16_t initvalue, uint8_t *data, uint32_t len)
    @brief      computes the crc. 
    @param      crctab          the table.
    @param      data            data to be used.
    @param      len             its size, at least four
 **/
extern uint16_t hal_utility_crc16_compute(const uint32_t polynom, const uint16_t *hal_utility_crc16table, uint16_t initvalue, uint8_t *data, uint32_t len);


// - doxy end ---------------------------------------------------------------------------------------------------------

/** @}            
    end of group hal_utility_crc16  
 **/

 
#endif  // include-guard


// - end-of-file (leave a blank line after)----------------------------------------------------------------------------


