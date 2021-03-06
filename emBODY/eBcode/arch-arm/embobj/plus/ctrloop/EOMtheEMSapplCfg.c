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

// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------


#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "EoCommon.h"


#include "EOsm.h"
#include "eOcfg_sm_EMSappl.h"

#include "EOVtheIPnet.h"
#include "EOMmutex.h"

#include "EOMtheSystem.h"
#include "EOVtheSystem.h"
#include "EOtheMemoryPool.h"
#include "EOtheErrormanager.h"
#include "EOMtheIPnet.h"


#include "EOtheARMenvironment.h"
#include "EOVtheEnvironment.h"

#include "EOMtheEMSsocket.h"

#include "EOMtheEMStransceiver.h"

#include "EOMtheEMSconfigurator.h"

#include "EOMtheEMSerror.h"

#include "EOMtheEMSrunner.h"

#include "EOaction.h"


#include "eEcommon.h"
#include "eEmemorymap.h"

#include "hal_cfg.h"
#include "osal_cfg.h"
#include "ipal_cfg.h"

#include "EOMtheSystem.h"

// to see the EOMTHEEMSAPPLCFG_* macros
#include "EOMtheEMSapplCfg_cfg.h"


// -------------------------------------------------------------------------------------------------------------------
// - declaration of extern variables defined elsewhere, for which we dont include the proper .h
// -------------------------------------------------------------------------------------------------------------------

extern const hal_cfg_t     hal_cfg;
extern const osal_cfg_t    osal_cfg;
extern const ipal_cfg_t    ipal_cfg;



// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------

#include "EOMtheEMSapplCfg.h"



// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------

#include "EOMtheEMSapplCfg_hid.h" 


// --------------------------------------------------------------------------------------------------------------------
// - #define with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of extern variables, but better using _get(), _set() 
// --------------------------------------------------------------------------------------------------------------------

extern EOVtheEMSapplCfgBody theapplbodyconfig;

const eEmoduleExtendedInfo_t eom_emsapplcfg_modinfo_extended __attribute__((at(EENV_MEMMAP_EAPPLICATION_ROMADDR+EENV_MODULEINFO_OFFSET))) = 
{
    .moduleinfo = 
    {
        .info           =
        {
            .entity     =
            {
                .type       = ee_entity_process,
                .signature  = ee_procApplication,
                .version    = 
                { 
                    .major = EOMTHEEMSAPPLCFG_VERSION_MAJOR, 
                    .minor = EOMTHEEMSAPPLCFG_VERSION_MINOR
                },  
                .builddate  = 
                {
                    .year  = EOMTHEEMSAPPLCFG_BUILDDATE_YEAR,
                    .month = EOMTHEEMSAPPLCFG_BUILDDATE_MONTH,
                    .day   = EOMTHEEMSAPPLCFG_BUILDDATE_DAY,
                    .hour  = EOMTHEEMSAPPLCFG_BUILDDATE_HOUR,
                    .min   = EOMTHEEMSAPPLCFG_BUILDDATE_MIN
                }
            },
            .rom        = 
            {   
                .addr   = EENV_MEMMAP_EAPPLICATION_ROMADDR,
                .size   = EENV_MEMMAP_EAPPLICATION_ROMSIZE
            },
            .ram        = 
            {   
                .addr   = EENV_MEMMAP_EAPPLICATION_RAMADDR,
                .size   = EENV_MEMMAP_EAPPLICATION_RAMSIZE
            },
            .storage    = 
            {
                .type   = ee_strg_none,
                .size   = 0,
                .addr   = 0
            },
            .communication  = ee_commtype_eth | ee_commtype_can1 | ee_commtype_can2,  
            .name           = EOMTHEEMSAPPLCFG_NAME
        },
        .protocols  =
        {
            .udpprotversion  = { .major = 1, .minor = 0},
            .can1protversion = { .major = 1, .minor = 0},
            .can2protversion = { .major = 1, .minor = 0},
            .gtwprotversion  = { .major = 0, .minor = 0}
        },
        .extra      = {"EXT"}
    },
    .compilationdatetime    = __DATE__ " " __TIME__,
    .userdefined            = {0}
};


