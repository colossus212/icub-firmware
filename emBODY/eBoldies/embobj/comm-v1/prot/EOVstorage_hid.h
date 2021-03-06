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
#ifndef _EOVSTORAGE_HID_H_
#define _EOVSTORAGE_HID_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @file       EOVstorage_hid.h
    @brief      This header file implements hidden interface to a mutex object.
    @author     marco.accame@iit.it
    @date       10/06/2009
**/


// - external dependencies --------------------------------------------------------------------------------------------

#include "EoCommon.h"

// - declaration of extern public interface ---------------------------------------------------------------------------
 
#include "EOVstorage.h"


// - #define used with hidden struct ----------------------------------------------------------------------------------

#define VF00_set                        0
#define VF01_get                        1
#define VTABLESIZE_storage              2


// - definition of the hidden struct implementing the object ----------------------------------------------------------


/** @struct     EOVstorage_hid
    @brief      Hidden definition. Implements private data used only internally by the 
                public or private (static) functions of the object and protected data
                used also by its derived objects.
 **/  
 
struct EOVstorage_hid 
{
     // - vtable: must be on top of the struct
    void * vtable[VTABLESIZE_storage];

    // - other stuff
    const void *        defval;
    uint32_t            id;
    uint32_t            capacity;
    EOVmutexDerived*    mutex;
};

typedef struct
{
    uint32_t    id;
    uint32_t    capacity;
    uint8_t     data[4];
} eov_strg_memmap_t;


// - declaration of extern hidden functions ---------------------------------------------------------------------------

 
/** @fn         extern EOVstorage* eov_strg_hid_New(void)
    @brief      Creates a new mutex object 
    @return     Pointer to the required mutex object.
    @warning    The EOVstorage cannot be used by itself, but inside a derived object.
 **/
extern EOVstorage* eov_strg_hid_New(uint32_t id, uint32_t capacity, const void *defval, EOVmutexDerived *mtx);

/** @fn         extern eOresult_t eov_strg_hid_SetVTABLE(EOVstorage *p, eOres_fp_voidp_uint32_t v_take, eOres_fp_voidp_t v_release)
    @brief      Specialise the virtual functions of the abstract object
    @param      p               The object
    @param      v_rst           the first virtual function
    @param      v_set           the second virtual function        
    @param      v_get           the second virtual function
    @return     eores_OK.
 **/
extern eOresult_t eov_strg_hid_SetVTABLE(EOVstorage *p, eOres_fp_voidp_uint32_uint32_cvoidp_t v_set, eOres_fp_voidp_uint32_uint32_voidp_t v_get);


#ifdef __cplusplus
}       // closing brace for extern "C"
#endif 
 
#endif  // include-guard

// - end-of-file (leave a blank line after)----------------------------------------------------------------------------



