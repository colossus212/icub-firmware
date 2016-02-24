#ifndef MC_CONTROLLER___
#define MC_CONTROLLER___

#include "EoCommon.h"

#include "EOemsControllerCfg.h"

#include "Joint.h"
#include "Motor.h"
#include "AbsEncoder.h"
#include "Pid.h"
#include "JointSet.h"

typedef struct //MController
{
    uint8_t nJoints;
    uint8_t nSets;
    
    JointSet *jointSet;
    
    uint8_t* set_dim;
    
    uint8_t** jos;
    uint8_t** mos;
    uint8_t** eos;
    
    uint8_t *j2s;
    uint8_t *m2s;
    uint8_t *e2s;
    
    Motor *motor;
    Joint *joint;
    
    float **Jjm;
    float **Jmj;
    float **Jje;
    
    uint8_t part_type;
    uint8_t actuation_type;
    
    AbsEncoder **absEncoder;
} MController;

extern MController* Controller_new(uint8_t nJoints); //

extern void Controller_init(void); //

extern void Controller_config_board(uint8_t board_type, uint8_t actuation_type);

extern void Controller_config_joint(int j, eOmc_joint_config_t* config); //
extern void Controller_config_motor(int m, uint8_t hardware_type, uint8_t motor_control_type, eOmc_motor_config_t* config); //
extern void Controller_config_absEncoder(uint8_t j, int32_t resolution, int16_t spike_limit); //
extern void Controller_config_Jjm(float **Jjm); //
extern void Controller_config_Jje(float **Jje); //

extern void Controller_update_joint_torque_fbk(uint8_t j, CTRL_UNITS trq_fbk); //
extern void Controller_update_absEncoder_fbk(uint8_t e, int32_t position); //
extern void Controller_invalid_absEncoder_fbk(uint8_t e);
extern void Controller_timeout_absEncoder_fbk(uint8_t e);

extern int32_t Controller_get_absEncoder(uint8_t j); //

extern void Controller_do(void); //

extern BOOL Controller_set_control_mode(uint8_t j, uint8_t control_mode);
extern void Controller_set_interaction_mode(uint8_t j, uint8_t interaction_mode);

#if 0
/** @typedef    typedef struct eOmc_calibrator32_t
    @brief      eOmc_calibrator32_t specifies a calibrator with type and parameters for teh new definition of measures
 **/
typedef struct                  // size is 1+3+4*4 = 20
{
    eOenum08_t                  type;                               /**< use eOmc_calibration_type_t */
    uint8_t                     filler03[3];
    union
    {
        uint32_t                                                any[6];
        eOmc_calibrator_params_type0_hard_stops_t               type0;
        eOmc_calibrator_params_type1_abs_sens_analog_t          type1;
        eOmc_calibrator_params_type2_hard_stops_diff_t          type2;
        eOmc_calibrator_params_type3_abs_sens_digital_t         type3;
        eOmc_calibrator_params_type4_abs_and_incremental_t      type4;
        eOmc_calibrator_params_type5_hard_stops_mc4plus_t       type5;
        eOmc_calibrator_params_type6_mais_t                     type6;
        eOmc_calibrator_params_type7_hall_sensor_t              type7;
        eOmc_calibration_type8_adc_and_incr_mc4plus_t           type8;
    } params;                                                       /**< the params of the calibrator */   
} eOmc_calibrator32_t;           EO_VERIFYsizeof(eOmc_calibrator32_t, 28);

typedef eOmc_calibrator32_t eOmc_calibrator_t;
#endif

extern void Controller_calibrate_encoder(uint8_t e, eOmc_calibrator_t *calibrator);

#endif
