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
#ifndef _EOROP_H_
#define _EOROP_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @file       EOrop.h
    @brief      This header file implements public interface to a remote oepration rop.
    @author     marco.accame@iit.it
    @date       09/06/2011
**/

/** @defgroup eo_rop Object EOrop
    The EOrop object contains a remote operation as received by the network.

     
    @{        
 **/



// - external dependencies --------------------------------------------------------------------------------------------

#include "EoCommon.h"
#include "EOnv.h"


// - public #define  --------------------------------------------------------------------------------------------------
// empty-section


  

// - declaration of public user-defined types ------------------------------------------------------------------------- 


/** @typedef    typedef struct EOrop_hid EOrop
    @brief      EOaction is an opaque struct. It is used to implement data abstraction for the datagram 
                object so that the user cannot see its private fields and he/she is forced to manipulate the
                object only with the proper public functions. 
 **/  
typedef struct EOrop_hid EOrop;


typedef uint8_t eOropcode_t;

/** @typedef    typedef enum eOropcodevalue_t
    @brief      eOropcodevalue_t contains possible ROP codes.
 **/
typedef enum
{
    eo_ropcode_none         = 0,
    eo_ropcode_usr          = 1,
    eo_ropcode_ask          = 2,
    eo_ropcode_say          = 3,
    eo_ropcode_set          = 4,
    eo_ropcode_sig          = 5,
    eo_ropcode_rst          = 6,
    eo_ropcode_upd          = 7
} eOropcodevalues_t;

enum 
{
    eo_ropcodevalues_numberofthem = 8  // in case you add more ropcodevalues or remove them, ... change this value
};


/** @typedef    typedef struct eOropconfig_t
    @brief      eOropconfig_t contains the configuration for optional fields of a ROP.
 **/
typedef struct
{
    eObool_t    confrqst;
    eObool_t    timerqst;
    eObool_t    plussign;
    eObool_t    plustime;
} eOropconfig_t;


/** @typedef    typedef enum eOropconfinfo_t
    @brief      eOropconfinfo_t contains possible confirmation values that a ROP may have.
 **/
typedef enum
{
    eo_ropconf_none = 0,
    eo_ropconf_nak  = 2,
    eo_ropconf_ack  = 3
} eOropconfinfo_t;


typedef struct      // 06 bytes
{
    eOnvEP_t        ep;
    eOnvID_t        id;
    eObool_t        plustime;
    uint8_t         filler;
} eOropSIGcfg_t;    EO_VERIFYsizeof(eOropSIGcfg_t, 6);


// typedef struct      // 16 bytes
// {
//     eOropconfig_t   ropconfig;      // 4B
//     eOropconfinfo_t ropconfinfo;    // 1B
//     eOropcode_t     ropcode;        // 1B
//     eOnvEP_t        ep;             // 2B
//     eOnvID_t        id;             // 2B
//     uint32_t        signature;      // 4B
// } eOropdescriptor_t;   


typedef struct
{
    uint8_t         confrqst    :1;
    uint8_t         timerqst    :1;
    uint8_t         plussign    :1;
    uint8_t         plustime    :1;
    uint8_t         confirm     :2;
    uint8_t         notused     :2;
} eOropconfiguration_t;

typedef struct      // 16 bytes
{
    eOropconfiguration_t    configuration;      // 1B
    eOropcode_t             ropcode;            // 1B
    eOnvEP_t                ep;                 // 2B
    eOnvID_t                id;                 // 2B
    uint16_t                size;               // 2B
    uint8_t*                data;               // 4B
    uint32_t                signature;          // 4B
} eOropdescriptor_t;        //EO_VERIFYsizeof(eOropdescriptor_t, 16); 

    
// - declaration of extern public variables, ... but better using use _get/_set instead -------------------------------

extern const eOropconfig_t eok_ropconfig_basic; // = {.confrqst=eobool_false, .timerqst=eobool_false, .plussign= eobool_false, .plustime=eobool_false} 

extern const eOropconfiguration_t eok_ropconfiguration_basic;

// - declaration of extern public functions ---------------------------------------------------------------------------
 
 
/** @fn         extern EOrop* eo_rop_New(void)
    @brief      Creates a new rop object. 
    @return     The pointer to the required object.
 **/
extern EOrop* eo_rop_New(uint16_t capacity);


/** @fn         extern eOresult_t eo_rop_Reset(EOrop *p)
    @brief      Resets a rop object. 
    @return     Always success unless argument is NULL.
 **/
extern eOresult_t eo_rop_Reset(EOrop *p);

/** @fn         extern eOresult_t eo_rop_Process(EOrop *p, EOrop *replyrop)
    @brief      Operates on a EOrop and executes whatever is specified by the ROP. If needed it also prepares a 
                ROP to send back to the sender. If a reply ROP is needed then the ropcode field shall be a valid one
                and different from eo_ropcode_none
    @param      p           The input ROP
    @param      replyrop    Pointer to a ROP to be sent back to the sender of @e p. If a reply is needed this shall contain
                            the ROP to use, otherwise a resetted EOrop.
    @return     Always success unless any argument is NULL.
 **/
extern eOresult_t eo_rop_Process(EOrop *p, EOrop *replyrop);


/** @fn         extern eOropcode_t eo_rop_GetROPCode(EOrop *p)
    @brief      Returns the ROP code.
    @param      p           The EOrop object 
    @return     The ropcode if the object is valid, or eo_ropcode_none if NULL
 **/
extern eOropcode_t eo_rop_GetROPcode(EOrop *p);

extern uint8_t* eo_rop_GetROPdata(EOrop *p);

extern uint16_t eo_rop_ComputeSize(eOropconfig_t ropcfg, eOropcode_t ropc, uint16_t sizeofdata);



/** @}            
    end of group eo_rop  
 **/

#ifdef __cplusplus
}       // closing brace for extern "C"
#endif 

#endif  // include-guard


// - end-of-file (leave a blank line after)----------------------------------------------------------------------------

