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


// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------

#include "stdlib.h"
#include "string.h"

#include "EoCommon.h"
#include "EOtheErrorManager.h"
#include "EoError.h"
#include "EOtheEntities.h"

#include "EOtheCANservice.h"
#include "EOtheCANmapping.h"
#include "EOtheCANprotocol.h"

#include "EoProtocolSK.h"

#include "EOMtheEMSappl.h"

#include "EOVtheCallbackManager.h"


// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------

#include "EOtheSKIN.h"



// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------

#include "EOtheSKIN_hid.h"


// --------------------------------------------------------------------------------------------------------------------
// - #define with internal scope
// --------------------------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of extern variables. deprecated: better using _get(), _set() on static variables 
// --------------------------------------------------------------------------------------------------------------------
// empty-section

// --------------------------------------------------------------------------------------------------------------------
// - typedef with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------



static eOresult_t s_eo_skin_onstop_search4mtbs(void *par, EOtheCANdiscovery2* p, eObool_t searchisok);

static void s_eo_skin_send_periodic_error_report(void *p);

static eOsk_skin_t* s_eo_skin_get_entity(eOcanframe_t *frame, eOcanport_t port, uint8_t *index);

static eObool_t s_eo_skin_activeskin_can_accept_canframe(void);
    
// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------

        
static EOtheSKIN s_eo_theskin = 
{
    .service = 
    {
        .servconfig             = { .type = eomn_serv_NONE },
        .initted                = eobool_false,
        .active                 = eobool_false,
        .activateafterverify    = eobool_false,
        .running                = eobool_false,
        .onverify               = NULL          
    },
    .diagnostics = 
    {
        .reportTimer            = NULL,
        .reportPeriod           = 10*EOK_reltime1sec,
        .errorDescriptor        = {0},
        .errorType              = eo_errortype_info,
        .errorCallbackCount     = 0,
        .repetitionOKcase       = 10
    },     
    .sharedcan =
    {
        .boardproperties        = NULL,
        .entitydescriptor       = NULL,
        .discoverytarget        = {0},
        .ondiscoverystop        = {0},
        .command                = {0}, 
    },
    .patchisrunning             = { eobool_false },    
    .numofskinpatches           = 0,
    .numofmtbs                  = 0,    
    .rxdata                     = { NULL },
    .skinpatches                = { NULL }
};

static const char s_eobj_ownname[] = "EOtheSKIN";


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------


extern EOtheSKIN* eo_skin_Initialise(void)
{
    if(eobool_true == s_eo_theskin.service.initted)
    {
        return(&s_eo_theskin);
    }
    
    s_eo_theskin.service.active = eobool_false;
        
    s_eo_theskin.numofskinpatches = 0;
    s_eo_theskin.numofmtbs = 0;
    s_eo_theskin.service.servconfig.type = eomn_serv_NONE;
    
    
    s_eo_theskin.sharedcan.boardproperties = eo_vector_New(sizeof(eOcanmap_board_properties_t), eo_skin_maxnumberofMTBboards, NULL, NULL, NULL, NULL);
    
    s_eo_theskin.sharedcan.entitydescriptor = eo_vector_New(sizeof(eOcanmap_entitydescriptor_t), eo_skin_maxnumberofMTBboards, NULL, NULL, NULL, NULL);
    
    uint8_t i=0;
    for(i=0; i<eomn_serv_skin_maxpatches; i++)
    {
        s_eo_theskin.skinpatches[i] = NULL;
        s_eo_theskin.patchisrunning[i] = eobool_false;
        
        s_eo_theskin.rxdata[i] = eo_vector_New(sizeof(eOsk_candata_t), 64, NULL, NULL, NULL, NULL); 
    }
    
    s_eo_theskin.diagnostics.reportTimer = eo_timer_New();
    s_eo_theskin.diagnostics.errorType = eo_errortype_error;
    s_eo_theskin.diagnostics.errorDescriptor.sourceaddress = eo_errman_sourcedevice_localboard;
    s_eo_theskin.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_skin_not_verified_yet);  

    
    s_eo_theskin.service.initted = eobool_true;
    s_eo_theskin.service.active = eobool_false;
    s_eo_theskin.service.running = eobool_false;
    
    return(&s_eo_theskin);   
}
 

    
extern EOtheSKIN* eo_skin_GetHandle(void)
{
    if(eobool_true == s_eo_theskin.service.initted)
    {
        return(&s_eo_theskin);
    }
    
    return(NULL);
}



