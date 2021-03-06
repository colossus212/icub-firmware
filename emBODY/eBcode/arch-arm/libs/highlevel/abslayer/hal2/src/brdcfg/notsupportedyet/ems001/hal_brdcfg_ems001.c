/*
 * Copyright (C) 2012 iCub Facility - Istituto Italiano di Tecnologia
 * Author:  Valentina Gaggero, Marco Accame
 * email:   valentina.gaggero@iit.it, marco.accame@iit.it
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

/* @file       hal_brdcfg_ems001.c
	@brief      This file implements low level functions which depend on board.
	@author     valentina.gaggero@iit.it, marco.accame@iit.it
    @date       11/16/2010
**/

// - modules to be built: contains the HAL_USE_* macros ---------------------------------------------------------------
#include "hal_brdcfg_modules.h"

// --------------------------------------------------------------------------------------------------------------------
// - external dependencies
// --------------------------------------------------------------------------------------------------------------------

#include "stdlib.h"
#include "string.h"

#include "hal_middleware_interface.h"


#include "hal_base.h"
#include "hal_base_hid.h"

#include "hal_cpu.h"
#include "hal_cpu_hid.h"

#include "hal_utility_bits_hid.h" 
#include "hal_utility_crc07_hid.h" 
#include "hal_utility_crc16_hid.h" 
#include "hal_utility_crc32_hid.h" 
#include "hal_utility_fifo_hid.h" 


#include "hal_periph_can_hid.h"
#include "hal_periph_spi_hid.h"
#include "hal_periph_eth_hid.h"
#include "hal_periph_trace_hid.h"
#include "hal_eeprom.h"
#include "hal_timer.h"
#include "hal_i2c.h"
#include "hal_watchdog.h"

#include "hal_switch.h"
#include "hal_device_ethtransceiver_hid.h"

#include "hal_chip_xx_eeprom_hid.h"
#include "hal_chip_st_l3g4200d_hid.h"
#include "hal_chip_st_lis3dh_hid.h"
#include "hal_chip_micrel_ks8893_hid.h"
#include "hal_chip_generic_ethtransceiver_hid.h"

// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern public interface
// --------------------------------------------------------------------------------------------------------------------

#include "hal_brdcfg_ems001.h"


// --------------------------------------------------------------------------------------------------------------------
// - declaration of extern hidden interface 
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - #define with internal scope
// --------------------------------------------------------------------------------------------------------------------

#if defined(HAL_USE_PERIPH_ETH) || defined(HAL_USE_DEVICE_ETHTRANSCEIVER) || defined(HAL_USE_DEVICE_SWITCH)
    #define HAL_ETH_PHYMODE_THEONE2USE     hal_eth_phymode_fullduplex100mbps
#endif


#if defined(HAL_USE_PERIPH_SPI) || defined(HAL_USE_PERIPH_CAN)
#warning HAL-WIP --> ON EMS001: PE1 must be configured as output and kept HIGH to feed SPI and CAN with 5VCC
#endif

// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of extern variables, but better using _get(), _set() 
// --------------------------------------------------------------------------------------------------------------------


// -- core

#ifdef  HAL_USE_BASE
    extern const hal_base_hid_brdcfg_t hal_brdcfg_base__theconfig  =
    {
        .dummy  = 0
    };    
#endif//HAL_USE_BASE  
    
    
#ifdef  HAL_USE_CPU
    extern const hal_cpu_hid_brdcfg_t hal_brdcfg_cpu__theconfig  =
    {
        .architecture       = hal_cpu_arc_armcm3,
        .family             = hal_cpu_fam_stm32f1,
        .name               = hal_cpu_nam_stm32f107,
        .speeds             =
        { 
            .max                =  72000000,
            .cpu                =  72000000,
            .fastbus            =  72000000,
            .slowbus            =  36000000
        },
        .clockcfg                      =
        {
            .keepinternalclockatstartup     = hal_false,      // false is the preferred mode
            .refclock                       = hal_cpu_refclock_pll_on_external_xtl,  
            .intclockspeed                  =   8000000,
            .extclockspeed                  =  25000000,
            .pllcfg                         =
            {   // with refclock = hal_cpu_refclock_pll_on_external_xtl -> pll2clock = pll2mul*(extclockspeed/prediv2), speeds.cpu = pllmul*(pll2clock/prediv1) 
                .prediv2            = 5, 
                .pll2mul            = 8,
                .prediv1            = 5,
                .pllmul             = 9
            }
        }   
    };    
#endif//HAL_USE_CPU  

#ifdef  HAL_USE_FLASH
    extern const hal_flash_hid_brdcfg_t hal_brdcfg_flash__theconfig =
    {
        .baseaddress    =   0x08000000,                 // on every stm
        .totalsize      =
        #if     defined(HAL_USE_CPU_NAM_STM32F107)
                            256*1024
        #elif   defined(HAL_USE_CPU_NAM_STM32F407)
                            1024*1024
        #endif
    };
#endif//HAL_USE_FLASH
 

#ifdef  HAL_USE_HEAP
    extern const hal_heap_hid_brdcfg_t hal_brdcfg_heap__theconfig =
    {
        .nothingsofar   = 0
    };
#endif//HAL_USE_HEAP    
   
    
#ifdef  HAL_USE_SYS
    extern const hal_sys_hid_brdcfg_t hal_brdcfg_sys__theconfig  =
    {
        .dummy             = 0
    };    
#endif//HAL_USE_SYS   
   

// --  extra: mpu peripherals

