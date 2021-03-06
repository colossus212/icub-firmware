
/* @file       hal_dspic_crc.c
	@brief      This file implements internal implementation of the hal crc module.
	@author     marco.accame@iit.it
    @date       09/16/2011
**/

// - modules to be built: contains the HAL_USE_* macros ---------------------------------------------------------------
#include "hal_brdcfg_modules.h"

#ifdef HAL_USE_CRC

// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------

#include "stdlib.h"
#include "string.h"
#include "hal_dspic_base_hid.h" 
#include "hal_brdcfg.h"

#include "./utils/crc16.h"
#include "./utils/crc32.h"


#include "stdio.h"



 
// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------

#include "hal_crc.h"



// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------

#include "hal_dspic_crc_hid.h"


// --------------------------------------------------------------------------------------------------------------------
// - #define with internal scope
// --------------------------------------------------------------------------------------------------------------------

#define HAL_crc_t2index(t)              ((uint8_t)((t)))



// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of extern variables, but better using _get(), _set() 
// --------------------------------------------------------------------------------------------------------------------

const hal_crc_cfg_t hal_crc_cfg_default = 
{   
    .order      = hal_crc_order_32,
    .polynomial = 0x04C11DB7,
    .crctblram  = NULL
}; // the crc-32 polynomial used for eth

const uint32_t hal_crc_poly_crc32 = 0x04C11DB7; 
const uint32_t hal_crc_poly_crc16_ccitt = 0x00001021; 

// --------------------------------------------------------------------------------------------------------------------
// - typedef with internal scope
// --------------------------------------------------------------------------------------------------------------------

typedef struct
{
    hal_crc_cfg_t       cfg;
    uint32_t            initialvalue;
} hal_crc_info_t;


// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------

static hal_boolval_t s_hal_crc_supported_is(hal_crc_t crc);
static void s_hal_crc_initted_set(hal_crc_t crc);
static hal_boolval_t s_hal_crc_initted_is(hal_crc_t crc);

//static hal_result_t s_hal_crc_hw_init(hal_crc_info_t *info);
//static uint32_t s_hal_crc32_hw_compute(hal_crc_info_t *info, const void *data, uint32_t size);
static uint32_t s_hal_crc32_sw_compute(hal_crc_info_t *info, const void *data, uint32_t size);
static uint32_t s_hal_crc16_sw_compute(hal_crc_info_t *info, const void *data, uint32_t size);


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------

static hal_boolval_t s_hal_crc_initted[hal_crcs_num] = { hal_false };

static hal_crc_info_t s_hal_crc_info[hal_crcs_num] = {{{hal_crc_order_32, 0, NULL}, 0xffffffff}, {{hal_crc_order_32, 0, NULL}, 0xffffffff}};


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------


extern hal_result_t hal_crc_init(hal_crc_t crc, const hal_crc_cfg_t *cfg)
{
    hal_result_t res = hal_res_NOK_generic; // dont remove ...
    hal_crc_info_t *info = NULL;

    if(hal_false == s_hal_crc_supported_is(crc))
    {
        return(hal_res_NOK_generic);
    }
     
    if(NULL == cfg)
    {
        cfg  = &hal_crc_cfg_default;
    }

    info = &s_hal_crc_info[HAL_crc_t2index(crc)];

    memcpy(&info->cfg, cfg, sizeof(hal_crc_cfg_t));
    info->initialvalue = 0xffffffff;

    switch(info->cfg.order)
    {
        case hal_crc_order_16:
        {   // on dspic use sw emulation only for now ... later on we should use the hw engine
        
            #define CRC16_USE_CRC16TABLE_IN_ROM
        
            if(hal_crc_poly_crc16_ccitt == info->cfg.polynomial)
            {   // for crc16-ccitt use rom-ed table or compute it ...
                #ifdef CRC16_USE_CRC16TABLE_IN_ROM
                info->cfg.crctblram = (void*)crc16_table_0x1021;
                #else
                crc16_table_get(info->cfg.polynomial, info->cfg.crctblram);
                #endif
                res = hal_res_OK;
            } 
            else if(NULL != info->cfg.crctblram)
            {   // else compute it.
                crc16_table_get(info->cfg.polynomial, info->cfg.crctblram);
                res = hal_res_OK;
            }
        } break;

        case hal_crc_order_32:
        {   // on arm use sw emulation. we never use a precomputed table 

            //if(hal_crc_poly_crc32 == info->cfg.polynomial)
            //{   // use hw emulation
            //    res = s_hal_crc_hw_init(info);
            //    info->cfg.crctblram = (void*)crc32_table_0x1234567;
            //}
            //else 
            if(NULL != info->cfg.crctblram)
            {
                crc32_table_get(info->cfg.polynomial, info->cfg.crctblram);
                res = hal_res_OK;
            }
        } break;

        default:
        {
            res = hal_res_NOK_generic;
        } break;
    }

    if(hal_res_OK != res)
    {
        return(res);
    }


    s_hal_crc_initted_set(crc);

    return(hal_res_OK);
}

