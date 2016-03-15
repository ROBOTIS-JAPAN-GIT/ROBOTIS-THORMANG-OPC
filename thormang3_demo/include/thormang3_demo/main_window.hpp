/**
 * @file /include/thor3_control/main_window.hpp
 *
 * @brief Qt based gui for thor3_control.
 *
 * @date November 2010
 **/
#ifndef thor3_control_MAIN_WINDOW_H
#define thor3_control_MAIN_WINDOW_H

/*****************************************************************************
** Includes
*****************************************************************************/

#include <QtGui/QMainWindow>
#include "ui_main_window.h"
#include "qnode.hpp"

/*****************************************************************************
** Namespace
*****************************************************************************/

namespace thor3_control {

/*****************************************************************************
** Interface [MainWindow]
*****************************************************************************/
/**
 * @brief Qt central, all operations relating to the view part here.
 */
class MainWindow : public QMainWindow {
        Q_OBJECT

    public:
        MainWindow(int argc, char** argv, QWidget *parent = 0);
        ~MainWindow();

        void ReadSettings(); // Load up qt program settings at startup
        void WriteSettings(); // Save qt program settings when closing

        void closeEvent(QCloseEvent *event); // Overloaded function
        void showNoMasterMessage();

    public Q_SLOTS:
        /******************************************
    ** Auto-connections (connectSlotsByName())
    *******************************************/
        void on_actionAbout_triggered();
        void on_button_assemble_lidar_clicked(bool check );
        void on_button_clear_log_clicked(bool check);

        void on_button_init_pose_clicked(bool check);
        void on_button_ft_air_clicked(bool check);
        void on_button_ft_gnd_clicked(bool check);
        void on_button_ft_calc_clicked(bool check);
        void on_button_ft_save_clicked(bool check);

        // Manipulation
        void on_inipose_button_clicked( bool check );
        void on_currjoint_button_clicked( bool check );
        void on_desjoint_button_clicked( bool check );
        void on_currpos_button_clicked( bool check );
        void on_despos_button_clicked( bool check );
        void on_button_grip_on_clicked(bool check);
        void on_button_grip_off_clicked(bool check);
        void on_button_pathplanning_ini_clicked(bool check);
        void on_button_pathplanning_a_clicked(bool check);
        void on_button_pathplanning_b_clicked(bool check);

        // Walking
        void on_A0_button_fl_clicked(bool check);
        void on_A1_button_f_clicked(bool check);
        void on_A2_button_fr_clicked(bool check);

        void on_B0_button_l_clicked(bool check);
        void on_B1_button_stop_clicked(bool check);
        void on_B2_button_r_clicked(bool check);

        void on_C0_button_bl_clicked(bool check);
        void on_C1_button_b_clicked(bool check);
        void on_C2_button_br_clicked(bool check);

        void on_button_balance_on_clicked(bool check);
        void on_button_balance_off_clicked(bool check);

        // Head Control
        void on_head_center_button_clicked(bool check);

        /******************************************
    ** Manual connections
    *******************************************/
        void updateLoggingView(); // no idea why this can't connect automatically
        void setMode(bool check);
        void updateCurrentJointMode(std::vector<int> mode);
        void setPreset(QString preset_name);
        void updateHeadAngles(double pan, double tilt);

        // Manipulation
        void updateCurrJointSpinbox( double value );
        void updateCurrPosSpinbox( double x , double y , double z  );
        void updateCurrOriSpinbox( double x , double y , double z , double w );

    private:
        Ui::MainWindowDesign ui;
        QNodeThor3 qnode_thor3;
        bool DEBUG;
        void setUserShortcut();
        void initModeUnit();
        bool is_updating_;
        std::map< std::string, QList<QWidget *> > module_ui_table_;
        void updateModuleUI();
        void setHeadAngle(double pan, double tilt);
        void sendWalkingCommand(const std::string &command);

        void sendDemoMsg(const std::string &demo_command);

        /******************************************
        ** Transformation
        *******************************************/
        Eigen::MatrixXd rx( double s );
        Eigen::MatrixXd ry( double s );
        Eigen::MatrixXd rz( double s );
        Eigen::MatrixXd rpy2rotation(double r, double p, double y);
        Eigen::Quaterniond rpy2quaternion(double r, double p, double y);

    protected Q_SLOTS:
        void setHeadAngle();
};

}  // namespace thor3_control

#endif // thor3_control_MAIN_WINDOW_H