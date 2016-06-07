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

#include "EoProtocolAS.h"
#include "EoProtocolMC.h"

#include "EOMtheEMSappl.h"

#include "EOmcController.h"
#include "Controller.h"

#include "hal_sys.h"
#include "hal_motor.h"
#include "hal_adc.h"
#include "hal_quadencoder.h"

#include "EOCurrentsWatchdog.h"

#include "EOVtheCallbackManager.h"



// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------

#include "EOtheMotionController.h"



// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------

#include "EOtheMotionController_hid.h"


// --------------------------------------------------------------------------------------------------------------------
// - #define with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of extern variables. deprecated: better using _get(), _set() on static variables 
// --------------------------------------------------------------------------------------------------------------------
// empty-section

// --------------------------------------------------------------------------------------------------------------------
// - typedef with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section

#warning marco.accame: remember to process the tags 'marco.accame.TODO'

// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------

static void s_eo_motioncontrol_send_periodic_error_report(void *par);

static eOresult_t s_eo_motioncontrol_mc4plus_onendofverify_encoder(EOaService* s, eObool_t operationisok);

static eOresult_t s_eo_motioncontrol_foc_onendofverify_encoder(EOaService* s, eObool_t operationisok);

static eOresult_t s_eo_motioncontrol_onstop_search4focs(void *par, EOtheCANdiscovery2* p, eObool_t searchisok);

static eOresult_t s_eo_motioncontrol_onendofverify_mais(EOaService* s, eObool_t operationisok);

static eOresult_t s_eo_motioncontrol_onstop_search4mc4s(void *par, EOtheCANdiscovery2* p, eObool_t searchisok);

static eOresult_t s_eo_motioncontrol_mc4plusmais_onendofverify_encoder_BIS(EOaService* s, eObool_t operationisok);
    
static void s_eo_motioncontrol_UpdateJointMotorStatus(EOtheMotionController *p);

static void s_eo_motioncontrol_proxy_config(EOtheMotionController* p, eObool_t on);

static eObool_t s_eo_motioncontrol_mc4based_variableisproxied(eOnvID32_t id);

static void s_eo_motioncontrol_mc4plusbased_hal_init_motors_adc_feedbacks(void);
static void s_eo_motioncontrol_mc4plusbased_hal_init_quad_enc_indexes_interrupt(void);


static eObool_t s_eo_motioncontrol_isID32relevant(uint32_t id32);

static eOresult_t s_eo_motioncontrol_updatedPositionsFromEncoders(EOtheMotionController *p);

// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------

static EOtheMotionController s_eo_themotcon = 
{
    .service = 
    {
        .tmpcfg                 = NULL,
        .servconfig             = { .type = eomn_serv_NONE },
        .initted                = eobool_false,
        .active                 = eobool_false,
        .activateafterverify    = eobool_false,
        .started                = eobool_false,
        .onverify               = NULL,
        .state                  = eomn_serv_state_notsupported         
    },
    .diagnostics = 
    {
        .reportTimer            = NULL,
        .reportPeriod           = 0, // 10*EOK_reltime1sec, // with 0 we dont periodically report
        .errorDescriptor        = {0},
        .errorType              = eo_errortype_info,
        .errorCallbackCount     = 0,
        .repetitionOKcase       = 0 // 10 // with 0 we transmit report only once at succesful activation
    },     
    .sharedcan =
    {
        .boardproperties        = NULL,
        .entitydescriptor       = NULL,
        .discoverytarget        = {0},
        .ondiscoverystop        = {0},
        .command                = {0}, 
    },
    
    .numofjomos                 = 0,
    
    .ctrlobjs =
    {
        .servconfigmais         = {0},
        .themais                = NULL,
        .themc4boards           = NULL,
        
        .jomodescriptors        = NULL,
        .thecontroller          = NULL,
        .theencoderreader       = NULL,


        .pwmvalue               = {0},
        .pwmport                = {hal_motorNONE},
        .currents               = {0},
        .voltage                = 0
    },

    .id32ofregulars             = NULL
};

static const char s_eobj_ownname[] = "EOtheMotionController";


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------


extern EOtheMotionController* eo_motioncontrol_Initialise(void)
{
    EOtheMotionController *p = &s_eo_themotcon;
    
    if(eobool_true == p->service.initted)
    {
        return(p);
    }

    p->numofjomos = 0;

    p->service.tmpcfg = NULL;
    p->service.servconfig.type = eomn_serv_NONE;
    
    // up to to 12 mc4 OR upto 4 foc (the MAIS is managed directly by the EOtheMAIS object)
    p->sharedcan.boardproperties = eo_vector_New(sizeof(eObrd_canproperties_t), eo_motcon_maxJOMOs, NULL, NULL, NULL, NULL);
    
    // up to 12 jomos
    p->sharedcan.entitydescriptor = eo_vector_New(sizeof(eOcanmap_entitydescriptor_t), eo_motcon_maxJOMOs, NULL, NULL, NULL, NULL);
    
    p->id32ofregulars = eo_array_New(motioncontrol_maxRegulars, sizeof(uint32_t), NULL);
    
    p->ctrlobjs.thecontroller = MController_new(eo_motcon_standardJOMOs, eo_motcon_standardJOMOs);
    p->ctrlobjs.theencoderreader = eo_encoderreader_Initialise();
            
    p->ctrlobjs.themais = eo_mais_Initialise();
           
    p->diagnostics.reportTimer = eo_timer_New();   
    p->diagnostics.errorType = eo_errortype_error;
    p->diagnostics.errorDescriptor.sourceaddress = eo_errman_sourcedevice_localboard;
    p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_not_verified_yet);    

    p->service.initted = eobool_true;
    p->service.active = eobool_false;
    p->service.started = eobool_false;
    p->service.state = eomn_serv_state_idle;    
    eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);
    
    return(p);
}


extern EOtheMotionController* eo_motioncontrol_GetHandle(void)
{
    if(eobool_true == s_eo_themotcon.service.initted)
    {
        return(&s_eo_themotcon);
    }
    
    return(NULL);
}


extern eOmn_serv_state_t eo_motioncontrol_GetServiceState(EOtheMotionController *p)
{
    if(NULL == p)
    {
        return(eomn_serv_state_notsupported);
    } 

    return(p->service.state);    
}


extern eOresult_t eo_motioncontrol_SendReport(EOtheMotionController *p)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }

    eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
    
    eOerror_value_t errorvalue = eoerror_code2value(p->diagnostics.errorDescriptor.code);
    
    switch(errorvalue)
    {
        case eoerror_value_CFG_mc_foc_failed_candiscovery_of_foc:
        case eoerror_value_CFG_mc_mc4_failed_candiscovery_of_mc4:
        {
            eo_candiscovery2_SendLatestSearchResults(eo_candiscovery2_GetHandle());            
        } break;
        
        case eoerror_value_CFG_mc_mc4_failed_mais_verify:
        case eoerror_value_CFG_mc_mc4plusmais_failed_candiscovery_of_mais:
        {
            eo_mais_SendReport(eo_mais_GetHandle());
        } break;
        
        case eoerror_value_CFG_mc_foc_failed_encoders_verify:
        case eoerror_value_CFG_mc_mc4plus_failed_encoders_verify:
        case eoerror_value_CFG_mc_mc4plusmais_failed_encoders_verify:
        {
            eo_encoderreader_SendReport(eo_encoderreader_GetHandle());
        } break;
        
        default:
        {
            // dont send any additional info
        } break;
    }
    
    
    return(eores_OK);      
}


extern eOmotioncontroller_mode_t eo_motioncontrol_GetMode(EOtheMotionController *p)
{
    if(NULL == p)
    {
        return(eo_motcon_mode_NONE);
    }
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eo_motcon_mode_NONE);
    } 

    return((eOmotioncontroller_mode_t)p->service.servconfig.type);
}