extern eOresult_t eo_skin_Verify(EOtheSKIN *p, const eOmn_serv_configuration_t * servcfg, eOservice_onendofoperation_fun_t onverify, eObool_t activateafterverify)
{
    if((NULL == p) || (NULL == servcfg))
    {
        return(eores_NOK_nullpointer);
    } 

    if(eomn_serv_SK_skin != servcfg->type)
    {
        return(eores_NOK_generic);
    }   
    
 
// DONT Deactivate ... we may want just to check again ....    
//    if(eobool_true == s_eo_theskin.service.active)
//    {
//        eo_skin_Deactivate(p);        
//    }   
    
    // make sure the timer is not running
    eo_timer_Stop(s_eo_theskin.diagnostics.reportTimer);    

    s_eo_theskin.service.onverify = onverify;
    s_eo_theskin.service.activateafterverify = activateafterverify;


    s_eo_theskin.sharedcan.discoverytarget.info.type = eobrd_cantype_mtb;
    s_eo_theskin.sharedcan.discoverytarget.info.protocol.major = servcfg->data.sk.skin.version.protocol.major; 
    s_eo_theskin.sharedcan.discoverytarget.info.protocol.minor = servcfg->data.sk.skin.version.protocol.minor;
    s_eo_theskin.sharedcan.discoverytarget.info.firmware.major = servcfg->data.sk.skin.version.firmware.major; 
    s_eo_theskin.sharedcan.discoverytarget.info.firmware.minor = servcfg->data.sk.skin.version.firmware.minor;    
    s_eo_theskin.sharedcan.discoverytarget.info.firmware.build = servcfg->data.sk.skin.version.firmware.build;    
    
    // now i must do discovery of the patches. all patches can be at most on the two can buses ...
    // moreover, we cannot have more than .... eo_skin_maxnumberofMTBboards boards

    s_eo_theskin.sharedcan.discoverytarget.canmap[eOcanport1] = s_eo_theskin.sharedcan.discoverytarget.canmap[eOcanport2] = 0x0000;
    uint8_t i=0;
    for(i=0; i<servcfg->data.sk.skin.numofpatches; i++)
    {
        s_eo_theskin.sharedcan.discoverytarget.canmap[eOcanport1] |= servcfg->data.sk.skin.canmapskin[i][eOcanport1];
        s_eo_theskin.sharedcan.discoverytarget.canmap[eOcanport2] |= servcfg->data.sk.skin.canmapskin[i][eOcanport2];
    }
    
    uint8_t numofboards = eo_common_hlfword_bitsetcount(s_eo_theskin.sharedcan.discoverytarget.canmap[eOcanport1]) +
                          eo_common_hlfword_bitsetcount(s_eo_theskin.sharedcan.discoverytarget.canmap[eOcanport2]);
    
    if(numofboards > eo_skin_maxnumberofMTBboards)
    {
        
        s_eo_theskin.diagnostics.errorDescriptor.sourcedevice       = eo_errman_sourcedevice_localboard;
        s_eo_theskin.diagnostics.errorDescriptor.sourceaddress      = 0;
        s_eo_theskin.diagnostics.errorDescriptor.par16              = (numofboards << 8) | (eo_skin_maxnumberofMTBboards & 0x00ff);
        s_eo_theskin.diagnostics.errorDescriptor.par64              = (s_eo_theskin.sharedcan.discoverytarget.canmap[eOcanport2] << 16) | (s_eo_theskin.sharedcan.discoverytarget.canmap[eOcanport1]);
       
        EOaction_strg astrg = {0};
        EOaction *act = (EOaction*)&astrg;
        eo_action_SetCallback(act, s_eo_skin_send_periodic_error_report, NULL, eov_callbackman_GetTask(eov_callbackman_GetHandle()));        
        
        s_eo_theskin.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_skin_failed_toomanyboards);
        s_eo_theskin.diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), s_eo_theskin.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_theskin.diagnostics.errorDescriptor);
        
        if(0 != s_eo_theskin.diagnostics.reportPeriod)
        {
            s_eo_theskin.diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(s_eo_theskin.diagnostics.reportTimer, eok_abstimeNOW, s_eo_theskin.diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);   
        }            
        
        return(eores_NOK_generic); 
    }
              
    

    s_eo_theskin.sharedcan.ondiscoverystop.function = s_eo_skin_onstop_search4mtbs;
    s_eo_theskin.sharedcan.ondiscoverystop.parameter = (void*)servcfg;
    
    // start discovery   
    eo_candiscovery2_Start(eo_candiscovery2_GetHandle(), &s_eo_theskin.sharedcan.discoverytarget, &s_eo_theskin.sharedcan.ondiscoverystop);   
   
    return(eores_OK);   
}