// --------------------------------------------------------------------------------------------------------------------
// - typedef with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------


static void s_emsapplcfg_singleton_verify(void);

// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------

//static const char s_eobj_ownname[] = "EOMtheEMSapplCfg";

static eObool_t s_emsapplcfg_singleton_verified = eobool_false;


static EOMtheEMSapplCfg s_emsapplcfg_singleton = 
{
    .wsyscfg         =
    {
        .msyscfg        =
        {
            .codespaceoffset        = (EENV_MEMMAP_EAPPLICATION_ROMADDR-EENV_ROMSTART),
            .halcfg                 = &hal_cfg,
            .osalcfg                = &osal_cfg,
            .fsalcfg                = NULL    
        },
        .mempoolcfg     =
        {   
            .mode                   = eo_mempool_alloc_dynamic,
            .conf                   = &eom_thesystem_mempool_alloc_config_heaposal
        },
        .errmancfg      =
        {
            .extfn.usr_on_error     = eom_emsapplcfg_hid_userdef_OnError
        },
        .tmrmancfg      =
        {
            .priority               = EOMTHEEMSAPPLCFG_TMRMAN_TASK_PRIORITYof,
            .stacksize              = EOMTHEEMSAPPLCFG_TMRMAN_TASK_STACKSIZEof, 
            .messagequeuesize       = EOMTHEEMSAPPLCFG_TMRMAN_TASK_QUEUESIZEof
        },
        .cbkmancfg      =
        {
            .priority               = EOMTHEEMSAPPLCFG_CBKMAN_TASK_PRIORITYof,
            .stacksize              = EOMTHEEMSAPPLCFG_CBKMAN_TASK_STACKSIZEof, 
            .queuesize              = EOMTHEEMSAPPLCFG_CBKMAN_TASK_QUEUESIZEof      
        }
    },
    .applcfg        =
    {
        .emsappinfo             = (eEmoduleInfo_t*)&eom_emsapplcfg_modinfo_extended, 
        .hostipv4addr           = EO_COMMON_IPV4ADDR(EOMTHEEMSAPPLCFG_HOSTIPADDR1, EOMTHEEMSAPPLCFG_HOSTIPADDR2, EOMTHEEMSAPPLCFG_HOSTIPADDR3, EOMTHEEMSAPPLCFG_HOSTIPADDR4)    
    },
    .wipnetcfg      =
    {
        .ipnetcfg       =
        {
            .procpriority           = EOMTHEEMSAPPLCFG_IPNETPROC_TASK_PRIORITYof,
            .procstacksize          = EOMTHEEMSAPPLCFG_IPNETPROC_TASK_STACKSIZEof,
            .procmaxidletime        = EOMTHEEMSAPPLCFG_IPNETPROC_TASK_MAXIDLETIMEof,
            .procwakeuponrxframe    = (eObool_t)EOMTHEEMSAPPLCFG_IPNETPROC_WAKEUPONRXFRAME,
            .tickpriority           = EOMTHEEMSAPPLCFG_IPNETTICK_TASK_PRIORITYof,
            .tickstacksize          = EOMTHEEMSAPPLCFG_IPNETTICK_TASK_STACKSIZEof 
        },
        .ipalcfg        = &ipal_cfg,
        .addrcfg        = NULL,
        .dtgskcfg       =
        {
            .numberofsockets        =  EOMTHEEMSAPPLCFG_IPNET_MAXNUMOFSOCKETS,
            .maxdatagramenqueuedintx = EOMTHEEMSAPPLCFG_IPNET_MAXNUMOFDATAGRAMSENQUEUEDINTX
        }   
    },
    .getipaddrFROMenvironment   =   EOMTHEEMSAPPLCFG_IPADDR_FROM_ENVIRONMENT,
    .errmng_haltrace_enabled    =   EOMTHEEMSAPPLCFG_HALTRACE_ENABLED,
    .boardid                    =   255,
    .disclistcfg    =
    {
        .taskpriority           = EOMTHEEMSAPPLCFG_LISTENER_TASK_PRIORITYof,
        .taskstacksize          = EOMTHEEMSAPPLCFG_LISTENER_TASK_STACKSIZEof,
        .inpdatagramnumber      = EOMTHEEMSAPPLCFG_DISCOVERY_INPDGRAMNUMBER,
        .outdatagramnumber      = EOMTHEEMSAPPLCFG_DISCOVERY_OUTDGRAMNUMBER,
        .inpdatagramsizeof      = EOMTHEEMSAPPLCFG_DISCOVERY_INPDGRAMSIZEOF,
        .outdatagramsizeof      = EOMTHEEMSAPPLCFG_DISCOVERY_OUTDGRAMSIZEOF,
        .localport              = EOMTHEEMSAPPLCFG_DISCOVERY_LOCALPORT, 
        .remoteport             = EOMTHEEMSAPPLCFG_DISCOVERY_REMOTEPORT,
        .remoteipaddr           = EO_COMMON_IPV4ADDR(EOMTHEEMSAPPLCFG_DISCOVERY_REMOTEIPADDR1, EOMTHEEMSAPPLCFG_DISCOVERY_REMOTEIPADDR2, EOMTHEEMSAPPLCFG_DISCOVERY_REMOTEIPADDR3, EOMTHEEMSAPPLCFG_DISCOVERY_REMOTEIPADDR4),
        .usemutex               = eobool_true                   
    },
    .backdoorcfg    =
    {
        .enabled                = EOMTHEEMSAPPLCFG_BACKDOOR_ENABLED,
        .taskpriority           = EOMTHEEMSAPPLCFG_BACKDOOR_TASK_PRIORITYof,
        .taskstacksize          = EOMTHEEMSAPPLCFG_BACKDOOR_TASK_STACKSIZEof,
        .inpdatagramnumber      = EOMTHEEMSAPPLCFG_BACKDOOR_INPDGRAMNUMBER,
        .outdatagramnumber      = EOMTHEEMSAPPLCFG_BACKDOOR_OUTDGRAMNUMBER,
        .inpdatagramsizeof      = EOMTHEEMSAPPLCFG_BACKDOOR_INPDGRAMSIZEOF,
        .outdatagramsizeof      = EOMTHEEMSAPPLCFG_BACKDOOR_OUTDGRAMSIZEOF,
        .localport              = EOMTHEEMSAPPLCFG_BACKDOOR_LOCALPORT, 
        .remoteport             = EOMTHEEMSAPPLCFG_BACKDOOR_REMOTEPORT,
        .remoteipaddr           = EO_COMMON_IPV4ADDR(EOMTHEEMSAPPLCFG_BACKDOOR_REMOTEIPADDR1, EOMTHEEMSAPPLCFG_BACKDOOR_REMOTEIPADDR2, EOMTHEEMSAPPLCFG_BACKDOOR_REMOTEIPADDR3, EOMTHEEMSAPPLCFG_BACKDOOR_REMOTEIPADDR4),
        .usemutex               = eobool_true     
    },
    .socketcfg      =
    {
        .inpdatagramnumber          = EOMTHEEMSAPPLCFG_SOCKET_INPDGRAMNUMBER, 
        .outdatagramnumber          = EOMTHEEMSAPPLCFG_SOCKET_OUTDGRAMNUMBER, 
        .inpdatagramsizeof          = EOMTHEEMSAPPLCFG_SOCKET_INPDGRAMSIZEOF, 
        .outdatagramsizeof          = EOMTHEEMSAPPLCFG_SOCKET_OUTDGRAMSIZEOF,
        .localport                  = EOMTHEEMSAPPLCFG_SOCKET_LOCALPORT, 
        .usemutex                   = eobool_true
    },  
    .transcfg       =
    {   // the same ipv4 addr and port as in applcfg ...   
        .nvsetbrdcfg            = &eonvset_BRDcfgBasic,  
        .hostipv4addr           = EO_COMMON_IPV4ADDR(EOMTHEEMSAPPLCFG_HOSTIPADDR1, EOMTHEEMSAPPLCFG_HOSTIPADDR2, EOMTHEEMSAPPLCFG_HOSTIPADDR3, EOMTHEEMSAPPLCFG_HOSTIPADDR4),
        .hostipv4port           = EOMTHEEMSAPPLCFG_TRANSCEIVER_HOSTIPPORT,
        .sizes                  =
        {
            .capacityoftxpacket             = EOMTHEEMSAPPLCFG_TRANSCEIVER_ROPFRAMECAPACITY, 
            .capacityofrop                  = EOMTHEEMSAPPLCFG_TRANSCEIVER_ROPCAPACITY, 
            .capacityofropframeregulars     = EOMTHEEMSAPPLCFG_TRANSCEIVER_ROPFRAMEREGULARSCAPACITY, 
            .capacityofropframeoccasionals  = EOMTHEEMSAPPLCFG_TRANSCEIVER_ROPFRAMEOCCASIONALSCAPACITY, 
            .capacityofropframereplies      = EOMTHEEMSAPPLCFG_TRANSCEIVER_ROPFRAMEREPLIESCAPACITY, 
            .maxnumberofregularrops         = EOMTHEEMSAPPLCFG_TRANSCEIVER_MAXNUMOFREGULARROPS         
        },
        .transprotection        = eo_trans_protection_none, //eo_trans_protection_enabled, //eo_trans_protection_none,
        .nvsetprotection        = eo_nvset_protection_none,//eo_nvset_protection_one_per_endpoint //eo_nvset_protection_none // eo_nvset_protection_one_per_netvar eo_nvset_protection_one_per_endpoint
        .proxycfg          =
        {
            .mode                   = (eOproxymode_t)EOMTHEEMSAPPLCFG_PROXY_MODE,
            .capacityoflistofropdes = EOMTHEEMSAPPLCFG_PROXY_MAXNUMOFREPLYROPS,
            .replyroptimeout        = EOMTHEEMSAPPLCFG_PROXY_REPLYTIMEOUTMILLISEC*1000,
            .mutex_fn_new           = NULL,
            .transceiver            = NULL
        }                      
    },
    .errobjcfg      =
    {
        .taskpriority               = EOMTHEEMSAPPLCFG_ERROBJ_TASK_PRIORITYof, 
        .taskstacksize              = EOMTHEEMSAPPLCFG_ERROBJ_TASK_STACKSIZEof
    },
    .cfgobjcfg      =
    {
        .taskpriority               = EOMTHEEMSAPPLCFG_CFGOBJ_TASK_PRIORITYof, 
        .taskstacksize              = EOMTHEEMSAPPLCFG_CFGOBJ_TASK_STACKSIZEof        
    },
    .runobjcfg      =
    {
        .taskpriority               = 
        {
            EOMTHEEMSAPPLCFG_RUNOBJ_TASK_RX_PRIORITYof, EOMTHEEMSAPPLCFG_RUNOBJ_TASK_DO_PRIORITYof, EOMTHEEMSAPPLCFG_RUNOBJ_TASK_TX_PRIORITYof
        },
        .taskstacksize              = 
        {
            EOMTHEEMSAPPLCFG_RUNOBJ_TASK_RX_STACKSIZEof, EOMTHEEMSAPPLCFG_RUNOBJ_TASK_DO_STACKSIZEof, EOMTHEEMSAPPLCFG_RUNOBJ_TASK_TX_STACKSIZEof
        }, 
        .haltimerstart                 = 
        {
            EOMTHEEMSAPPLCFG_RUNOBJ_TIMER_ID_STARTRX, EOMTHEEMSAPPLCFG_RUNOBJ_TIMER_ID_STARTDO, EOMTHEEMSAPPLCFG_RUNOBJ_TIMER_ID_STARTTX
        },  
        .haltimeralert                 = 
        {
            EOMTHEEMSAPPLCFG_RUNOBJ_TIMER_ID_ALERTRX, EOMTHEEMSAPPLCFG_RUNOBJ_TIMER_ID_ALERTDO, EOMTHEEMSAPPLCFG_RUNOBJ_TIMER_ID_ALERTTX
        },        
        .period                     = EOMTHEEMSAPPLCFG_RUNOBJ_PERIOD,
        .execRXafter                = EOMTHEEMSAPPLCFG_RUNOBJ_RX_AFTER,            
        .safeRXexecutiontime        = EOMTHEEMSAPPLCFG_RUNOBJ_RX_SAFETIME,
        .execDOafter                = EOMTHEEMSAPPLCFG_RUNOBJ_DO_AFTER,
        .safeDOexecutiontime        = EOMTHEEMSAPPLCFG_RUNOBJ_DO_SAFETIME,
        .execTXafter                = EOMTHEEMSAPPLCFG_RUNOBJ_TX_AFTER,
        .safeTXexecutiontime        = EOMTHEEMSAPPLCFG_RUNOBJ_TX_SAFETIME,
        .maxnumofRXpackets          = EOMTHEEMSAPPLCFG_RUNOBJ_RX_MAXPACKETS,                // add a control that is is lower equal to inpdatagramnumber.
        .maxnumofTXpackets          = EOMTHEEMSAPPLCFG_RUNOBJ_TX_MAXPACKETS,                // so far it can be only 0 or 1 
        .modeatstartup              = (eOemsrunner_mode_t) EOMTHEEMSAPPLCFG_RUNOBJ_MODE_AT_STARTUP,
        .defaultTXdecimationfactor  = EOMTHEEMSAPPLCFG_RUNOBJ_TXDECIMATIONFACTOR        
    },
    .applbodycfg    =  &theapplbodyconfig
};


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------