extern eOresult_t eo_motioncontrol_Verify(EOtheMotionController *p, const eOmn_serv_configuration_t * servcfg, eOservice_onendofoperation_fun_t onverify, eObool_t activateafterverify)
{
    //eo_errman_Trace(eo_errman_GetHandle(), "called: eo_motioncontrol_Verify()", s_eobj_ownname);
    
    if((NULL == p) || (NULL == servcfg))
    {
        s_eo_themotcon.service.state = eomn_serv_state_failureofverify;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, s_eo_themotcon.service.state);
        if(NULL != onverify)
        {
            onverify(p, eobool_false); 
        }        
        return(eores_NOK_nullpointer);
    }  
    
    
    if((eo_motcon_mode_foc != servcfg->type) && (eo_motcon_mode_mc4 != servcfg->type) && (eo_motcon_mode_mc4plus != servcfg->type) && (eo_motcon_mode_mc4plusmais != servcfg->type))
    {
        p->service.state = eomn_serv_state_failureofverify;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);
        if(NULL != onverify)
        {
            onverify(p, eobool_false); 
        }         
        return(eores_NOK_generic);
    }

    if(eobool_true == p->service.active)
    {
        eo_motioncontrol_Deactivate(p);        
    } 
    
    p->service.state = eomn_serv_state_verifying;
    eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);

    // make sure the timer is not running
    eo_timer_Stop(p->diagnostics.reportTimer);        
    
    
    // marco.accame on 13 may 2016: i prefer copying what the pointer servcfg contains inside the object. 
    // the reason is that we are NOT guaranteed that the memory pointer by servcfg stays constant all the life of _Verify() actions, which can last well after the exit of the function.
    // in the past, servcfg was a pointer to ROM data stored in FLASH, hence ist stayed constant, but with runtime configuration it is data kept in the RAM of a UDP packet which is not guaranteed to stay unchanged
    // in here we use memmove() to be more guaranteed in case that source and destination are in the same location.
    memmove(&p->service.servconfig, servcfg, sizeof(eOmn_serv_configuration_t));
    
    if(eo_motcon_mode_foc == p->service.servconfig.type)
    {
        p->ctrlobjs.jomodescriptors = eo_constarray_Load((EOarray*)&p->service.servconfig.data.mc.foc_based.arrayofjomodescriptors);
        
        p->service.onverify = onverify;
        p->service.activateafterverify = activateafterverify;

        // 1. prepare the can discovery for foc boards 
        memset(&p->sharedcan.discoverytarget, 0, sizeof(p->sharedcan.discoverytarget));
        p->sharedcan.discoverytarget.info.type = eobrd_cantype_foc;
        p->sharedcan.discoverytarget.info.protocol.major = p->service.servconfig.data.mc.foc_based.version.protocol.major; 
        p->sharedcan.discoverytarget.info.protocol.minor = p->service.servconfig.data.mc.foc_based.version.protocol.minor;
        p->sharedcan.discoverytarget.info.firmware.major = p->service.servconfig.data.mc.foc_based.version.firmware.major; 
        p->sharedcan.discoverytarget.info.firmware.minor = p->service.servconfig.data.mc.foc_based.version.firmware.minor;   
        p->sharedcan.discoverytarget.info.firmware.build = p->service.servconfig.data.mc.foc_based.version.firmware.build;   
                
        
        uint8_t numofjomos = eo_constarray_Size(p->ctrlobjs.jomodescriptors);
        uint8_t i = 0;
        for(i=0; i<numofjomos; i++)
        {
            const eOmn_serv_jomo_descriptor_t *jomodes = (eOmn_serv_jomo_descriptor_t*) eo_constarray_At(p->ctrlobjs.jomodescriptors, i);
            eo_common_hlfword_bitset(&p->sharedcan.discoverytarget.canmap[jomodes->actuator.foc.canloc.port], jomodes->actuator.foc.canloc.addr);         
        }
                
        p->sharedcan.ondiscoverystop.function = s_eo_motioncontrol_onstop_search4focs;
        p->sharedcan.ondiscoverystop.parameter = (void*)&p->service.servconfig;
        
        // 2. at first i verify the encoders. then, function s_eo_motioncontrol_foc_onendofverify_encoder() shall either issue an encoder error or start discovery of foc boards
        eo_encoderreader_Verify(eo_encoderreader_GetHandle(), &p->service.servconfig.data.mc.foc_based.arrayofjomodescriptors, s_eo_motioncontrol_foc_onendofverify_encoder, eobool_true); 
        
    }
    else if(eo_motcon_mode_mc4 == p->service.servconfig.type)
    {
        p->ctrlobjs.jomodescriptors = NULL;
        
        p->service.onverify = onverify;
        p->service.activateafterverify = activateafterverify;
        
        // 1. prepare the verify of mais
        p->ctrlobjs.servconfigmais.type = eomn_serv_AS_mais;
        memmove(&p->ctrlobjs.servconfigmais.data.as.mais, &p->service.servconfig.data.mc.mc4_based.mais, sizeof(eOmn_serv_config_data_as_mais_t)); 

        // 2. prepare the can discovery of mc4 boards
        memset(&p->sharedcan.discoverytarget, 0, sizeof(p->sharedcan.discoverytarget));
        p->sharedcan.discoverytarget.info.type = eobrd_cantype_mc4;
        p->sharedcan.discoverytarget.info.protocol.major = p->service.servconfig.data.mc.mc4_based.mc4version.protocol.major; 
        p->sharedcan.discoverytarget.info.protocol.minor = p->service.servconfig.data.mc.mc4_based.mc4version.protocol.minor;
        p->sharedcan.discoverytarget.info.firmware.major = p->service.servconfig.data.mc.mc4_based.mc4version.firmware.major; 
        p->sharedcan.discoverytarget.info.firmware.minor = p->service.servconfig.data.mc.mc4_based.mc4version.firmware.minor; 
        p->sharedcan.discoverytarget.info.firmware.build = p->service.servconfig.data.mc.mc4_based.mc4version.firmware.build;        
        
        uint8_t i = 0;
        for(i=0; i<eo_motcon_maxJOMOs; i++)
        {            
            const eObrd_canlocation_t *canloc = &p->service.servconfig.data.mc.mc4_based.mc4joints[i];
            eo_common_hlfword_bitset(&p->sharedcan.discoverytarget.canmap[canloc->port], canloc->addr);         
        }
                       
        p->sharedcan.ondiscoverystop.function = s_eo_motioncontrol_onstop_search4mc4s;
        p->sharedcan.ondiscoverystop.parameter = (void*)&p->service.servconfig;
             
        // 3. at first i verify the mais. then, function s_eo_motioncontrol_onendofverify_mais() shall either issue a mais error or start discovery of mc4 boards      
        eo_mais_Verify(eo_mais_GetHandle(), &p->ctrlobjs.servconfigmais, s_eo_motioncontrol_onendofverify_mais, eobool_true);
      
    }
    else if(eo_motcon_mode_mc4plus == p->service.servconfig.type)
    {
        p->ctrlobjs.jomodescriptors = eo_constarray_Load((EOarray*)&p->service.servconfig.data.mc.mc4plus_based.arrayofjomodescriptors);
        
        // marco.accame.TODO: fix it. we must be sure that in encoder-reader the read from adc is ok during its _Verify()
        // for now this call is inside eo_motioncontrol_Activate(), just before eo_encoderreader_Activate() ... 
        // marco.accame: it is required to read adc values if the encoder is absanalog (aka adc).
        // s_eo_motioncontrol_mc4plusbased_hal_init_motors_adc_feedbacks();
        
        // for now verify the encoder reader. 
        // we dont verify the pwm actuators. only way to do that is to add a hal_pwm_supported_is()
        p->service.onverify = onverify;
        p->service.activateafterverify = activateafterverify;
        
        eo_encoderreader_Verify(eo_encoderreader_GetHandle(), &p->service.servconfig.data.mc.mc4plus_based.arrayofjomodescriptors, s_eo_motioncontrol_mc4plus_onendofverify_encoder, eobool_true);           
    }
    else if(eo_motcon_mode_mc4plusmais == p->service.servconfig.type)
    {
        p->ctrlobjs.jomodescriptors = eo_constarray_Load((EOarray*)&p->service.servconfig.data.mc.mc4plusmais_based.arrayofjomodescriptors);
        
        // marco.accame.TODO: fix it. we must be sure that in encoder-reader the read from adc is ok during its _Verify()
        // marco.accame: it is required to read adc values if the encoder is absanalog (aka adc).
        // s_eo_motioncontrol_mc4plusbased_hal_init_motors_adc_feedbacks();       
        
        p->service.onverify = onverify;
        p->service.activateafterverify = activateafterverify;
        
        // prepare the verify of mais
        p->ctrlobjs.servconfigmais.type = eomn_serv_AS_mais;
        memmove(&p->ctrlobjs.servconfigmais.data.as.mais, &p->service.servconfig.data.mc.mc4plusmais_based.mais, sizeof(eOmn_serv_config_data_as_mais_t));    
   
        // at first i verify the mais. then, function s_eo_motioncontrol_onendofverify_mais() shall either issue a mais error or start verification of encoders (if mode is eo_motcon_mode_mc4plusmais)     
        eo_mais_Verify(eo_mais_GetHandle(), &p->ctrlobjs.servconfigmais, s_eo_motioncontrol_onendofverify_mais, eobool_true);          
    }

    return(eores_OK);   
}


