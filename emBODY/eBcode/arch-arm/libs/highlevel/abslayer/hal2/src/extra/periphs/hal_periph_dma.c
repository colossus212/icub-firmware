/*
 * Copyright (C) 2012 iCub Facility - Istituto Italiano di Tecnologia
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

/* @file       hal_periph_dma.c
	@brief      This file implements the hal dma module.
	@author     marco.accame@iit.it
    @date       12/21/2012
**/

// - modules to be built: contains the HAL_USE_* macros ---------------------------------------------------------------
#include "hal_brdcfg_modules.h"

#ifdef HAL_USE_DMA

// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------

#include "stdlib.h"

#include "hal_middleware_interface.h"

#include "string.h"

#include "hal_brdcfg.h"
#include "hal_heap.h" 
#include "hal_base_hid.h" 
#include "hal_periph_gpio_hid.h" 
#include "hal_utility_fifo.h"
#include "hl_bits.h" 


// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------
#include "hal_dma.h"


// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------

#include "hal_periph_dma_hid.h" 


// --------------------------------------------------------------------------------------------------------------------
// - #define with internal scope
// --------------------------------------------------------------------------------------------------------------------

#define HAL_dma_id2index(p)             ((uint8_t)((p)))
#define HAL_dma_index2id(i)             ((hal_dma_t)((i)))
#define HAL_dma_id2peripheral(p)        (s_hal_dma_memory_mapping_of_ports[HAL_dma_id2index(p)])


// #if     defined(HAL_USE_CPU_FAM_STM32F1)
// #elif    defined(HAL_USE_CPU_FAM_STM32F4)
// #else   //defined(HAL_USE_CPU_FAM_*)
//     #error ERR --> choose a HAL_USE_CPU_FAM_*
// #endif   





// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of extern variables, but better using _get(), _set() 
// --------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------
// - typedef with internal scope
// --------------------------------------------------------------------------------------------------------------------

typedef struct
{
    hal_dma_cfg_t           cfg;
    DMA_InitTypeDef         stm32dmainit;
#if     defined(HAL_USE_CPU_FAM_STM32F1)    
    DMA_Channel_TypeDef*    stm32dmaperiph;
#elif   defined(HAL_USE_CPU_FAM_STM32F4)   
    DMA_Stream_TypeDef*     stm32dmaperiph;
#else //defined(HAL_USE_CPU_FAM_*)
    #error ERR --> choose a HAL_USE_CPU_FAM_*
#endif    
//    void*                   source;
//    void*                   destin;
    hal_bool_t              enabled;
    hal_bool_t              stopit;
} hal_dma_internal_item_t;


typedef struct
{
    uint16_t                    initted;
    hal_dma_internal_item_t*    items[hal_dmas_number];   
} hal_dma_theinternals_t;


// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------

static hal_boolval_t s_hal_dma_supported_is(hal_dma_t id);
static void s_hal_dma_initted_set(hal_dma_t id);
static hal_boolval_t s_hal_dma_initted_is(hal_dma_t id);


// hw related initialisation whcih may change with different versions of stm32fx mpus
static void s_hal_dma_hw_nvic_init(hal_dma_t id);
// static void s_hal_dma_isr_enable(hal_dma_t id);
// static void s_hal_dma_isr_disable(hal_dma_t id);


static void s_hal_dma_isr_portx(hal_dma_t id);

static void s_hal_dma_isr_clear_flag(hal_dma_t id);


// enable or disab


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static const variables
// --------------------------------------------------------------------------------------------------------------------

#if     defined(HAL_USE_CPU_FAM_STM32F1)

static DMA_Channel_TypeDef* const s_hal_dma_memory_mapping_of_ports[hal_dmas_number] = 
{
    DMA1_Channel1, DMA1_Channel2, DMA1_Channel3, DMA1_Channel4, DMA1_Channel5, DMA1_Channel6, DMA1_Channel7,
    DMA2_Channel1, DMA2_Channel2, DMA2_Channel3, DMA2_Channel4, DMA2_Channel5,
    NULL, NULL, NULL, NULL
}; 

