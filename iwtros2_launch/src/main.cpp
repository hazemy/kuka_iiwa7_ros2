#include <iwtros2_launch/gripper_controller.hpp>
#include <iwtros2_launch/iiwa_manipulation.hpp>

using GripperCommand = control_msgs::action::GripperCommand;
using ClientGoalHandle = rclcpp_action::ClientGoalHandle<GripperCommand>;

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::NodeOptions options;
    options.automatically_declare_parameters_from_overrides(true);
    auto node = rclcpp::Node::make_shared("iiwa_motion_controller_node", options);
    auto node_g = rclcpp::Node::make_shared("plc_control_sub_pub_node", options);

    auto executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
    auto executor_g = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
    executor->add_node(node);
    std::thread([&executor]() { executor->spin(); }).detach();
    executor_g->add_node(node_g);

    // Setup Move group planner
    auto group = std::make_shared<moveit::planning_interface::MoveGroupInterface>(node, "iiwa_arm");
    group->setPlanningPipelineId("pilz");
    group->setPlannerId("PTP");
    group->setMaxVelocityScalingFactor(0.1);
    group->setMaxAccelerationScalingFactor(0.2);
    group->setPoseReferenceFrame("iiwa7_link_0");
    group->setEndEffector("iiwa7_link_7");
    group->allowReplanning(true);

    auto iiwa_move = std::make_shared<iwtros2::IiwaMove>(node, group);
    auto plc_contl = std::make_shared<iwtros2::ControlPLC>(node_g);

    geometry_msgs::msg::PoseStamped table_pose_0_place = 
        iiwa_move->generatePose(-0.141, 0.585, 1.265, M_PI, 0, 3 * M_PI / 4, "iiwa7_link_0");
    geometry_msgs::msg::PoseStamped table_pose_1_place = 
        iiwa_move->generatePose(0.0530, 0.585, 1.265, M_PI, 0, 3 * M_PI / 4, "iiwa7_link_0");
    geometry_msgs::msg::PoseStamped table_pose_2_place = 
        iiwa_move->generatePose(0.0515, 0.756, 1.265, M_PI, 0, 3 * M_PI / 4, "iiwa7_link_0");
    geometry_msgs::msg::PoseStamped table_pose_3_place = 
        iiwa_move->generatePose(-0.1423, 0.760, 1.265, M_PI, 0, 3 * M_PI / 4, "iiwa7_link_0");

    geometry_msgs::msg::PoseStamped table_pose_0_pick = 
        iiwa_move->generatePose(-0.141, 0.5850, 1.260, M_PI, 0, 3 * M_PI / 4, "iiwa7_link_0");
    geometry_msgs::msg::PoseStamped table_pose_1_pick = 
        iiwa_move->generatePose(0.0530, 0.5885, 1.2615, M_PI, 0, 3 * M_PI / 4, "iiwa7_link_0");
    geometry_msgs::msg::PoseStamped table_pose_2_pick = 
        iiwa_move->generatePose(0.0500, 0.7590, 1.266, M_PI, 0, 3 * M_PI / 4, "iiwa7_link_0");
    geometry_msgs::msg::PoseStamped table_pose_3_pick = 
        iiwa_move->generatePose(-0.1423, 0.7590, 1.262, M_PI, 0, 3 * M_PI / 4, "iiwa7_link_0");
        
    geometry_msgs::msg::PoseStamped home_pose =
        iiwa_move->generatePose(0.5, 0, 1.65896, -M_PI, 0, M_PI, "iiwa7_link_0");
    geometry_msgs::msg::PoseStamped conveyor_pose =
        // iiwa_move->generatePose(0.235, -0.43, 1.263, M_PI, 0, M_PI / 4, "iiwa7_link_0"); // default
        iiwa_move->generatePose(0.2360, -0.4298, 1.263, M_PI, 0, M_PI / 4, "iiwa7_link_0");
    geometry_msgs::msg::PoseStamped hochregallager_pose =
        iiwa_move->generatePose(0.555, 0.069, 1.345, M_PI, 0, 3 * M_PI / 4, "iiwa7_link_0"); 
    geometry_msgs::msg::PoseStamped loading_pose =
        iiwa_move->generatePose(0.0, 0.5, 1.245, M_PI, 0, 3 * M_PI / 4, "iiwa7_link_0");

    geometry_msgs::msg::PoseStamped slot_pose;
    bool use_table = true;
    

    rclcpp::Rate rate(1);
    executor_g->spin_once();
    while (rclcpp::ok())
    {
        if (plc_contl->conveyor_pick){
            switch (plc_contl->slot_id)
            {
                case 0:
                    slot_pose = table_pose_0_place;
                    break;
                case 1:
                    slot_pose = table_pose_1_place;
                    break;
                case 2:
                    slot_pose = table_pose_2_place;
                    break;
                case 3:
                    slot_pose = table_pose_3_place;
                    break;
                default:
                    RCLCPP_ERROR(rclcpp::get_logger("iiwa_motion_controller_node"), "Invalid slot ID!");
                    plc_contl->conveyor_pick = false;
                    plc_contl->table_pick = false;
            }
        }

        if (plc_contl->table_pick){
            switch (plc_contl->slot_id)
            {
                case 0:
                    slot_pose = table_pose_0_pick;
                    break;
                case 1:
                    slot_pose = table_pose_1_pick;
                    break;
                case 2:
                    slot_pose = table_pose_2_pick;
                    break;
                case 3:
                    slot_pose = table_pose_3_pick;
                    break;
                default:
                    RCLCPP_ERROR(rclcpp::get_logger("iiwa_motion_controller_node"), "Invalid slot ID!");
                    plc_contl->conveyor_pick = false;
                    plc_contl->table_pick = false;
            }
        }

        if (plc_contl->move_home)
        {
            iiwa_move->go_home(false);
            plc_contl->move_home = false;
            plc_contl->plc_publish(true, false, false, false, false, use_table);
        }
        if (plc_contl->conveyor_pick)
        {
            // std::cout<<"slot ID: "<< plc_contl->slot_id<<std::endl;
            // RCLCPP_INFO(rclcpp::get_logger("iiwa_motion_controller_node"), "Slot ID: "+std::to_string(plc_contl->slot_id>));
            if (!use_table)
            {
                iiwa_move->pnpPipeLine(conveyor_pose, hochregallager_pose, 0.15, false, true, false); //hochregallager_pose
                plc_contl->conveyor_pick = false;
                plc_contl->plc_publish(false, false, false, true, false, use_table); // Placed the product on Hochregallager
            }
            else
            {
                iiwa_move->pnpPipeLine(conveyor_pose, slot_pose, 0.15, false, true, false);
                plc_contl->conveyor_pick = false;
                plc_contl->plc_publish(false, false, false, false, true, use_table); // Placed the product on table
            }
        }
        if (plc_contl->table_pick)
        {
            // std::cout<<"slot ID: "<< plc_contl->slot_id<<std::endl;
            // RCLCPP_INFO(rclcpp::get_logger("iiwa_motion_controller_node"), "Slot ID: "+std::to_string(plc_contl->slot_id>));
            iiwa_move->pnpPipeLine(slot_pose, conveyor_pose, 0.15, false, true, true);
            plc_contl->table_pick = false;
            plc_contl->plc_publish(false, false, true, false, false, use_table); // Placed the product on conveyor belt
        }

        if (plc_contl->hochregallager_pick)
        {
            iiwa_move->pick_action(hochregallager_pose, 0.15);
            plc_contl->plc_publish(false, true, false, false, false, use_table);
            iiwa_move->go_home(false);
            iiwa_move->place_action(conveyor_pose, 0.15);
            // iiwa_move->pnpPipeLine(hochregallager_pose, conveyor_pose, 0.15, false, false);
            plc_contl->hochregallager_pick = false;
            plc_contl->plc_publish(false, false, true, false, false, use_table); // Placed the product on conveyor belt.
        }
        else
        {
            RCLCPP_INFO(rclcpp::get_logger("iiwa_motion_controller_node"), "Doing nothing and I am Sad!");
        }

        // executor->spin_once();
        executor_g->spin_once();
        rate.sleep();
    }

    rclcpp::shutdown();

    return 0;
}