#ifdef  HAL_USE_PERIPH_CAN
    extern const hal_can_hid_brdcfg_t hal_brdcfg_can__theconfig =
    {
        .supported_mask     = (1 << hal_can1) | (1 << hal_can2),
        .gpio_rx            =
        {
            {   // hal_can1 
                .gpio   = { .port = hal_gpio_portD,     .pin = hal_gpio_pin0 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            },            
            {   // hal_can2 
                .gpio   = { .port = hal_gpio_portB,     .pin = hal_gpio_pin5 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            }
        },
        .gpio_tx            =
        {

            {   // hal_can1 
                .gpio   = { .port = hal_gpio_portD,     .pin = hal_gpio_pin1 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            },            
            {   // hal_can2 
                .gpio   = { .port = hal_gpio_portB,     .pin = hal_gpio_pin6 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            }
        } 
    };
#endif//HAL_USE_PERIPH_CAN

#ifdef  HAL_USE_PERIPH_CRC
    extern const hal_crc_hid_brdcfg_t hal_brdcfg_crc__theconfig =
    {
        .supported_mask     = (1 << hal_crc1) | (1 << hal_crc2)
    };
#endif//HAL_USE_PERIPH_CRC
    
#ifdef  HAL_USE_PERIPH_DMA
    extern const hal_dma_hid_brdcfg_t hal_brdcfg_dma__theconfig =
    {
        .supported_mask     = (1 << hal_dma1) | (1 << hal_dma2) | (1 << hal_dma3) | (1 << hal_dma4) | (1 << hal_dma5) |
                              (1 << hal_dma6) | (1 << hal_dma7) | (1 << hal_dma8) | (1 << hal_dma9) | (1 << hal_dma10)|
                              (1 << hal_dma11)| (1 << hal_dma12)

    };
#endif//HAL_USE_PERIPH_DMA       

#ifdef  HAL_USE_PERIPH_ETH
    
    extern const hal_eth_hid_brdcfg_t hal_brdcfg_eth__theconfig     =
    {
        .supported          = hal_true,
        .mif                = hal_eth_mif_rmii,
        .gpio_mif.rmii      =
        {
            .ETH_RMII_REF_CLK   =            
            {   
                .gpio   = { .port = hal_gpio_portA,     .pin = hal_gpio_pin1 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_max,        .altcfg = NULL }
            },  
            .ETH_RMII_TX_EN     =            
            {   
                .gpio   = { .port = hal_gpio_portB,     .pin = hal_gpio_pin11 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_max,        .altcfg = NULL }
            },
            .ETH_RMII_TXD0      =            
            {   
                .gpio   = { .port = hal_gpio_portB,     .pin = hal_gpio_pin12 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_max,        .altcfg = NULL }
            },
            .ETH_RMII_TXD1      =            
            {   
                .gpio   = { .port = hal_gpio_portB,     .pin = hal_gpio_pin13 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_max,        .altcfg = NULL }
            },
            .ETH_RMII_CRS_DV    =            
            {   
                .gpio   = { .port = hal_gpio_portD,     .pin = hal_gpio_pin8 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_max,        .altcfg = NULL }
            },
            .ETH_RMII_RXD0      =            
            {   
                .gpio   = { .port = hal_gpio_portD,     .pin = hal_gpio_pin9 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_max,        .altcfg = NULL }
            },
            .ETH_RMII_RXD1      =            
            {   
                .gpio   = { .port = hal_gpio_portD,     .pin = hal_gpio_pin10 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_max,        .altcfg = NULL }
            }     
        },
        .gpio_smi       =
        {
            .ETH_MDC        =            
            {   
                .gpio   = { .port = hal_gpio_portC,     .pin = hal_gpio_pin1 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_max,        .altcfg = NULL }
            },     
            .ETH_MDIO       =            
            {   
                .gpio   = { .port = hal_gpio_portA,     .pin = hal_gpio_pin2 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_max,        .altcfg = NULL }
            }     
        }
    };

#endif//HAL_USE_PERIPH_ETH



#ifdef HAL_USE_PERIPH_GPIO
    extern const hal_gpio_hid_brdcfg_t hal_brdcfg_gpio__theconfig =
    {
        .supported_mask_byport    =
        {   // ok, i enable every pin of every port. 
            0xffff,     // port a
            0xffff,     // port b
            0xffff,     // port c
            0xffff,     // port d
            0xffff,     // port e
            0xffff,     // port f
            0xffff,     // port g
            0x0000,     // port h
            0x0000      // port i
        }
    };
#endif//HAL_USE_PERIPH_GPIO


#ifdef  HAL_USE_PERIPH_I2C
    extern const hal_i2c_hid_brdcfg_t hal_brdcfg_i2c__theconfig =
    {
        .supported_mask         = (1 << hal_i2c1),
        .gpio_scl               =
        {
            {   // hal_i2c1 
                .gpio   = { .port = hal_gpio_portB,     .pin = hal_gpio_pin8 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            },              
            {   // hal_i2c2 
                .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            },             
            {   // hal_i2c3 
                .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            }            
        },
        .gpio_sda               =
        {
            {   // hal_i2c1 
                .gpio   = { .port = hal_gpio_portB,     .pin = hal_gpio_pin9 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            },              
            {   // hal_i2c2 
                .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            },             
            {   // hal_i2c3 
                .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            }                  
        }
    };
#endif//HAL_USE_PERIPH_I2C


#ifdef  HAL_USE_PERIPH_SPI
    extern const hal_spi_hid_brdcfg_t hal_brdcfg_spi__theconfig =
    {
        .supported_mask         = (1 << hal_spi1) | (0 << hal_spi2) | (1 << hal_spi3),
        .gpio_sck               =
        {
            {   // hal_spi1 
                .gpio   = { .port = hal_gpio_portA,     .pin = hal_gpio_pin5 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            },              
            {   // hal_spi2 
                .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            }, 
            {   // hal_spi3 
                .gpio   = { .port = hal_gpio_portC,     .pin = hal_gpio_pin10}, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            }
        },
        .gpio_miso              =
        {
            {   // hal_spi1 
                .gpio   = { .port = hal_gpio_portA,     .pin = hal_gpio_pin6 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            },              
            {   // hal_spi2 
                .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            }, 
            {   // hal_spi3 
                .gpio   = { .port = hal_gpio_portC,     .pin = hal_gpio_pin11}, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            }            
        },
        .gpio_mosi              =
        {
            {   // hal_spi1 
                .gpio   = { .port = hal_gpio_portA,     .pin = hal_gpio_pin7 }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            },              
            {   // hal_spi2 
                .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE }, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            }, 
            {   // hal_spi3 
                .gpio   = { .port = hal_gpio_portC,     .pin = hal_gpio_pin12}, 
                .config = { .dir = hal_gpio_dirALT,     .speed = hal_gpio_speed_default,    .altcfg = NULL }
            }            
        }      
    };
#endif//HAL_USE_PERIPH_SPI
    
#ifdef  HAL_USE_PERIPH_TIMER
    extern const hal_timer_hid_brdcfg_t hal_brdcfg_timer__theconfig =
    {
        .supported_mask     = (0 << hal_timer1) | (1 << hal_timer2) | (1 << hal_timer3) | (1 << hal_timer4) | (1 << hal_timer5) | (1 << hal_timer6) | (1 << hal_timer7)
    };       
#endif//HAL_USE_PERIPH_TIMER

    
#ifdef  HAL_USE_PERIPH_TRACE
    extern const hal_trace_hid_brdcfg_t hal_brdcfg_trace__theconfig =
    {
        .supported          = hal_true
    };       
#endif//HAL_USE_PERIPH_TRACE
  

#ifdef  HAL_USE_PERIPH_UNIQUEID
    extern const hal_uniqueid_hid_brdcfg_t hal_brdcfg_uniqueid__theconfig =
    {
        .supported_mask     = (1 << hal_uniqueid_id64bit) | (0 << hal_uniqueid_macaddr),
        .macoui             = 0
    };       
#endif//HAL_USE_PERIPH_UNIQUEID
    

#ifdef  HAL_USE_PERIPH_WATCHDOG
    extern const hal_watchdog_hid_brdcfg_t hal_brdcfg_watchdog__theconfig =
    {
        .supported_mask     = (1 << hal_watchdog1_normal) | (1 << hal_watchdog2_window)
    };       
#endif//HAL_USE_PERIPH_WATCHDOG


// -- devices

#ifdef  HAL_USE_DEVICE_ACCELEROMETER   
    extern const hal_device_accelerometer_hid_brdcfg_t hal_brdcfg_device_accelerometer__theconfig =
    {
        .supported_mask         = 0x1,
        .devcfg         =
        {
            {   // hal_accelerometer_port1
                .chipif             =      
                {   // use the lis3dh chip by st
                    .init       = (hal_res_fp_voidp_t)hal_chip_st_lis3dh_init,
                    .initpar    = NULL,
                    .read       = hal_chip_st_lis3dh_accel_get  
                }                    
            }
         }
    };       
#endif//HAL_USE_DEVICE_ACCELEROMETER


#ifdef  HAL_USE_DEVICE_CANTRANSCEIVER

    static hal_result_t s_hal_device_cantransceiver_simple_init(hal_cantransceiver_t id, void* initpar);
    static hal_result_t s_hal_device_cantransceiver_simple_enable(hal_cantransceiver_t id);
    static hal_result_t s_hal_device_cantransceiver_simple_disable(hal_cantransceiver_t id);
    
    static const hal_gpio_map_t s_hal_device_cantransceiver__gpiomapof_enable[hal_cantransceivers_number] = 
    {
        {   // hal_cantransceiver1 
            .gpio   = { .port = hal_gpio_portB,     .pin = hal_gpio_pin14 }, 
            .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_low,        .altcfg = NULL }
        },        
        {   // hal_cantransceiver2 
            .gpio   = { .port = hal_gpio_portB,     .pin = hal_gpio_pin15 }, 
            .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_low,        .altcfg = NULL }
        }        
    };
       
    static hal_result_t s_hal_device_cantransceiver_simple_init(hal_cantransceiver_t id, void* initpar)
    {
        initpar = initpar;
        const hal_gpio_map_t* map = &s_hal_device_cantransceiver__gpiomapof_enable[(uint8_t)id];        
        hal_gpio_init(map->gpio, &map->config);        
        s_hal_device_cantransceiver_simple_disable(id);          
        return(hal_res_OK);
    }

    static hal_result_t s_hal_device_cantransceiver_simple_enable(hal_cantransceiver_t id)
    {  
        const hal_gpio_map_t* map = &s_hal_device_cantransceiver__gpiomapof_enable[(uint8_t)id];          
        hal_gpio_setval(map->gpio, hal_gpio_valLOW);
        return(hal_res_OK);
    }

    static hal_result_t s_hal_device_cantransceiver_simple_disable(hal_cantransceiver_t id)
    {   
        const hal_gpio_map_t* map = &s_hal_device_cantransceiver__gpiomapof_enable[(uint8_t)id];         
        hal_gpio_setval(map->gpio, hal_gpio_valHIGH);
        return(hal_res_OK);
    }
    
    extern const hal_device_cantransceiver_hid_brdcfg_t hal_brdcfg_device_cantransceiver__theconfig =
    {
        .supported_mask     =  (1 << hal_cantransceiver1) | (1 << hal_cantransceiver2),
        .devcfg             =
        {            
            .chipif             =
            {
                .init               = s_hal_device_cantransceiver_simple_init,
                .initpar            = NULL,
                .enable             = s_hal_device_cantransceiver_simple_enable,
                .disable            = s_hal_device_cantransceiver_simple_disable
            }
        }   
    };    
#endif//HAL_USE_DEVICE_CANTRANSCEIVER     
    

#ifdef HAL_USE_DEVICE_DISPLAY    
    extern const hal_device_display_hid_brdcfg_t hal_brdcfg_device_display__theconfig =
    {
        .supported_mask     = (0 << hal_display1),
        .devcfg             =
        {  
            {   // hal_display_dev1
                .chipif             =
                {
                    .init               = NULL,
                    .initpar            = NULL,
                    .clear              = NULL,
                    .settextproperties  = NULL,
                    .clearline          = NULL,
                    .putchar            = NULL,
                    .putstring          = NULL
                }
            }
        }   
    };
#endif//HAL_USE_DEVICE_DISPLAY

  
#ifdef  HAL_USE_DEVICE_EEPROM
    static const hal_gpio_val_t hal_brdcfg_device_eeprom__wp_gpio_enableval    = hal_gpio_valHIGH;      // write protection is on value high                          
    static const hal_gpio_val_t hal_brdcfg_device_eeprom__wp_gpio_disableval   = hal_gpio_valLOW;       // write protection is on value high
    
    static const hal_gpio_map_t hal_brdcfg_device_eeprom__gpiomapof_wp  = 
    {    
        .gpio   = { .port = hal_gpio_portB,     .pin = hal_gpio_pin10 }, 
        .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_high,       .altcfg = NULL }
    };

    

    static hal_result_t s_hal_brdcfg_device_eeprom__wp_init(void)
    {
        const hal_gpio_map_t* map = &hal_brdcfg_device_eeprom__gpiomapof_wp;
        return(hal_gpio_init(map->gpio, &map->config));
    }

    static hal_result_t s_hal_brdcfg_device_eeprom__wp_enable(void)
    {
        const hal_gpio_map_t* map = &hal_brdcfg_device_eeprom__gpiomapof_wp;
        return(hal_gpio_setval(map->gpio, hal_brdcfg_device_eeprom__wp_gpio_enableval));
    }

    static hal_result_t s_hal_brdcfg_device_eeprom__wp_disable(void)
    {
        const hal_gpio_map_t* map = &hal_brdcfg_device_eeprom__gpiomapof_wp;
        return(hal_gpio_setval(map->gpio, hal_brdcfg_device_eeprom__wp_gpio_disableval));
    }

    
    // used by hal_brdcfg_device_eeprom__theconfig
    static const hal_chip_xx_eeprom_cfg_t s_initpar_eeprom_i2c_01 =
    {
        .chipcfg    =
        {
            .chip           = hal_chip_xx_eeprom_chip_atmel_at24c512b,
            .i2cid          = hal_i2c1,
            .hwaddra2a1a0   = 0 // a0 = a1 = a2 = 0
        },
        .wrprcfg    =
        {
            .wrpr_init              = s_hal_brdcfg_device_eeprom__wp_init,
            .wrpr_deinit            = NULL,
            .wrpr_enable            = s_hal_brdcfg_device_eeprom__wp_enable,
            .wrpr_disable           = s_hal_brdcfg_device_eeprom__wp_disable            
        }
    };    

    extern const hal_device_eeprom_hid_brdcfg_t hal_brdcfg_device_eeprom__theconfig =
    {
        .supported_mask     =  (0 << hal_eeprom1_emulatedflash) | (1 << hal_eeprom2_i2c_01) | (0 << hal_eeprom3_i2c_02),
        .devcfg             =
        {            
            {   // hal_device_eeprom1_emulatedflash 
                .flashemul  =
                {
                    .baseaddress    = 0x08000000,
                    .totalsize      = 0
                }
            },
            {   // hal_device_eeprom2_i2c_01
                .i2cbased   =
                {
                    .baseaddress    = 0,
                    .totalsize      = 8*1024,
                    .i2cid          = hal_i2c1,
                    .chipif         =
                    {   // use the chip xx_eeprom
                        .init           = (hal_res_fp_voidp_t)hal_chip_xx_eeprom_init,
                        .initpar        = (void*)&s_initpar_eeprom_i2c_01,
                        .read           = hal_chip_xx_eeprom_read,
                        .write          = hal_chip_xx_eeprom_write                               
                    }                    
                }           
            },
            {   // hal_device_eeprom3_i2c_02
                .i2cbased   =
                {
                    .baseaddress    = 0,
                    .totalsize      = 0,
                    .i2cid          = hal_i2c1,
                    .chipif         =
                    {   // use the chip xx_eeprom
                        .init           = NULL,
                        .initpar        = NULL,
                        .read           = NULL,
                        .write          = NULL                               
                    }                    
                }           
            }                
        }
    
    };    
#endif//HAL_USE_DEVICE_EEPROM 

    
#ifdef HAL_USE_DEVICE_ENCODER
    // hal_encoder1 is on connector P6 of ems, hal_encoder2 is on connector P7 of ems, hal_encoder3 is on connector P8 of ems
    // hal_encoder4 is on connector P9 of ems, hal_encoder5 is on connector P10 of ems, hal_encoder6 is on connector P11 of ems
    extern const hal_device_encoder_hid_brdcfg_t hal_brdcfg_device_encoder__theconfig =
    {
        .supported_mask     = (1 << hal_encoder1) | (1 << hal_encoder2) | (1 << hal_encoder3) | (1 << hal_encoder4) | (1 << hal_encoder5) | (1 << hal_encoder6), 
        .muxid              = 
        {   
            hal_mux1,       hal_mux3,       hal_mux1,       hal_mux3,       hal_mux1,       hal_mux3           
        },
        .muxsel             = 
        {   
            hal_mux_selA,   hal_mux_selA,   hal_mux_selB,   hal_mux_selB,   hal_mux_selC,   hal_mux_selC           
        },  
        .spiid              = 
        {   
            hal_spi1,       hal_spi3,       hal_spi1,       hal_spi3,       hal_spi1,       hal_spi3           
        }
    };
#endif//HAL_USE_DEVICE_ENCODER  
    

#ifdef HAL_USE_DEVICE_ETHTRANSCEIVER
    extern const hal_chip_micrel_ks8893_cfg_t s_micrel_ks8893_cfg;
    extern const hal_device_ethtransceiver_hid_brdcfg_t hal_brdcfg_device_ethtransceiver__theconfig =
    {
        .supported      = hal_true,
        .devcfg         =
        {
            .targetphymode      = HAL_ETH_PHYMODE_THEONE2USE,
            .chipif             =
            {   // use the micrel 
                .init           = (hal_res_fp_voidp_t)hal_chip_micrel_ks8893_init,
                .initpar        = (void*)&s_micrel_ks8893_cfg,
                .config         = hal_chip_micrel_ks8893_configure, 
                .getphymode     = hal_chip_micrel_ks8893_mii_getphymode                
            }
         }
    };   
#endif//HAL_USE_DEVICE_ETHTRANSCEIVER

    
#ifdef  HAL_USE_DEVICE_GYROSCOPE
    extern const hal_device_gyroscope_hid_brdcfg_t hal_brdcfg_device_gyroscope__theconfig =
    {
        .supported_mask         = (1 << hal_gyroscope1),
        .devcfg         =
        {
            {   // hal_gyroscope_port1
                .chipif             =      
                {   // use the l3g4200d chip by st
                    .init       = (hal_res_fp_voidp_t)hal_chip_st_l3g4200d_init,
                    .initpar    = NULL,
                    .read       = hal_chip_st_l3g4200d_angrate_get
                }                    
            }
         }
    };          
#endif//HAL_USE_DEVICE_GYROSCOPE
 

#ifdef HAL_USE_DEVICE_LED
    extern const hal_device_led_hid_brdcfg_t hal_brdcfg_device_led__theconfig =
    {
        .supported_mask             = (1 << hal_led1) | (1 << hal_led2) | (1 << hal_led3) | (1 << hal_led4) | (1 << hal_led5) | (1 << hal_led6), // = 0x3F: only first 6 leds ...
        .value_on                   = hal_gpio_valLOW,
        .value_off                  = hal_gpio_valHIGH,
        .gpiomaps                    =
        {
            {   // hal_led1 
                .gpio   = { .port = hal_gpio_portE,     .pin = hal_gpio_pin8 }, 
                .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_low,    .altcfg = NULL }
            },
            {   // hal_led2 
                .gpio   = { .port = hal_gpio_portE,     .pin = hal_gpio_pin10 }, 
                .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_low,    .altcfg = NULL }
            },            
            {   // hal_led3 
                .gpio   = { .port = hal_gpio_portE,     .pin = hal_gpio_pin12 }, 
                .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_low,    .altcfg = NULL }
            },            
            {   // hal_led4 
                .gpio   = { .port = hal_gpio_portE,     .pin = hal_gpio_pin15 }, 
                .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_low,    .altcfg = NULL }
            },            
            {   // hal_led5 
                .gpio   = { .port = hal_gpio_portD,     .pin = hal_gpio_pin11 }, 
                .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_low,    .altcfg = NULL }
            },   
            {   // hal_led6 
                .gpio   = { .port = hal_gpio_portB,     .pin = hal_gpio_pin7 }, 
                .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_low,    .altcfg = NULL }
            },   
            {   // hal_led7 
                 .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE}, 
                 .config = { .dir = hal_gpio_dirNONE,    .speed = hal_gpio_speed_low,    .altcfg = NULL }
            },               
            {   // hal_led8 
                 .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE}, 
                 .config = { .dir = hal_gpio_dirNONE,    .speed = hal_gpio_speed_low,    .altcfg = NULL }
            }
