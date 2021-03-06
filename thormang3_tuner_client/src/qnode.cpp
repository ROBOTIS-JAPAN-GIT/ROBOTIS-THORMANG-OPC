/*******************************************************************************
* Copyright 2017 ROBOTIS CO., LTD.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/* Author: Kayman Jung */

/*****************************************************************************
 ** Includes
 *****************************************************************************/

#include <ros/ros.h>
#include <ros/network.h>
#include <string>
#include <std_msgs/String.h>
#include <sstream>
#include "../include/thormang3_tuner_client/qnode.hpp"

/*****************************************************************************
 ** Namespaces
 *****************************************************************************/

namespace thormang3_tuner_client
{

/*****************************************************************************
 ** Implementation
 *****************************************************************************/

QNode::QNode(int argc, char** argv)
    : init_argc_(argc),
      init_argv_(argv),
      is_refresh_(false),
      ft_right_(0.0),
      ft_left_(0.0)
{

}

QNode::~QNode()
{
  if (ros::isStarted())
  {
    ros::shutdown();  // explicitly needed since we use ros::start();
    ros::waitForShutdown();
  }
  wait();
}

bool QNode::init()
{
  ros::init(init_argc_, init_argv_, "thormang3_tuner_client");

  ros::start();  // explicitly needed since our nodehandle is going out of scope.
  ros::NodeHandle ros_node;

  // Add your ros communications here
  joint_offset_data_pub_ = ros_node.advertise<thormang3_tuning_module_msgs::JointOffsetData>(
      "/robotis/tuning_module/joint_offset_data", 0);
  joint_gain_data_pub_ = ros_node.advertise<thormang3_tuning_module_msgs::JointOffsetData>(
      "/robotis/tuning_module/joint_gain_data", 0);
  torque_enable_pub_ = ros_node.advertise<thormang3_tuning_module_msgs::JointTorqueOnOffArray>(
      "/robotis/tuning_module/torque_enable", 0);
  command_pub_ = ros_node.advertise<std_msgs::String>("/robotis/tuning_module/command", 0);
  tuning_pose_pub_ = ros_node.advertise<std_msgs::String>("/robotis/tuning_module/tuning_pose", 0);
  present_joint_offset_data_sub_ = ros_node.subscribe("/robotis/tuning_module/present_joints_data", 1, &QNode::presentJointOffsetDataCallback, this);
  imu_sub_ = ros_node.subscribe("/robotis/sensor/imu/imu", 1, &QNode::imuCallback, this);
  ft_right_sub_ = ros_node.subscribe("/robotis/sensor/ft_right_foot/scaled", 1, &QNode::ftRightCallback, this);
  ft_left_sub_ = ros_node.subscribe("/robotis/sensor/ft_left_foot/scaled", 1, &QNode::ftLeftCallback, this);

  get_present_joint_offset_data_client_ = ros_node.serviceClient<thormang3_tuning_module_msgs::GetPresentJointOffsetData>(
      "/robotis/tuning_module/get_present_joint_offset_data");

  std::string default_config_path = ros::package::getPath(ROS_PACKAGE_NAME) + "/config/tm3_joint_data.yaml";
  parseOffsetGroup(default_config_path);

  start();
  return true;
}

void QNode::run()
{

  ros::Rate loop_rate(125);

  while (ros::ok())
  {
    ros::spinOnce();
    loop_rate.sleep();
  }

  std::cout << "Ros shutdown, proceeding to close the gui." << std::endl;
  Q_EMIT rosShutdown();  // used to signal the gui for a shutdown (useful to roslaunch)
}

void QNode::sendTorqueEnableMsg(thormang3_tuning_module_msgs::JointTorqueOnOffArray msg)
{
  torque_enable_pub_.publish(msg);

  log(Info, "Joint Torque On/Off");
}

void QNode::sendJointOffsetDataMsg(thormang3_tuning_module_msgs::JointOffsetData msg)
{
  joint_offset_data_pub_.publish(msg);

  log(Info, msg.joint_name + " : Send Joint Offset Data");
}

void QNode::sendJointGainDataMsg(thormang3_tuning_module_msgs::JointOffsetData msg)
{
  joint_gain_data_pub_.publish(msg);

  log(Info, msg.joint_name + " : Send Joint Gain Data");
}

void QNode::sendCommandMsg(std_msgs::String msg)
{
  command_pub_.publish(msg);

  std::stringstream log_msg;
  log_msg << "Send Command : " << msg.data;

  log(Info, log_msg.str());
}

void QNode::sendTuningPoseMsg(std_msgs::String msg)
{
  tuning_pose_pub_.publish(msg);

  std::stringstream log_msg;
  log_msg << "Send Tuning Pose : " << msg.data;

  log(Info, log_msg.str());
}

void QNode::getPresentJointOffsetData(bool recalculate_offset)
{
  is_refresh_ = true;

  thormang3_tuning_module_msgs::GetPresentJointOffsetData _get_present_joint_offset_data;

  //request

  //response
  if (get_present_joint_offset_data_client_.call(_get_present_joint_offset_data))
  {
    for (int id = 0; id < _get_present_joint_offset_data.response.present_data_array.size(); id++)
    {
      thormang3_tuning_module_msgs::JointOffsetPositionData _temp =
          _get_present_joint_offset_data.response.present_data_array[id];

//      if(recalculate_offset == true)
//        _temp.offset_value = _temp.present_value - _temp.goal_value;

      Q_EMIT updatePresentJointOffsetData(_temp);
    }
  }
  else
    log(Error, "Fail to get joint offset data");

  is_refresh_ = false;
}

void QNode::presentJointOffsetDataCallback(const thormang3_tuning_module_msgs::JointsOffsetPositionData::ConstPtr &msg)
{
  is_refresh_ = true;

  for (int id = 0; id < msg->present_data_array.size(); id++)
  {
    thormang3_tuning_module_msgs::JointOffsetPositionData _temp =
        msg->present_data_array[id];

    Q_EMIT updatePresentJointOffsetData(_temp);
  }

  is_refresh_ = false;
}

void QNode::imuCallback(const sensor_msgs::Imu::ConstPtr &msg)
{
  Eigen::Quaterniond eigen_quaternion;
  tf::quaternionMsgToEigen(msg->orientation, eigen_quaternion);

  Eigen::Vector3d rpy = robotis_framework::convertQuaternionToRPY(eigen_quaternion);
  double roll = rpy[0] * 180.0 / M_PI;
  if(roll < -90)
    roll += 180;
  if(roll > 90)
    roll -= 180;
  double pitch = rpy[1] * 180.0 / M_PI;
  Q_EMIT updateIMU(roll, pitch);
}

void QNode::ftRightCallback(const geometry_msgs::WrenchStamped::ConstPtr &msg)
{
  double ratio = 0.1;
  ft_right_ = (ft_right_ == 0.0) ? msg->wrench.force.z : ft_right_ * (1 - ratio) + msg->wrench.force.z * ratio;
  Q_EMIT updateFT(true, ft_right_);
}

void QNode::ftLeftCallback(const geometry_msgs::WrenchStamped::ConstPtr &msg)
{
  double ratio = 0.1;
  ft_left_ = (ft_left_ == 0.0) ? msg->wrench.force.z : ft_left_ * (1 - ratio) + msg->wrench.force.z * ratio;
  Q_EMIT updateFT(false, ft_left_);
}

void QNode::log(const LogLevel &level, const std::string &msg)
{
  logging_model_.insertRows(logging_model_.rowCount(), 1);
  std::stringstream logging_model_msg;
  switch (level)
  {
    case (Debug):
    {
      ROS_DEBUG_STREAM(msg);
      logging_model_msg << "[DEBUG] [" << ros::Time::now() << "]: " << msg;
      break;
    }
    case (Info):
    {
      ROS_INFO_STREAM(msg);
      logging_model_msg << "[INFO] [" << ros::Time::now() << "]: " << msg;
      break;
    }
    case (Warn):
    {
      ROS_WARN_STREAM(msg);
      logging_model_msg << "[INFO] [" << ros::Time::now() << "]: " << msg;
      break;
    }
    case (Error):
    {
      ROS_ERROR_STREAM(msg);
      logging_model_msg << "[ERROR] [" << ros::Time::now() << "]: " << msg;
      break;
    }
    case (Fatal):
    {
      ROS_FATAL_STREAM(msg);
      logging_model_msg << "[FATAL] [" << ros::Time::now() << "]: " << msg;
      break;
    }
  }

  QVariant new_row(QString(logging_model_msg.str().c_str()));
  logging_model_.setData(logging_model_.index(logging_model_.rowCount() - 1), new_row);
  Q_EMIT loggingUpdated();  // used to readjust the scrollbar
}

void QNode::clearLog()
{
  if (logging_model_.rowCount() == 0)
    return;

  logging_model_.removeRows(0, logging_model_.rowCount());
}

void QNode::parseOffsetGroup(const std::string &path)
{
  YAML::Node doc;
  try
  {
    // load yaml
    doc = YAML::LoadFile(path.c_str());
  } catch (const std::exception& e)
  {
    ROS_ERROR("Fail to load offset config yaml.");
    return;
  }

  // parse right_arm
  YAML::Node right_arm_node = doc["right_arm"];
  for (YAML::iterator yaml_it = right_arm_node.begin(); yaml_it != right_arm_node.end(); ++yaml_it)
  {
    int index;
    std::string joint_name;

    index = yaml_it->first.as<int>();
    joint_name = yaml_it->second.as<std::string>();

    right_arm_offset_group_[index] = joint_name;
  }

  YAML::Node left_arm_node = doc["left_arm"];
  for (YAML::iterator yaml_it = left_arm_node.begin(); yaml_it != left_arm_node.end(); ++yaml_it)
  {
    int index;
    std::string joint_name;

    index = yaml_it->first.as<int>();
    joint_name = yaml_it->second.as<std::string>();

    left_arm_offset_group_[index] = joint_name;
  }

  YAML::Node legs_node = doc["legs"];
  for (YAML::iterator yaml_it = legs_node.begin(); yaml_it != legs_node.end(); ++yaml_it)
  {
    int index;
    std::string joint_name;

    index = yaml_it->first.as<int>();
    joint_name = yaml_it->second.as<std::string>();

    legs_offset_group_[index] = joint_name;
  }

  YAML::Node body_node = doc["body"];
  for (YAML::iterator yaml_it = body_node.begin(); yaml_it != body_node.end(); ++yaml_it)
  {
    int index;
    std::string joint_name;

    index = yaml_it->first.as<int>();
    joint_name = yaml_it->second.as<std::string>();

    body_offset_group_[index] = joint_name;
  }
}

}  // namespace thormang3_tuner_client
