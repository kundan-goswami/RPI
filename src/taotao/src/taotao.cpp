#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>


#include <iostream>
#include <thread>

#include "ros/ros.h"
#include "std_msgs/String.h"
#include <std_msgs/Empty.h>
#include <boost/thread/thread.hpp>

#include <sstream>

#include "bipropellant-hoverboard-api/src/HoverboardAPI.h"

#include "taotao/Mdu.h"
#include "motion_controller/Pwm.h"


//#define R_PI
using namespace std;
int sfd;

int serialWrapper(unsigned char *data, int len) {
 return write(sfd,data,len);
}

HoverboardAPI hoverboard = HoverboardAPI(serialWrapper);
// Variable for PWM
PROTOCOL_PWM_DATA PWMData;
PROTOCOL_BUZZER_DATA buzzer = {8, 0, 50};

int init_serial() {
    sfd = open("/dev/serial0", O_RDWR | O_NOCTTY);
     if (sfd == -1) {
        printf("Error no is : %d\n", errno);
        printf("Error description is : %s\n", strerror(errno));
        return (-1);
    }
    struct termios options;
    tcgetattr(sfd, &options);
    cfsetspeed(&options, B9600);
    cfmakeraw(&options);
    options.c_cflag &= ~CSTOPB;
    options.c_cflag |= CLOCAL;
    options.c_cflag |= CREAD;
    options.c_cc[VTIME]=0;
    options.c_cc[VMIN]=0;
    tcsetattr(sfd, TCSANOW, &options);
    return sfd;
}



void serial_read(int sfd)
{
    char rx_buf[20];
    char c;
    int count = 0;
    int i = 0;

    while(1) {
       count = read(sfd,&c,1);
       if (count!=0) {
            printf("%d %c\n",count , c);
            rx_buf[i++] = c;
            if (c == 0) {
                printf("%s",rx_buf);
                memset(rx_buf, 0, sizeof(rx_buf));
                i = 0;
                fflush(stdout);
            }
        }
    }
}

void tx_node(int sfd){
    char data[] = "Hello Mister";
    //printf("transmitter Thread started\n");
    int count = write(sfd, data, strlen(data)+1);
}
void write_data_hoverboard(const motion_controller::Pwm::ConstPtr&  msg)
{
  printf("I heard: [%f]", msg->pwm_l);
}

void rx_node(int* publish_rate) {
    char c;
    int count = 0;


    ros::NodeHandlePtr node = boost::make_shared<ros::NodeHandle>();

    //ros::Subscriber pwm_data = node->subscribe("pwm",100,write_data_hoverboard) ;
    //ros::Publisher mdu_pub = n.advertise<taotao::Mdu>("mdu", 100);
    ros::Rate loop_rate(*publish_rate);
    printf("receiver Thread started\n");
    fflush(stdout);
    while (ros::ok()) {
		// Print Speed on debug Serial.
		printf("Motor Speed: %lf km/h (average wheel1 and wheel2)\n",hoverboard.getSpeed_kmh());
		// Print battery Voltage on debug Serial.
		printf("Battery Voltage: %lf V\n", hoverboard.getBatteryVoltage());
		fflush(stdout);
#if defined(R_PI)
		count = read(sfd,&c,1);
    	if (count!=0) {
    		hoverboard.protocolPush(c);
    	}
#endif
        loop_rate.sleep();
        ros::spinOnce();
	}

}