//             {   // hal_lednone 
//                 .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE}, 
//                 .config = { .dir = hal_gpio_dirNONE,    .speed = hal_gpio_speed_low,    .altcfg = NULL }
//             }

        }
    };
#endif//HAL_USE_DEVICE_LED
    
    
#ifdef HAL_USE_DEVICE_MUX
    extern const hal_device_mux_hid_brdcfg_t hal_brdcfg_device_mux__theconfig =
    {
        .supported_mask             = (1 << hal_mux1) | (0 << hal_mux2) | (1 << hal_mux3) | (0 << hal_mux4), 
        .gpio_sel0                  =
        {
            {   // hal_mux1 
                .gpio   = { .port = hal_gpio_portD,     .pin = hal_gpio_pin2 }, 
                .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_high,   .altcfg = NULL }
            },
            {   // hal_mux2 
                .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE }, 
                .config = { .dir = hal_gpio_dirNONE,    .speed = hal_gpio_speed_NONE,   .altcfg = NULL }
            },            
            {   // hal_mux3 
                .gpio   = { .port = hal_gpio_portD,     .pin = hal_gpio_pin5 }, 
                .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_high,   .altcfg = NULL }
            },            
            {   // hal_mux4 
                .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE }, 
                .config = { .dir = hal_gpio_dirNONE,    .speed = hal_gpio_speed_NONE,   .altcfg = NULL }
            }                                   
        },
        .gpio_sel1                  =
        {
            {   // hal_mux1 
                .gpio   = { .port = hal_gpio_portD,     .pin = hal_gpio_pin3 }, 
                .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_high,   .altcfg = NULL }
            },
            {   // hal_mux2 
                .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE }, 
                .config = { .dir = hal_gpio_dirNONE,    .speed = hal_gpio_speed_NONE,   .altcfg = NULL }
            },            
            {   // hal_mux3 
                .gpio   = { .port = hal_gpio_portD,     .pin = hal_gpio_pin6 }, 
                .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_high,   .altcfg = NULL }
            },            
            {   // hal_mux4 
                .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE }, 
                .config = { .dir = hal_gpio_dirNONE,    .speed = hal_gpio_speed_NONE,   .altcfg = NULL }
            }                                  
        }, 
        .gpio_enable                =
        {
            {   // hal_mux1 
                .gpio   = { .port = hal_gpio_portD,     .pin = hal_gpio_pin4 }, 
                .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_high,   .altcfg = NULL }
            },
            {   // hal_mux2 
                .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE }, 
                .config = { .dir = hal_gpio_dirNONE,    .speed = hal_gpio_speed_NONE,   .altcfg = NULL }
            },            
            {   // hal_mux3 
                .gpio   = { .port = hal_gpio_portD,     .pin = hal_gpio_pin7 }, 
                .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_high,   .altcfg = NULL }
            },            
            {   // hal_mux4 
                .gpio   = { .port = hal_gpio_portNONE,  .pin = hal_gpio_pinNONE }, 
                .config = { .dir = hal_gpio_dirNONE,    .speed = hal_gpio_speed_NONE,   .altcfg = NULL }
            }                        
        }        
    };