extern eOresult_t eo_skin_Deactivate(EOtheSKIN *p)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }

    if(eobool_false == s_eo_theskin.service.active)
    {
        return(eores_OK);        
    } 
    

    eo_skin_TXstop(&s_eo_theskin);

        
    eo_canmap_DeconfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_skin, eoprot_entity_sk_skin, s_eo_theskin.sharedcan.entitydescriptor); 
    
    eo_canmap_UnloadBoards(eo_canmap_GetHandle(), s_eo_theskin.sharedcan.boardproperties);
     
    
    eo_entities_SetNumOfSkins(eo_entities_GetHandle(), 0);
    
    s_eo_theskin.numofmtbs = 0;
    
    uint8_t i=0;
    for(i=0; i<eomn_serv_skin_maxpatches; i++)
    {
        s_eo_theskin.skinpatches[i] = NULL;
        s_eo_theskin.patchisrunning[i] = eobool_false;
        
        eo_vector_Clear(s_eo_theskin.rxdata[i]);
    }
    
    memset(&s_eo_theskin.service.servconfig, 0, sizeof(eOmn_serv_configuration_t));
    s_eo_theskin.service.servconfig.type = eomn_serv_NONE;
    
    eo_vector_Clear(s_eo_theskin.sharedcan.boardproperties);
    eo_vector_Clear(s_eo_theskin.sharedcan.entitydescriptor);
    
    // make sure the timer is not running
    eo_timer_Stop(s_eo_theskin.diagnostics.reportTimer);  
    
    s_eo_theskin.service.active = eobool_false;
    
    return(eores_OK);
}


extern eOresult_t eo_skin_Activate(EOtheSKIN *p, const eOmn_serv_configuration_t * servcfg)
{
    if((NULL == p) || (NULL == servcfg))
    {
        return(eores_NOK_nullpointer);
    } 

    if(eomn_serv_SK_skin != servcfg->type)
    {
        return(eores_NOK_generic);
    }
    
    if(eobool_true == s_eo_theskin.service.active)
    {
        eo_skin_Deactivate(p);        
    }   
    
    eo_entities_SetNumOfSkins(eo_entities_GetHandle(), servcfg->data.sk.skin.numofpatches);
    

    if(0 == eo_entities_NumOfSkins(eo_entities_GetHandle()))
    {
        s_eo_theskin.service.active = eobool_false;
        return(eores_NOK_generic);
    }
    else
    {         
        memcpy(&s_eo_theskin.service.servconfig, servcfg, sizeof(eOmn_serv_configuration_t));

        s_eo_theskin.numofskinpatches = servcfg->data.sk.skin.numofpatches;
        
        uint8_t i = 0;
        for(i=0; i<s_eo_theskin.numofskinpatches; i++)
        {
            s_eo_theskin.skinpatches[i] = eo_entities_GetSkin(eo_entities_GetHandle(), i);
        }
        
        // now i must add all the mtb boards. i iterate per patch and then per canbus
        
        eOcanmap_board_properties_t prop = 
        {
            .type               = eobrd_cantype_mtb, 
            .location           = { .port = 0, .addr = 0, .insideindex = eocanmap_insideindex_none },
            .requiredprotocol   = { .major = servcfg->data.sk.skin.version.protocol.major, .minor = servcfg->data.sk.skin.version.protocol.minor }
        };  
        
        eOcanmap_entitydescriptor_t des = 
        {
            .location   = { .port = 0, .addr = 0, .insideindex = eocanmap_insideindex_none },
            .index      = entindexNONE
        };        
        
        s_eo_theskin.numofmtbs = 0;
        
        uint8_t j = 0;
        uint8_t k = 0;
        for(i=0; i<s_eo_theskin.numofskinpatches; i++)
        {
            for(j=0; j<eOcanports_number; j++)
            {
                uint16_t canbusmapping = servcfg->data.sk.skin.canmapskin[i][j];
                for(k=1; k<15; k++)
                {
                    if(eobool_true == eo_common_hlfword_bitcheck(canbusmapping, k))
                    {   // i pushback. i dont verify vs teh capacity of the vector because eo_skin_Verify() has already done it
                        prop.location.port = j;
                        prop.location.addr = k;
                        eo_vector_PushBack(s_eo_theskin.sharedcan.boardproperties, &prop);
                        
                        des.location.port = j;
                        des.location.addr = k;
                        des.index = (eOcanmap_entityindex_t)i;                        
                        eo_vector_PushBack(s_eo_theskin.sharedcan.entitydescriptor, &des);
                        
                        s_eo_theskin.numofmtbs++;
                    }
                }
            }
        }
        
        // load the can mapping 
        eo_canmap_LoadBoards(eo_canmap_GetHandle(), s_eo_theskin.sharedcan.boardproperties); 
        
        // load the entity mapping.
        eo_canmap_ConfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_skin, eoprot_entity_sk_skin, s_eo_theskin.sharedcan.entitydescriptor);   

        s_eo_theskin.service.active = eobool_true;        
    }
    
    return(eores_OK);   
}