int main(int argc , char ** argv) {
#if defined(R_PI)
	// initializer serial0
    sfd = init_serial();
    if (sfd == -1)
        return 1;

    int count = 0;
    char c;
    // Request Hall sensor data every 100ms
	hoverboard.scheduleRead(HoverboardAPI::Codes::sensHall, -1, 100);

	// Request Electrical Information every second
	hoverboard.scheduleRead(hoverboard.Codes::sensElectrical, -1, 1000);

	// Register Variable and send PWM values periodically
	hoverboard.updateParamVariable(HoverboardAPI::Codes::setPointPWM, &PWMData, sizeof(PWMData));
	hoverboard.scheduleTransmission(HoverboardAPI::Codes::setPointPWM, -1, 30);

	// Send PWM to 10% duty cycle for wheel1, 15% duty cycle for wheel2. Wheel2 is running backwards.
	PWMData.pwm[0] = 100.0;
	PWMData.pwm[1] = -150.0;

	// Register Variable and send Buzzer values periodically
	hoverboard.updateParamVariable(HoverboardAPI::Codes::setBuzzer, &buzzer, sizeof(buzzer));
	hoverboard.scheduleTransmission(HoverboardAPI::Codes::setBuzzer, -1, 200);

	// Set maxium PWM to 400, Minimum to -400 and threshold to 30. Require ACK (Message will be resend when not Acknowledged)
	hoverboard.sendPWMData(0, 0, 400, -400, 30, PROTOCOL_SOM_ACK);
#endif
	taotao::Mdu msg;
	// init ros node
	ros::init(argc, argv, "taotao");

	ros::NodeHandle n;

	ros::Subscriber pwm_data_sub = n.subscribe("pwm",100,write_data_hoverboard) ;
	ros::Publisher  mdu_pub      = n.advertise<taotao::Mdu>("mdu", 100);
    //ros::Publisher gyroscope_pub    = n.advertise<std_msgs::String>("gyroscope", 100);
    //ros::Publisher acclerometer_pub = n.advertise<std_msgs::String>("acclerometer", 1000);
    //ros::Publisher hall_sensor_pub  = n.advertise<std_msgs::String>("hall_sensor", 1000);
    //ros::Publisher electrical_pub   = n.advertise<std_msgs::String>("electrical", 1000);

    // spawn thread for receiving data
    //boost::thread rx_thread(rx_node, &rx_rate);

    //ros::NodeHandlePtr node = boost::make_shared<ros::NodeHandle>();

    // transmitter
    //ros::Publisher gyrosp = node->advertise<std_msgs::Empty>("gyroscope", 10);
    //pwm_data.publish(msg);
    // hoverboard.sendPWMData(0, 0, 400, -400, 30, PROTOCOL_SOM_ACK);
    ros::Rate loop_rate(1); // 1 Hz
    msg.battery_state.voltage = 10;
    msg.battery_state.current = 1;
	msg.battery_state.charge = 23;
	msg.battery_state.capacity = 45;
	msg.battery_state.design_capacity = 50;
	msg.battery_state.percentage = 20;
	msg.battery_state.power_supply_status = 10;
	msg.battery_state.power_supply_health = 30;
	msg.battery_state.power_supply_technology = 50;
	msg.battery_state.present = 1;

	msg.motor_l.current_ph_a = 30;
	msg.motor_l.current_ph_b = 40;
	msg.motor_l.current_ph_c = 12;
	msg.motor_l.voltage = 34;
	msg.motor_l.hall_sensor = 45;
	msg.motor_l.speed = 12;
	msg.motor_l.torque = 45;

	msg.motor_r.current_ph_a = 30;
	msg.motor_r.current_ph_b = 40;
	msg.motor_r.current_ph_c = 12;
	msg.motor_r.voltage = 34;
	msg.motor_r.hall_sensor = 45;
	msg.motor_r.speed = 12;
	msg.motor_r.torque = 45;

	msg.board.power_on_status = 1;
	msg.board.temprature = 45;

    while (ros::ok()) {
#if defined(PI)
		count = read(sfd,&c,1);
		if (count!=0) {
			hoverboard.protocolPush(c);
		}

        hoverboard.protocolTick();

        // Print Speed on debug Serial.
		printf("Motor Speed: %lf km/h (average wheel1 and wheel2)\n",hoverboard.getSpeed_kmh());
		// Print battery Voltage on debug Serial.
		printf("Battery Voltage: %lf V\n", hoverboard.getBatteryVoltage());
		fflush(stdout);
#endif
		mdu_pub.publish(msg);
		ros::spinOnce();

		loop_rate.sleep();
    }
    // wait the second thread to finish
    //rx_thread.join();

    //thread rx(rx_node, sfd);
    //thread tx(tx_node, sfd);

    // Wait for the threads to finish
    // Wait for thread t1 to finish
    //rx.join();
    // Wait for thread t2 to finish
    //tx.join();
    close(sfd);
    return 0;
}

