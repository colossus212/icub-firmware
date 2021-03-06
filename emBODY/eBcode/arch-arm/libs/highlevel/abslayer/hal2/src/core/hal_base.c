/*
 * Copyright (C) 2012 iCub Facility - Istituto Italiano di Tecnologia
 * Author:  Valentina Gaggero, Marco Accame
 * email:   valentina.gaggero@iit.it, marco.accame@iit.it
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

/* @file       hal_base.c
	@brief      This file keeps implementation of the base hal module for hal.
	@author     marco.accame@iit.it
    @date       09/12/2011
**/

// - modules to be built: contains the HAL_USE_* macros ---------------------------------------------------------------
#include "hal_brdcfg_modules.h"
// - middleware interface: contains hl, stm32 etc. --------------------------------------------------------------------
//#include "hal_middleware_interface.h"

#ifdef HAL_USE_BASE

// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------

#include "stdlib.h"
#include "string.h"

#include "hal_sys.h"



// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------

#include "hal_base.h"


// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------

#include "hal_base_hid.h" 


// --------------------------------------------------------------------------------------------------------------------
// - #define with internal scope
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of extern variables, but better using _get(), _set() 
// --------------------------------------------------------------------------------------------------------------------
// empty-section



// --------------------------------------------------------------------------------------------------------------------
// - typedef with internal scope
// --------------------------------------------------------------------------------------------------------------------

typedef enum
{
    hal_base_status_zero            = 0,
    hal_base_status_initialised     = 1    
} hal_base_status_t;


typedef struct
{
    hal_base_status_t           status;
    hal_base_cfg_t              config;
    hal_void_fp_void_t          fn_osal_system_scheduling_suspend;
    hal_void_fp_void_t          fn_osal_system_scheduling_restart;
    hal_errorhandler_t          fn_on_error; 
    hal_voidp_fp_uint32_t       fn_heap_new;
    hal_void_fp_voidp_t         fn_heap_delete;
} hal_base_theinternals_t;

// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------

static void* s_hal_base_default_new(uint32_t size);
static void s_hal_base_default_delete(void* mem);

// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static const variables
// --------------------------------------------------------------------------------------------------------------------
// empty-section

// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------

static hal_base_theinternals_t s_hal_base_theinternals =
{
    .status                             = hal_base_status_zero,
    .config                             = { .extfn = NULL },
    .fn_osal_system_scheduling_suspend  = NULL,
    .fn_osal_system_scheduling_restart  = NULL,
    .fn_on_error                        = NULL,
    .fn_heap_new                        = NULL,
    .fn_heap_delete                     = NULL
};


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------


extern hal_result_t hal_base_init(const hal_base_cfg_t *cfg) 
{
    if(NULL == cfg)
    {   
        hal_base_on_fatalerror(hal_fatalerror_missingconfiguration, "hal_base_init() needs cfg");
        return(hal_res_NOK_generic);
    }
    
    if(hal_base_status_initialised == s_hal_base_theinternals.status)
    {
        hal_base_on_fatalerror(hal_fatalerror_generic, "hal_base_init() already called");
        return(hal_res_NOK_generic);    
    }


    // override functions
    s_hal_base_theinternals.fn_on_error                       = cfg->extfn.usr_on_fatal_error;
    s_hal_base_theinternals.fn_osal_system_scheduling_suspend = cfg->extfn.osal_system_scheduling_suspend;
    s_hal_base_theinternals.fn_osal_system_scheduling_restart = cfg->extfn.osal_system_scheduling_restart;
    s_hal_base_theinternals.fn_heap_new = (NULL != cfg->extfn.ext_heap_new) ? (cfg->extfn.ext_heap_new) : (s_hal_base_default_new);
    s_hal_base_theinternals.fn_heap_delete = (NULL != cfg->extfn.ext_heap_delete) ? (cfg->extfn.ext_heap_delete) : (s_hal_base_default_delete);


    
    // finally ... sets used config
    memcpy(&s_hal_base_theinternals.config, cfg, sizeof(hal_base_cfg_t));

    s_hal_base_theinternals.status = hal_base_status_initialised;
    
    return(hal_res_OK);
}

extern hal_bool_t hal_base_initted_is(void)
{
    return( (hal_base_status_initialised == s_hal_base_theinternals.status) ? (hal_true) : (hal_false) );    
}

extern void hal_base_osal_scheduling_suspend(void)
{
	if(hal_true == hal_sys_irq_is_running())
	{	// no need if it is called by an isr
	 	return;
	}

    if(NULL != s_hal_base_theinternals.fn_osal_system_scheduling_suspend)
    {
		s_hal_base_theinternals.fn_osal_system_scheduling_suspend();
    }
}


extern void hal_base_osal_scheduling_restart(void)
{
	if(hal_true == hal_sys_irq_is_running())
	{	// no need if it is called by an isr
	 	return;
	}

    if(NULL != s_hal_base_theinternals.fn_osal_system_scheduling_restart)
    {
		s_hal_base_theinternals.fn_osal_system_scheduling_restart();
    }
} 

extern void hal_base_on_warning(const char * warningmsg)
{
    hal_base_osal_scheduling_suspend();
    
    if(NULL != s_hal_base_theinternals.fn_on_error)
    {
        s_hal_base_theinternals.fn_on_error(hal_fatalerror_warning, warningmsg);
    } 
    
    hal_base_osal_scheduling_restart();
}

extern void hal_base_on_fatalerror(hal_fatalerror_t errorcode, const char * errormsg)
{
    hal_base_osal_scheduling_suspend();
    
    if(NULL != s_hal_base_theinternals.fn_on_error)
    {
        s_hal_base_theinternals.fn_on_error(errorcode, errormsg);
        if(errorcode == hal_fatalerror_warning)
        {
            hal_base_osal_scheduling_restart();
            return;
        }
        for(;;);
    } 
    else
    {  
        if(errorcode == hal_fatalerror_warning)
        {
            hal_base_osal_scheduling_restart();
            return;
        }
        for(;;)
        {
            errorcode = errorcode;
        }
    }
}


extern void * hal_base_heap_new(uint32_t size)
{
    void *p = NULL;
    if(NULL != s_hal_base_theinternals.fn_heap_new)
    {
		p = s_hal_base_theinternals.fn_heap_new(size);
    }
    
    return(p);
}

extern void hal_base_heap_delete(void* mem)
{
    if(NULL != s_hal_base_theinternals.fn_heap_delete)
    {
		s_hal_base_theinternals.fn_heap_delete(mem);
    }
}


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------

// ---- isr of the module: begin ----
// empty-section
// ---- isr of the module: end ------







// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------


static void* s_hal_base_default_new(uint32_t size)
{
    return(calloc(size, 1));
}

static void s_hal_base_default_delete(void* mem)
{
    free(mem);
}


#endif//HAL_USE_BASE

// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------





