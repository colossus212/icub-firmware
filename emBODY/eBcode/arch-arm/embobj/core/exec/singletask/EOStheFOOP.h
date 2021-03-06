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
#ifndef _EOSTHEFOOP_H_
#define _EOSTHEFOOP_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @file       EOStheFOOP.h
    @brief      This header file implements public interface to the FOOP singleton 
    @author     marco.accame@iit.it
    @date       08/04/2011
**/


/** @defgroup eos_theFOOP Singleton EOStheFOOP
    The EOStheFOOP is derived from abstract object EOVtask to give to EmbObj a singletask object which is
    the only one in the SEE (singletask execution environment).
    The EOStheFOOP contains a forever loop able to intercept and process: events, messages callbacks plus a
    special high priority signal dedicated for the EOStheFOOP.
     
    @{        
 **/


// - external dependencies --------------------------------------------------------------------------------------------

#include "EoCommon.h"


// - public #define  --------------------------------------------------------------------------------------------------
// empty-section
  

// - declaration of public user-defined types ------------------------------------------------------------------------- 


/** @typedef    typedef struct eOsfoop_userdef_fn_t
    @brief      eOsfoop_userdef_fn_t contains the functions whcih  the user can configure in the behaviour of the object: 
                a function  which is executed at startup after the system tick (see objects EOStheSystem) has started, 
                a function to be executed at reception of an event, and a function executed at reception of a message.
 **/
typedef struct
{
    eOvoid_fp_voidp_uint32_t        on_startup; // arg1 is the EOStheFOOP*, arg2 is 0
    eOvoid_fp_uint32_uint32_t       on_event;
    eOvoid_fp_uint32_t              on_message;
} eOsfoop_userdef_fn_t;


/** @typedef    typedef struct eOsfoop_cfg_t
    @brief      eOsfoop_cfg_t contains the complete configuration of the object EOStheFOOP: the sizes of the internal
                FIFOs used to buffer incoming messages and callback requests, and some user-defined functions.
 **/
typedef struct
{
    uint8_t                         messagefifosize;
    uint8_t                         callbackfifosize;
    eOsfoop_userdef_fn_t            usrfn;
} eOsfoop_cfg_t;

/** @typedef    typedef struct eOsfoop_hal_fn_t
    @brief      contains the basic system functions typically given by a HAL which are be used for instance
                by the objects of the single-task execution environment (or SEE) to run. They have similar name and APIs as the
                ones in the HAL.
 **/  
typedef struct
{
    eOvoid_fp_void_t                    hal_start;                      /**< initialise and start the hal. it may also relocate the sys vector. */
    eOvoid_fp_voidfpvoiduint32uint8_t   hal_sys_systick_sethandler;     /**< starts a tick function w/ a given period at a given priority**/
    eOvoid_fp_vuint32p_uint32_t         hal_sys_atomic_bitwiseAND;      /**< performs atomic bitwise AND on a uint32_t passed as volatile pointer **/
    eOvoid_fp_vuint32p_uint32_t         hal_sys_atomic_bitwiseOR;       /**< performs atomic bitwise OR on a uint32_t passed as volatile pointer **/
    eOres_fp_voidp_uint32_t             hal_sys_criticalsection_take;   /**< takes a critical section on a dummy object (also for ISRs) with a dummy timeout**/
    eOres_fp_voidp_t                    hal_sys_criticalsection_release;/**< releases the critical section on the dummy object **/
    eOvoid_fp_void_t                    hal_sys_irq_disable;            /**< disable all irqs **/
    eOvoid_fp_void_t                    hal_sys_irq_enable;             /**< enable all irqs **/
} eOsfoop_hal_fn_t;


/** @typedef    typedef struct EOStheFOOP_hid EOStheFOOP
    @brief      EOStheFOOP is an opaque struct. It is used to implement data abstraction for  
                object so that the user cannot see its private fields and he/she is forced to manipulate the
                object only with the proper public functions. 
 **/  
typedef struct EOStheFOOP_hid EOStheFOOP;


/** @typedef    typedef struct EOStheFOOP_hid EOStask
    @brief      EOStask is an alias for the EOStheFOOP. 
 **/  
typedef struct EOStheFOOP_hid EOStask;

    
// - declaration of extern public variables, ... but better using use _get/_set instead -------------------------------

extern const eOsfoop_cfg_t eos_foop_DefaultCfg; // = {0, 0, {NULL, NULL, NULL}};


// - declaration of extern public functions ---------------------------------------------------------------------------


/** @fn         extern EOStheFOOP * eos_foop_Initialise(eOsfoop_cfg_t *cfg, eOsfoop_hal_fn_t *hfn)
    @brief      Initialises the singleton EOStheFOOP.
    @param      cfg             The configuratio for the object.
    @param      hfn             The functions required to manage the basic hardware-related issues. We pass it
                                as an argument to avoid any internal dependency from HAL.
    @return     The handle to the singleton.
    @warning    The function calls the EOtheErrorManager with a fatal error in case of incorrect settings
 **/
extern EOStheFOOP * eos_foop_Initialise(eOsfoop_cfg_t *cfg, eOsfoop_hal_fn_t *hfn);


/** @fn         extern EOStheFOOP* eos_foop_GetHandle(void)
    @brief      Returns a handle to the singleton. The singleton must have been initialised
                with eos_foop_Initialise(), otherwise this function call will return NULL.
    @return     The handle to the singleton (or NULL upon in-initialised singleton)
 **/
extern EOStheFOOP* eos_foop_GetHandle(void);


/** @fn         extern eOresult_t eos_foop_SetEvent(EOStheFOOP *p, eOevent_t evt)
    @brief      Set an event for the singleton. The singleton shall execute a callback function defined by the user
                in phase of initialisation.
    @param      p               Pointer to the singleton.  
    @param      evt             The mask wich contains the events to be set. 
    @return     eores_NOK_nullpointer if the FOOP pointer is NULL or eores_OK.
 **/
extern eOresult_t eos_foop_SetEvent(EOStheFOOP *p, eOevent_t evt);


/** @fn         extern eOresult_t eos_foop_SendMessage(EOStheFOOP *p, eOmessage_t msg)
    @brief      Sends a message to the singleton. The singleton shall execute a callback function defined by the user
                in phase of initialisation.
    @param      p               Pointer to the singleton.  
    @param      msg             The message to be sent. 
    @return     eores_NOK_nullpointer if the FOOP pointer is NULL, eores_NOK_generic if the FOOP does not have a
                message queue, eores_OK on success
 **/
extern eOresult_t eos_foop_SendMessage(EOStheFOOP *p, eOmessage_t msg);


/** @fn         extern eOresult_t eos_foop_ExecCallback(EOStheFOOP *p, eOcallback_t cbk)
    @brief      Sends a request of execution of a callback to the singleton. The singleton shall execute 
                the callback in its main loop.
    @param      p               Pointer to the singleton.  
    @param      cbk             The callback requested to execute. 
    @param      arg             The argument of the callback. 
    @return     eores_NOK_nullpointer if the FOOP pointer is NULL, eores_NOK_generic if the FOOP does not have a
                callback queue, eores_OK on success
 **/
extern eOresult_t eos_foop_ExecCallback(EOStheFOOP *p, eOcallback_t cbk, void *arg);



/** @}            
    end of group eos_theFOOP  
 **/

#ifdef __cplusplus
}       // closing brace for extern "C"
#endif 

#endif  // include-guard


// - end-of-file (leave a blank line after)----------------------------------------------------------------------------