static const uint32_t s_hal_dma_periphclocks[hal_dmas_number] = 
{
    RCC_AHBPeriph_DMA1, RCC_AHBPeriph_DMA1, RCC_AHBPeriph_DMA1, RCC_AHBPeriph_DMA1, RCC_AHBPeriph_DMA1, RCC_AHBPeriph_DMA1, RCC_AHBPeriph_DMA1,
    RCC_AHBPeriph_DMA2, RCC_AHBPeriph_DMA2, RCC_AHBPeriph_DMA2, RCC_AHBPeriph_DMA2, RCC_AHBPeriph_DMA2,
    0, 0, 0, 0    
};

static const uint32_t s_hal_dma_irqflag_gl[hal_dmas_number] =
{
    DMA1_FLAG_GL1, DMA1_FLAG_GL2, DMA1_FLAG_GL3, DMA1_FLAG_GL4, DMA1_FLAG_GL5, DMA1_FLAG_GL6, DMA1_FLAG_GL7,
    DMA2_FLAG_GL1, DMA2_FLAG_GL2, DMA2_FLAG_GL3, DMA2_FLAG_GL4, DMA2_FLAG_GL5,   
    0, 0, 0, 0
};

static const IRQn_Type s_hal_dma_irqnumber[hal_dmas_number] =
{
    DMA1_Channel1_IRQn, DMA1_Channel2_IRQn, DMA1_Channel3_IRQn, DMA1_Channel4_IRQn, DMA1_Channel5_IRQn, DMA1_Channel6_IRQn, DMA1_Channel7_IRQn,
    DMA2_Channel1_IRQn, DMA2_Channel2_IRQn, DMA2_Channel3_IRQn, DMA2_Channel4_IRQn, DMA2_Channel5_IRQn,   
    UsageFault_IRQn, UsageFault_IRQn, UsageFault_IRQn, UsageFault_IRQn // use UsageFault_IRQn when there is no dma port available
};

#elif   defined(HAL_USE_CPU_FAM_STM32F4)
           
static DMA_Stream_TypeDef* const s_hal_dma_memory_mapping_of_ports[hal_dmas_number] = 
{
    DMA1_Stream0, DMA1_Stream1, DMA1_Stream2, DMA1_Stream3, DMA1_Stream4, DMA1_Stream5, DMA1_Stream6, DMA1_Stream7, 
    DMA2_Stream0, DMA2_Stream1, DMA2_Stream2, DMA2_Stream3, DMA2_Stream4, DMA2_Stream5, DMA2_Stream6, DMA2_Stream7
}; 

static const uint32_t s_hal_dma_periphclocks[hal_dmas_number] = 
{
    RCC_AHB1Periph_DMA1, RCC_AHB1Periph_DMA1, RCC_AHB1Periph_DMA1, RCC_AHB1Periph_DMA1, RCC_AHB1Periph_DMA1, RCC_AHB1Periph_DMA1, RCC_AHB1Periph_DMA1, RCC_AHB1Periph_DMA1,
    RCC_AHB1Periph_DMA2, RCC_AHB1Periph_DMA2, RCC_AHB1Periph_DMA2, RCC_AHB1Periph_DMA2, RCC_AHB1Periph_DMA2, RCC_AHB1Periph_DMA2, RCC_AHB1Periph_DMA2, RCC_AHB1Periph_DMA2
};