extern EOMtheEMSapplCfg * eom_emsapplcfg_Initialise(void)
{  
    
    if(eobool_true != s_emsapplcfg_singleton_verified)
    {
        s_emsapplcfg_singleton_verify();
        s_emsapplcfg_singleton_verified = eobool_true;        
    }
    
    return(&s_emsapplcfg_singleton);
}


extern EOMtheEMSapplCfg* eom_emsapplcfg_GetHandle(void) 
{
    if(eobool_true != s_emsapplcfg_singleton_verified)
    {
        return(eom_emsapplcfg_Initialise());
    }
    
    return(&s_emsapplcfg_singleton);
}




// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------

__weak extern void eom_emsapplcfg_hid_userdef_OnError(eOerrmanErrorType_t errtype, const char *info, eOerrmanCaller_t *caller, const eOerrmanDescriptor_t *des)
{
    const char empty[] = "EO?";
    const char *err = eo_errman_ErrorStringGet(eo_errman_GetHandle(), errtype);
    char str[64];
    EOMtheEMSapplCfg *emsapplcfg = eom_emsapplcfg_GetHandle();
    
    const char *eobjstr = (NULL != caller) ? (caller->eobjstr) : (empty);
    uint32_t taskid = (NULL != caller) ? (caller->taskid) : (0);

    if(emsapplcfg->errmng_haltrace_enabled)
    {
        snprintf(str, sizeof(str), "[%s] %s in tsk %d: %s", err, eobjstr, taskid, info);
        hal_trace_puts(str);
    }
    if(errtype <= eo_errortype_error)
    {
        return;
    }
    
    eov_sys_Stop(eov_sys_GetHandle());

    for(;;);
}


// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------


static void s_emsapplcfg_singleton_verify(void)
{

}







// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------