extern eOresult_t eo_motioncontrol_Deactivate(EOtheMotionController *p)
{
    // eo_errman_Trace(eo_errman_GetHandle(), "called: eo_motioncontrol_Deactivate()", s_eobj_ownname);
    
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }

    if(eobool_false == p->service.active)
    {
        // i force to eomn_serv_state_idle because it may be that state was eomn_serv_state_verified or eomn_serv_state_failureofverify
        p->service.state = eomn_serv_state_idle; 
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);
        return(eores_OK);        
    } 
    
// in here we deactivate everything    
//    // for now i allow deactivate() only for the mc4-based control .. because at date of 13 april 2016 the local mc control cannot be destructed.
//    // then we deconfig things ... so far only for eo_motcon_mode_mc4.
//    if(eo_motcon_mode_mc4 != p->service.servconfig.type) 
//    {
//        return(eo_motioncontrol_Stop(p));
//    }
    
    
    // at first we stop the service
    if(eobool_true == p->service.started)
    {
        eo_motioncontrol_Stop(p);   
    }    

    eo_motioncontrol_SetRegulars(p, NULL, NULL);
    

    if(eo_motcon_mode_foc == p->service.servconfig.type)
    {       
        // for foc-based, we must ... deconfig mc foc boards, unload them, set num of entities = 0, clear status, deactivate encoder 
        
        eo_canmap_DeconfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_joint, p->sharedcan.entitydescriptor); 
        eo_canmap_DeconfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_motor, p->sharedcan.entitydescriptor); 
        

        eo_canmap_UnloadBoards(eo_canmap_GetHandle(), p->sharedcan.boardproperties); 
        
        eo_encoderreader_Deactivate(eo_encoderreader_GetHandle());
        
        // marco.accame.TODO: call deinit of mcontroller
        
        // now i reset 
        eo_vector_Clear(p->sharedcan.entitydescriptor);
        eo_vector_Clear(p->sharedcan.boardproperties);     

        // proxy deconfig
        s_eo_motioncontrol_proxy_config(p, eobool_false);        
    
    }
    else if(eo_motcon_mode_mc4 == p->service.servconfig.type)
    {        
        // for mc4-based, we must ... deconfig mc4 foc boards, unload them, set num of entities = 0, clear status, deactivate mais 
        
        eo_canmap_DeconfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_joint, p->sharedcan.entitydescriptor); 
        eo_canmap_DeconfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_motor, p->sharedcan.entitydescriptor); 
        

        eo_canmap_UnloadBoards(eo_canmap_GetHandle(), p->sharedcan.boardproperties); 
               
        // now i reset 
        eo_vector_Clear(p->sharedcan.entitydescriptor);
        eo_vector_Clear(p->sharedcan.boardproperties);
        
        eo_mais_Deactivate(eo_mais_GetHandle());        
        memset(&p->ctrlobjs.servconfigmais, 0, sizeof(p->ctrlobjs.servconfigmais));   

        // proxy deconfig
        s_eo_motioncontrol_proxy_config(p, eobool_false); 

    }
    else if((eo_motcon_mode_mc4plus == p->service.servconfig.type) || (eo_motcon_mode_mc4plusmais == p->service.servconfig.type))   
    {
        
        if(eo_motcon_mode_mc4plusmais == p->service.servconfig.type)
        {
            eo_mais_Deactivate(eo_mais_GetHandle());        
            memset(&p->ctrlobjs.servconfigmais, 0, sizeof(p->ctrlobjs.servconfigmais));     
        }            
        
        eo_encoderreader_Deactivate(eo_encoderreader_GetHandle());
        // marco.accame.TODO: in eo_motioncontrol_Deactivate() for mc4plus call: eo_emsController_Deinit()
        // marco.accame.TODO: in eo_motioncontrol_Deactivate() for mc4plus call: disable the motors ...
            
        // marco.accame.TODO: call deinit of mcontroller
            
    }        
  
    
    p->numofjomos = 0;
    eo_entities_SetNumOfJoints(eo_entities_GetHandle(), 0);
    eo_entities_SetNumOfMotors(eo_entities_GetHandle(), 0);
    
    p->ctrlobjs.jomodescriptors = NULL;
    
    memset(&p->service.servconfig, 0, sizeof(eOmn_serv_configuration_t));
    p->service.servconfig.type = eo_motcon_mode_NONE;
    memset(&p->sharedcan.discoverytarget, 0, sizeof(eOcandiscovery_target_t));
    
    
    p->service.onverify = NULL;
    p->service.activateafterverify = eobool_false;
    p->sharedcan.ondiscoverystop.function = NULL;
    p->sharedcan.ondiscoverystop.parameter = NULL;
    
    // make sure the timer is not running
    eo_timer_Stop(p->diagnostics.reportTimer);  
    
    p->service.active = eobool_false;    
    p->service.state = eomn_serv_state_idle; 
    eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);    
    
    return(eores_OK);
}