static const uint32_t s_hal_dma_irqflag_gl[hal_dmas_number] =
{
    DMA_FLAG_TCIF0|DMA_FLAG_HTIF0|DMA_FLAG_TEIF0|DMA_FLAG_DMEIF0|DMA_FLAG_FEIF0, 
    DMA_FLAG_TCIF1|DMA_FLAG_HTIF1|DMA_FLAG_TEIF1|DMA_FLAG_DMEIF1|DMA_FLAG_FEIF1,
    DMA_FLAG_TCIF2|DMA_FLAG_HTIF2|DMA_FLAG_TEIF2|DMA_FLAG_DMEIF2|DMA_FLAG_FEIF2,
    DMA_FLAG_TCIF3|DMA_FLAG_HTIF3|DMA_FLAG_TEIF3|DMA_FLAG_DMEIF3|DMA_FLAG_FEIF3,
    DMA_FLAG_TCIF4|DMA_FLAG_HTIF4|DMA_FLAG_TEIF4|DMA_FLAG_DMEIF4|DMA_FLAG_FEIF4,
    DMA_FLAG_TCIF5|DMA_FLAG_HTIF5|DMA_FLAG_TEIF5|DMA_FLAG_DMEIF5|DMA_FLAG_FEIF5,
    DMA_FLAG_TCIF6|DMA_FLAG_HTIF6|DMA_FLAG_TEIF6|DMA_FLAG_DMEIF6|DMA_FLAG_FEIF6,
    
    DMA_FLAG_TCIF0|DMA_FLAG_HTIF0|DMA_FLAG_TEIF0|DMA_FLAG_DMEIF0|DMA_FLAG_FEIF0, 
    DMA_FLAG_TCIF1|DMA_FLAG_HTIF1|DMA_FLAG_TEIF1|DMA_FLAG_DMEIF1|DMA_FLAG_FEIF1,
    DMA_FLAG_TCIF2|DMA_FLAG_HTIF2|DMA_FLAG_TEIF2|DMA_FLAG_DMEIF2|DMA_FLAG_FEIF2,
    DMA_FLAG_TCIF3|DMA_FLAG_HTIF3|DMA_FLAG_TEIF3|DMA_FLAG_DMEIF3|DMA_FLAG_FEIF3,
    DMA_FLAG_TCIF4|DMA_FLAG_HTIF4|DMA_FLAG_TEIF4|DMA_FLAG_DMEIF4|DMA_FLAG_FEIF4,
    DMA_FLAG_TCIF5|DMA_FLAG_HTIF5|DMA_FLAG_TEIF5|DMA_FLAG_DMEIF5|DMA_FLAG_FEIF5,
    DMA_FLAG_TCIF6|DMA_FLAG_HTIF6|DMA_FLAG_TEIF6|DMA_FLAG_DMEIF6|DMA_FLAG_FEIF6    
};

static const IRQn_Type s_hal_dma_irqnumber[hal_dmas_number] =
{
    DMA1_Stream0_IRQn, DMA1_Stream1_IRQn, DMA1_Stream2_IRQn, DMA1_Stream3_IRQn, DMA1_Stream4_IRQn, DMA1_Stream5_IRQn, DMA1_Stream6_IRQn, DMA1_Stream7_IRQn,
    DMA2_Stream0_IRQn, DMA2_Stream1_IRQn, DMA2_Stream2_IRQn, DMA2_Stream3_IRQn, DMA2_Stream4_IRQn, DMA2_Stream5_IRQn, DMA2_Stream6_IRQn, DMA2_Stream7_IRQn   
    // use UsageFault_IRQn when there is no dma port available
};


#else //defined(HAL_USE_CPU_FAM_*)
    #error ERR --> choose a HAL_USE_CPU_FAM_*
#endif 


// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------

static hal_dma_theinternals_t s_hal_dma_theinternals =
{
    .initted            = 0,
    .items              = { NULL }   
};


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------

