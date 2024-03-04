#include "mano_refine.h"

#if defined(__ANDROID__)
#include <android/log.h>
#define HANDTRACKING_LOG_TRACE(...) __android_log_print(ANDROID_LOG_VERBOSE, "HandTracking", ##__VA_ARGS__);
#endif()

float compute_bone_length(std::vector<Eigen::Vector3f> joint_temp) {
	//   std::vector<std::vector<float>> bone_length_array(5,
	//                                                     std::vector<float>(4,
	//                                                     0));
	float bone_length_sum = 0;

	for (int finger_index = 0; finger_index < 5; finger_index++) {
		bone_length_sum += (joint_temp[0] - joint_temp[1 + finger_index * 4]).norm();
		bone_length_sum += (joint_temp[1 + finger_index * 4] - joint_temp[2 + finger_index * 4]).norm();
		bone_length_sum += (joint_temp[2 + finger_index * 4] - joint_temp[3 + finger_index * 4]).norm();
		bone_length_sum += (joint_temp[3 + finger_index * 4] - joint_temp[4 + finger_index * 4]).norm();
	}
	return bone_length_sum / 20;
}

ManoOptimizer::ManoOptimizer() : m_model(new smpl()) {}

ManoOptimizer::ManoOptimizer(std::string hand_model_path) : m_model(new smpl()) {
	m_model->loadModel(hand_model_path);

	m_template.resize(21);

	for (size_t i = 0; i < 21; i++) {
		m_template[i] << m_model->skeleton.joints_21_resvec[i * 3] / 1000.,
			m_model->skeleton.joints_21_resvec[i * 3 + 1] / 1000.,
			m_model->skeleton.joints_21_resvec[i * 3 + 2] / 1000.;
	}
}
ManoOptimizer::ManoOptimizer(std::string hand_model_path, int hand_mode) : m_model(new smpl()) {
	m_model->handmode = (1 - hand_mode);
	m_model->loadModel(hand_model_path);

	m_template.resize(21);

	for (size_t i = 0; i < 21; i++) {
		m_template[i] << m_model->skeleton.joints_21_resvec[i * 3] / 1000.,
			m_model->skeleton.joints_21_resvec[i * 3 + 1] / 1000.,
			m_model->skeleton.joints_21_resvec[i * 3 + 2] / 1000.;
	}
};

std::vector<Eigen::Vector3f> ManoOptimizer::forward(const std::vector<Eigen::Vector3f> joint_pre) {
	std::vector<Eigen::Vector3f> j3d_pre_process_(21);

	//   float ratio = (m_template[9] - m_template[0]).norm() /
	//                 (j3d_pre_[9] - j3d_pre_[0]).norm();
	// std::cout << "template: " << std::endl;
	// for (int i = 0; i < 21; i++) {
	//   std::cout << m_template[i][0] << " " << m_template[i][1] << " "
	//             << m_template[i][2];
	//   std::cout << std::endl;
	// }
	float template_len = compute_bone_length(m_template);
	float j3d_pre_len  = compute_bone_length(joint_pre);
	// std::cout << template_len << " " << j3d_pre_len << std::endl;
	float ratio = template_len / j3d_pre_len;
	for (size_t i = 0; i < joint_pre.size(); i++) {
		j3d_pre_process_[i] = joint_pre[i] * ratio;
	}

	Eigen::Vector3f base_pnt = j3d_pre_process_[0];

	for (size_t i = 0; i < j3d_pre_process_.size(); i++) {
		j3d_pre_process_[i] = j3d_pre_process_[i] - base_pnt + m_template[0];
	}
	// std::cout << "IK input: " << std::endl;
	// for (int i = 0; i < 21; i++) {
	//   std::cout << j3d_pre_process_[i][0] << " " << j3d_pre_process_[i][1] << "
	//   "
	//             << j3d_pre_process_[i][2];
	//   std::cout << std::endl;
	// }

	std::vector<Eigen::Matrix3f> pose_R = constraint_IK(m_template, j3d_pre_process_, (m_model->handmode == 0));
	// // if need update shape set this vector
	std::vector<float> shape(m_model->shape_basis_dim, 0);

	m_model->updateMesh(shape, pose_R);

	//   return m_model->skeleton.joints_21_resvec;
	// todo： ratio??
	//   scale = 1.0
	//     j3d_recon = j3d_recon.cpu().numpy().reshape((21, 3))
	//     pred_xyz_recon = scale * (j3d_recon - j3d_recon[0]) / ratio / 1000 +
	//     pred_xyz[0] pred_xyz = pred_xyz_recon.copy()
	std::vector<Eigen::Vector3f> j3d_recon(21);
	for (size_t i = 0; i < 21; i++) {
		j3d_recon[i] << m_model->skeleton.joints_21_resvec[i * 3], m_model->skeleton.joints_21_resvec[i * 3 + 1],
			m_model->skeleton.joints_21_resvec[i * 3 + 2];
	}
	std::vector<Eigen::Vector3f> pred_xyz(21);
	for (size_t i = 0; i < 21; i++) {
		pred_xyz[i] = (j3d_recon[i] - j3d_recon[0]) / ratio / 1000. + joint_pre[0];
	}

	return pred_xyz;
}

void ManoOptimizer::set_modelpath(std::string hand_model_path, int hand_mode) {
	m_model->handmode = hand_mode;

	m_model->loadModel(hand_model_path);

	m_template.resize(21);

	for (size_t i = 0; i < 21; i++) {
		m_template[i] << m_model->skeleton.joints_21_resvec[i * 3] / 1000.,
			m_model->skeleton.joints_21_resvec[i * 3 + 1] / 1000.,
			m_model->skeleton.joints_21_resvec[i * 3 + 2] / 1000.;
	}
}