extern eOresult_t eo_motioncontrol_Activate(EOtheMotionController *p, const eOmn_serv_configuration_t * servcfg)
{
    // eo_errman_Trace(eo_errman_GetHandle(), "called: eo_motioncontrol_Activate()", s_eobj_ownname);
    
    if((NULL == p) || (NULL == servcfg))
    {
        return(eores_NOK_nullpointer);
    }  

    if((eo_motcon_mode_foc != servcfg->type) && (eo_motcon_mode_mc4 != servcfg->type) && (eo_motcon_mode_mc4plus != servcfg->type) && (eo_motcon_mode_mc4plusmais != servcfg->type))
    {
        return(eores_NOK_generic);
    }    
    
    if(eobool_true == p->service.active)
    {
        eo_motioncontrol_Deactivate(p);        
    } 

    // we use memmove() because it may be that source and destination are in the same location.
    memmove(&p->service.servconfig, servcfg, sizeof(eOmn_serv_configuration_t));    

    if(eo_motcon_mode_foc == p->service.servconfig.type)
    {

        p->ctrlobjs.jomodescriptors = eo_constarray_Load((EOarray*)&p->service.servconfig.data.mc.foc_based.arrayofjomodescriptors);
        
        uint8_t numofjomos = eo_constarray_Size(p->ctrlobjs.jomodescriptors);
        
        eo_entities_SetNumOfJoints(eo_entities_GetHandle(), numofjomos);
        eo_entities_SetNumOfMotors(eo_entities_GetHandle(), numofjomos);
        

        if(0 == eo_entities_NumOfJoints(eo_entities_GetHandle()))
        {
            p->service.active = eobool_false;
            return(eores_NOK_generic);
        }
        else
        {                          
            p->numofjomos = numofjomos;
            
            uint8_t i = 0;
                        
            // load the can mapping 
            for(i=0; i<numofjomos; i++)
            {
                const eOmn_serv_jomo_descriptor_t *jomodes = (eOmn_serv_jomo_descriptor_t*) eo_constarray_At(p->ctrlobjs.jomodescriptors, i);
                
                eObrd_canproperties_t prop = {0};
                
                prop.type = eobrd_cantype_foc;
                prop.location.port = jomodes->actuator.foc.canloc.port;
                prop.location.addr = jomodes->actuator.foc.canloc.addr;
                prop.location.insideindex = jomodes->actuator.foc.canloc.insideindex;
                prop.requiredprotocol.major = p->service.servconfig.data.mc.foc_based.version.protocol.major;
                prop.requiredprotocol.minor = p->service.servconfig.data.mc.foc_based.version.protocol.minor;
                
                eo_vector_PushBack(p->sharedcan.boardproperties, &prop);            
            }
            eo_canmap_LoadBoards(eo_canmap_GetHandle(), p->sharedcan.boardproperties); 
            
            // load the entity mapping.
            for(i=0; i<numofjomos; i++)
            {
                const eOmn_serv_jomo_descriptor_t *jomodes = (eOmn_serv_jomo_descriptor_t*) eo_constarray_At(p->ctrlobjs.jomodescriptors, i);
                
                eOcanmap_entitydescriptor_t des = {0};
                
                des.location.port = jomodes->actuator.foc.canloc.port;
                des.location.addr = jomodes->actuator.foc.canloc.addr;
                des.location.insideindex = jomodes->actuator.foc.canloc.insideindex;
                des.index = (eOcanmap_entityindex_t)i;

                eo_vector_PushBack(p->sharedcan.entitydescriptor, &des);            
            }        
            eo_canmap_ConfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_joint, p->sharedcan.entitydescriptor); 
            eo_canmap_ConfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_motor, p->sharedcan.entitydescriptor);        

            // init the encoders
            eo_encoderreader_Activate(eo_encoderreader_GetHandle(), (const eOmn_serv_arrayof_4jomodescriptors_t *) p->ctrlobjs.jomodescriptors);

            
            // init the emscontroller.
            // marco.accame.TODO: change the emsController. see comments below
            //                the emscontroller is not a singleton which can be initted and deinitted. 
            // it should have a _Initialise(), a _GetHandle(), a _Config(cfg) and a _Deconfig().
//            if(NULL == p->ctrlobjs.thecontroller)
//            {
//                //p->ctrlobjs.thecontroller = eo_emsController_Init((eOemscontroller_board_t)p->service.servconfig.data.mc.foc_based.boardtype4mccontroller, emscontroller_actuation_2FOC, numofjomos);
//                p->ctrlobjs.thecontroller = MController_new(numofjomos, numofjomos);
//                //MController_config_board((eOemscontroller_board_t)p->service.servconfig.data.mc.foc_based.boardtype4mccontroller, HARDWARE_2FOC);                
//                MController_config_board(&p->service.servconfig);
//            }
            
            MController_config_board(&p->service.servconfig);
            
            // proxy config
            s_eo_motioncontrol_proxy_config(p, eobool_true);
            
            p->service.active = eobool_true;
            p->service.state = eomn_serv_state_activated;
            eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);            
        }
    
    }
    else if(eo_motcon_mode_mc4 == p->service.servconfig.type)
    {
        p->ctrlobjs.jomodescriptors = NULL;
        
        const uint8_t numofjomos = eo_motcon_maxJOMOs;
        
        eo_entities_SetNumOfJoints(eo_entities_GetHandle(), numofjomos);
        eo_entities_SetNumOfMotors(eo_entities_GetHandle(), numofjomos);
        

        if(0 == eo_entities_NumOfJoints(eo_entities_GetHandle()))
        {
            p->service.active = eobool_false;
            return(eores_NOK_generic);
        }
        else
        {                
           
            p->numofjomos = numofjomos;
            
            uint8_t i = 0;
          
            eObrd_canproperties_t prop = {0};
            const eObrd_canlocation_t *canloc = NULL;
            
            // load the can mapping for the 12 boards ... (only mc4 boards as teh mais was added bt eo_mais_Activate()
            
            // push the 12 mc4 boards
            for(i=0; i<numofjomos; i++)
            {
                canloc = &p->service.servconfig.data.mc.mc4_based.mc4joints[i];
                               
                prop.type = eobrd_cantype_mc4;
                prop.location.port = canloc->port;
                prop.location.addr = canloc->addr;
                prop.location.insideindex = canloc->insideindex;
                prop.requiredprotocol.major = p->service.servconfig.data.mc.mc4_based.mc4version.protocol.major;
                prop.requiredprotocol.minor = p->service.servconfig.data.mc.mc4_based.mc4version.protocol.minor;
                
                eo_vector_PushBack(p->sharedcan.boardproperties, &prop);            
            }
           

            // load the 12 of them
            eo_canmap_LoadBoards(eo_canmap_GetHandle(), p->sharedcan.boardproperties); 
            
            // load the entity mapping.
            eOcanmap_entitydescriptor_t des = {0};
            for(i=0; i<numofjomos; i++)
            {               
                canloc = &p->service.servconfig.data.mc.mc4_based.mc4joints[i];
                                              
                des.location.port = canloc->port;
                des.location.addr = canloc->addr;
                des.location.insideindex = canloc->insideindex;
                des.index = (eOcanmap_entityindex_t)i;

                eo_vector_PushBack(p->sharedcan.entitydescriptor, &des);            
            }        
            eo_canmap_ConfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_joint, p->sharedcan.entitydescriptor); 
            eo_canmap_ConfigEntity(eo_canmap_GetHandle(), eoprot_endpoint_motioncontrol, eoprot_entity_mc_motor, p->sharedcan.entitydescriptor);        

//            // start the mais which was activated in s_eo_motioncontrol_onendofverify_mais()            
//            //eo_mais_Activate(eo_mais_GetHandle(), &p->ctrlobjs.servconfigmais);
//            eo_mais_Start(eo_mais_GetHandle());
//            eo_mais_Transmission(eo_mais_GetHandle(), eobool_true);
            
            // do something with the mc4s....
            
            p->ctrlobjs.themc4boards = eo_mc4boards_Initialise(NULL);            
            eo_mc4boards_Config(eo_mc4boards_GetHandle());
            
            // init others
            eo_virtualstrain_Initialise();
            // notused anymore: eo_motiondone_Initialise();
            
            
            // proxy config
            s_eo_motioncontrol_proxy_config(p, eobool_true);
                    
            p->service.active = eobool_true;
            p->service.state = eomn_serv_state_activated;
            eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);            
        }            
        
    }
    else if((eo_motcon_mode_mc4plus == p->service.servconfig.type) || (eo_motcon_mode_mc4plusmais == p->service.servconfig.type))
    {
        
        if(eo_motcon_mode_mc4plus == p->service.servconfig.type)
        {
            p->ctrlobjs.jomodescriptors = eo_constarray_Load((EOarray*)&p->service.servconfig.data.mc.mc4plus_based.arrayofjomodescriptors);
        }
        else //eomn_serv_MC_mc4plusmais
        {
            p->ctrlobjs.jomodescriptors = eo_constarray_Load((EOarray*)&p->service.servconfig.data.mc.mc4plusmais_based.arrayofjomodescriptors);
        }
        
        uint8_t numofjomos = eo_constarray_Size(p->ctrlobjs.jomodescriptors);
        
        eo_entities_SetNumOfJoints(eo_entities_GetHandle(), numofjomos);
        eo_entities_SetNumOfMotors(eo_entities_GetHandle(), numofjomos);
        

        if(0 == eo_entities_NumOfJoints(eo_entities_GetHandle()))
        {
            p->service.active = eobool_false;
            return(eores_NOK_generic);
        }
        else
        {            
            p->numofjomos = numofjomos;
            
//            // if also mais, then i activate it.
//            if(eo_motcon_mode_mc4plusmais == p->service.servconfig.type)
//            {
//                // start the mais which was activated in s_eo_motioncontrol_onendofverify_mais()
//                //eo_mais_Activate(eo_mais_GetHandle(), &p->ctrlobjs.servconfigmais);
//                eo_mais_Start(eo_mais_GetHandle());   
//                eo_mais_Transmission(eo_mais_GetHandle(), eobool_true);                
//            }
            

            uint8_t i=0;
           
           
            // marco.accame.TODO: clean comments in here
            // marco.accame: i keep the same initialisation order as davide.pollarolo did in his EOmcService.c (a, b, c etc)
            // a. init the emscontroller.
            // marco.accame.TODO: change the emsController. see comments below
            // the emscontroller is not a singleton which can be initted and deinitted. 
            // it should have a _Initialise(), a _GetHandle(), a _Config(cfg) and a _Deconfig().
//            if(NULL == p->ctrlobjs.thecontroller)
//            {
////                eOemscontroller_board_t controller_type;
////                if(eo_motcon_mode_mc4plus == p->service.servconfig.type)
////                {
////                    controller_type = (eOemscontroller_board_t)p->service.servconfig.data.mc.mc4plus_based.boardtype4mccontroller;
////                }
////                else //eomn_serv_MC_mc4plusmais
////                {
////                    controller_type = (eOemscontroller_board_t)p->service.servconfig.data.mc.mc4plusmais_based.boardtype4mccontroller;
////                }
//                //p->ctrlobjs.thecontroller = eo_emsController_Init(controller_type, emscontroller_actuation_LOCAL, numofjomos);
//                p->ctrlobjs.thecontroller = MController_new(numofjomos, 4);   
//                MController_config_board(&p->service.servconfig);                
//            } 

            MController_config_board(&p->service.servconfig);

            // b. clear the pwm values and the port mapping, currents and voltage
            memset(p->ctrlobjs.pwmvalue, 0, sizeof(p->ctrlobjs.pwmvalue));
            memset(p->ctrlobjs.pwmport, hal_motorNONE, sizeof(p->ctrlobjs.pwmport));
            memset(p->ctrlobjs.currents, 0, sizeof(p->ctrlobjs.currents));
            p->ctrlobjs.voltage = 0;
            
            // b1. init the port mapping
            for(i=0; i<numofjomos; i++)
            {
                const eOmn_serv_jomo_descriptor_t *jomodes = (eOmn_serv_jomo_descriptor_t*) eo_constarray_At(p->ctrlobjs.jomodescriptors, i);            
                p->ctrlobjs.pwmport[i] = (jomodes->actuator.pwm.port == eomn_serv_mc_port_none) ? (hal_motorNONE) : ((hal_motor_t)jomodes->actuator.pwm.port);                
            }
            
            // c. low level init for motors and adc           
            s_eo_motioncontrol_mc4plusbased_hal_init_motors_adc_feedbacks();

            // d. init the encoders            
            eo_encoderreader_Activate(eo_encoderreader_GetHandle(), (const eOmn_serv_arrayof_4jomodescriptors_t *) p->ctrlobjs.jomodescriptors);

            
            // e. activate interrupt line for quad_enc indexes check
            // marco.accame: maybe it is better to move it inside eo_appEncReader_Activate()
            s_eo_motioncontrol_mc4plusbased_hal_init_quad_enc_indexes_interrupt();
            
            eo_currents_watchdog_Initialise();
            
            p->service.active = eobool_true;
            p->service.state = eomn_serv_state_activated;
            eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);            
        }
        
    }
    
    return(eores_OK);   
}