extern eOresult_t eo_skin_TXstop(EOtheSKIN *p)
{   
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if((eobool_false == s_eo_theskin.service.active))
    {   // nothing to do because we dont have skin service.active
        return(eores_OK);
    }
    
          
    // now, i do things. 
    
    icubCanProto_as_sigmode_t sigmode = icubCanProto_as_sigmode_dontsignal;
    s_eo_theskin.sharedcan.command.class = eocanprot_msgclass_pollingSkin;
    s_eo_theskin.sharedcan.command.type  = ICUBCANPROTO_POL_AS_CMD__SET_TXMODE;       
    s_eo_theskin.sharedcan.command.value = &sigmode;

    uint8_t i=0;
    eOsk_skin_t *skin = NULL;
    for(i=0; i<s_eo_theskin.numofskinpatches; i++)
    {
        
        if(NULL == (skin = s_eo_theskin.skinpatches[i]))
        {   
            continue;   // i dont have this skin ... i go the next one
        }
        
        // i stop this skin only if it was started before
        if(s_eo_theskin.patchisrunning[i] == eobool_false)
        {
            continue;
        }
         
        //#warning marco.accame: see if we can remove it .....
        // if we are in here it means that robotinterface has activated the skin after that the verify() was done succesfully
        // thus, we can remove the continue. BUT who cares t send the silence command or not if the patch is already silent ....
        if(eosk_sigmode_dontsignal == skin->config.sigmode)
        {   
            // if the skin was not initted by robot-interface, then i dont deinit it. the reason is twofold:
            // 1. if the skin boards dont transmit, there is no need to silence them,
            // 2. in case the .ini file of robotinterface has disable the skin because the skin is not mounted, i dont want to tx on a disconnected can bus.
            continue;   // i dont need to silence this skin ... i go the next one
        }
            
        // i set the skin as not transmitting as soon as possible. because in such a way, can messages being received
        // are not pushed back in skin->status.arrayofcandata and its does not overflow.
        skin->config.sigmode = eosk_sigmode_dontsignal;
              
        // then i stop transmission of each skin can board
       
        // i get the addresses of the can boards of the i-th skin.
        // the simplification we use is that they all are on the same CAN bus and all have consecutive addresses.
        // we send the same s_eo_theskin.sharedcan.command to all of them
        eOprotID32_t id32 = eoprot_ID_get(eoprot_endpoint_skin, eoprot_entity_sk_skin, i, 0);
        eo_canserv_SendCommandToAllBoardsInEntity(eo_canserv_GetHandle(), &s_eo_theskin.sharedcan.command, id32);    
        
        s_eo_theskin.patchisrunning[i] = eobool_false;
    }
        
    return(eores_OK);

}


extern eOresult_t eo_skin_SetMode(EOtheSKIN *p, uint8_t patchindex, eOsk_sigmode_t mode)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == s_eo_theskin.service.active)
    {   // nothing to do because we dont have skin service.active
        return(eores_OK);
    } 

    if(patchindex >= s_eo_theskin.numofskinpatches)
    {
        return(eores_NOK_generic);
    }
        
    s_eo_theskin.sharedcan.command.class = eocanprot_msgclass_pollingSkin;    
    s_eo_theskin.sharedcan.command.type  = ICUBCANPROTO_POL_AS_CMD__SET_TXMODE;
    s_eo_theskin.sharedcan.command.value = NULL;   

    eOprotID32_t id32 = eoprot_ID_get(eoprot_endpoint_skin, eoprot_entity_sk_skin, patchindex, eoprot_tag_none);
    
    switch(mode)
    {
        case eosk_sigmode_dontsignal:
        {
            // in old way it does not exist
            // in new way:
            icubCanProto_as_sigmode_t sigmode2use = icubCanProto_as_sigmode_dontsignal;
            s_eo_theskin.sharedcan.command.value = &sigmode2use;
            // and now we send the command to all the skin boards
            eo_canserv_SendCommandToAllBoardsInEntity(eo_canserv_GetHandle(), &s_eo_theskin.sharedcan.command, id32);
            // and we set this patch as not tx
            s_eo_theskin.patchisrunning[patchindex] = eobool_false;
            
        } break;
        
        case eosk_sigmode_signal:
        {
            icubCanProto_as_sigmode_t sigmode2use = icubCanProto_as_sigmode_signal;
            s_eo_theskin.sharedcan.command.value = &sigmode2use;
            // and now we send the command to all the skin boards
            eo_canserv_SendCommandToAllBoardsInEntity(eo_canserv_GetHandle(), &s_eo_theskin.sharedcan.command, id32);
            // and we set this patch as tx
            s_eo_theskin.patchisrunning[patchindex] = eobool_true;

        } break;

        case eosk_sigmode_signal_oldway:
        {
            // we need to change the class and type as well
            s_eo_theskin.sharedcan.command.class = eocanprot_msgclass_pollingSkin;    
            s_eo_theskin.sharedcan.command.type  = ICUBCANPROTO_POL_SK_CMD__TACT_SETUP;
            s_eo_theskin.sharedcan.command.value = NULL;     
            // and now we send the command to all the skin boards
            eo_canserv_SendCommandToAllBoardsInEntity(eo_canserv_GetHandle(), &s_eo_theskin.sharedcan.command, id32);
            // and we set this patch as tx
            s_eo_theskin.patchisrunning[patchindex] = eobool_true;
                       
            
            eOerrmanDescriptor_t errdes = {0};
            errdes.code             = eoerror_code_get(eoerror_category_Skin, eoerror_value_SK_obsoletecommand);
            errdes.sourcedevice     = eo_errman_sourcedevice_localboard;
            errdes.sourceaddress    = 0;
            errdes.par16            = 0;
            errdes.par64            = 0;
            eo_errman_Error(eo_errman_GetHandle(), eo_errortype_warning, NULL, s_eobj_ownname, &errdes);   
            
//            #warning TBD: VERY IMPORTANT -> see the following code, where we need also a special message to board with address 0xE
//
//            for(i=skconfig_ptr->boardAddrStart; i<boardEndAddr; i++)
//            {
//                
//                msgdest.dest = ICUBCANPROTO_MSGDEST_CREATE(0, i);
//                res = eo_appCanSP_SendCmd(appCanSP_ptr, skconfig_ptr->connected2emsport, msgdest, msgCmd, NULL);
//                if(eores_OK != res)
//                {
//                    return;
//                }
//                
//                if(0xE == i)
//                {
//                    eOicubCanProto_msgCommand_t msgCmd2 = 
//                    {
//                        EO_INIT(.class) icubCanProto_msgCmdClass_skinBoard,
//                        EO_INIT(.cmdId) ICUBCANPROTO_POL_SK_CMD__TACT_SETUP2
//                    };
//                    
//                    res = eo_appCanSP_SendCmd(appCanSP_ptr, skconfig_ptr->connected2emsport, msgdest, msgCmd2, NULL);
//                    if(eores_OK != res)
//                    {
//                        return;
//                    }
//                }
//            }
        } break;
        
        default:
        {
        } break;
    }        

    
    return(eores_OK);
}


