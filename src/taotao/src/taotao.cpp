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


using namespace std;

int init_serial();

int init_serial() {
    int sfd = open("/dev/serial0", O_RDWR | O_NOCTTY);
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


// A dummy function
void rx_node(int sfd)
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

void do_stuff(int* publish_rate)
{
  ros::NodeHandlePtr node = boost::make_shared<ros::NodeHandle>();
  ros::Publisher pub_b = node->advertise<std_msgs::Empty>("topic_b", 10);

  ros::Rate loop_rate(*publish_rate);
  while (ros::ok())
  {
    std_msgs::Empty msg;
    pub_b.publish(msg);
    loop_rate.sleep();
  }
}

int main(int argc , char ** argv) {
    // This thread is launched by using
    // function pointer as callable

    int sfd = init_serial();
    if (sfd == -1)
        return 1;
    int rate_b = 1; // 1 Hz
    ros::init(argc, argv, "taotao");
    // spawn another thread
    boost::thread thread_b(do_stuff, &rate_b);
    ros::NodeHandlePtr node = boost::make_shared<ros::NodeHandle>();
    ros::Publisher pub_a = node->advertise<std_msgs::Empty>("topic_a", 10);
    ros::Rate loop_rate(10); // 10 Hz
    while (ros::ok()) {
        std_msgs::Empty msg;
        pub_a.publish(msg);
        loop_rate.sleep();
        // process any incoming messages in this thread
        ros::spinOnce();
    }
  // wait the second thread to finish
  thread_b.join();

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