extern hal_result_t hal_crc_compute(hal_crc_t crc, hal_crc_compute_mode_t mode, const void *data, uint32_t size, uint32_t *out)
{
    //hal_crc_cfg_t* cfg = NULL;
    hal_crc_info_t *info = NULL;

 
    if((NULL == data) || (NULL == out) || (0 == size))
    {
        return(hal_res_NOK_generic);
    }
    
    if(hal_false == s_hal_crc_initted_is(crc))
    {
        *out = 0;
        return(hal_res_NOK_generic);
    }

    // do something 

    info = &s_hal_crc_info[HAL_crc_t2index(crc)];
    //cfg = &s_hal_crc_info[HAL_crc_t2index(crc)].cfg;

    if(hal_crc_mode_clear == mode)
    {
        info->initialvalue = 0xffffffff;
    }

    // first the case of hw support
    //if(hal_crc_poly_crc32 == info->cfg.polynomial)
    //{
    //    *out = info->initialvalue = s_hal_crc32_hw_compute(info, data, size);
    //}
    //else 
    if(hal_crc_order_16 == info->cfg.order)
    {
        *out = info->initialvalue = s_hal_crc16_sw_compute(info, data, size);
    }
    else if(hal_crc_order_32 == info->cfg.order)
    {
        *out = info->initialvalue = s_hal_crc32_sw_compute(info, data, size);
    }
    else
    {
        *out = 0;
        return(hal_res_NOK_generic);
    }
           
    return(hal_res_OK);
}




// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------

// ---- isr of the module: begin ----
// empty-section
// ---- isr of the module: end ------


extern uint32_t hal_crc_hid_getsize(const hal_cfg_t *cfg)
{
    // no memory needed
    return(0);
}

extern hal_result_t hal_crc_hid_setmem(const hal_cfg_t *cfg, uint32_t *memory)
{
    // no memory needed
//    if(NULL == memory)
//    {
//        hal_base_hid_on_fatalerror(hal_fatalerror_missingmemory, "hal_xxx_hid_setmem(): memory missing");
//        return(hal_res_NOK_generic);
//    }


    memset(s_hal_crc_info, 0, sizeof(s_hal_crc_info));
    memset(s_hal_crc_initted, hal_false, sizeof(s_hal_crc_initted));
    return(hal_res_OK);  
}

// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------

static hal_boolval_t s_hal_crc_supported_is(hal_crc_t crc)
{
    return(hal_base_hid_byte_bitcheck(hal_brdcfg_crc__supported_mask, HAL_crc_t2index(crc)) );
}

static void s_hal_crc_initted_set(hal_crc_t crc)
{
    s_hal_crc_initted[HAL_crc_t2index(crc)] = hal_true;
}

static hal_boolval_t s_hal_crc_initted_is(hal_crc_t crc)
{
    return(s_hal_crc_initted[HAL_crc_t2index(crc)]);
}


//static hal_result_t s_hal_crc_hw_init(hal_crc_info_t *info)
//{
//    info->initialvalue = 0xffffffff;
//
//    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
//    return(hal_res_OK);
//}

//static uint32_t s_hal_crc32_hw_compute(hal_crc_info_t *info, const void *data, uint32_t size)
//{
//    uint8_t tailsize = 0;
//
//    if(0xffffffff == info->initialvalue)
//    {
//        CRC_ResetDR();
//    }
//
//    info->initialvalue = CRC_CalcBlockCRC_networkorder((uint32_t*)data, size/4);
//   
//    tailsize = size%4; 
//
//    if(0 != (tailsize))
//    {
//        uint8_t tail[4];
//        memset(tail, 0, 4);
//        memcpy(tail, ((uint8_t*)data) + size-tailsize, tailsize);
//        info->initialvalue = CRC_CalcBlockCRC((uint32_t*)tail, 1); 
//    }
//
//    return(info->initialvalue);
//}

static uint32_t s_hal_crc32_sw_compute(hal_crc_info_t *info, const void *data, uint32_t size)
{
    uint8_t tailsize = size%4;

    uint32_t size4 = size-tailsize;

    info->initialvalue = crc32_compute(info->cfg.crctblram, info->initialvalue, (uint8_t*)data, size4);
   
    if(0 != (tailsize))
    {
        uint8_t tail[4];
        memset(tail, 0, 4);
        memcpy(tail, ((uint8_t*)data) + size-tailsize, tailsize);
        info->initialvalue = crc32_compute(info->cfg.crctblram, info->initialvalue, tail, 4); 
    }

    return(info->initialvalue);
}


static uint32_t s_hal_crc16_sw_compute(hal_crc_info_t *info, const void *data, uint32_t size)
{
    uint8_t tailsize = size%4;

    uint32_t size4 = size-tailsize;

    info->initialvalue = crc16_compute(info->cfg.crctblram, info->initialvalue, (uint8_t*)data, size4);
   
    if(0 != (tailsize))
    {
        uint8_t tail[4];
        memset(tail, 0, 4);
        memcpy(tail, ((uint8_t*)data) + size-tailsize, tailsize);
        info->initialvalue = crc16_compute(info->cfg.crctblram, info->initialvalue, tail, 4); 
    }

    return(info->initialvalue);
}

#endif//HAL_USE_CRC


// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------