extern eOresult_t eo_skin_SetBoardsConfig(EOtheSKIN *p, uint8_t patchindex, eOsk_cmd_boardsCfg_t *brdcfg)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == s_eo_theskin.service.active)
    {   // nothing to do because we dont have skin service.active
        return(eores_OK);
    } 

    if(patchindex >= s_eo_theskin.numofskinpatches)
    {
        return(eores_NOK_generic);
    }
    
    eOprotID32_t id32 = eoprot_ID_get(eoprot_endpoint_skin, eoprot_entity_sk_skin, patchindex, eoprot_tag_none);
    
    icubCanProto_skinboard_config_t canProto_skcfg = {.skintype = icubCanProto_skinType__withtempcomp, .period = 0, .noload = 0};
    canProto_skcfg.skintype = (icubCanProto_skinType_t)brdcfg->cfg.skintype; 
    canProto_skcfg.period   = brdcfg->cfg.period;
    canProto_skcfg.noload   = brdcfg->cfg.noload;
    
    s_eo_theskin.sharedcan.command.class = eocanprot_msgclass_pollingSkin;    
    s_eo_theskin.sharedcan.command.type  = ICUBCANPROTO_POL_SK_CMD__SET_BRD_CFG;
    s_eo_theskin.sharedcan.command.value = &canProto_skcfg; 
    
    // and now we send the s_eo_theskin.sharedcan.command to all the skin boards
    eo_canserv_SendCommandToAllBoardsInEntity(eo_canserv_GetHandle(), &s_eo_theskin.sharedcan.command, id32); 
    
    return(eores_OK);          
}


extern eOresult_t eo_skin_SetTrianglesConfig(EOtheSKIN *p, uint8_t patchindex, eOsk_cmd_trianglesCfg_t *trgcfg)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == s_eo_theskin.service.active)
    {   // nothing to do because we dont have a skin board
        return(eores_OK);
    } 

    if(patchindex >= s_eo_theskin.numofskinpatches)
    {
        return(eores_NOK_generic);
    }
    
    eOprotID32_t id32 = eoprot_ID_get(eoprot_endpoint_skin, eoprot_entity_sk_skin, patchindex, eoprot_tag_none);
    
    icubCanProto_skintriangles_config_t canProto_trgscfg = {0};
    canProto_trgscfg.idstart   = trgcfg->idstart;
    canProto_trgscfg.idend     = trgcfg->idend;
    canProto_trgscfg.flags     = trgcfg->cfg.enable;
    canProto_trgscfg.shift     = trgcfg->cfg.shift;
    canProto_trgscfg.CDCoffset = trgcfg->cfg.CDCoffset;
    
    
    s_eo_theskin.sharedcan.command.class = eocanprot_msgclass_pollingSkin;    
    s_eo_theskin.sharedcan.command.type  = ICUBCANPROTO_POL_SK_CMD__SET_TRIANG_CFG;
    s_eo_theskin.sharedcan.command.value = &canProto_trgscfg; 
    
    eOcanmap_location_t location = {0};
    eo_canmap_GetEntityLocation(eo_canmap_GetHandle(), id32, &location, NULL, NULL);
    // the function eo_canmap_GetEntityLocation() puts in location.addr the address of the first board of the entity (the patch).
    // we call eo_canmap_GetEntityLocation() to retrieve the canbus (port1 or port2) and insideindex.
    // then, as we already have the address in trgcfg->boardaddr, we copy it in location.addr.
    // if eOsk_cmd_trianglesCfg_t contained also port, we would not need to do that.... 
    location.addr = trgcfg->boardaddr;
    eo_canserv_SendCommandToLocation(eo_canserv_GetHandle(), &s_eo_theskin.sharedcan.command, location);    
   
    return(eores_OK);          
}


