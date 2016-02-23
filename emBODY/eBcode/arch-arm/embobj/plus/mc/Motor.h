/////////////////////////////////////////////////////////
// Motor

#ifndef MC_MOTOR_H___
#define MC_MOTOR_H___

#include "EoCommon.h"
#include "EoMotionControl.h"
#include "EOtheCANprotocol.h"
#include "EOemsControllerCfg.h"

#include "WatchDog.h"
#include "Pid.h"

typedef struct //CanOdometry2FocMsg
{
    int16_t current;
    int16_t velocity;
    int32_t position;
} CanOdometry2FocMsg; 

typedef struct //CanState2FocMsg
{
    uint8_t control_mode;
    
    union
    {
        struct
        {
            unsigned dirty           :1;
            unsigned stuck           :1;
            unsigned index_broken    :1;
            unsigned phase_broken    :1;
        
            unsigned not_calibrated  :1;
            unsigned unused          :3;
        } bits;
        uint8_t bitmask;
    } qe_state;
    
    uint16_t pwm_fbk;
    
    union
    {
        struct
        {
            //B0 L
            unsigned UnderVoltageFailure    :1;      
            unsigned OverVoltageFailure     :1;
            unsigned ExternalFaultAsserted  :1;
            unsigned OverCurrentFailure     :1;
            //B0 H
            unsigned DHESInvalidValue       :1;
            unsigned AS5045CSumError        :1;
            unsigned DHESInvalidSequence    :1;
            unsigned CANInvalidProtocol     :1;
            //B1 L
            unsigned CAN_BufferOverRun      :1;
            unsigned SetpointExpired        :1;
            unsigned CAN_TXIsPasv           :1;
            unsigned CAN_RXIsPasv           :1;
            //B1 H
            unsigned CAN_IsWarnTX           :1;
            unsigned CAN_IsWarnRX           :1;
            unsigned unused                 :1;
            unsigned OverHeating            :1;
            //B2 L
            unsigned ADCCalFailure          :1; 
            unsigned I2TFailure             :1;                     
            unsigned EMUROMFault            :1;
            unsigned EMUROMCRCFault         :1;
            //B2 H
            unsigned EncoderFault           :1;
            unsigned FirmwareSPITimingError :1;		
            unsigned AS5045CalcError        :1;
            unsigned FirmwarePWMFatalError  :1;
            //B3 L
            unsigned CAN_TXWasPasv          :1;
            unsigned CAN_RXWasPasv          :1;
            unsigned CAN_RTRFlagActive      :1;
            unsigned CAN_WasWarn            :1;
            //B3 H
            unsigned CAN_DLCError           :1;
            unsigned SiliconRevisionFault   :1;
            unsigned PositionLimitUpper     :1; 
            unsigned PositionLimitLower     :1; 
        } bits;
        
        uint32_t bitmask;
        
    } fault_state;
    
} CanState2FocMsg;

typedef struct //Motor
{
    // consts
    uint8_t ID;

    // UNKNOWN              0
    // HARDWARE_2FOC        1
    // HARDWARE_MC4p        2
    uint8_t HARDWARE_TYPE;

    // UNKNOWN              0
    // PWM_CONTROLLED_MOTOR 1
    // VEL_CONTROLLED_MOTOR 2
    // IQQ_CONTROLLED_MOTOR 3
    // POS_CONTROLLED_MOTOR 4
    uint8_t MOTOR_CONTROL_TYPE;

    int16_t GEARBOX;

    BOOL HAS_TEMP_SENSOR;
    int16_t temperature_max;
    int16_t temperature_fbk;

    int32_t pwm_max;
    int32_t pwm_fbk;
    int32_t pwm_ref;
    
    int32_t pos_raw_fbk;
    int32_t vel_raw_fbk;
    
    int32_t pos_calib_offset;
    
    int32_t pos_min;
    int32_t pos_max;
    int32_t pos_ref;
    int32_t pos_fbk;
    int32_t pos_err;
    
    int32_t vel_max;
    int32_t vel_ref;
    int32_t vel_fbk;
    int32_t vel_err;
    
    int32_t Iqq_max;
    int32_t Iqq_ref;
    int32_t Iqq_fbk;
    int32_t Iqq_err;
    
    CTRL_UNITS trq_max;
    CTRL_UNITS trq_ref;
    CTRL_UNITS trq_fbk;
    CTRL_UNITS trq_err;
    
    int16_t output;
    
    PID trqPID;
    
    BOOL not_calibrated;
    
    uint32_t fault_state_mask;
    uint8_t  qe_state_mask;
    uint8_t  control_mode;
    uint8_t  control_mode_req;
    WatchDog control_mode_req_wdog;

    // 2FOC specific data
    
    WatchDog can_2FOC_alive_wdog;
    //CanState2FocMsg state_2FOC_fbk;
    
    // MC4p specific data

} Motor;

