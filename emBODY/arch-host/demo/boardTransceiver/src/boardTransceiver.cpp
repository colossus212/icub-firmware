
/* @file       boardTransceiver.c
    @brief      This file implements internal implementation of a nv object.
    @author     marco.accame@iit.it
    @date       09/03/2010
**/


// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------

using namespace std;

#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "boardTransceiver.hpp"


extern "C" {
#include "EoCommon.h"
#include "EOnv_hid.h"

}

// static EOtransceiver *ems00txrx = NULL;
static EOtransceiver *boardTransceiver = NULL;
EOpacket*      pkt;

extern EOtransceiver* boardTransceiver_new(uint32_t _localipaddr, uint32_t _remoteipaddr, uint16_t _ipport, uint16_t _pktsize = EOK_BOARDTRANSCEIVER_capacityofpacket)
{
    eOboardtransceiver_cfg_t boardTransceiverCfg =
    {
    		EO_INIT(.vectorof_endpoint_cfg)         eo_cfg_EPs_vectorof_loc_board,
    		EO_INIT(.hashfunction_ep2index)       	eo_cfg_nvsEP_loc_board_fptr_hashfunction_ep2index,
    		EO_INIT(.remotehostipv4addr)            _remoteipaddr,
    		EO_INIT(.remotehostipv4port)            _ipport
    };
    boardTransceiver = eo_boardtransceiver_Initialise(&boardTransceiverCfg);

    pkt = eo_packet_New(_pktsize);
    return (boardTransceiver);
}


// somebody passes the received packet - this is used just as an interface
void onMsgReception(uint8_t *data, uint16_t size)
{
	//this->pkt = data;
//	SetReceived(data, size);
}

// and Processes it
void SetReceived(uint8_t *data, uint16_t size, uint32_t remoteipaddr, uint16_t port)
{
    uint16_t numofrops;
    uint64_t txtime;

    eo_packet_Payload_Set(pkt, data, size);
    eo_packet_Addressing_Set(pkt, remoteipaddr, port);
    eo_transceiver_Receive(boardTransceiver, pkt, &numofrops, &txtime);
}


// somebody retrieves what must be transmitted
void boardTransceiver_GetTransmit(uint8_t **data, uint16_t *size)
{
    uint16_t numofrops;

    eo_transceiver_Transmit(boardTransceiver, &pkt, &numofrops);

    eo_packet_Payload_Get(pkt, data, size);

}


#if 0

void BoardTransceiver::init(uint32_t _localipaddr, uint32_t _remoteipaddr, uint16_t _ipport, uint16_t _pktsize)
{
    // the configuration of the transceiver: it is specific of a given remote board
    eOboardtransceiver_cfg_t boardTransceiverCfg =
    {
    		EO_INIT(.vectorof_endpoint_cfg)           eo_cfg_EPs_vectorof_loc_board,
    		EO_INIT(.hashfunction_ep2index)           eo_cfg_nvsEP_loc_board_fptr_hashfunction_ep2index,
    		EO_INIT(.remotehostipv4addr)              _remoteipaddr,
    		EO_INIT(.remotehostipv4port)              _ipport
    };
    
    localipaddr  = _localipaddr;
    remoteipaddr = _remoteipaddr;
    ipport       = _ipport;
    
    // initialise the transceiver: it creates a EOtransceiver and its nvsCfg by loading all the endpoints and returns the pointer
    boardTransceiver     = eo_boardtransceiver_Initialise(&boardTransceiverCfg);
    
    // retrieve teh transceiver
    //pc104txrx    = eo_boardtransceiver_GetHandle(boardTransceiver);
    
    // retrieve the nvscfg
    boardnvscfg  = eo_boardTransceiver_NVsCfg(boardTransceiver);

}

void BoardTransceiver::s_eom_hostprotoc_extra_protocoltransceiver_load_occasional_rop(eOropcode_t opc, uint16_t ep, uint16_t nvid)
{
    eo_transceiver_ropinfo_t ropinfo;

    ropinfo.ropcfg      = eok_ropconfig_basic;
    ropinfo.ropcode     = opc;
    ropinfo.nvep        = ep;

    ropinfo.nvid = nvid;
    eo_transceiver_rop_occasional_Load(pc104txrx, &ropinfo);
}