extern eOresult_t eo_skin_AcceptCANframe(EOtheSKIN *p, eOcanframe_t *frame, eOcanport_t port)
{
    if((NULL == p) || (NULL == frame))
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == s_eo_theskin.service.active)
    {   // nothing to do because we dont have skin service.active
        return(eores_OK);
    } 
    
    if(eobool_false == s_eo_skin_activeskin_can_accept_canframe())
    {
        return(eores_OK);
    }

 
    uint8_t index = 0;    
    eOsk_skin_t *skin = s_eo_skin_get_entity(frame, port, &index);

    if(index >= s_eo_theskin.numofskinpatches)
    {
        return(eores_NOK_generic);
    }
    
    eOsk_candata_t candata = {0};
    uint16_t info = EOSK_CANDATA_INFO(frame->size, frame->id);
    candata.info = info;    
    memcpy(candata.data, frame->data, sizeof(candata.data));  
    
    EOarray *array = (EOarray*)(&skin->status.arrayofcandata);
    
    if(eobool_false == eo_array_Full(array))
    {   // put it inside the status array
        eo_array_PushBack(array, &candata);        
    }
    else if(eobool_false == eo_vector_Full(s_eo_theskin.rxdata[index]))
    {   // put it inside the vector
        eo_vector_PushBack(s_eo_theskin.rxdata[index], &candata);
    }
    else
    {   // damn... a loss of can frames
        eOerrmanDescriptor_t des = {0};
        des.code            = eoerror_code_get(eoerror_category_Skin, eoerror_value_SK_arrayofcandataoverflow);
        des.par16           = (frame->id & 0x0fff) | ((frame->size & 0x000f) << 12);
        des.par64           = eo_common_canframe_data2u64((eOcanframe_t*)frame);
        des.sourceaddress   = EOCANPROT_FRAME_GET_SOURCE(frame);
        des.sourcedevice    = (eOcanport1 == port) ? (eo_errman_sourcedevice_canbus1) : (eo_errman_sourcedevice_canbus2);
        eo_errman_Error(eo_errman_GetHandle(), eo_errortype_warning, NULL, NULL, &des);         
    }

    return(eores_OK);
}


extern eOresult_t eo_skin_SendReport(EOtheSKIN *p)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }

    eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
    
    eOerror_value_t errorvalue = eoerror_code2value(p->diagnostics.errorDescriptor.code);
    
    switch(errorvalue)
    {
        case eoerror_value_CFG_skin_failed_candiscovery:
        {
            eo_candiscovery2_SendLatestSearchResults(eo_candiscovery2_GetHandle());            
        } break;
        
        default:
        {
            // dont send any additional info
        } break;
    }       

    
    return(eores_OK);      
}


extern eOresult_t eo_skin_Start(EOtheSKIN *p)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == s_eo_theskin.service.active)
    {   // nothing to do because we dont have a skin
        return(eores_OK);
    }  
    
    uint8_t i = 0;
    for(i=0; i<s_eo_theskin.numofskinpatches; i++)
    {
        eo_skin_SetMode(p, i, eosk_sigmode_signal);
    }
    
    return(eores_OK);   
}


extern eOresult_t eo_skin_Stop(EOtheSKIN *p)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == s_eo_theskin.service.active)
    {   // nothing to do because we dont have a skin board
        return(eores_OK);
    }  
    
    return(eo_skin_TXstop(p));        
}


extern eOresult_t eo_skin_Tick(EOtheSKIN *p, eObool_t regularROPSjustTransmitted)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == s_eo_theskin.service.active)
    {   // nothing to do because we dont have skin service.active
        return(eores_OK);
    } 
    
    uint8_t i = 0;
    
    
    for(i=0; i<s_eo_theskin.numofskinpatches; i++)
    {
        EOarray *array = (EOarray*) (&s_eo_theskin.skinpatches[i]->status.arrayofcandata);
        EOvector *vector = (EOvector*) s_eo_theskin.rxdata[i];
        
        if(eobool_true == regularROPSjustTransmitted)
        {
            eo_array_Reset(array);            
        }
        
        if(eobool_true == eo_array_Full(array))
        {
            continue;
        }
        
        if(eobool_true == eo_vector_Empty(vector))
        {
            continue;
        }
        
        // load all what i can from vector into array.
        
        uint8_t availabledestination = eo_array_Available(array);
        uint8_t sizesource = eo_vector_Size(vector);
        uint8_t numofitems2move = EO_MIN(availabledestination, sizesource);
        uint8_t j = 0;
        for(j=0; j<numofitems2move; j++)
        {
            eOsk_candata_t *candata = (eOsk_candata_t*)eo_vector_Front(vector);
            eo_array_PushBack(array, candata);
            eo_vector_PopFront(vector);            
        }               
    }

    
    return(eores_OK);
}


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------