extern eOresult_t eo_motioncontrol_Start(EOtheMotionController *p)
{ 
    // eo_errman_Trace(eo_errman_GetHandle(), "called: eo_motioncontrol_Start()", s_eobj_ownname);  
    
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }    
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    } 
        
    if(eobool_true == p->service.started)
    {   // it is already running
        return(eores_OK);
    }
       
    p->service.started = eobool_true;    
    p->service.state = eomn_serv_state_started;
    eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);    
    
    // mc4based: enable broadcast etc
    // focbased: just init a read of the encoder
    if(eo_motcon_mode_foc == p->service.servconfig.type)
    {   
        // just start a reading of encoders        
        eo_encoderreader_StartReading(eo_encoderreader_GetHandle());
    }
    else if(eo_motcon_mode_mc4 == p->service.servconfig.type)
    {
        eo_mais_Start(eo_mais_GetHandle());
        eo_mais_Transmission(eo_mais_GetHandle(), eobool_true);
        eo_mc4boards_BroadcastStart(eo_mc4boards_GetHandle());
    }
    else if((eo_motcon_mode_mc4plus == p->service.servconfig.type) || (eo_motcon_mode_mc4plusmais == p->service.servconfig.type))
    {
        if(eo_motcon_mode_mc4plusmais == p->service.servconfig.type)
        {
            eo_mais_Start(eo_mais_GetHandle());   
            eo_mais_Transmission(eo_mais_GetHandle(), eobool_true);                
        }
        eo_encoderreader_StartReading(eo_encoderreader_GetHandle());     
    }
    
    // eo_errman_Trace(eo_errman_GetHandle(), "eo_motioncontrol_Start()", s_eobj_ownname);
    
    return(eores_OK);    
}


extern eOresult_t eo_motioncontrol_SetRegulars(EOtheMotionController *p, eOmn_serv_arrayof_id32_t* arrayofid32, uint8_t* numberofthem)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    }  
    
    return(eo_service_hid_SetRegulars(p->id32ofregulars, arrayofid32, s_eo_motioncontrol_isID32relevant, numberofthem));
}

extern eOresult_t eo_motioncontrol_AddRegulars(EOtheMotionController *p, eOmn_serv_arrayof_id32_t* arrayofid32, uint8_t* numberofthem)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated
        return(eores_OK);
    }  
    
    return(eo_service_hid_AddRegulars(p->id32ofregulars, arrayofid32, s_eo_motioncontrol_isID32relevant, numberofthem));
}

extern eOresult_t eo_motioncontrol_Tick(EOtheMotionController *p)
{   
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }    
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated 
        return(eores_OK);
    } 
    
    if(eobool_false == p->service.started)
    {   // not running, thus we do nothing
        return(eores_OK);
    }
    
    if(eo_motcon_mode_mc4 == p->service.servconfig.type)
    {
        // motion done
        // not used anymore: eo_motiondone_Tick(eo_motiondone_GetHandle());
    
        // virtual strain
        eo_virtualstrain_Tick(eo_virtualstrain_GetHandle());        
        return(eores_OK);
    }
    
    
    // first of all check current limits
    if((eo_motcon_mode_mc4plus == p->service.servconfig.type) || (eo_motcon_mode_mc4plusmais == p->service.servconfig.type))
    {
        for(uint8_t i=0; i<p->numofjomos; i++)
        {
            p->ctrlobjs.currents[i] = hal_adc_get_current_motor_mA(p->ctrlobjs.pwmport[i]);
        }
        p->ctrlobjs.voltage = (int16_t)hal_adc_get_supplyVoltage_mV();
        eo_currents_watchdog_Tick(eo_currents_watchdog_GetHandle(), p->ctrlobjs.voltage, p->ctrlobjs.currents);
    }
    
    // 1) read positions from both joint and motor encoders and update motor controller engine.
    s_eo_motioncontrol_updatedPositionsFromEncoders(p);
    
    eo_encoderreader_StartReading(eo_encoderreader_GetHandle()); // Restart the reading of the encoders
    
    // 2) perform motor control
    MController_do();
    
    // 3) update status of motors and joints
    s_eo_motioncontrol_UpdateJointMotorStatus(p);
    

    return(eores_OK);    
}


extern eOresult_t eo_motioncontrol_Stop(EOtheMotionController *p)
{ 
    // eo_errman_Trace(eo_errman_GetHandle(), "called: eo_motioncontrol_Stop()", s_eobj_ownname);
    
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }    
    
    if(eobool_false == p->service.active)
    {   // nothing to do because object must be first activated 
        return(eores_OK);
    } 
    
    if(eobool_false == p->service.started)
    {   // it is already stopped
        return(eores_OK);
    }
    
    
    if(eo_motcon_mode_mc4 == p->service.servconfig.type)
    {
        // just stop broadcast of the mc4 boards
        eo_mc4boards_BroadcastStop(eo_mc4boards_GetHandle()); 
        // then stop the mais
        eo_mais_Stop(eo_mais_GetHandle());
    }
    else //foc, mc4plus, mc4plusmais
    {
         MController_go_idle();
    }
      
    p->service.started = eobool_false;
    p->service.state = eomn_serv_state_activated;
    eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);
    
    // remove all regulars related to motion control entities: joint, motor, controller ... no, dont do that
    //eo_motioncontrol_SetRegulars(p, NULL, NULL);
    
    // eo_errman_Trace(eo_errman_GetHandle(), "eo_motioncontrol_Stop()", s_eobj_ownname);
    
    return(eores_OK);    
}



// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------

// - in here i put the functions used to initialise the values in ram of the joints and motors ... better in here rather than elsewhere.

