#!/usr/bin/env python

import rospy
from motion_controller.msg import Pwm
from std_msgs.msg import String
from geometry_msgs.msg import Twist
from taotao.msg import Mdu

def callback(data):
	rospy.loginfo(rospy.get_caller_id() + "I heard %s", data)

def talker():
	data = Twist()
	rospy.init_node('motion_controller_py', anonymous=True)
	pub = rospy.Publisher('cmd_vel', Twist, queue_size=10)

	rospy.Subscriber("mdu", Mdu, callback)

	rate = rospy.Rate(1) # 10hz
	data.linear.x = 10;
	while not rospy.is_shutdown():
		hello_str = "hello world %s" % rospy.get_time()
		rospy.loginfo(hello_str)
		pub.publish(data)
		rate.sleep()

if __name__ == '__main__':
    try:
        talker()
    except rospy.ROSInterruptException:
        pass