// - in here i put the functions used to initialise the values in ram of the skin ... better in here rather than elsewhere.

// marco.accame: we start with a silent skin and with an empty status_arrayofcandata.
// then we put skin in sigmode eosk_sigmode_signal only if robotInterface orders it to the board.


extern void eoprot_fun_INIT_sk_skin_config_sigmode(const EOnv* nv)
{
    eOsk_sigmode_t *sigmode = (eOsk_sigmode_t*)eo_nv_RAM(nv);
    
    *sigmode = eosk_sigmode_dontsignal; 
}


extern void eoprot_fun_INIT_sk_skin_status_arrayofcandata(const EOnv* nv)
{
    EOarray_of_skincandata_t *tmp = (EOarray_of_skincandata_t*)eo_nv_RAM(nv);
    tmp = tmp;
    // marco.accame: items of array are eOsk_candata_t. its capacity is:
    uint16_t capacity = sizeof(tmp->data) / sizeof(eOsk_candata_t);    
    // eo_array_New() initialises capacity and itemsize and also sets size to 0 
    EOarray *array = eo_array_New(capacity, sizeof(eOsk_candata_t), eo_nv_RAM(nv));
}

extern eObool_t eocanprotSKperiodic_redefinable_SkipParsingOf_ANY_PERIODIC_SKIN_MSG(eOsk_skin_t *skin)
{    
    if(NULL == skin)
    {
        return(eobool_true);
    }
    
    // marco.accame:        
    // if skin->config.sigmode is eosk_sigmode_dontsignal then we dont need using the payload of the can frame. 
    // however, also if skin->config.sigmode is eosk_sigmode_signal but we are not in RUN mode we should not put 
    // frames inside the arrayofcandata. this latter for example is tha case if we are still in the cfg->run transition 
    // and thus not  yet inside the control-loop which empties the arrayofcandata, or also if  the udp packet with go2run 
    // rop<> gets lost.
    
    // we may decode some canframes of this kind if we pass from run to config mode and we process frame buffered in the rx-fifo    
    if(eosk_sigmode_dontsignal == skin->config.sigmode)
    {
        return(eobool_true);
    } 

    eOsmStatesEMSappl_t applstate = eo_sm_emsappl_STcfg;
    eom_emsappl_GetCurrentState(eom_emsappl_GetHandle(), &applstate);   
    if(eo_sm_emsappl_STrun != applstate)
    {
        return(eobool_true);
    }
    
    return(eobool_false);
}


// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------

//#define EOSKIN_ALWAYS_ACCEPT_CANFRAMES_IN_ACTIVE_MODE
static eObool_t s_eo_skin_activeskin_can_accept_canframe(void)
{
#if     defined(EOSKIN_ALWAYS_ACCEPT_CANFRAMES_IN_ACTIVE_MODE)
    // in this case the service.active skin always accepts can frames, even if the run mode is not.
    // it is up to robotInterface to put system in run mode, so that the overflow of fifo is avoided
    return(eobool_true);
#else
    // in this mode the service.active skin accepts can frames only if we are in run mode, thus the fifo can be emptied.    
    eOsmStatesEMSappl_t applstate = eo_sm_emsappl_STerr;
    eom_emsappl_GetCurrentState(eom_emsappl_GetHandle(), &applstate);   
    if(eo_sm_emsappl_STrun == applstate)
    {
        return(eobool_true);
    }    
    return(eobool_false);    
#endif       
}


static eOsk_skin_t* s_eo_skin_get_entity(eOcanframe_t *frame, eOcanport_t port, uint8_t *index)
{
    eOsk_skin_t * ret = NULL;
    uint8_t ii = 0;
    eOcanmap_location_t loc = {0};
    
    loc.port = port;
    loc.addr = EOCANPROT_FRAME_GET_SOURCE(frame);    
    loc.insideindex = eocanmap_insideindex_none;
    
    ii = eo_canmap_GetEntityIndexExtraCheck(eo_canmap_GetHandle(), loc, eoprot_endpoint_skin, eoprot_entity_sk_skin);
    
    if(EOK_uint08dummy == ii)
    {  
        *index = ii;
        return(NULL);
    }
    
    ret = s_eo_theskin.skinpatches[ii];
    
    *index = ii;        

    return(ret);   
}