/*
void BoardTransceiver::s_eom_hostprotoc_extra_protocoltransceiver_configure_regular_rops_on_board(void)
{
    EOarray *upto10 = (EOarray*) & eo_cfg_nvsEP_mngmnt_usr_rem_board_mem_local->upto10rop2signal;
    eOropSIGcfg_t sigcfg;
    char str[128];
    static uint8_t reset = 0;

    eo_array_Reset(upto10);

    if(0 == reset)
    {

        sigcfg.ep = EOK_cfg_nvsEP_base_endpoint;
        sigcfg.id = EOK_cfg_nvsEP_base_NVID_ipnetwork;
        sigcfg.plustime = 1;
        eo_array_PushBack(upto10, &sigcfg);


        sigcfg.ep = EOK_cfg_nvsEP_base_endpoint;
        sigcfg.id = EOK_cfg_nvsEP_base_NVID__bootprocess;
        sigcfg.plustime = 1;
        eo_array_PushBack(upto10, &sigcfg);

        sigcfg.ep = EOK_cfg_nvsEP_base_endpoint;
        sigcfg.id = EOK_cfg_nvsEP_base_NVID__applicationinfo;
        sigcfg.plustime = 0;
        eo_array_PushBack(upto10, &sigcfg);

        sigcfg.ep = EOK_cfg_nvsEP_base_endpoint;
        sigcfg.id = EOK_cfg_nvsEP_base_NVID__boardinfo;
        sigcfg.plustime = 0;
        eo_array_PushBack(upto10, &sigcfg);

        sigcfg.ep = EOK_cfg_nvsEP_base_endpoint;
        sigcfg.id = EOK_cfg_nvsEP_base_NVID_ipnetwork__ipnetmask;
        sigcfg.plustime = 0;
        eo_array_PushBack(upto10, &sigcfg);

        sigcfg.ep = EOK_cfg_nvsEP_base_endpoint;
        sigcfg.id = EOK_cfg_nvsEP_base_NVID_ipnetwork__ipaddress;
        sigcfg.plustime = 0;
        eo_array_PushBack(upto10, &sigcfg);

        sigcfg.ep = EOK_cfg_nvsEP_base_endpoint;
        sigcfg.id = EOK_cfg_nvsEP_base_NVID__remoteipaddress;
        sigcfg.plustime = 0;
        eo_array_PushBack(upto10, &sigcfg);

        sigcfg.ep = EOK_cfg_nvsEP_base_endpoint;
        sigcfg.id = EOK_cfg_nvsEP_base_NVID__remoteipport;
        sigcfg.plustime = 0;
        eo_array_PushBack(upto10, &sigcfg);

    }

    s_eom_hostprotoc_extra_protocoltransceiver_load_occasional_rop(eo_ropcode_set, EOK_cfg_nvsEP_mngmnt_endpoint, EOK_cfg_nvsEP_mngmnt_NVID__upto10rop2signal);

    if(1 == reset)
    {
        printf( "added a set<__upto10rop2signal, empty-list>");
    }
    else
    {
        printf("added a set<__upto10rop2signal, list>");
    }
    reset = (0==reset) ? (1) : (0);
}


// vecchio //
void BoardTransceiver::boardTransceiver_ConfigureRegularsOnRemote(void)
{
    // 1. at first write the nv that we want to set: the EOK_cfg_nvsEP_mngmnt_NVID__upto15epid2signal
    //    there are two alternative ways to do that:
    //    A. QUICK MODE - to write directly onto the local struct of the endpoint. WE USE THIS SECOND WAY because we know
    //       the name of the specific variable to use in the rop
    //    B. FORMAL MODE (and more generic): to use teh nvscfg to retrieve the handle of a EOnv which is relative to the 
    //       pair (endpoint, id) which we wnat to manipulate, and then to use the methods of EOnv to set the value of that variable.
    //       we shall use this mode in function hostTransceiver_AddSetROP()
    
    EOarray *upto15 = (EOarray*) & eo_cfg_nvsEP_mngmnt_usr_rem_board_mem_local->upto10rop2signal;
#warning "cambiato?? per compilare scommentato definizione eOnvEPID_t in EOnv.h"
//    eOnvEPID_t epid;

    // clear the variable.
//    eo_array_Reset(upto15);

    // push back in teh array the first nv that the remote board shall signal
//    epid.ep = EOK_cfg_nvsEP_base_endpoint;
//    epid.id = EOK_cfg_nvsEP_base_NVID_ipnetwork__ipaddress;
//    eo_array_PushBack(upto15, &epid);

    // push back in teh array the second nv that the remote board shall signal
//    epid.ep = EOK_cfg_nvsEP_base_endpoint;
//    epid.id = EOK_cfg_nvsEP_base_NVID__bootprocess;
//    eo_array_PushBack(upto15, &epid);
    
    // 2. then put the rop set<upto15epid2signal, data> inside the transceiver
//    s_hostTransceiver_AddSetROP_with_data_already_set(EOK_cfg_nvsEP_mngmnt_endpoint, EOK_cfg_nvsEP_mngmnt_NVID__upto10rop2signal);
}
*/

