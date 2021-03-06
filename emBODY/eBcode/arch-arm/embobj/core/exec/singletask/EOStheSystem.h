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
#ifndef _EOSTHESYSTEM_H_
#define _EOSTHESYSTEM_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @file       EOStheSystem.h
    @brief      This header file implements public interface to the system singleton.
    @author     marco.accame@iit.it
    @date       04/07/2011
**/

/** @defgroup eos_thesystem Singleton EOStheSystem
    The EOStheSystem is derived from abstract object EOVtheSystem to give to EmbObj a singletask execution environment
    (SEE) based on just a timer.  The EOStheSystem must be first initialised and then started. 
    The initialisation requires parameters such as the configuration for the memory pool and the error manager
     
    @{        
 **/


// - external dependencies --------------------------------------------------------------------------------------------

#include "EoCommon.h"
#include "EOtheMemoryPool.h"
#include "EOtheErrorManager.h"
#include "EOStheTimerManager.h"
#include "EOStheCallbackManager.h"
#include "EOStheTimerManager.h"
#include "EOStheFOOP.h"




// - public #define  --------------------------------------------------------------------------------------------------

#define     EOSSYS_min_systickperiod    10ul
#define     EOSSYS_max_systickperiod    999999ul
  

// - declaration of public user-defined types ------------------------------------------------------------------------- 

/** @typedef    typedef struct eOssystem_userdef_cfg_t
    @brief      eOssystem_userdef_cfg_t contains the specialisation that the user have have in the single-task
                execution environment (or SEE).
 **/ 
typedef struct
{
    eOreltime_t                     systickperiod;      /**< the system tick in micro-seconds */
    uint8_t                         systickpriority;    /**< priority of the system tick interrupt: from 0 to 7 */
    eOvoid_fp_void_t                on_systick;         /**< executed after the current tick time has been incremented */
} eOssystem_userdef_cfg_t;

/** @typedef    typedef struct eOssystem_cfg_t
    @brief      eOssystem_cfg_t contains the specification for the system singleton in the single-task execution
                environment (SEE).
 **/  
typedef struct
{
    eOsfoop_hal_fn_t            hal_fns;                        /**< gives to the system access to required hal functions */
    eOssystem_userdef_cfg_t     userdef;
} eOssystem_cfg_t;


/** @typedef    typedef struct EOStheSystem_hid EOStheSystem
    @brief      EOStheSystem is an opaque struct. It is used to implement data abstraction for the timer manager 
                object so that the user cannot see its private fields and he/she is forced to manipulate the
                object only with the proper public functions. 
 **/  
typedef struct EOStheSystem_hid EOStheSystem;

    
// - declaration of extern public variables, ... but better using use _get/_set instead -------------------------------
// empty-section


// - declaration of extern public functions ---------------------------------------------------------------------------


/** @fn         extern EOStheSystem * eos_sys_Initialise(const eOssystem_cfg_t *syscfg)

    @brief      Initialise the singleton EOStheSystem and all the other entities/objects that are required by the system:
                the memory pool, the error manager. The timer manager is started separately.
    @param      syscfg          The configuration.
    @return     A not NULL handle to the singleton.  In case of errors it is called the EOtheErrorManager
 
 **/
extern EOStheSystem * eos_sys_Initialise(const eOssystem_cfg_t *syscfg,
                                         const eOmempool_cfg_t *mpoolcfg, 
                                         const eOerrman_cfg_t *errmancfg,
                                         const eOstimerman_cfg_t *tmrmancfg,
                                         const eOscallbackman_cfg_t *cbkmancfg,
                                         const eOsfoop_cfg_t *foopcfg);


extern eOabstime_t eos_sys_LifeTimeGet(EOStheSystem *p);

extern eOresult_t eos_sys_LifeTimeSet(EOStheSystem *p, eOabstime_t ltime);

extern eOresult_t eos_sys_NanoTimeGet(EOStheSystem *p, eOnanotime_t *nt);
 
 
/** @fn         extern EOStheSystem* eos_sys_GetHandle(void)
    @brief      Returns an handle to the singleton EOStheSystem. The singleton must have been initialised otherwise 
                this function call will return NULL.
    @return     The pointer to the required EOStheSystem (or NULL upon in-initialised singleton).
 **/
extern EOStheSystem* eos_sys_GetHandle(void);


/** @fn         extern void eos_sys_Start(EOStheSystem *p, const void *cfg, eOvoid_fp_void_t init_fn)
    @brief      It starts EOStheSystem.  The singleton must have been initialised otherwise 
                this function call will issue a fatal error to the EOtheErrorManager.
    @param      p               The handler to the singleton.
    @param      userinit_fn     The function executed at startup just before the system is activated (which in
                                the SEE means just before the systick is started and the foop enters in its forever loop).
    @return     The function does not return and the execution stays in there forever.
 **/
extern void eos_sys_Start(EOStheSystem *p, eOvoid_fp_void_t userinit_fn);



/** @}            
    end of group eos_thesystem  
 **/

#ifdef __cplusplus
}       // closing brace for extern "C"
#endif 

#endif  // include-guard


// - end-of-file (leave a blank line after)----------------------------------------------------------------------------