static const eOmc_joint_t s_joint_default_value =
{
    .config =             
    {
        .pidposition =
        {
            .kp =                    0,
            .ki =                    0,
            .kd =                    0,
            .limitonintegral =       0,
            .limitonoutput =         0,
            .scale =                 0,
            .offset =                0,
            .kff =                   0,
            .stiction_up_val =       0,
            .stiction_down_val =     0,
            .filler =                {0}
        },
        .pidvelocity =
        {
            .kp =                    0,
            .ki =                    0,
            .kd =                    0,
            .limitonintegral =       0,
            .limitonoutput =         0,
            .scale =                 0,
            .offset =                0,
            .kff =                   0,
            .stiction_up_val =       0,
            .stiction_down_val =     0,
            .filler =                {0}
        },
        .pidtorque =
        {
            .kp =                    0,
            .ki =                    0,
            .kd =                    0,
            .limitonintegral =       0,
            .limitonoutput =         0,
            .scale =                 0,
            .offset =                0,
            .kff =                   0,
            .stiction_up_val =       0,
            .stiction_down_val =     0,
            .filler =                {0}
        }, 
        .limitsofjoint =
        {
            .min =                   0,
            .max =                   0
        },
        .impedance =
        {
            .stiffness =             0,
            .damping =               0,
            .offset =                0          
        },               
        .maxvelocityofjoint =        0,
        .motor_params =
        {
            .bemf_value =            0,
            .ktau_value =            0,
            .bemf_scale =            0,
            .ktau_scale =            0,
            .filler02 =              {0}
        },
        .velocitysetpointtimeout =   0,
        .tcfiltertype =              0,
        .jntEncoderType =            0,
        .filler04 =                  {0}
    },
    .status =                       
    {
        .core =
        {        
            .measures =
            {
                .meas_position =          0,
                .meas_velocity =          0,
                .meas_acceleration =      0,
                .meas_torque =            0
            },
            .ofpid =                     {0},
            .modes = 
            {
                .controlmodestatus =        eomc_controlmode_notConfigured,
                .interactionmodestatus =    eOmc_interactionmode_stiff,
                .ismotiondone =             eobool_false,
                .filler =                   {0}
            }
        },
        .target = {0}
    },
    .inputs =                        {0},
    .cmmnds =                       
    {
        .calibration =               {0},
        .setpoint =                  {0},
        .stoptrajectory =            0,
        .controlmode =               eomc_controlmode_cmd_switch_everything_off,
        .interactionmode =           eOmc_interactionmode_stiff,
        .filler =                    {0}        
    }
}; 

static const eOmc_motor_t s_motor_default_value =
{
    .config =             
    {
        .pidcurrent =
        {
            .kp =                    0,
            .ki =                    0,
            .kd =                    0,
            .limitonintegral =       0,
            .limitonoutput =         0,
            .scale =                 0,
            .offset =                0,
            .kff =                   0,
            .stiction_up_val =       0,
            .stiction_down_val =     0,
            .filler =                {0}
        },
        .gearboxratio =              0,
        .rotorEncoderResolution =    0,
        .maxvelocityofmotor =        0,
        .currentLimits =             {0},
        .rotorIndexOffset =          0,
        .motorPoles =                0,
        .hasHallSensor =             eobool_false,
        .hasTempSensor =             eobool_false,
        .hasRotorEncoder =           eobool_false,
        .hasRotorEncoderIndex =      eobool_false,
        .rotorEncoderType =          0,
        .filler02 =                  {0},
        .limitsofrotor =
        {
            .max = 0,
            .min = 0
        }
    },
    .status =                       {0}
}; 


extern void eoprot_fun_INIT_mc_joint_config(const EOnv* nv)
{
    eOmc_joint_config_t *cfg = (eOmc_joint_config_t*)eo_nv_RAM(nv);
    memmove(cfg, &s_joint_default_value.config, sizeof(eOmc_joint_config_t));
}

extern void eoprot_fun_INIT_mc_joint_status(const EOnv* nv)
{
    eOmc_joint_status_t *sta = (eOmc_joint_status_t*)eo_nv_RAM(nv);
    memmove(sta, &s_joint_default_value.status, sizeof(eOmc_joint_status_t));
}

extern void eoprot_fun_INIT_mc_motor_config(const EOnv* nv)
{
    eOmc_motor_config_t *cfg = (eOmc_motor_config_t*)eo_nv_RAM(nv);
    memmove(cfg, &s_motor_default_value.config, sizeof(eOmc_motor_config_t));
}

extern void eoprot_fun_INIT_mc_motor_status(const EOnv* nv)
{
    eOmc_motor_status_t *sta = (eOmc_motor_status_t*)eo_nv_RAM(nv);
    memmove(sta, &s_motor_default_value.status, sizeof(eOmc_motor_status_t));
}


// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------

/*This function gets values from encoders at joints and at motors(if present)and updates the motionController engine.
returns:
 - eores_NOK_timeout if readings are not available
 - eores_NOK_generic if an error occurred during reading at least one encoder 
 - eores_OK in case of success*/
static eOresult_t s_eo_motioncontrol_updatedPositionsFromEncoders(EOtheMotionController *p)
{

    eOresult_t res = eores_OK;
    uint8_t error_mask = 0; 
    
    // wait for the encoders for some time
    for (uint8_t i=0; i<30; ++i)
    {
        if (eo_encoderreader_IsReadingAvailable(eo_encoderreader_GetHandle()))
        {
            break;
        }
        
        hal_sys_delay(5*hal_RELTIME_1microsec);
    }        
        
    
    if(eobool_false == eo_encoderreader_IsReadingAvailable(eo_encoderreader_GetHandle()))
    {
        return(eores_NOK_timeout);
    }
    
    // read the encoders        
    for(uint8_t i=0; i<p->numofjomos; i++)
    {
        eOencoderreader_valueInfo_t primary, secondary;
        const eOmn_serv_jomo_descriptor_t *jomodes = (eOmn_serv_jomo_descriptor_t*) eo_constarray_At(p->ctrlobjs.jomodescriptors, i);   

        res = eo_encoderreader_Read(eo_encoderreader_GetHandle(), i, &primary, &secondary);
        if (res != eores_OK)
        {
            error_mask |= 1<<(i<<1);
            MController_invalid_absEncoder_fbk(i, primary.errortype);
            res = eores_NOK_generic;
        }
        else
        {
            MController_update_absEncoder_fbk(i, primary.value);
        
            if(eomn_serv_mc_sensor_pos_atmotor == jomodes->extrasensor.pos) 
            {
                MController_update_motor_pos_fbk(i, (int32_t)secondary.value[0]);
            }
        }
    } 
    return(res);

}


static void s_eo_motioncontrol_send_periodic_error_report(void *par)
{
    EOtheMotionController* p = (EOtheMotionController*)par;
    p->diagnostics.errorDescriptor.par64 ++;
    eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
    
    if(eoerror_value_CFG_mc_foc_failed_candiscovery_of_foc == eoerror_code2value(p->diagnostics.errorDescriptor.code) ||
       eoerror_value_CFG_mc_mc4_failed_candiscovery_of_mc4 == eoerror_code2value(p->diagnostics.errorDescriptor.code)
      )
    {   // if i dont find the focs / mc4s, i keep on sending the discovery results up. it is a temporary diagnostics tricks until we use the verification of services at bootstrap
        eo_candiscovery2_SendLatestSearchResults(eo_candiscovery2_GetHandle());
    }
    
    if(EOK_int08dummy != p->diagnostics.errorCallbackCount)
    {
        p->diagnostics.errorCallbackCount--;
    }
    if(0 == p->diagnostics.errorCallbackCount)
    {
        eo_timer_Stop(p->diagnostics.reportTimer);
    }
}