extern hal_result_t hal_dma_init(hal_dma_t id, const hal_dma_cfg_t *cfg)
{
    hal_dma_internal_item_t* intitem = s_hal_dma_theinternals.items[HAL_dma_id2index(id)];

    if(hal_false == s_hal_dma_supported_is(id))
    {
        return(hal_res_NOK_generic);
    }

    if(NULL == cfg)
    {
        return(hal_res_NOK_generic);
    }
    
    if(NULL == intitem)
    {
        // the internal entry
        intitem = s_hal_dma_theinternals.items[HAL_dma_id2index(id)] = hal_heap_new(sizeof(hal_dma_internal_item_t));
    }

   
    // set config
    memcpy(&intitem->cfg, cfg, sizeof(hal_dma_cfg_t));
    
    // set the stm32dma
    intitem->stm32dmaperiph = HAL_dma_id2peripheral(id);
    
    
    // enable the peripheral clock
    
#if     defined(HAL_USE_CPU_FAM_STM32F1)
    RCC_AHBPeriphClockCmd(s_hal_dma_periphclocks[HAL_dma_id2index(id)], ENABLE);
#elif   defined(HAL_USE_CPU_FAM_STM32F4)
    RCC_AHB1PeriphResetCmd(s_hal_dma_periphclocks[HAL_dma_id2index(id)], ENABLE);
#else //defined(HAL_USE_CPU_FAM_*)
    #error ERR --> choose a HAL_USE_CPU_FAM_*
#endif    
 
#if     defined(HAL_USE_CPU_FAM_STM32F1)
#elif   defined(HAL_USE_CPU_FAM_STM32F4)
    RCC_AHB1PeriphClockCmd(s_hal_dma_periphclocks[HAL_dma_id2index(id)], ENABLE);
    #warning --> verifica nel progetto stm32f4x spi con dma come viene inizializzato il dma. vedi anche i registri poiche' sono diversi dal stm32f1  
#else //defined(HAL_USE_CPU_FAM_*)
    #error ERR --> choose a HAL_USE_CPU_FAM_*
#endif 
    
    
    
    // set stm32dmainit
#if     defined(HAL_USE_CPU_FAM_STM32F1)    
    intitem->stm32dmainit.DMA_PeripheralBaseAddr    = (uint32_t)cfg->source;
    intitem->stm32dmainit.DMA_MemoryBaseAddr        = (uint32_t)cfg->destin;
    intitem->stm32dmainit.DMA_DIR                   = (hal_dma_transfer_per2mem == cfg->transfer) ? (DMA_DIR_PeripheralSRC) : (DMA_DIR_PeripheralDST);
    intitem->stm32dmainit.DMA_BufferSize            = (uint32_t)cfg->datasize;
    intitem->stm32dmainit.DMA_PeripheralInc         = (hal_dma_transfer_per2mem == cfg->transfer) ? (DMA_PeripheralInc_Disable) : (DMA_PeripheralInc_Disable);
    intitem->stm32dmainit.DMA_MemoryInc             = (hal_dma_transfer_per2mem == cfg->transfer) ? (DMA_MemoryInc_Enable) : (DMA_MemoryInc_Enable);
    intitem->stm32dmainit.DMA_PeripheralDataSize    = DMA_PeripheralDataSize_Byte; // decide granularity
    intitem->stm32dmainit.DMA_MemoryDataSize        = DMA_MemoryDataSize_Byte; // decide granularity
    intitem->stm32dmainit.DMA_Mode                  = DMA_Mode_Normal;
    intitem->stm32dmainit.DMA_Priority              = DMA_Priority_VeryHigh; // or decide which priority is given
    intitem->stm32dmainit.DMA_M2M                   = DMA_M2M_Disable; // or DMA_M2M_Enable in memory2memory
#elif    defined(HAL_USE_CPU_FAM_STM32F4)
    intitem->stm32dmainit.DMA_Channel               = DMA_Channel_0;
    #warning --> VERIFICA SE DMA_Channel_0 va bene oppure cosa ...
    intitem->stm32dmainit.DMA_PeripheralBaseAddr    = (uint32_t)cfg->source;
    intitem->stm32dmainit.DMA_Memory0BaseAddr       = (uint32_t)cfg->destin;
    intitem->stm32dmainit.DMA_DIR                   = (hal_dma_transfer_per2mem == cfg->transfer) ? (DMA_DIR_PeripheralToMemory) : (DMA_DIR_MemoryToPeripheral);
    intitem->stm32dmainit.DMA_BufferSize            = (uint32_t)cfg->datasize;
    intitem->stm32dmainit.DMA_PeripheralInc         = (hal_dma_transfer_per2mem == cfg->transfer) ? (DMA_PeripheralInc_Disable) : (DMA_PeripheralInc_Disable);    
    intitem->stm32dmainit.DMA_MemoryInc             = (hal_dma_transfer_per2mem == cfg->transfer) ? (DMA_MemoryInc_Enable) : (DMA_MemoryInc_Enable);
    intitem->stm32dmainit.DMA_PeripheralDataSize    = DMA_PeripheralDataSize_Byte; // decide granularity
    intitem->stm32dmainit.DMA_MemoryDataSize        = DMA_MemoryDataSize_Byte; // decide granularity
    intitem->stm32dmainit.DMA_Mode                  = DMA_Mode_Normal;    
    intitem->stm32dmainit.DMA_Priority              = DMA_Priority_VeryHigh; // or decide which priority is given
    intitem->stm32dmainit.DMA_FIFOMode              = DMA_FIFOMode_Disable;
    #warning --> VERIFICA DMA_FIFOMode_Disable, DMA_FIFOThreshold_Full, DMA_MemoryBurst_Single, 
    intitem->stm32dmainit.DMA_FIFOThreshold         = DMA_FIFOThreshold_Full;
    intitem->stm32dmainit.DMA_MemoryBurst           = DMA_MemoryBurst_Single;
    intitem->stm32dmainit.DMA_PeripheralBurst       = DMA_PeripheralBurst_Single;       
    
#else //defined(HAL_USE_CPU_FAM_*)
    #error ERR --> choose a HAL_USE_CPU_FAM_*
#endif   
    
    DMA_DeInit(intitem->stm32dmaperiph);
    DMA_Init(intitem->stm32dmaperiph, &intitem->stm32dmainit);
 
    intitem->enabled = hal_false;
    intitem->stopit = hal_false;

    s_hal_dma_initted_set(id);
    
    s_hal_dma_hw_nvic_init(id);

    return(hal_res_OK);
}