#endif//HAL_USE_DEVICE_MUX      
    

#ifdef HAL_USE_DEVICE_SWITCH
    extern const hal_chip_micrel_ks8893_cfg_t s_micrel_ks8893_cfg;
    extern const hal_device_switch_hid_brdcfg_t hal_brdcfg_device_switch__theconfig =
    {
        .supported      = hal_true,
        .devcfg         =
        {
            .targetphymode  = HAL_ETH_PHYMODE_THEONE2USE,
            .chipif         =
            {
                .init       = (hal_res_fp_voidp_t)hal_chip_micrel_ks8893_init,
                .initpar    = (void*)&s_micrel_ks8893_cfg,
                .config     = hal_chip_micrel_ks8893_configure,          
            }
        }
    };
#endif//HAL_USE_DEVICE_SWITCH

    
#ifdef  HAL_USE_DEVICE_TERMOMETER 
    extern const hal_device_termometer_hid_brdcfg_t hal_brdcfg_device_termometer__theconfig =
    {
        .supported_mask         = (1 << hal_termometer1),
        .devcfg         =
        {
            {   // hal_termometer_port1
                .chipif             =      
                {   // use the l3g4200d chip by st
                    .init       = (hal_res_fp_voidp_t)hal_chip_st_l3g4200d_init,
                    .initpar    = NULL,
                    .read       = hal_chip_st_l3g4200d_temp_get
                }                    
            }
         }
    };           