static eOresult_t s_eo_skin_onstop_search4mtbs(void *par, EOtheCANdiscovery2* p, eObool_t searchisok)
{
    const eOmn_serv_configuration_t * servcfg = (const eOmn_serv_configuration_t *)par;
    
    if((eobool_true == searchisok) && (eobool_true == s_eo_theskin.service.activateafterverify))
    {
        eo_skin_Activate(&s_eo_theskin, servcfg);        
    }

    s_eo_theskin.diagnostics.errorDescriptor.sourcedevice       = eo_errman_sourcedevice_localboard;
    s_eo_theskin.diagnostics.errorDescriptor.sourceaddress      = 0;
    s_eo_theskin.diagnostics.errorDescriptor.par16              = servcfg->data.sk.skin.numofpatches;
    s_eo_theskin.diagnostics.errorDescriptor.par64              = (servcfg->data.sk.skin.version.firmware.minor)      | (servcfg->data.sk.skin.version.firmware.major << 8)  |
                                                                 (servcfg->data.sk.skin.version.protocol.minor << 16) | (servcfg->data.sk.skin.version.protocol.major << 24) |
                                                                 ((uint64_t)s_eo_theskin.sharedcan.discoverytarget.canmap[eOcanport1] << 32) | ((uint64_t)s_eo_theskin.sharedcan.discoverytarget.canmap[eOcanport2] << 48);
   
    EOaction_strg astrg = {0};
    EOaction *act = (EOaction*)&astrg;
    eo_action_SetCallback(act, s_eo_skin_send_periodic_error_report, NULL, eov_callbackman_GetTask(eov_callbackman_GetHandle()));

    if(eobool_true == searchisok)
    {        
        s_eo_theskin.diagnostics.errorType = eo_errortype_debug;
        s_eo_theskin.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_skin_ok);
        eo_errman_Error(eo_errman_GetHandle(), s_eo_theskin.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_theskin.diagnostics.errorDescriptor);
        
        if((0 != s_eo_theskin.diagnostics.repetitionOKcase) && (0 != s_eo_theskin.diagnostics.reportPeriod))
        {
            s_eo_theskin.diagnostics.errorCallbackCount = s_eo_theskin.diagnostics.repetitionOKcase;        
            eo_timer_Start(s_eo_theskin.diagnostics.reportTimer, eok_abstimeNOW, s_eo_theskin.diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    }
    
    
    if(eobool_false == searchisok)
    {   
        // in case we fail the discovery, it is better to put inside par64 the failure of discovery.
        // maskofmissing, maskofwronglydetected, 
        
        const eOcandiscovery_detection_t* detection = eo_candiscovery2_GetDetection(eo_candiscovery2_GetHandle());
                
        uint16_t maskofmissingCAN1 = s_eo_theskin.sharedcan.discoverytarget.canmap[0] & (~detection->replies[0]);
        uint16_t maskofmissingCAN2 = s_eo_theskin.sharedcan.discoverytarget.canmap[1] & (~detection->replies[1]);
        uint16_t maskofincompatibleCAN1 = detection->incompatibilities[0]; 
        uint16_t maskofincompatibleCAN2 = detection->incompatibilities[1]; 
        
        s_eo_theskin.diagnostics.errorDescriptor.par64 = ((uint64_t)maskofmissingCAN1) | ((uint64_t)maskofmissingCAN2<<16) | ((uint64_t)maskofincompatibleCAN1<<32) | ((uint64_t)maskofincompatibleCAN2<<32);
        
        s_eo_theskin.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_skin_failed_candiscovery);
        s_eo_theskin.diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), s_eo_theskin.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_theskin.diagnostics.errorDescriptor);
        
        if(0 != s_eo_theskin.diagnostics.reportPeriod)
        {
            s_eo_theskin.diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(s_eo_theskin.diagnostics.reportTimer, eok_abstimeNOW, s_eo_theskin.diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    }  
    
    if(NULL != s_eo_theskin.service.onverify)
    {
        s_eo_theskin.service.onverify(&s_eo_theskin, searchisok); 
    }    
    
    return(eores_OK);
    
}


static void s_eo_skin_send_periodic_error_report(void *p)
{
    eo_errman_Error(eo_errman_GetHandle(), s_eo_theskin.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_theskin.diagnostics.errorDescriptor);
    
    if(eoerror_value_CFG_skin_failed_candiscovery == eoerror_code2value(s_eo_theskin.diagnostics.errorDescriptor.code))
    {   // if i dont find the mtbs, i keep on sending the discovery results up. it is a temporary diagnostics tricks until we use the verification of services at bootstrap
        eo_candiscovery2_SendLatestSearchResults(eo_candiscovery2_GetHandle());
    }    
    
    if(EOK_int08dummy != s_eo_theskin.diagnostics.errorCallbackCount)
    {
        s_eo_theskin.diagnostics.errorCallbackCount--;
    }
    if(0 == s_eo_theskin.diagnostics.errorCallbackCount)
    {
        eo_timer_Stop(s_eo_theskin.diagnostics.reportTimer);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------