extern hal_result_t hal_dma_enable(hal_dma_t id)
{
	hal_dma_internal_item_t *intitem = s_hal_dma_theinternals.items[HAL_dma_id2index(id)];

    if(hal_false == s_hal_dma_initted_is(id))
    {
        return(hal_res_NOK_generic);
    }

    intitem->enabled = hal_true;
         
    DMA_ITConfig(HAL_dma_id2peripheral(id), DMA_IT_TC, ENABLE);
    hal_dma_isr_enable(id);
    
    DMA_Cmd(intitem->stm32dmaperiph, ENABLE);
    
	return(hal_res_OK);
}


extern hal_result_t hal_dma_disable(hal_dma_t id) 
{
	hal_dma_internal_item_t *intitem = s_hal_dma_theinternals.items[HAL_dma_id2index(id)];

    if(hal_false == s_hal_dma_initted_is(id))
    {
        return(hal_res_NOK_generic);
    }

    intitem->enabled = hal_false;
    
    DMA_Cmd(intitem->stm32dmaperiph, DISABLE);
    
    hal_dma_isr_disable(id);
    DMA_ITConfig(HAL_dma_id2peripheral(id), DMA_IT_TC, DISABLE);
        
	return(hal_res_OK);
}


extern void hal_dma_isr_enable(hal_dma_t id)
{
    hal_dma_internal_item_t *intitem = s_hal_dma_theinternals.items[HAL_dma_id2index(id)];
    IRQn_Type IRQn = s_hal_dma_irqnumber[HAL_dma_id2index(id)];
    if(hal_int_priorityNONE != intitem->cfg.intpriority)
    {
        hal_sys_irqn_enable(IRQn);
    }
}

extern void hal_dma_isr_disable(hal_dma_t id)
{
    hal_dma_internal_item_t *intitem = s_hal_dma_theinternals.items[HAL_dma_id2index(id)];
    IRQn_Type IRQn = s_hal_dma_irqnumber[HAL_dma_id2index(id)];
    if(hal_int_priorityNONE != intitem->cfg.intpriority)
    {
        hal_sys_irqn_disable(IRQn);
    }
}

extern hal_result_t hal_dma_retrigger(hal_dma_t id)
{
	hal_dma_internal_item_t *intitem = s_hal_dma_theinternals.items[HAL_dma_id2index(id)];

    if(hal_false == s_hal_dma_initted_is(id))
    {
        return(hal_res_NOK_generic);
    }
    
    intitem->stopit = hal_false;
    
    DMA_Cmd(intitem->stm32dmaperiph, DISABLE);
    intitem->stm32dmaperiph->NDTR  = intitem->stm32dmainit.DMA_BufferSize;
    DMA_Cmd(intitem->stm32dmaperiph, ENABLE);
    
    return(hal_res_OK);
}

extern hal_result_t hal_dma_dontdisable(hal_dma_t id)
{
	hal_dma_internal_item_t *intitem = s_hal_dma_theinternals.items[HAL_dma_id2index(id)];

    if(hal_false == s_hal_dma_initted_is(id))
    {
        return(hal_res_NOK_generic);
    }
    
    intitem->stopit = hal_false;
        
    return(hal_res_OK);
}