// somebody adds a set-rop  plus data.
void BoardTransceiver::boardTransceiver_AddSetROP(uint16_t ep, uint16_t id, uint8_t* data, uint16_t size)
{
    uint16_t ss;
    EOnv nv;

    // 1. search for teh EOnv with pair (ep, id)
            
 //   if(eores_OK != eo_nvscfg_GetNV(thehosttransceiver.boardnvscfg, thehosttransceiver.remoteipaddr, ep, id, &nv))
    {
        // there is no such variable with (remoteipaddr-ep-id) 
 //       return;
    }
    
    // 1bis. verify that the datasize is correct.
    ss = eo_nv_Size(&nv, data);
    if(ss < size)
    {
        // non faccio nulla intanto quando scrivo uso la capacita' della nv.
        // sarebbe bene pero' emettere un warning        
    }
    
    
    // 2. set the data. dont force the set so that we dont write if the nv is readonly.
    
    if(eores_OK != eo_nv_Set(&nv, data, eobool_false, eo_nv_upd_dontdo))
    {   
        // teh nv is not writeable
        return;
    }
    
    
    // 3. add the rop 
    s_boardTransceiver_AddSetROP_with_data_already_set(ep, id);

}


// somebody passes the received packet
void BoardTransceiver::onMsgReception(uint8_t *data, uint16_t size)
{
	//this->pkt = data;
	SetReceived(data, size);
}

void BoardTransceiver::SetReceived(uint8_t *data, uint16_t size)
{
    uint16_t numofrops;
    uint64_t txtime;
    pkt = eo_packet_New(size);
    eo_packet_Payload_Set(pkt, data, size);
    eo_packet_Addressing_Set(pkt, remoteipaddr, ipport);
         
    eo_transceiver_Receive(pc104txrx, pkt, &numofrops, &txtime);
    free(pkt);
}


// somebody retrieves what must be transmitted
void BoardTransceiver::getTransmit(uint8_t **data, uint16_t *size)
{
    uint16_t numofrops;
    
    eo_transceiver_Transmit(pc104txrx, &pkt, &numofrops);
    
    eo_packet_Payload_Get(pkt, data, size);
    
}



// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------

void BoardTransceiver::s_boardTransceiver_AddSetROP_with_data_already_set(uint16_t ep, uint16_t id)
{
    eo_transceiver_ropinfo_t ropinfo;

    ropinfo.ropcfg      = eok_ropconfig_basic;
    ropinfo.ropcode     = eo_ropcode_set;
    ropinfo.nvep        = ep;
    ropinfo.nvid        = id;
    
    eo_transceiver_rop_occasional_Load(pc104txrx, &ropinfo);
}

void BoardTransceiver::s_boardTransceiver_AddGetROP(uint16_t ep, uint16_t id)
{
    eo_transceiver_ropinfo_t ropinfo;

    ropinfo.ropcfg      = eok_ropconfig_basic;
    ropinfo.ropcode     = eo_ropcode_ask;
    ropinfo.nvep        = ep;
    ropinfo.nvid        = id;

    eo_transceiver_rop_occasional_Load(pc104txrx, &ropinfo);
}

#endif

// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------




