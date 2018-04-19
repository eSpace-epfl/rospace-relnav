
// Use the image_transport classes instead.
#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <rospace_msgs/PoseVelocityStamped.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

// Short alias for this namespace
namespace pt = boost::property_tree;

void pose_callback(const rospace_msgs::PoseVelocityStamped::ConstPtr& msg)
{
	pt::ptree root;
	std::cout << msg->pose.orientation << std::endl;

	//add pose node
	pt::ptree pose_node;
	
	pt::ptree pose_x_node;		
	pose_x_node.put<float>("", msg->pose.orientation.x);
	pose_node.push_back(std::make_pair("",pose_x_node));

	pt::ptree pose_y_node;		
	pose_y_node.put<float>("", msg->pose.orientation.y);
	pose_node.push_back(std::make_pair("",pose_y_node));

	pt::ptree pose_z_node;		
	pose_z_node.put<float>("", msg->pose.orientation.z);
	pose_node.push_back(std::make_pair("",pose_z_node));

	pt::ptree pose_w_node;		
	pose_w_node.put<float>("", msg->pose.orientation.w);
	pose_node.push_back(std::make_pair("",pose_w_node));

	root.add_child("sat_pose", pose_node);


	pt::ptree pos_node;
	
	pt::ptree pos_x_node;		
	pos_x_node.put<float>("", 0);
	pos_node.push_back(std::make_pair("",pos_x_node));

	pt::ptree pos_y_node;		
	pos_y_node.put<float>("", 0);
	pos_node.push_back(std::make_pair("",pos_y_node));

	pt::ptree pos_z_node;		
	pos_z_node.put<float>("", -3000);
	pos_node.push_back(std::make_pair("",pos_z_node));

	

	root.add_child("sat_position", pos_node);


	pt::write_json("/tmp/test_ipc", root);

}


int main(int argc, char **argv)
{
  ros::init(argc,argv,"ros_adapter");

ros::NodeHandle nh;
image_transport::ImageTransport it(nh);
image_transport::Publisher pub = it.advertise("nav_cam", 1);

ros::Subscriber sub = nh.subscribe("pose", 1, pose_callback);



  ros::Rate loop_rate(5);
 while (ros::ok())
  {cv_bridge::CvImage cv_image;
    cv_image.image = cv::imread("/tmp/test.png",CV_LOAD_IMAGE_COLOR);

    if(cv_image.image.size().width == 0)
	{
	std::cout << "INVALID IMAGE" << std::endl;
	continue;
	}	
    cv_image.encoding = "bgr8";
    sensor_msgs::Image ros_image;
    cv_image.toImageMsg(ros_image);
pub.publish(ros_image);
ros::spinOnce();
 loop_rate.sleep();
}




}