#endif//HAL_USE_DEVICE_TERMOMETER
    

// -- chips
      
#ifdef  HAL_USE_CHIP_MICREL_KS8893     
    static hal_result_t s_hal_brdcfg_device_switch__extclock_init(void);    
    static const hal_chip_micrel_ks8893_cfg_t s_micrel_ks8893_cfg =
    {
        .i2cid          = hal_i2c1,
        .resetpin       =
        {   
            .gpio   = { .port = hal_gpio_portB,     .pin = hal_gpio_pin2 }, 
            .config = { .dir = hal_gpio_dirOUT,     .speed = hal_gpio_speed_low,   .altcfg = NULL }
        },
        .extclockinit   = s_hal_brdcfg_device_switch__extclock_init
    }; 
#endif//HAL_USE_CHIP_MICREL_KS8893
    
    

// -- extbrds
// none
    
    


// --------------------------------------------------------------------------------------------------------------------
// - typedef with internal scope
// --------------------------------------------------------------------------------------------------------------------
// empty-section

// --------------------------------------------------------------------------------------------------------------------
// - declaration of static functions
// --------------------------------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------------------------------
// - definition (and initialisation) of static variables
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - definition of extern public functions
// --------------------------------------------------------------------------------------------------------------------





// --------------------------------------------------------------------------------------------------------------------
// - definition of extern hidden functions 
// --------------------------------------------------------------------------------------------------------------------
// empty-section


