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

#include "EOMtheEMSappl.h"

#include "EOtheCANprotocol_functions.h"

#include "EOMtheEMSconfigurator.h"

#include "EOMtheEMStransceiver.h"

#include "EOemsController.h"

#include "EOVtheCallbackManager.h"

// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------

#include "EOtheEncoderReader.h"



// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------

#include "EOtheEncoderReader_hid.h"


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


// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------

static eOresult_t s_eo_encoderreader_onstop_verifyreading(void *par, eObool_t readingisok);

static void s_eo_encodereader_send_periodic_error_report(void *p);


static void s_eo_encoderreader_init_ereader(const eOmn_serv_arrayof_4jomodescriptors_t * jomodes, eOcallback_t callback, void* arg);

static void s_eo_encoderreader_read_encoders(void* p);

//static eo_appEncReader_stream_number_t s_eo_encoderreader_encoder_get_spi_stream(eo_appEncReader_encoder_port_t pos);


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------

static EOtheEncoderReader s_eo_theencoderreader = 
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
    .arrayofjomodes             = { 0 },   
    .waitreadtimer              = NULL,
    .numofjomos                 = 0,
    .numofencoders              = 0,
    .errorflags                 = {0},
    .failuremask                = 0,
    .reader                     = NULL
};

static const char s_eobj_ownname[] = "EOtheEncoderReader";


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------


extern EOtheEncoderReader* eo_encoderreader_Initialise(void)
{
    if(eobool_true == s_eo_theencoderreader.service.initted)
    {
        return(&s_eo_theencoderreader);
    }
    
    s_eo_theencoderreader.service.active = eobool_false;



    s_eo_theencoderreader.waitreadtimer = eo_timer_New();
    
    s_eo_theencoderreader.reader = eo_appEncReader_Initialise(); 
    
    s_eo_theencoderreader.diagnostics.reportTimer = eo_timer_New();
    s_eo_theencoderreader.diagnostics.errorType = eo_errortype_error;
    s_eo_theencoderreader.diagnostics.errorDescriptor.sourceaddress = eo_errman_sourcedevice_localboard;
    s_eo_theencoderreader.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_encoders_not_verified_yet);      
        
    s_eo_theencoderreader.service.initted = eobool_true;
    
    return(&s_eo_theencoderreader);   
}


extern EOtheEncoderReader* eo_encoderreader_GetHandle(void)
{
    if(eobool_true == s_eo_theencoderreader.service.initted)
    {
        return(&s_eo_theencoderreader);
    }
    
    return(NULL);
}


extern eOresult_t eo_encoderreader_Verify(EOtheEncoderReader *p, const eOmn_serv_arrayof_4jomodescriptors_t * jomodes, eOservice_onendofoperation_fun_t onverify, eObool_t activateafterverify)
{
    if((NULL == p) || (NULL == jomodes))
    {
        return(eores_NOK_nullpointer);
    }  
 
// DONT Deactivate ... we may want just to check again ....    
//    if(eobool_true == s_eo_theencoderreader.service.active)
//    {
//        eo_encoderreader_Deactivate(p);        
//    }   
    
    // make sure the timer is not running
    eo_timer_Stop(s_eo_theencoderreader.diagnostics.reportTimer);   
    
    
    s_eo_theencoderreader.service.onverify = onverify;
    s_eo_theencoderreader.service.activateafterverify = activateafterverify;
                    
    s_eo_encoderreader_init_ereader(jomodes, NULL, NULL);
    
    eo_appEncReader_StartRead(s_eo_theencoderreader.reader);
    
    EOaction_strg astrg = {0};
    EOaction *act = (EOaction*)&astrg;
    
    // now i start a timer of 20 ms. and at its expiry i exec s_eo_encoderreader_read_encoders().  
    // for SPI encoders 2 ms would be more than enough, but we give it 20, so that in case of mais we are able to receive data.
    eo_action_SetCallback(act, s_eo_encoderreader_read_encoders, (void*)jomodes, eov_callbackman_GetTask(eov_callbackman_GetHandle()));     
    eo_timer_Start(s_eo_theencoderreader.waitreadtimer, eok_abstimeNOW, 20*eok_reltime1ms, eo_tmrmode_ONESHOT, act);   
     
    return(eores_OK);   
}


extern eOresult_t eo_encoderreader_Deactivate(EOtheEncoderReader *p)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }

    if(eobool_false == s_eo_theencoderreader.service.active)
    {
        return(eores_OK);        
    } 
    
    #warning TODO: eo_encoderreader_Deactivate() must be completed. and tested. see comments below
    
    // in order to do this function we should change EOappEncReader so that it can be de-initted. 
    // even better to merge what is inside EOappEncReader into object EOtheEncoderReader 
    
    // to do: 
    // deinit EOappEncReader, undo what in s_eo_encoderreader_init_ereader()

//    
//    s_eo_theencoderreader.service.active = eobool_false;


    // make sure the timer is not running
    eo_timer_Stop(s_eo_theencoderreader.diagnostics.reportTimer);  
    
    return(eores_OK);
}