extern hal_result_t hal_dma_source_set(hal_dma_t id, void* source)
{
	hal_dma_internal_item_t *intitem = s_hal_dma_theinternals.items[HAL_dma_id2index(id)];

    if(hal_false == s_hal_dma_initted_is(id))
    {
        return(hal_res_NOK_generic);
    }
    
    if(NULL != source)
    {
        intitem->cfg.source                             = source;
        intitem->stm32dmainit.DMA_PeripheralBaseAddr    = (uint32_t)intitem->cfg.source;   
    }
    
    intitem->stopit = hal_false;
    
    DMA_Cmd(intitem->stm32dmaperiph, DISABLE);
    intitem->stm32dmaperiph->PAR   = intitem->stm32dmainit.DMA_PeripheralBaseAddr;
    intitem->stm32dmaperiph->NDTR  = intitem->stm32dmainit.DMA_BufferSize;
    DMA_Cmd(intitem->stm32dmaperiph, ENABLE);
    
    return(hal_res_OK);
}


extern hal_result_t hal_dma_destin_set(hal_dma_t id, void* destin)
{
	hal_dma_internal_item_t *intitem = s_hal_dma_theinternals.items[HAL_dma_id2index(id)];

    if(hal_false == s_hal_dma_initted_is(id))
    {
        return(hal_res_NOK_generic);
    }
    
    if(NULL != destin)
    {
        intitem->cfg.destin                         = destin;
#if     defined(HAL_USE_CPU_FAM_STM32F1)    
        intitem->stm32dmainit.DMA_MemoryBaseAddr    = (uint32_t)intitem->cfg.destin;
#elif   defined(HAL_USE_CPU_FAM_STM32F4)   
        intitem->stm32dmainit.DMA_Memory0BaseAddr   = (uint32_t)intitem->cfg.destin;
#else //defined(HAL_USE_CPU_FAM_*)
    #error ERR --> choose a HAL_USE_CPU_FAM_*
#endif                         
    }  

    intitem->stopit = hal_false;    
    
    DMA_Cmd(intitem->stm32dmaperiph, DISABLE);
    
#if     defined(HAL_USE_CPU_FAM_STM32F1)    
    intitem->stm32dmaperiph->CMAR   = intitem->stm32dmainit.DMA_MemoryBaseAddr;
    intitem->stm32dmaperiph->CNDTR  = intitem->stm32dmainit.DMA_BufferSize;
#elif   defined(HAL_USE_CPU_FAM_STM32F4)   
    intitem->stm32dmaperiph->M0AR   = intitem->stm32dmainit.DMA_Memory0BaseAddr;
    intitem->stm32dmaperiph->NDTR  = intitem->stm32dmainit.DMA_BufferSize;
#else //defined(HAL_USE_CPU_FAM_*)
    #error ERR --> choose a HAL_USE_CPU_FAM_*
#endif          

    DMA_Cmd(intitem->stm32dmaperiph, ENABLE);
    
    return(hal_res_OK);
}

// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------


// ---- isr of the module: begin ----

#if     defined(HAL_USE_CPU_FAM_STM32F1)

void DMA1_Channel1_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma1);
}

void DMA1_Channel2_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma2);  
}

void DMA1_Channel3_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma3);
}

void DMA1_Channel4_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma4);
}

void DMA1_Channel5_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma5);
}

void DMA1_Channel6_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma6);
}

void DMA1_Channel7_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma7);
}

void DMA2_Channel1_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma8);
}

void DMA2_Channel2_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma9);
}

void DMA2_Channel3_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma10);
}

void DMA2_Channel4_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma11);
}

void DMA2_Channel5_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma12);
}

#elif   defined(HAL_USE_CPU_FAM_STM32F4)

void DMA1_Stream0_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma1);
}

void DMA1_Stream1_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma2);  
}

void DMA1_Stream2_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma3);
}

void DMA1_Stream3_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma4);
}

void DMA1_Stream4_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma5);
}

void DMA1_Stream5_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma6);
}

void DMA1_Stream6_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma7);
}

void DMA1_Stream7_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma8);
}

void DMA2_Stream0_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma9);
}

void DMA2_Stream1_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma10);
}

void DMA2_Stream2_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma11);
}

void DMA2_Stream3_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma12);
}

void DMA2_Stream4_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma13);
}

void DMA2_Stream5_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma14);
}