// --------------------------------------------------------------------------------------------------------------------
// - definition of static functions
// --------------------------------------------------------------------------------------------------------------------



#ifdef  HAL_USE_DEVICE_SWITCH
#ifdef  HAL_USE_CHIP_MICREL_KS8893

static void s_hal_brdcfg_device_switch__mco_initialise(void)
{
    // this function initialises MCO in order to provide clock ref to switch.
    // PA8 is MCO. it must be configured = Output mode, max speed 50 MHz + Alternate function output Push-pull (B)
    // also, we connect pll3 at 50mhz to it
    
    // clock gpioa as alternate function
    RCC->APB2ENR    |= 0x00000005;
    // init pa8
    GPIOA->CRH   &= 0xFFFFFFF0;
    GPIOA->CRH   |= 0x0000000B;	


    // set pll3 clock output to 50mhz: (25mhz/5)*10 = 50mhz, thus we use multiplier 10
    RCC_PLL3Config(RCC_PLL3Mul_10);
        
    // enable pll3 
    RCC_PLL3Cmd(ENABLE);
    
    // wait until it is ready
    while(RCC_GetFlagStatus(RCC_FLAG_PLL3RDY) == RESET);
    
    // connect mco on pa8 with pll3
    RCC_MCOConfig(RCC_MCO_PLL3CLK);
}

static hal_result_t s_hal_brdcfg_device_switch__extclock_init(void)
{
    s_hal_brdcfg_device_switch__mco_initialise();  
    return(hal_res_OK);    
}

#endif//HAL_USE_CHIP_MICREL_KS8893
#endif//HAL_USE_DEVICE_SWITCH





// --------------------------------------------------------------------------------------------------------------------
// - end-of-file (leave a blank line after)
// --------------------------------------------------------------------------------------------------------------------


// removed code


#if 0
#ifdef HAL_USE_SPI4ENCODER

/*Note: if there will be some speed problem try to transform  hal_brdcfg_spi4encoder__encoder_enable and  hal_brdcfg_spi4encoder__encoder_disable as inline function,
but pay attention with the project with scatter file.probably it will give you trouble*/

extern void  hal_brdcfg_spi4encoder__encoder_enable(hal_spi_port_t spix,  hal_spi_mux_t e)
{
    HAL_BRDCFG_SPI4ENCODER__SPI_CS_SEL(spix, e);
    HAL_BRDCFG_SPI4ENCODER__SPI_CS_ENA(spix);	
}

extern void  hal_brdcfg_spi4encoder__encoder_disable(hal_spi_port_t spix,  hal_spi_mux_t e)
{
    HAL_BRDCFG_SPI4ENCODER__SPI_CS_DESEL(spix);
    HAL_BRDCFG_SPI4ENCODER__SPI_CS_DISA(spix);	
}

void  hal_brdcfg_spi4encoder__chipSelect_init(hal_spi_port_t spix)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //output push-pull mode
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

