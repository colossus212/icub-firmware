
// - include guard ----------------------------------------------------------------------------------------------------
#ifndef _EOaxisController_H_
#define _EOaxisController_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @file       EOaxisController.h
    @brief      This header file implements public interface to a motor controller.
    @author     alessandro.scalzo@iit.it
    @date       27/03/2012
**/

/** @defgroup eo_axisController Object EOaxisController
    Does something.
     
    @{        
 **/


// - external dependencies --------------------------------------------------------------------------------------------

#include "EoCommon.h"
#include "EoMotionControl.h"
#include "EOpid.h"
#include "EOaxisController_hid.h"

#include "EOemsControllerCfg.h"

// - public #define  --------------------------------------------------------------------------------------------------

//#define MC_CAN_DEBUG

// - declaration of public user-defined types ------------------------------------------------------------------------- 

/*
typedef enum { 
    CM_IDLE                 = eomc_controlmode_idle,                // = 0x00  
    CM_POSITION             = eomc_controlmode_position,            // = 0x01
    CM_POS_VEL              = eomc_controlmode_velocity,            // = 0x02
    CM_TORQUE               = eomc_controlmode_torque,              // = 0x03
    CM_IMPEDANCE_POS        = eomc_controlmode_impedance_pos,       // = 0x04
    CM_IMPEDANCE_VEL        = eomc_controlmode_impedance_vel,       // = 0x05
    CM_CURRENT              = eomc_controlmode_current,
    CM_VELOCITY             = 0x07,

    //CM_CALIB_ABS_POS_SENS   = eomc_controlmode_calib_abs_pos_sens,   // = 0x10
    
    CM_OPENLOOP             = eomc_controlmode_openloop,             // = 0x50
    
    CM_FORCE_POSITION       = 101//,
    
    //CM_SWITCH_OFF           = eomc_controlmode_switch_everything_off // = 0xF0
} control_mode_t;
*/

/** @typedef    typedef struct EOaxisController_hid EOaxisController
    @brief      EOtrajectory is an opaque struct. It is used to implement data abstraction for the 
                object so that the user cannot see its private fields and he/she is forced to manipulate the
                object only with the proper public functions. 
 **/  
typedef struct EOaxisController_hid EOaxisController;


// - declaration of extern public variables, ... but better using use _get/_set instead -------------------------------
// empty-section


// - declaration of extern public functions ---------------------------------------------------------------------------

 
 
/** @fn         extern EOaxisController* eo_axisController_New(void)
    @brief      Creates a new motor controller object 
    @return     The pointer to the required object.
 **/
extern EOaxisController* eo_axisController_New(uint8_t);

//extern eObool_t eo_axisController_HasLimits(EOaxisController *o);

extern void    eo_axisController_SetAxisCalibrationZero (EOaxisController* o, int32_t calibration_zero);
extern int32_t eo_axisController_GetAxisCalibrationZero (EOaxisController* o);

extern void eo_axisController_SetEncPos(EOaxisController *o, int32_t pos); 
extern void eo_axisController_SetEncVel(EOaxisController *o, int32_t vel);
extern void eo_axisController_SetTorque(EOaxisController *o, int16_t trq);

//extern int32_t eo_axisController_GetVelocity(EOaxisController *o);

//extern void eo_axisController_GetPosRef(EOaxisController *o, int32_t *pos, int32_t *avg_vel);

extern eObool_t eo_axisController_SetPosRef(EOaxisController *o, int32_t pos, int32_t avg_vel);
extern eObool_t eo_axisController_SetVelRef(EOaxisController *o, int32_t vel, int32_t avg_acc);
extern int16_t eo_axisController_GetVelRef(EOaxisController *o);
extern eObool_t eo_axisController_SetTrqRef(EOaxisController *o, int32_t trq);
extern eObool_t eo_axisController_SetPosRaw(EOaxisController *o, int32_t pos);
extern eObool_t eo_axisController_SetOutput(EOaxisController *o, int16_t out);

extern float eo_axisController_PWM(EOaxisController *o, eObool_t *stiff);
extern float eo_axisController_FrictionCompensation(EOaxisController *o, float input_pwm, int32_t motor_velocity);

extern void eo_axisController_Stop(EOaxisController *o);

extern void eo_axisController_SetHardwareFault(EOaxisController *o);
extern eObool_t eo_axisController_IsTorqueSensorFault(EOaxisController* o);
extern eObool_t eo_axisController_IsHardwareFault(EOaxisController *o);
extern eObool_t eo_axisController_SetControlMode(EOaxisController *o, eOmc_controlmode_command_t cmc);
extern eOmc_controlmode_t eo_axisController_GetControlMode(EOaxisController *o);

extern eObool_t eo_axisController_SetInteractionMode(EOaxisController *o, eOmc_interactionmode_t mode);
//extern eOmc_interactionmode_t eo_axisController_GetInteractionMode(EOaxisController *o);

extern void eo_axisController_SetImpedance(EOaxisController *o, int32_t  stiffness, int32_t  damping, int32_t  offset);
extern void eo_axisController_GetImpedance(EOaxisController *o, int32_t *stiffness, int32_t *damping, int32_t *offset);

extern void eo_axisController_SetVelTimeout(EOaxisController *o, int32_t vel_timeout);
extern void eo_axisController_SetLimits(EOaxisController *o, int32_t pos_min, int32_t pos_max, int32_t vel_max);
extern void eo_axisController_SetPosMin(EOaxisController *o, int32_t pos_min);
extern void eo_axisController_SetPosMax(EOaxisController *o, int32_t pos_max);
extern void eo_axisController_SetVelMax(EOaxisController *o, int32_t vel_max);

extern void eo_axisController_GetActivePidStatus(EOaxisController *o, eOmc_joint_status_ofpid_t* pidStatus);
extern void eo_axisController_GetJointStatus(EOaxisController *o, eOmc_joint_status_t* jointStatus);
extern eObool_t eo_axisController_GetMotionDone(EOaxisController *o);

extern EOpid* eo_axisController_GetPosPidPtr(EOaxisController *o);
extern EOpid* eo_axisController_GetTrqPidPtr(EOaxisController *o);

extern void eo_axisController_SetBemf(EOaxisController *o, float Kbemf);
extern void eo_axisController_SetKtau(EOaxisController *o, float Ktau);
extern void eo_axisController_SetTcFilterType(EOaxisController *o, uint8_t filterType);
extern void eo_axisController_SetPosPid(EOaxisController *o, float K, float Kd, float Ki, float Imax, int32_t Ymax, int32_t Yoff, float stiction_up, float stiction_down);
extern void eo_axisController_SetTrqPid(EOaxisController *o, float K, float Kd, float Ki, float Imax, int32_t Ymax, int32_t Yoff, float Kff, float stiction_up, float stiction_down);

extern void eo_axisController_StartCalibration_type3(EOaxisController *o);
extern void eo_axisController_StartCalibration_type0(EOaxisController *o, int16_t pwmlimit, int16_t vel);
extern void eo_axisController_SetCalibrated(EOaxisController *o);
extern eObool_t eo_axisController_IsCalibrated(EOaxisController *o);

extern int32_t eo_axisController_EncRangeAdj(EOaxisController *o, int32_t *enc_pos); // shoulder joint 2 only

/** @}            
    end of group eo_axisController  
 **/

#ifdef __cplusplus
}       // closing brace for extern "C"
#endif 

#endif  // include-guard


// - end-of-file (leave a blank line after)----------------------------------------------------------------------------