void DMA2_Stream6_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma15);
}

void DMA2_Stream7_IRQHandler(void)
{
    s_hal_dma_isr_portx(hal_dma16);
}

#else //defined(HAL_USE_CPU_FAM_*)
    #error ERR --> choose a HAL_USE_CPU_FAM_*
#endif 

// ---- isr of the module: end ------







// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions 
// --------------------------------------------------------------------------------------------------------------------

static hal_boolval_t s_hal_dma_supported_is(hal_dma_t id)
{
    return((hal_boolval_t)hl_bits_hlfword_bitcheck(hal_brdcfg_dma__theconfig.supported_mask, HAL_dma_id2index(id)));
}

static void s_hal_dma_initted_set(hal_dma_t id)
{
    hl_bits_hlfword_bitset(&s_hal_dma_theinternals.initted, HAL_dma_id2index(id));
}

static hal_boolval_t s_hal_dma_initted_is(hal_dma_t id)
{
    return((hal_boolval_t)hl_bits_hlfword_bitcheck(s_hal_dma_theinternals.initted, HAL_dma_id2index(id)));
}


static void s_hal_dma_isr_portx(hal_dma_t id)
{   
    hal_dma_internal_item_t *intitem = s_hal_dma_theinternals.items[HAL_dma_id2index(id)];
    
    s_hal_dma_isr_clear_flag(id);
   
    
    if(hal_dma_mode_oneshot == intitem->cfg.mode)
    {
        intitem->stopit = hal_true;
    }
    
    if(NULL != intitem->cfg.cbk_on_transfer_done)
    {
        intitem->cfg.cbk_on_transfer_done(intitem->cfg.arg_on_transfer_done);
    }      
    

    if(hal_dma_mode_oneshot == intitem->cfg.mode)
    {
        // if ... inside intitem->cfg.cbk_on_transfer_done() we call any retrigger function, then  we dont disable
        if(hal_true == intitem->stopit)
        {
            hal_dma_disable(id);
        }
        
        intitem->stopit = hal_false;
    }  
       
}






static void s_hal_dma_hw_nvic_init(hal_dma_t id)
{
    hal_dma_internal_item_t *intitem = s_hal_dma_theinternals.items[HAL_dma_id2index(id)];   
    IRQn_Type IRQn = s_hal_dma_irqnumber[HAL_dma_id2index(id)];


    if(hal_int_priorityNONE != intitem->cfg.intpriority)
    {
        // enable rx irq in nvic
        hal_sys_irqn_priority_set(IRQn, intitem->cfg.intpriority);
        hal_sys_irqn_disable(IRQn);
    }

}


// static void s_hal_dma_isr_enable(hal_dma_t id)
// {
//     hal_dma_internal_item_t *intitem = s_hal_dma_theinternals.items[HAL_dma_id2index(id)];
//     IRQn_Type IRQn = s_hal_dma_irqnumber[HAL_dma_id2index(id)];
//     if(hal_int_priorityNONE != intitem->cfg.intpriority)
//     {
//         hal_sys_irqn_enable(IRQn);
//     }
// }

// static void s_hal_dma_isr_disable(hal_dma_t id)
// {
//     hal_dma_internal_item_t *intitem = s_hal_dma_theinternals.items[HAL_dma_id2index(id)];
//     IRQn_Type IRQn = s_hal_dma_irqnumber[HAL_dma_id2index(id)];
//     if(hal_int_priorityNONE != intitem->cfg.intpriority)
//     {
//         hal_sys_irqn_disable(IRQn);
//     }
// }

static void s_hal_dma_isr_clear_flag(hal_dma_t id) 
{
#if     defined(HAL_USE_CPU_FAM_STM32F1)
    DMA_ClearFlag(s_hal_dma_irqflag_gl[HAL_dma_id2index(id)]);
#elif   defined(HAL_USE_CPU_FAM_STM32F4)    
    DMA_ClearFlag(s_hal_dma_memory_mapping_of_ports[HAL_dma_id2index(id)], s_hal_dma_irqflag_gl[HAL_dma_id2index(id)]);
#else //defined(HAL_USE_CPU_FAM_*)
    #error ERR --> choose a HAL_USE_CPU_FAM_*
#endif 
}


#endif//HAL_USE_DMA

// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------