static eOresult_t s_eo_motioncontrol_onstop_search4focs(void *par, EOtheCANdiscovery2* cd2, eObool_t searchisok)
{
    EOtheMotionController* p = &s_eo_themotcon;
    
    if(eobool_true == searchisok)
    {
        p->service.state = eomn_serv_state_verified;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);
    }
    else
    {   
        p->service.state = eomn_serv_state_failureofverify;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);
    }    
    
    if((eobool_true == searchisok) && (eobool_true == p->service.activateafterverify))
    {
        const eOmn_serv_configuration_t * mcserv = &p->service.servconfig; // or also: (const eOmn_serv_configuration_t *)par;
        eo_motioncontrol_Activate(p, mcserv);
    }
    
    p->diagnostics.errorDescriptor.sourcedevice     = eo_errman_sourcedevice_localboard;
    p->diagnostics.errorDescriptor.sourceaddress    = 0;
    p->diagnostics.errorDescriptor.par16            = 0;
    p->diagnostics.errorDescriptor.par64            = 0;    
    EOaction_strg astrg = {0};
    EOaction *act = (EOaction*)&astrg;
    eo_action_SetCallback(act, s_eo_motioncontrol_send_periodic_error_report, p, eov_callbackman_GetTask(eov_callbackman_GetHandle()));    
    
    if(eobool_true == searchisok)
    {        
        p->diagnostics.errorType = eo_errortype_debug;
        p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_foc_ok);
        eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
        
        if((0 != p->diagnostics.repetitionOKcase) && (0 != p->diagnostics.reportPeriod))
        {
            p->diagnostics.errorCallbackCount = p->diagnostics.repetitionOKcase;        
            eo_timer_Start(p->diagnostics.reportTimer, eok_abstimeNOW, p->diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    } 

    if(eobool_false == searchisok)
    {
        p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_foc_failed_candiscovery_of_foc);
        p->diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
        
        if(0 != p->diagnostics.reportPeriod)
        {
            p->diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(p->diagnostics.reportTimer, eok_abstimeNOW, p->diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    }     
           
    if(NULL != p->service.onverify)
    {
        p->service.onverify(p, searchisok); 
    }    
    
    return(eores_OK);   
}

static eOresult_t s_eo_motioncontrol_mc4plus_onendofverify_encoder(EOaService* s, eObool_t operationisok)
{
    EOtheMotionController* p = &s_eo_themotcon;
    
    if(eobool_true == operationisok)
    {
        p->service.state = eomn_serv_state_verified;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);
    }
    else
    {   
        p->service.state = eomn_serv_state_failureofverify;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);
    }    
    
    if((eobool_true == operationisok) && (eobool_true == p->service.activateafterverify))
    {
        const eOmn_serv_configuration_t * mcserv = &p->service.servconfig;
        eo_motioncontrol_Activate(p, mcserv);
    }
    
    p->diagnostics.errorDescriptor.sourcedevice     = eo_errman_sourcedevice_localboard;
    p->diagnostics.errorDescriptor.sourceaddress    = 0;
    p->diagnostics.errorDescriptor.par16            = 0;
    p->diagnostics.errorDescriptor.par64            = 0;    
    EOaction_strg astrg = {0};
    EOaction *act = (EOaction*)&astrg;
    eo_action_SetCallback(act, s_eo_motioncontrol_send_periodic_error_report, p, eov_callbackman_GetTask(eov_callbackman_GetHandle()));    
    
    if(eobool_true == operationisok)
    {           
        p->diagnostics.errorType = eo_errortype_debug;
        p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_mc4plus_ok);
        eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
        
        if((0 != p->diagnostics.repetitionOKcase) && (0 != p->diagnostics.reportPeriod))
        {
            p->diagnostics.errorCallbackCount = p->diagnostics.repetitionOKcase;        
            eo_timer_Start(p->diagnostics.reportTimer, eok_abstimeNOW, p->diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    } 

    if(eobool_false == operationisok)
    {
        p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_mc4plus_failed_encoders_verify);
        p->diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
        
        if(0 != p->diagnostics.reportPeriod)
        {
            p->diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(p->diagnostics.reportTimer, eok_abstimeNOW, p->diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    }     
           
    if(NULL != p->service.onverify)
    {
        p->service.onverify(p, operationisok); 
    }    
    
    return(eores_OK);       
    
}

static eOresult_t s_eo_motioncontrol_foc_onendofverify_encoder(EOaService* s, eObool_t operationisok)
{    
    EOtheMotionController* p = &s_eo_themotcon;
    
    if(eobool_true == operationisok)
    {
        eo_candiscovery2_Start(eo_candiscovery2_GetHandle(), &p->sharedcan.discoverytarget, &p->sharedcan.ondiscoverystop);        
    }    
    else
    {
        // the encoder reader fails. we dont even start the discovery of the foc boards. we just issue an error report and call onverify() w/ false argument
        
        p->service.state = eomn_serv_state_failureofverify;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);
        
        // prepare things
        p->diagnostics.errorDescriptor.sourcedevice     = eo_errman_sourcedevice_localboard;
        p->diagnostics.errorDescriptor.sourceaddress    = 0;
        p->diagnostics.errorDescriptor.par16            = 0;
        p->diagnostics.errorDescriptor.par64            = 0;    
        EOaction_strg astrg = {0};
        EOaction *act = (EOaction*)&astrg;
        eo_action_SetCallback(act, s_eo_motioncontrol_send_periodic_error_report, p, eov_callbackman_GetTask(eov_callbackman_GetHandle()));    
        

        // fill error description. and transmit it
        p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_foc_failed_encoders_verify);
        p->diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
        
        if(0 != p->diagnostics.reportPeriod)
        {
            p->diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(p->diagnostics.reportTimer, eok_abstimeNOW, p->diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
  
        // call onverify
        if(NULL != p->service.onverify)
        {
            p->service.onverify(p, eobool_false); 
        }            
                
    }  
    
    return(eores_OK);    
}



static eOresult_t s_eo_motioncontrol_onendofverify_mais(EOaService* s, eObool_t operationisok)
{
    EOtheMotionController* p = &s_eo_themotcon;
    
    const eOmn_serv_configuration_t * servcfg = &p->service.servconfig; //  not anymore: p->service.tmpcfg; 
    // we can be either mc4 or mc4plusmais
    
    if(eobool_true == operationisok)
    {
        // mais is already activated.... i .... DONT START IT !!!
//        eo_mais_Start(eo_mais_GetHandle());
//        eo_mais_Transmission(eo_mais_GetHandle(), eobool_true);
        
        // then: start verification of mc4s or encoders
        if(eo_motcon_mode_mc4 == servcfg->type)
        {
            eo_candiscovery2_Start(eo_candiscovery2_GetHandle(), &p->sharedcan.discoverytarget, &p->sharedcan.ondiscoverystop); 
        }
        else // mc4plusmais: verify encoders
        {
            eo_encoderreader_Verify(eo_encoderreader_GetHandle(), &servcfg->data.mc.mc4plusmais_based.arrayofjomodescriptors, s_eo_motioncontrol_mc4plusmais_onendofverify_encoder_BIS, eobool_true);             
        }
    } 
    else
    {
        // the mais fails. we dont even start the discovery of the mc4 boards. we just issue an error report and call onverify() w/ false argument
        
        p->service.state = eomn_serv_state_failureofverify;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);
        
        eOerror_value_CFG_t errorvalue = (eo_motcon_mode_mc4 == servcfg->type) ? (eoerror_value_CFG_mc_mc4_failed_mais_verify) : (eoerror_value_CFG_mc_mc4plusmais_failed_candiscovery_of_mais);
        
        // prepare things        
        p->diagnostics.errorDescriptor.sourcedevice     = eo_errman_sourcedevice_localboard;
        p->diagnostics.errorDescriptor.sourceaddress    = 0;
        p->diagnostics.errorDescriptor.par16            = 0;
        p->diagnostics.errorDescriptor.par64            = 0;    
        EOaction_strg astrg = {0};
        EOaction *act = (EOaction*)&astrg;
        eo_action_SetCallback(act, s_eo_motioncontrol_send_periodic_error_report, p, eov_callbackman_GetTask(eov_callbackman_GetHandle()));    
               
        // fill error description. and transmit it
        p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, errorvalue);
        p->diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
        
        if(0 != p->diagnostics.reportPeriod)
        {
            p->diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(p->diagnostics.reportTimer, eok_abstimeNOW, p->diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
 
        // call onverify
        if(NULL != p->service.onverify)
        {
            p->service.onverify(p, eobool_false); 
        }            
        
    }
 
    return(eores_OK);    
}


static eOresult_t s_eo_motioncontrol_onstop_search4mc4s(void *par, EOtheCANdiscovery2* cd2, eObool_t searchisok)
{
    EOtheMotionController* p = &s_eo_themotcon;
    
    if(eobool_true == searchisok)
    {
        p->service.state = eomn_serv_state_verified;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);
    }
    else
    {   
        p->service.state = eomn_serv_state_failureofverify;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);
    }    
    
    if((eobool_true == searchisok) && (eobool_true == p->service.activateafterverify))
    {
        const eOmn_serv_configuration_t * mcserv = &p->service.servconfig; // or also:(const eOmn_serv_configuration_t *)par;
        eo_motioncontrol_Activate(p, mcserv);
    }

    p->diagnostics.errorDescriptor.sourcedevice     = eo_errman_sourcedevice_localboard;
    p->diagnostics.errorDescriptor.sourceaddress    = 0;
    p->diagnostics.errorDescriptor.par16            = 0;
    p->diagnostics.errorDescriptor.par64            = 0;    
    EOaction_strg astrg = {0};
    EOaction *act = (EOaction*)&astrg;
    eo_action_SetCallback(act, s_eo_motioncontrol_send_periodic_error_report, p, eov_callbackman_GetTask(eov_callbackman_GetHandle()));    
    
    if(eobool_true == searchisok)
    {        
        p->diagnostics.errorType = eo_errortype_debug;
        p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_mc4_ok);
        eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
        
        if((0 != p->diagnostics.repetitionOKcase) && (0 != p->diagnostics.reportPeriod))
        {
            p->diagnostics.errorCallbackCount = p->diagnostics.repetitionOKcase;        
            eo_timer_Start(p->diagnostics.reportTimer, eok_abstimeNOW, p->diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    } 
    
    if(eobool_false == searchisok)
    {
        p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_mc4_failed_candiscovery_of_mc4);
        p->diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
        
        if(0 != p->diagnostics.reportPeriod)
        {
            p->diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(p->diagnostics.reportTimer, eok_abstimeNOW, p->diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    }    
    
    
    if(NULL != p->service.onverify)
    {
        p->service.onverify(p, searchisok); 
    }    
        
    return(eores_OK);   
}



static eOresult_t s_eo_motioncontrol_mc4plusmais_onendofverify_encoder_BIS(EOaService* s, eObool_t operationisok)
{  
    EOtheMotionController* p = &s_eo_themotcon;
    
    if(eobool_true == operationisok)
    {
        p->service.state = eomn_serv_state_verified;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);
    }
    else
    {   
        p->service.state = eomn_serv_state_failureofverify;
        eo_service_hid_SynchServiceState(eo_services_GetHandle(), eomn_serv_category_mc, p->service.state);
    }    
    
    if((eobool_true == operationisok) && (eobool_true == p->service.activateafterverify))
    {
        const eOmn_serv_configuration_t * mcserv = &p->service.servconfig;
        eo_motioncontrol_Activate(p, mcserv);
    }
    
    
    p->diagnostics.errorDescriptor.sourcedevice     = eo_errman_sourcedevice_localboard;
    p->diagnostics.errorDescriptor.sourceaddress    = 0;
    p->diagnostics.errorDescriptor.par16            = 0;
    p->diagnostics.errorDescriptor.par64            = 0;    
    EOaction_strg astrg = {0};
    EOaction *act = (EOaction*)&astrg;
    eo_action_SetCallback(act, s_eo_motioncontrol_send_periodic_error_report, p, eov_callbackman_GetTask(eov_callbackman_GetHandle()));    
    
    if(eobool_true == operationisok)
    {        
        p->diagnostics.errorType = eo_errortype_debug;
        p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_mc4plusmais_ok);
        eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
        
        if((0 != p->diagnostics.repetitionOKcase) && (0 != p->diagnostics.reportPeriod))
        {
            p->diagnostics.errorCallbackCount = p->diagnostics.repetitionOKcase;        
            eo_timer_Start(p->diagnostics.reportTimer, eok_abstimeNOW, p->diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    } 

    if(eobool_false == operationisok)
    {
        p->diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_mc_mc4plusmais_failed_encoders_verify);
        p->diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
        
        if(0 != p->diagnostics.reportPeriod)
        {
            p->diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(p->diagnostics.reportTimer, eok_abstimeNOW, p->diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    }     
           
    if(NULL != p->service.onverify)
    {
        p->service.onverify(p, eoerror_value_CFG_mc_mc4plusmais_failed_encoders_verify); 
    }    
    
    return(eores_OK);        
}


static void s_eo_motioncontrol_UpdateJointMotorStatus(EOtheMotionController *p)
{
    const uint8_t transmit_decoupled_pwms = 0;
    
    eOmc_joint_status_t *jstatus = NULL;
    eOmc_motor_status_t *mstatus = NULL;
    
    uint8_t jId = 0;
        
    for(jId = 0; jId<p->numofjomos; jId++)
    {
        if(NULL != (jstatus = eo_entities_GetJointStatus(eo_entities_GetHandle(), jId)))
        {
            MController_get_joint_state(jId, jstatus);
            
            MController_get_pid_state(jId, &jstatus->core.ofpid, transmit_decoupled_pwms);
        }
    }   
    
    for(jId = 0; jId<p->numofjomos; jId++)
    {
        if(NULL != (mstatus = eo_entities_GetMotorStatus(eo_entities_GetHandle(), jId)))
        {
            MController_get_motor_state(jId, mstatus);
        }
    }
}


static void s_eo_motioncontrol_proxy_config(EOtheMotionController* p, eObool_t on)
{       
    if(eo_motcon_mode_mc4 == p->service.servconfig.type)
    {
        if(eobool_true == on)
        {
            eoprot_config_proxied_variables(eoprot_board_localboard, eoprot_endpoint_motioncontrol, s_eo_motioncontrol_mc4based_variableisproxied);
        }
        else
        {
            eoprot_config_proxied_variables(eoprot_board_localboard, eoprot_endpoint_motioncontrol, NULL);
        }       
    }
    else
    {   // no proxied variables
        eoprot_config_proxied_variables(eoprot_board_localboard, eoprot_endpoint_motioncontrol, NULL); 
    }
          
}


static eObool_t s_eo_motioncontrol_mc4based_variableisproxied(eOnvID32_t id)
{    
    eOprotEndpoint_t ep = eoprot_ID2endpoint(id);
    
    eOprotEntity_t ent = eoprot_ID2entity(id);
    
    eOprotTag_t tag = eoprot_ID2tag(id);
    
    if(eoprot_entity_mc_joint == ent)
    {
        switch(tag)
        {
            case eoprot_tag_mc_joint_config_pidposition:
            case eoprot_tag_mc_joint_config_pidtorque:
            case eoprot_tag_mc_joint_config_limitsofjoint:
            case eoprot_tag_mc_joint_config_impedance:
            case eoprot_tag_mc_joint_status_core_modes_ismotiondone:
            {
                return(eobool_true);
            }
            
            default:
            {
                return(eobool_false);
            }
         }
    }
    else if(eoprot_entity_mc_motor == ent)
    {
        switch(tag)
        {
            case eoprot_tag_mc_motor_config_pwmlimit:
            {
                return(eobool_true);
            }
            
            default:
            {
                return(eobool_false);
            }
         }
    }
    else
    {
        return(eobool_false);
    }

}


static void s_eo_motioncontrol_mc4plusbased_hal_init_motors_adc_feedbacks(void)
{   // low level init for motors and adc in mc4plus
    static eObool_t initted = eobool_false;
    
    if(eobool_true == initted)
    {
        return;
    }
    
    initted = eobool_true;

    // currently the motors are initialized all together and without config
    hal_motor_init(hal_motorALL, NULL);
        
    // first initialize all ADC as indipendents
    hal_adc_common_structure_init();
        
    // init the ADC to read the (4) current values of the motor and (4) analog inputs (hall_sensors, if any) using ADC1/ADC3
    // always initialized at the moment, but the proper interface for reading the values is in EOappEncodersReader
    hal_adc_dma_init_ADC1_ADC3_hall_sensor_current();    
    
    //init ADC2 to read supplyVoltage
    hal_adc_dma_init_ADC2_tvaux_tvin_temperature();
}


static void s_eo_motioncontrol_mc4plusbased_hal_init_quad_enc_indexes_interrupt(void)
{   // activate interrupt line for quad_enc indexes check. the call of hal_quadencoder_init() is inside eo_appEncReader_Activate() ... maybe move it inside there too
    hal_quadencoder_init_indexes_flags();      
}


static eObool_t s_eo_motioncontrol_isID32relevant(uint32_t id32)
{
    static const uint32_t mask0 = (((uint32_t)eoprot_endpoint_motioncontrol) << 24) | (((uint32_t)eoprot_entity_mc_joint) << 16);
    static const uint32_t mask1 = (((uint32_t)eoprot_endpoint_motioncontrol) << 24) | (((uint32_t)eoprot_entity_mc_motor) << 16);
    static const uint32_t mask2 = (((uint32_t)eoprot_endpoint_motioncontrol) << 24) | (((uint32_t)eoprot_entity_mc_controller) << 16);
    
    if((id32 & mask0) == mask0)
    {
        return(eobool_true);
    }
    if((id32 & mask1) == mask1)
    {
        return(eobool_true);
    } 
    if((id32 & mask2) == mask2)
    {
        return(eobool_true);
    }    
    
    return(eobool_false); 
}


// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------