//1) init data struct and GPIO regiter
	switch(spix)
	{
		case hal_spi_port1:
		{
    		hal_SPI4ENCODER_ENCDATA_GET(spix).cs_encoder[0].BRR_reg = SPI1_GPIO_SEL_LOW_ENCODER_0; //pin che devono valere zero
    		hal_SPI4ENCODER_ENCDATA_GET(spix).cs_encoder[0].BSRR_reg = SPI1_GPIO_SEL_HIGH_ENCODER_0;	//pin che devono valere 1
    
    		hal_SPI4ENCODER_ENCDATA_GET(spix).cs_encoder[1].BRR_reg = SPI1_GPIO_SEL_LOW_ENCODER_1;
    		hal_SPI4ENCODER_ENCDATA_GET(spix).cs_encoder[1].BSRR_reg = SPI1_GPIO_SEL_HIGH_ENCODER_1;
    
    		hal_SPI4ENCODER_ENCDATA_GET(spix).cs_encoder[2].BRR_reg = SPI1_GPIO_SEL_LOW_ENCODER_2;
    		hal_SPI4ENCODER_ENCDATA_GET(spix).cs_encoder[2].BSRR_reg = SPI1_GPIO_SEL_HIGH_ENCODER_2;
    
    		hal_SPI4ENCODER_ENCDATA_GET(spix).cs_pin_ena_slave = SPI1_GPIO_ENA_SLAVE;
    		hal_SPI4ENCODER_ENCDATA_GET(spix).encoder_gpio_port = HAL_BRDCFG_SPI4ENCODER__SPI1_GPIO_PORT_CS;	
    
    		GPIO_InitStructure.GPIO_Pin = SPI1_GPIO_SEL_ENCODER_PIN_1| SPI1_GPIO_SEL_ENCODER_PIN_2 | SPI1_GPIO_ENA_SLAVE; //tutti i pin sono sulla stessa porta.
    		GPIO_Init(HAL_BRDCFG_SPI4ENCODER__SPI1_GPIO_PORT_CS, &GPIO_InitStructure);
		
		} break;
		
		case hal_spi_port3:
		{
    		hal_SPI4ENCODER_ENCDATA_GET(spix).cs_encoder[0].BRR_reg = SPI3_GPIO_SEL_LOW_ENCODER_0; //pin che devono valere zero
    		hal_SPI4ENCODER_ENCDATA_GET(spix).cs_encoder[0].BSRR_reg = SPI3_GPIO_SEL_HIGH_ENCODER_0;	//pin che devono valere 1
    
    		hal_SPI4ENCODER_ENCDATA_GET(spix).cs_encoder[1].BRR_reg = SPI3_GPIO_SEL_LOW_ENCODER_1;
    		hal_SPI4ENCODER_ENCDATA_GET(spix).cs_encoder[1].BSRR_reg = SPI3_GPIO_SEL_HIGH_ENCODER_1;
    
    		hal_SPI4ENCODER_ENCDATA_GET(spix).cs_encoder[2].BRR_reg = SPI3_GPIO_SEL_LOW_ENCODER_2;
    		hal_SPI4ENCODER_ENCDATA_GET(spix).cs_encoder[2].BSRR_reg = SPI3_GPIO_SEL_HIGH_ENCODER_2;
    
    		hal_SPI4ENCODER_ENCDATA_GET(spix).cs_pin_ena_slave = SPI3_GPIO_ENA_SLAVE;
    		hal_SPI4ENCODER_ENCDATA_GET(spix).encoder_gpio_port = HAL_BRDCFG_SPI4ENCODER__SPI3_GPIO_PORT_CS;	
    
    		GPIO_InitStructure.GPIO_Pin = SPI3_GPIO_SEL_ENCODER_PIN_1| SPI3_GPIO_SEL_ENCODER_PIN_2 | SPI3_GPIO_ENA_SLAVE; //tutti i pin sono sulla stessa porta.
    		GPIO_Init(HAL_BRDCFG_SPI4ENCODER__SPI3_GPIO_PORT_CS, &GPIO_InitStructure);
		
		} break;

	}


//3) set pin high value
	HAL_BRDCFG_SPI4ENCODER__SPI_CS_DESEL(spix);
	HAL_BRDCFG_SPI4ENCODER__SPI_CS_DISA(spix);
	
//4)  enable powe supply
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE); 
	GPIOE->CRL &= 0xFFFFFF0F;  //reset Pin 1
	GPIOE->CRL |= 0x00000020;  //Configure pin 1 as General Purpose output Push Pull 2Mhz 
	GPIOE->BSRR |= 0x0002; //enable power supply
}

#endif//HAL_USE_SPI4ENCODER
#endif




#if 0
extern void hal_brdcfg_sys__clock_config(void)
{
 
}