extern eOresult_t eo_encoderreader_Activate(EOtheEncoderReader *p, const eOmn_serv_arrayof_4jomodescriptors_t * jomodes)
{
    if((NULL == p) || (NULL == jomodes))
    {
        return(eores_NOK_nullpointer);
    }    
    
    if(eobool_true == s_eo_theencoderreader.service.active)
    {
        eo_encoderreader_Deactivate(p);        
    }   
 
    #warning TODO: eo_encoderreader_Activate() should be changed. see comments below.
    // since EOappEncReader can only be created, but not initted or de-inittedd, we must change it so that it can do that.
    

//    memcpy(&s_eo_theencoderreader.arrayofjomodes, jomodes, sizeof(eOmn_serv_arrayof_4jomodescriptors_t));
//                    
//    s_eo_encoderreader_init_ereader(NULL, NULL);
//    
//
    eo_appEncReader_StartRead(s_eo_theencoderreader.reader);
                     
    s_eo_theencoderreader.service.active = eobool_true;        
    
    return(eores_OK);   
}


extern eOresult_t eo_encoderreader_SendReport(EOtheEncoderReader *p)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }

    eo_errman_Error(eo_errman_GetHandle(), p->diagnostics.errorType, NULL, s_eobj_ownname, &p->diagnostics.errorDescriptor);
        
    return(eores_OK);      
}



extern eOresult_t eo_encoderreader_StartReading(EOtheEncoderReader *p)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == s_eo_theencoderreader.service.active)
    {   // nothing to do because we dont have it
        return(eores_OK);
    }   

    eo_appEncReader_StartRead(s_eo_theencoderreader.reader);
    
    
    return(eores_OK);
}


extern eObool_t eo_encoderreader_IsReadingAvailable(EOtheEncoderReader *p)
{
    if(NULL == p)
    {
        return(eobool_false);
    }
    
    if(eobool_false == s_eo_theencoderreader.service.active)
    {   // nothing to do because we dont have it
        return(eobool_false);
    }   

    eObool_t res = eo_appEncReader_isReady(s_eo_theencoderreader.reader);

    return(res);
}



extern eOresult_t eo_encoderreader_Read(EOtheEncoderReader *p, uint8_t position, uint32_t *primary, uint32_t *secondary, hal_spiencoder_errors_flags *errors)
{
    if(NULL == p)
    {
        return(eores_NOK_nullpointer);
    }
    
    if(eobool_false == s_eo_theencoderreader.service.active)
    {   // nothing to do because we dont have it
        return(eores_OK);
    }   

    if(position >= eOappEncReader_jomos_maxnumberof)
    {
        return(eores_NOK_generic);
    }

    eOresult_t res = eo_appEncReader_GetValue(s_eo_theencoderreader.reader, position, primary, secondary, errors); 
    
    
    return(res);
}




// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------

// - in here i put the functions used to initialise the values in ram of the joints and motors ... better in here rather than elsewhere.



// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------


