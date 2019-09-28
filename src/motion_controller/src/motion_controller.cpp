#include "ros/ros.h"
#include "std_msgs/String.h"

#include "motion_controller/Pwm.h"
#include "taotao/Mdu.h"

#include <sstream>

void read_data_hoverboard(const taotao::Mdu::ConstPtr&  msg) {
  printf("I heard: [%f]", msg->motor_l.voltage);
}


int main(int argc, char **argv) {
	motion_controller::Pwm msg;
	ros::init(argc, argv, "motion_controller");

	ros::NodeHandle n;
	ros::Publisher pwm_pub = n.advertise<motion_controller::Pwm>("pwm", 100);

	ros::Subscriber pwm_data_sub    = n.subscribe("mdu",100,read_data_hoverboard);

	ros::Rate loop_rate(1);
	while (ros::ok()) {
		msg.pwm_l = 100.0;
		msg.pwm_r = 150.0;

		pwm_pub.publish(msg);

		ros::spinOnce();

		loop_rate.sleep();
  }

  return 0;
}