extern Motor* Motor_new(uint8_t n);  //
extern void Motor_init(Motor* o);  //
extern void Motor_config(Motor* o, uint8_t ID, uint8_t HARDWARE_TYPE, uint8_t MOTOR_CONTROL_TYPE, eOmc_motor_config_t* config); //
extern void Motor_destroy(Motor* o); //

extern void Motor_config_trqPID(Motor* o, eOmc_PID_t* pid); //
extern void Motor_config_filter(Motor* o, uint8_t filter); //
extern void Motor_config_friction(Motor* o, float Bemf, float Ktau); //
extern void Motor_config_pos_offset(Motor* o, int32_t offset); //

extern void Motor_set_run(Motor* o); //
extern void Motor_set_idle(Motor* o); //
extern void Motor_force_idle(Motor* o); //

extern void Motor_motion_reset(Motor *o); //
extern BOOL Motor_is_calibrated(Motor* o); //
extern BOOL Motor_check_faults(Motor* o); //
extern void Motor_clear_faults(Motor* o); //

extern CTRL_UNITS Motor_do_trq_control(Motor* o, CTRL_UNITS trq_ref, CTRL_UNITS trq_fbk); //
extern void Motor_update_state_fbk_can(Motor* o, CanState2FocMsg* can_state_msg); //
extern void Motor_update_odometry_fbk_can(Motor* o, CanOdometry2FocMsg* data); //

extern void Motor_actuate(Motor* o, uint8_t N); //

extern void Motor_set_pwm_ref(Motor* o, int32_t pwm_ref);
extern void Motor_set_Iqq_ref(Motor* o, int32_t Iqq_ref);
//extern void Motor_set_pos_ref(Motor* o, int32_t vel_ref);
extern void Motor_set_vel_ref(Motor* o, int32_t vel_ref);
//extern void Motor_set_trq_ref(Motor* o, CTRL_UNITS trq_ref);

/*
extern void Motor_update_temperature_fbk(Motor* o, int16_t temperature_fbk);
extern void Motor_update_pos_raw_fbk(Motor* o, int32_t pos_raw_fbk);
extern void Motor_update_vel_raw_fbk(Motor* o, int32_t vel_raw_fbk);
extern void Motor_update_pwm_fbk(Motor* o, int16_t pwm_fbk);
extern void Motor_update_Iqq_fbk(Motor* o, int16_t Iqq_fbk);
extern void Motor_update_pos_fbk(Motor* o, int32_t pos_fbk);
extern void Motor_update_vel_fbk(Motor* o, int32_t vel_fbk);
extern void Motor_update_trq_fbk(Motor* o, CTRL_UNITS trq_fbk);

extern int16_t Motor_get_temperature_fbk(Motor* o);
extern int32_t Motor_get_pos_raw_fbk(Motor* o);
extern int32_t Motor_get_vel_raw_fbk(Motor* o);
extern int16_t Motor_get_pwm_fbk(Motor* o);
extern int16_t Motor_get_Iqq_fbk(Motor* o);
extern int32_t Motor_get_pos_fbk(Motor* o);
extern int32_t Motor_get_vel_fbk(Motor* o);
extern CTRL_UNITS Motor_get_trq_fbk(Motor* o);
*/

#endif

// Motor
/////////////////////////////////////////////////////////