static eOresult_t s_eo_encoderreader_onstop_verifyreading(void *par, eObool_t readingisok)
{
    const eOmn_serv_arrayof_4jomodescriptors_t * jomodes = (const eOmn_serv_arrayof_4jomodescriptors_t *)par;
        
    if((eobool_true == readingisok) && (eobool_true == s_eo_theencoderreader.service.activateafterverify))
    {
        eo_encoderreader_Activate(&s_eo_theencoderreader, jomodes);        
    }

    s_eo_theencoderreader.diagnostics.errorDescriptor.sourcedevice     = eo_errman_sourcedevice_localboard;
    s_eo_theencoderreader.diagnostics.errorDescriptor.sourceaddress    = 0;
    s_eo_theencoderreader.diagnostics.errorDescriptor.par16            = s_eo_theencoderreader.numofjomos | (s_eo_theencoderreader.numofencoders << 8);
    // in the 4 lsb i put the errorflags.
    // in the msb i put the failuremask
    memcpy(&s_eo_theencoderreader.diagnostics.errorDescriptor.par64, &s_eo_theencoderreader.errorflags[0], sizeof(s_eo_theencoderreader.errorflags));   
    uint64_t mmm =  s_eo_theencoderreader.failuremask;
    s_eo_theencoderreader.diagnostics.errorDescriptor.par64 |= (mmm << 56);
    EOaction_strg astrg = {0};
    EOaction *act = (EOaction*)&astrg;
    eo_action_SetCallback(act, s_eo_encodereader_send_periodic_error_report, NULL, eov_callbackman_GetTask(eov_callbackman_GetHandle()));
        
    if(eobool_true == readingisok)
    {        
        s_eo_theencoderreader.diagnostics.errorType = eo_errortype_debug;
        s_eo_theencoderreader.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_encoders_ok);
        eo_errman_Error(eo_errman_GetHandle(), s_eo_theencoderreader.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_theencoderreader.diagnostics.errorDescriptor);
        
        if((0 != s_eo_theencoderreader.diagnostics.repetitionOKcase) && (0 != s_eo_theencoderreader.diagnostics.reportPeriod))
        {
            s_eo_theencoderreader.diagnostics.errorCallbackCount = 20;        
            eo_timer_Start(s_eo_theencoderreader.diagnostics.reportTimer, eok_abstimeNOW, s_eo_theencoderreader.diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    }    
       
    if(eobool_false == readingisok)
    {
        s_eo_theencoderreader.diagnostics.errorDescriptor.code = eoerror_code_get(eoerror_category_Config, eoerror_value_CFG_encoders_failed_verify);
        s_eo_theencoderreader.diagnostics.errorType = eo_errortype_error;                
        eo_errman_Error(eo_errman_GetHandle(), s_eo_theencoderreader.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_theencoderreader.diagnostics.errorDescriptor);
        
        if(0 != s_eo_theencoderreader.diagnostics.reportPeriod)
        {
            s_eo_theencoderreader.diagnostics.errorCallbackCount = EOK_int08dummy;
            eo_timer_Start(s_eo_theencoderreader.diagnostics.reportTimer, eok_abstimeNOW, s_eo_theencoderreader.diagnostics.reportPeriod, eo_tmrmode_FOREVER, act);
        }
    }    
    
    if(NULL != s_eo_theencoderreader.service.onverify)
    {
        s_eo_theencoderreader.service.onverify(&s_eo_theencoderreader, readingisok); 
    }    
    
    return(eores_OK);   
}


static void s_eo_encodereader_send_periodic_error_report(void *p)
{
    eo_errman_Error(eo_errman_GetHandle(), s_eo_theencoderreader.diagnostics.errorType, NULL, s_eobj_ownname, &s_eo_theencoderreader.diagnostics.errorDescriptor);
    
    if(EOK_int08dummy != s_eo_theencoderreader.diagnostics.errorCallbackCount)
    {
        s_eo_theencoderreader.diagnostics.errorCallbackCount--;
    }
    if(0 == s_eo_theencoderreader.diagnostics.errorCallbackCount)
    {
        eo_timer_Stop(s_eo_theencoderreader.diagnostics.reportTimer);
    }
}






static void s_eo_encoderreader_init_ereader(const eOmn_serv_arrayof_4jomodescriptors_t * jomodes, eOcallback_t callback, void* arg)
{   
    memcpy(&s_eo_theencoderreader.arrayofjomodes, jomodes, sizeof(eOmn_serv_arrayof_4jomodescriptors_t));
    
    EOconstarray* carray = eo_constarray_Load((EOarray*)&s_eo_theencoderreader.arrayofjomodes);

    uint8_t numofjomos = eo_constarray_Size(carray);
    s_eo_theencoderreader.numofjomos = numofjomos;

    
    eo_appEncReader_Activate(s_eo_theencoderreader.reader, &s_eo_theencoderreader.arrayofjomodes);    
}


static void s_eo_encoderreader_read_encoders(void* p)
{
    eObool_t readingisok = eobool_true;
    
    uint8_t i = 0;
    
    memset(&s_eo_theencoderreader.errorflags, 0, sizeof(s_eo_theencoderreader.errorflags));
    s_eo_theencoderreader.failuremask = 0;
    
    for(i=0; i< s_eo_theencoderreader.numofjomos; i++)
    {
        uint32_t primary = 0;
        uint32_t secondary = 0;

        eo_appEncReader_GetValue(s_eo_theencoderreader.reader, i, &primary, &secondary, &s_eo_theencoderreader.errorflags[i]);
        if((0 != s_eo_theencoderreader.errorflags[i].chip_error) || (0 != s_eo_theencoderreader.errorflags[i].data_error) || 
           (0 != s_eo_theencoderreader.errorflags[i].data_notready) || (0 != s_eo_theencoderreader.errorflags[i].tx_error) )
        {
            eo_common_byte_bitset(&s_eo_theencoderreader.failuremask, i);
            readingisok = eobool_false;
        }   
        // so far we assume that we have so many encoders as joints ...
        s_eo_theencoderreader.numofencoders ++;        
    }
    
    
    s_eo_encoderreader_onstop_verifyreading((void*)p, readingisok);   
}


//// if we merge encoder reader with EOtheEncoderReader ... change it
//static eo_appEncReader_stream_number_t s_eo_encoderreader_encoder_get_spi_stream(eo_appEncReader_encoder_port_t pos)
//{
//    uint8_t pp = pos;
//    if(eo_appEncReader_encoder_positionNONE == pp)
//    {
//        return(eo_appEncReader_streamNONE);
//    }
//    // eo_appEncReader_stream_position_numberof is 3, so that we can map eo_appEncReader_encoder_position[1, 2, 3, 4, 5, 6] into: eo_appEncReader_stream[0, 0, 0, 1, 1, 1]
//    
//    pp = pp/eo_appEncReader_stream_position_numberof;
//    return((eo_appEncReader_stream_number_t)pp);  
//}

// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------