extern void hal_brdcfg_sys__gpio_default_init(void)
{

#if 0

//
//#define   IN_ANALOG          0x0
//#define   IN_FLOATING        0x04
//#define   OUT_PUSH_PULL_2M   0x2
//#define   IN_PUL_UP          0x8



// 1) abilito il clock per tutte le porte GPIO (A, B, C, D, E) una volta per tutte.
    RCC->APB2ENR |= 0x0000007D;
// 2) configuro i pin di tutte le porte

/*
    PORT A: pin 0 to 7
        - pin 0: lascia cosi' come e' inizializzato (input -floating)
        - pin 1: lascia cosi' come e' inizializzato (input -floating)
        - pin 2: GPOUT 2MHz 
        - pin 3: Analog input 
        - pin 4: Analog input
        - pin 5: GPOUT 2MHz   (valore basso)
        - pin 6: input PullUp
        - pin 7: GPOUT 2MHz    (valore basso)
*/
    GPIOA->CRL   &= 0x000000FF;  //reset
    GPIOA->CRL   |= 0x28288200;  //config
    GPIOA->BRR   |= ((1 << 5) |  (1 << 7)); //set val low


/*
    PORT A: pin 8 to 15
        - pin 8: GPOUT 2MHz
        - pin 9: GPOUT 2MHz (valore alto)
        - pin 10: input PullUp
        - pin 11: input PullUp
        - pin 12:  input PullUp
        - pin 13:  lascia cosi' come e' inizializzato  (Jtag nopn toccare)
        - pin 14:  lascia cosi' come e' inizializzato  (Jtag nopn toccare)
        - pin 15:  lascia cosi' come e' inizializzato   (Jtag nopn toccare)
*/
 GPIOA->CRH   &= 0xFFF00000;  //reset
 GPIOA->CRH   |= 0x00088822;   //config

 GPIOA->BSRR   |= ((1 << 9) ); //set val high




/*
    PORT B: pin 0 to 7
        - pin 0: input PullUp
        - pin 1: input PullUp
        - pin 2: GPOUT 2MHz   (valore basso)
        - pin 3: lascia cosi' come e' inizializzato   (Jtag nopn toccare)
        - pin 4: lascia cosi' come e' inizializzato   (Jtag nopn toccare)
        - pin 5: input PullUp
        - pin 6: GPOUT 2MHz    (valore alto)
        - pin 7: GPOUT 2MHz    (valore alto)
*/
    GPIOB->CRL   &= 0x00000000;  //reset
    GPIOB->CRL   |= 0x22800288;  //config
    GPIOB->BSRR   |= ((1 << 6) |  (1 << 7)); //set val high
    GPIOB->BRR   |= ((1 << 2) ); //set val low


/*
    PORT B pin 8 to 15
        - pin 8: lascia cosi' come e' inizializzato (perche' hanno la resistenza)
        - pin 9: lascia cosi' come e' inizializzato (perche' hanno la resistenza)
        - pin 10: lascia cosi' come e' inizializzato (perche' hanno la resistenza)
        - pin 11: GPOUT 2MHz    (valore basso)
        - pin 12: GPOUT 2MHz    (valore basso)
        - pin 13: GPOUT 2MHz    (valore basso)
        - pin 14:  GPOUT 2MHz    (valore alto)  (anche se hanno la loro resistenza)
        - pin 15: GPOUT 2MHz    (valore alto)   (anche se hanno la loro resistenza)
*/
    GPIOB->CRH   &= 0x00000FFF;  //reset
    GPIOB->CRH   |= 0x22222000;   //config
    
    GPIOB->BSRR   |= ((1 << 14) | (1 << 15) ); //set val high
    GPIOB->BRR   |= ((1 << 11) | (1 << 12) | (1 << 13)); //set val low






/*
    PORT C: pin 0 to 7
        - pin 0: lascia cosi' come e' inizializzato (input -floating)
        - pin 1: GPOUT 2MHz   (valore basso)
        - pin 2: Analog input
        - pin 3: Analog input 
        - pin 4: Analog input
        - pin 5: Analog input
        - pin 6: input PullUp
        - pin 7: input PullUp
*/
    GPIOC->CRL   &= 0x0000000F;  //reset
    GPIOC->CRL   |= 0x88000020;  //config
    GPIOC->BRR   |= ((1 << 1)); //set val low


/*
    PORT C: pin 8 to 15
        - pin 8: input PullUp
        - pin 9: input PullUp (perche' A8 e' output e se no fa cortocircuito)
        - pin 10: GPOUT 2MHz   (valore basso)
        - pin 11: input PullUp
        - pin 12:  GPOUT 2MHz   (valore basso)
        - pin 13:  GPOUT 2MHz  
        - pin 14:  GPOUT 2MHz
        - pin 15:  GPOUT 2MHz
*/
    GPIOC->CRH   &= 0x00000000;  //reset
    GPIOC->CRH   |= 0x22228288;   //config
    
    GPIOC->BSRR   |= ((1 << 10) | (1 << 12)  ); //set val low




/*
    PORT D: pin 0 to 7
        - pin 0: input PullUp
        - pin 1: GPOUT 2MHz   (valore alto)
        - pin 2: lascia cosi' come e' inizializzato (input -floating) (perche' ci sono i pull up)
        - pin 3: lascia cosi' come e' inizializzato (input -floating) (perche' ci sono i pull up)
        - pin 4: GPOUT 2MHz   (valore alto)
        - pin 5: lascia cosi' come e' inizializzato (input -floating) (perche' ci sono i pull up)
        - pin 6: lascia cosi' come e' inizializzato (input -floating) (perche' ci sono i pull up)
        - pin 7: GPOUT 2MHz   (valore alto)
*/
    GPIOD->CRL   &= 0x0FF0FF00;  //reset
    GPIOD->CRL   |= 0x20020028;  //config
    GPIOD->BSRR   |= ((1 << 1) |  (1 << 4) |  (1 << 7)); //set val low


/*
    PORT D: pin 8 to 15
        - pin 8: lascia cosi' come e' inizializzato (input -floating)
        - pin 9: lascia cosi' come e' inizializzato (input -floating)
        - pin 10: lascia cosi' come e' inizializzato (input -floating)
        - pin 11: GPOUT 2MHz   (valore alto)
        - pin 12: lascia cosi' come e' inizializzato (input -floating) (perche' ci sono i pull up)
        - pin 13: lascia cosi' come e' inizializzato (input -floating) (perche' ci sono i pull up)
        - pin 14: lascia cosi' come e' inizializzato (input -floating) (perche' ci sono i pull up)
        - pin 15: lascia cosi' come e' inizializzato (input -floating) (perche' ci sono i pull up)
*/
    GPIOD->CRH   &= 0xFFFF0FFF;  //reset
    GPIOD->CRH   |= 0x00002000;   //config
    
    GPIOD->BSRR   |= ((1 << 11) ); //set val high




/*
    PORT E: pin 0 to 7
        - pin 0: lascia cosi' come e' inizializzato (input -floating)
        - pin 1: GPOUT 2MHz (valore alto)
        - pin 2: lascia cosi' come e' inizializzato   (Jtag nopn toccare)
        - pin 3: lascia cosi' come e' inizializzato   (Jtag nopn toccare)
        - pin 4: lascia cosi' come e' inizializzato   (Jtag nopn toccare)
        - pin 5: lascia cosi' come e' inizializzato   (Jtag nopn toccare)
        - pin 6: lascia cosi' come e' inizializzato   (Jtag nopn toccare)
        - pin 7: input PullUp
*/
    GPIOE->CRL   &= 0x0FFFFF0F;  //reset
    GPIOE->CRL   |= 0x80000020;  //config
    GPIOE->BSRR   |= ((1 << 1) ); //set val high


/*
    PORT E: pin 8 to 15
        - pin 8: GPOUT 2MHz (valore alto)
        - pin 9: input PullUp
        - pin 10: GPOUT 2MHz (valore alto)
        - pin 11: input PullUp
        - pin 12: GPOUT 2MHz (valore alto)
        - pin 13: input PullUp
        - pin 14:  input PullUp
        - pin 15: GPOUT 2MHz (valore alto)
*/
    GPIOE->CRH   &= 0x00000000;  //reset
    GPIOE->CRH   |= 0x28828282;   //config
    
    GPIOE->BSRR   |= ((1 << 8) | (1 << 10) |  (1 << 12) |  (1 << 15) ); //set val high


#endif

}

#endif


// end of file






