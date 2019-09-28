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

void rx_node(int* publish_rate) {
    char rx_buf[20];
    char c;
    int count = 0;
    int i = 0;


    ros::NodeHandlePtr node = boost::make_shared<ros::NodeHandle>();
    ros::Publisher pub_b = node->advertise<std_msgs::Empty>("pwm", 10);

    ros::Rate loop_rate(*publish_rate);
    while (ros::ok()) {
    	std_msgs::Empty msg;
    	pub_b.publish(msg);
    	count = read(sfd,&c,1);

    	if (count!=0) {
    		//rx_buf[i++] = c;
    		hoverboard.protocolPush(c);
    		printf("%x",c);
    		fflush(stdout);
    		/*
			if (c == 0) {
				printf("%s",rx_buf);
				memset(rx_buf, 0, sizeof(rx_buf));
				i = 0;
				fflush(stdout);
		}*/
	}
    loop_rate.sleep();
  }
}

int main(int argc , char ** argv) {
	// initializer serial0
    sfd = init_serial();
    if (sfd == -1)
        return 1;

    int rx_rate = 1; // 1 Hz
    // init ros node
    ros::init(argc, argv, "taotao");

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


    // spawn thread for receiving data
    boost::thread rx_thread(rx_node, &rx_rate);

    ros::NodeHandlePtr node = boost::make_shared<ros::NodeHandle>();

    // transmitter
    ros::Publisher pwm_data = node->advertise<std_msgs::Empty>("pwm", 10);
    ros::Rate loop_rate(10); // 100 Hz
    while (ros::ok()) {
        std_msgs::Empty msg;

        pwm_data.publish(msg);
        hoverboard.sendPWMData(0, 0, 400, -400, 30, PROTOCOL_SOM_ACK);
        hoverboard.protocolTick();
        loop_rate.sleep();
        // process any incoming messages in this thread
        ros::spinOnce();
    }
    // wait the second thread to finish
    rx_thread.join();

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

