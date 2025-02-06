#include "AIK.h"
#include <map>
// #include <android/log.h>
// #define HANDTRACKING_LOG_TRACE(...) __android_log_print(ANDROID_LOG_VERBOSE, "HandTracking", ##__VA_ARGS__);

const std::vector<int> kinematic_tree = {2, 3, 4, 6, 7, 8, 10, 11, 12, 14, 15, 16, 18, 19, 20};

const std::vector<int> kinematic_tree_all = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

const std::vector<std::vector<int>> finger_indexes = {
	{0, 0, 0, 0, 0}, {1, 5, 9, 13, 17}, {2, 6, 10, 14, 18}, {3, 7, 11, 15, 19}, {4, 8, 12, 16, 20}};

std::map<int, int> ranges_index = {{5 + 1, 0},  {6 + 1, 1},  {7 + 1, 2},  {9 + 1, 3},  {10 + 1, 4},  {11 + 1, 5},
								   {13 + 1, 6}, {14 + 1, 7}, {15 + 1, 8}, {17 + 1, 9}, {18 + 1, 10}, {19 + 1, 11}};

const std::vector<Eigen::Vector3f> rhand_template = {{0., 0., 0.},
													 {-0.0359584, 0.0191577, 0.0280285},
													 {-0.0592187, 0.0343076, 0.0449557},
													 {-0.0831233, 0.0437118, 0.06691261},
													 {-0.094403, 0.0505685, 0.08139791},
													 {-0.09599619, 0.00731645, 0.0235507},
													 {-0.133777, 0.0106487, 0.0236269},
													 {-0.1579863, 0.01278401, 0.02367579},
													 {-0.176852, 0.01486339, 0.02400584},
													 {-0.0956466, 0.00254315, 0.00172591},
													 {-0.138116, 0.00702145, -0.002640291},
													 {-0.1653715, 0.009895517, -0.005442409},
													 {-0.186555, 0.0128932, -0.008040725},
													 {-0.0886938, 0.00652931, -0.0174652},
													 {-0.126314, 0.0108848, -0.0267611},
													 {-0.1519506, 0.01385279, -0.03309564},
													 {-0.1720999, 0.01562605, -0.03772666},
													 {-0.0778958, 0.0136912, -0.0350541},
													 {-0.107947, 0.0181548, -0.0421763},
													 {-0.1283033, 0.02093556, -0.04666972},
													 {-0.1462481, 0.02307901, -0.0500113}};

const std::vector<Eigen::Vector3f> lhand_template = {{0., 0., 0.},
													 {-0.03595843, 0.01915772, -0.0280285},
													 {-0.05921866, 0.03430764, -0.04495572},
													 {-0.08312328, 0.04371182, -0.06691258},
													 {-0.09440295, 0.05056853, -0.08139791},
													 {-0.09599624, 0.007316454, -0.02355068},
													 {-0.1337768, 0.01064868, -0.02362694},
													 {-0.1579864, 0.01278396, -0.02367581},
													 {-0.1768517, 0.0148633, -0.02400584},
													 {-0.0956466, 0.002543155, -0.001725905},
													 {-0.1381155, 0.007021449, 0.002640287},
													 {-0.1653711, 0.009895517, 0.005442412},
													 {-0.1865548, 0.01289324, 0.008040722},
													 {-0.08869379, 0.006529307, 0.01746524},
													 {-0.1263144, 0.01088481, 0.02676108},
													 {-0.1519505, 0.01385281, 0.03309562},
													 {-0.1721002, 0.01562609, 0.03772666},
													 {-0.07789585, 0.01369118, 0.03505407},
													 {-0.1079471, 0.01815484, 0.04217627},
													 {-0.1283029, 0.02093554, 0.04666972},
													 {-0.1462478, 0.02307904, 0.05001136}};

const std::vector<std::vector<float>> angles_around_x = {{-15., 90.}, {0., 90.},   {0., 90.},   {-15., 90.},
														 {0., 90.},   {0., 90.},   {-15., 90.}, {0., 90.},
														 {0., 90.},   {-15., 90.}, {0., 90.},   {0., 90.}};

const std::vector<std::vector<float>> angles_around_z = {{-90, 90}, {-15., 15.}, {-15., 15.}, {-15, 15},
														 {0, 0},    {0, 0},      {-30, 30},   {0, 0},
														 {0, 0},    {-45, 45},   {0, 0},      {0, 0}};

//记录了节点的父亲节点信息，下标为index的节点的父亲节点为SNAP_PARENT[index]
const std::vector<int> SNAP_PARENT = {
	0,  // 0's parent
	0,  // 1's parent
	1,  2,  3,
	0,  // 5's parent
	5,  6,  7,
	0,  // 9's parent
	9,  10, 11,
	0,  // 13's parent
	13, 14, 15,
	0,  // 17's parent
	17, 18, 19,
};

const std::map<int, int> ID2ROT = {
	{2, 13}, {3, 14},  {4, 15},  {6, 1},   {7, 2},  {8, 3},  {10, 4}, {11, 5},
	{12, 6}, {14, 10}, {15, 11}, {16, 12}, {18, 7}, {19, 8}, {20, 9},
};

//设置多个点的约束
const std::set<int> dof1_indexs = {8, 12, 16, 20};
const std::set<int> dof2_indexs = {};//{6, 7, 10, 11, 14, 15, 18, 19};

const std::vector<std::vector<int>> finger_index = {{5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16}, {17, 18, 19, 20}};
const std::map<int, int> plane_index = {{5, 0},  {6, 0},  {7, 0},  {8, 0},  {9, 1},  {10, 1}, {11, 1}, {12, 1},
										{13, 2}, {14, 2}, {15, 2}, {16, 2}, {17, 3}, {18, 3}, {19, 3}, {20, 3}};

float compute_angle(float cos_alpha, float sin_alpha) {
	float alpha;
	cos_alpha = cos_alpha < 1. ? cos_alpha : 1.;
	cos_alpha = cos_alpha > -1. ? cos_alpha : -1.;
	sin_alpha = sin_alpha < 1. ? sin_alpha : 1.;
	sin_alpha = sin_alpha > -1. ? sin_alpha : -1.;

	if ((cos_alpha <= 0 && sin_alpha <= 0) || (cos_alpha >= 0 && sin_alpha <= 0)) {
		alpha = std::acos(cos_alpha);
		alpha = -alpha;
	} else if ((cos_alpha >= 0 && sin_alpha >= 0) || (cos_alpha <= 0 && sin_alpha >= 0)) {
		alpha = std::acos(cos_alpha);
	} else {
		alpha = 0;
	}
	return alpha;
}


/// @brief 计算骨骼长度
/// @param joint_temp, 传入的原始数据，对这个里面的关键点进行计算 
/// @param fingers_len, 某个手指，一根指节的长度
/// @param bones_len, 手掌长度
void compute_bone_length(const std::vector<Eigen::Vector3f> &joint_temp, std::vector<float> &fingers_len,
						 std::vector<float> &bones_len) {
	std::vector<std::vector<float>> bone_length_array(5, std::vector<float>(4, 0));
	fingers_len.clear();
	fingers_len.resize(5);
	bones_len.clear();
	bones_len.resize(5);

	for (int finger_index = 0; finger_index < 5; finger_index++) {
		bones_len[finger_index]   = (joint_temp[0] - joint_temp[1 + finger_index * 4]).norm();
		fingers_len[finger_index] = ((joint_temp[1 + finger_index * 4] - joint_temp[2 + finger_index * 4]).norm() +
									 (joint_temp[2 + finger_index * 4] - joint_temp[3 + finger_index * 4]).norm() +
									 (joint_temp[3 + finger_index * 4] - joint_temp[4 + finger_index * 4]).norm()) /
									3;
	}
	return;
}

void point_project_to_plane(Eigen::Vector3f &point_out, const Eigen::Vector3f &point, const Eigen::Vector4f plane) {
	//     # point: (x, y, z).
	//     # plane: Ax+By+Cz+D=0.
	//     # plane_point: (x, y, z) or None. If plane_point is None, plane_point
	//     is computerd based on point + plane.
	float           z = (plane(0) * point(0) + plane(1) * point(1) + plane(3)) / (-plane(2));
	Eigen::Vector3f plane_point;
	plane_point << point(0), point(1), z;
	Eigen::Vector3f normal;
	normal << plane(0), plane(1), plane(2);
	normal.normalize();

	float t   = (plane_point.dot(normal) - point.dot(normal)) / normal.norm();
	point_out = point + t * normal;
}

/// @brief 计算绕x轴的旋转矩阵
/// @param T 
/// @param v_p_k_pa 以pa节点为中心点的局部坐标系中k的目标点相对于pa的向量
/// @param k 子节点
/// @param pa 父亲节点
/// @param left_hand 是否左手
/// @param ranges 旋转角度
/// @return 
Eigen::Matrix3f constraint_around_x(std::vector<Eigen::Vector3f> T, Eigen::Vector3f v_p_k_pa, int k, int pa,
									bool left_hand, std::vector<float> ranges) {
	//t_v_coord_z是一个垂直于掌心，从掌心方向射出的向量
	Eigen::Vector3f t_v_0_9     = T[17] - T[0];
	Eigen::Vector3f t_v_0_5     = T[5] - T[0];
	Eigen::Vector3f t_v_coord_z = t_v_0_5.cross(t_v_0_9);
	if (left_hand) {
		t_v_coord_z = -t_v_coord_z;
	}

	t_v_coord_z                        = t_v_coord_z.normalized();
	Eigen::Vector3f t_v_k_pa           = (T[k] - T[pa]);
	Eigen::Vector3f normal_t_k_pa_papa = t_v_k_pa.cross(t_v_coord_z);
	normal_t_k_pa_papa                 = normal_t_k_pa_papa.normalized();

    //计算点v_p_k_pa到由法向量normal_t_k_pa_papa和点T[pa]定义的平面的投影点上的投影点v_p_k_pa_proj
	v_p_k_pa = v_p_k_pa + T[pa];
	float t =
		(normal_t_k_pa_papa.dot(T[pa]) - normal_t_k_pa_papa.dot(v_p_k_pa)) / (normal_t_k_pa_papa.norm());  // + 1e-8
	Eigen::Vector3f v_p_k_pa_proj = v_p_k_pa + t * normal_t_k_pa_papa;
	v_p_k_pa_proj = v_p_k_pa_proj - T[pa];

	// compute axis and alpha
	Eigen::Vector3f v_t_k_pa = T[k] - T[pa];
	Eigen::Vector3f delta_t_k = v_t_k_pa;
	Eigen::Vector3f delta_p_k = v_p_k_pa_proj;
	delta_t_k                 = delta_t_k.normalized();
	delta_p_k                 = delta_p_k.normalized();

    //计算旋转轴
	Eigen::Vector3f axis = delta_t_k.cross(delta_p_k);
	axis                 = axis.normalized();

    //计算旋转角度
	float cos_alpha = delta_t_k.dot(delta_p_k) / (delta_t_k.norm() * delta_p_k.norm());
	float sin_alpha = delta_t_k.cross(delta_p_k).norm() / (delta_t_k.norm() * delta_p_k.norm());
	if (delta_t_k.cross(t_v_coord_z).dot(delta_t_k.cross(delta_p_k)) < 0) {
		sin_alpha = -sin_alpha;
		axis      = -axis;
	}

	float alpha = compute_angle(cos_alpha, sin_alpha);

	//约束旋转角度
	float upper_bound = M_PI / 2 / 90 * ranges[1];
	float lower_bound = M_PI / 2 / 90 * ranges[0];
	if (alpha > upper_bound) {
		alpha = upper_bound;
	} else if (alpha < lower_bound) {
		alpha = lower_bound;
	} else {
		//do nothing
	}

    //通过旋转轴和旋转角度，计算旋转矩阵
	Eigen::Matrix3f R_x = axisang2mat(axis, alpha);
	return R_x;
}


/// @brief 计算绕z轴的旋转矩阵
/// @param T 
/// @param v_p_k_pa 以pa节点为中心点的局部坐标系中k的目标点相对于pa的向量
/// @param k 子节点
/// @param pa 父节点
/// @param left_hand 是否左手
/// @param ranges 旋转角度
/// @return 
Eigen::Matrix3f constraint_around_z(std::vector<Eigen::Vector3f> T, Eigen::Vector3f v_p_k_pa, int k, int pa,
									bool left_hand, std::vector<float> ranges) {
	//t_v_coord_z是一个垂直于掌心，从掌心方向射出的向量
	Eigen::Vector3f t_v_0_9     = T[17] - T[0];
	Eigen::Vector3f t_v_0_5     = T[5] - T[0];
	Eigen::Vector3f t_v_coord_z = t_v_0_5.cross(t_v_0_9);
	if (left_hand) {
		t_v_coord_z = -t_v_coord_z;
	}

	t_v_coord_z = t_v_coord_z.normalized();

	//计算点v_p_k_pa到由法向量t_v_coord_z和点T[pa]定义的平面的投影点v_p_k_pa_proj_z
	v_p_k_pa = v_p_k_pa + T[pa];
	float t = (t_v_coord_z.dot(T[pa]) - t_v_coord_z.dot(v_p_k_pa)) / (t_v_coord_z.norm());  // + 1e-8
	Eigen::Vector3f v_p_k_pa_proj_z = v_p_k_pa + t * t_v_coord_z;
	v_p_k_pa_proj_z = v_p_k_pa_proj_z - T[pa];

	// compute axis and alpha
	Eigen::Vector3f v_t_k_pa = (T[k] - T[pa]);
	Eigen::Vector3f delta_t_k = v_t_k_pa;
	Eigen::Vector3f delta_p_k = v_p_k_pa_proj_z;
	delta_t_k                 = delta_t_k.normalized();
	delta_p_k                 = delta_p_k.normalized();

	//计算旋转轴
	Eigen::Vector3f axis      = delta_t_k.cross(delta_p_k);
	axis                      = axis.normalized();

    //计算旋转角度
	float cos_alpha = delta_t_k.dot(delta_p_k) / (delta_t_k.norm() * delta_p_k.norm());
	float sin_alpha = delta_t_k.cross(delta_p_k).norm() / (delta_t_k.norm() * delta_p_k.norm());
	float alpha     = compute_angle(cos_alpha, sin_alpha);

	//约束旋转角度
	float upper_bound = M_PI / 2 / 90 * ranges[1];
	float lower_bound = M_PI / 2 / 90 * ranges[0];
	if (alpha > upper_bound) {
		alpha = upper_bound;
	} else if (alpha < lower_bound) {
		alpha = lower_bound;
	} else {
		//do nothing
	}

    //通过旋转轴和旋转角，计算旋转矩阵
	Eigen::Matrix3f R_x = axisang2mat(axis, alpha);
	return R_x;
}

/// @brief 将一个旋转轴和旋转角度，转化为3*3的旋转矩阵
/// @param axis 三维向量，表示旋转轴
/// @param angle 旋转角
/// @return 返回转换完成的旋转矩阵
Eigen::Matrix3f axisang2mat(Eigen::Vector3f axis, float angle) {
	axis.normalize();
	Eigen::Matrix3f rot_mat = Eigen::AngleAxisf(angle, axis).matrix();
	return rot_mat;
}

std::vector<Eigen::Matrix3f> constraint_IK(std::vector<Eigen::Vector3f> T, std::vector<Eigen::Vector3f> P,
										   bool left_hand) {
	std::vector<Eigen::Matrix3f> R;
	R.resize(21);
	std::vector<Eigen::Matrix3f> R_pa_k;
	R_pa_k.resize(21);
	std::vector<Eigen::Matrix3f> pose_R;
	pose_R.resize(16);

	std::vector<Eigen::Vector3f> q;
	q.resize(T.size());

	q[0] = T[0];

	Eigen::MatrixXf P_0(3, 5);
	P_0 << P[1] - P[0], P[5] - P[0], P[9] - P[0], P[13] - P[0], P[17] - P[0];
	Eigen::MatrixXf T_0(3, 5);
	T_0 << T[1] - T[0], T[5] - T[0], T[9] - T[0], T[13] - T[0], T[17] - T[0];

	Eigen::Matrix3f H = T_0 * P_0.transpose();

	Eigen::JacobiSVD<Eigen::MatrixXf> svd(H, Eigen::ComputeThinV | Eigen::ComputeThinU);
	Eigen::Matrix3f                   U = svd.matrixU();
	Eigen::Matrix3f                   V = svd.matrixV();

	Eigen::Matrix3f R0 = V * U.transpose();

	std::cout << "det: " << R0.determinant() << std::endl;

	Eigen::Matrix3f mod;
	mod << 1, 0, 0, 0, 1, 0, 0, 0, -1;

	if (R0.determinant() < 0) {
		R0 = V * mod * U.transpose();
	}

	R[0] = R0;

	R[1]  = R[0];
	R[5]  = R[0];
	R[9]  = R[0];
	R[13] = R[0];
	R[17] = R[0];

	std::cout << "R0: " << R0 << std::endl;
	// std::cout << "H: " << H << std::endl;
	// std::cout << "U: " << U << std::endl;
	// std::cout << "V: " << V << std::endl;
	// std::cout << left_hand << std::endl;

	for (auto k : kinematic_tree) {
		int pa    = SNAP_PARENT[k];
		int pa_pa = SNAP_PARENT[pa];

		q[pa]                    = R[pa] * (T[pa] - T[pa_pa]) + q[pa_pa];
		Eigen::Vector3f v_p_k_pa = R[pa].inverse() * (P[k] - q[pa]);
		// std::cout << k << "," << pa << ": " << R[pa] << std::endl;
		// std::cout << "K: " << k << std::endl;
		// std::cout << q[pa] << std::endl;

		if (dof1_indexs.count(k)) {
			Eigen::Matrix3f R_x = constraint_around_x(T, v_p_k_pa, k, pa, left_hand, angles_around_x[ranges_index[k]]);
			R_pa_k[k]           = R_x;
			// Eigen::Matrix3f R_z = constraint_around_z(T, v_p_k_pa, k, pa, left_hand);
			// std::cout << k << ": " << R_pa_k[k] << std::endl;
		}

		else if (dof2_indexs.count(k)) {
			Eigen::Matrix3f R_x = constraint_around_x(T, v_p_k_pa, k, pa, left_hand, angles_around_x[ranges_index[k]]);
			v_p_k_pa            = R_x.inverse() * v_p_k_pa;
			Eigen::Matrix3f R_z = constraint_around_z(T, v_p_k_pa, k, pa, left_hand, angles_around_z[ranges_index[k]]);
			R_pa_k[k]           = R_x * R_z;
			// std::cout << k << ": " << R_x << std::endl;
		}

		else {
			Eigen::Vector3f v_t_k_pa = T[k] - T[pa];
			Eigen::Vector3f axis     = v_t_k_pa.cross(v_p_k_pa);
			// std::cout << "K: " << k << std::endl;
			// std::cout << axis.normalized() << std::endl;
			axis            = axis.normalized();
			float cos_alpha = v_t_k_pa.dot(v_p_k_pa) / (v_t_k_pa.norm() * v_p_k_pa.norm());
			float alpha     = acos(cos_alpha);
			R_pa_k[k]       = axisang2mat(axis, alpha);
		}
		R[k] = R[pa] * R_pa_k[k];
	}

	pose_R[0] = R[0];

	for (auto iter = ID2ROT.begin(); iter != ID2ROT.end(); iter++) {
		pose_R[iter->second] = R_pa_k[iter->first];
	}
	return pose_R;
}


/// @brief 核心约束函数，最主要功能是将初始坐标系的点旋转到对应的目的坐标系的位置
/// @param T, 初始坐标系的三维点的数据
/// @param P, 目标坐标系的三维点的数据
/// @param left_hand, 区分左手和右手
/// @return 返回约束完成的手部数据结果
std::vector<Eigen::Vector3f> constraint_hand_kernel(std::vector<Eigen::Vector3f> T, std::vector<Eigen::Vector3f> P,
													bool left_hand) {
	//初始化变量
	std::vector<Eigen::Matrix3f> R;  //旋转矩阵
	R.resize(21);
	std::vector<Eigen::Matrix3f> R_pa_k;  //父子节点之间的旋转矩阵
	R_pa_k.resize(21);
	std::vector<Eigen::Vector3f> j3d_recon(21);  //优化后的关键点
	std::vector<Eigen::Vector3f> q;  //临时存储旋转后的关键点
	q.resize(T.size());
	q[0] = T[0];  //根节点（初始坐标系和目标坐标系的手腕点是重合的）

	/**
	 * step1: 计算出一个旋转矩阵。
	 * 使得，原始坐标系T中的每个点与这个旋转矩阵进行乘积，即可将整个手掌转到和目标坐标系P的手掌部分重合。
	 * 同时，乘积之后，1，5，9，13，17这五个掌心点在目标坐标系中的位置就确定了。后续只需要处理其他指节相关节点
	 * 
	 * 具体操作如下：
	 * 初始坐标系，构建五个向量（五个掌心点和一个腕点构成），分别是P[1->0], P[5->0], P[9->0], P[13->0], P[17->0]，组成矩阵P_0
	 * 目标坐标系，构建五个向量（五个掌心点和一个腕点构成），分别是T[1->0], T[5->0], T[9->0], T[13->0], T[17->0]，组成矩阵T_0
	 * 首先构建协方差矩阵H，
	 * 通过svd对H进行分解，
	 * 对分解的矩阵进行乘积运算V * U.transpose()，就能得到旋转矩阵
	*/

    //构建初始坐标系矩阵和目标坐标系矩阵
	Eigen::MatrixXf P_0(3, 5);
	P_0 << P[1] - P[0], P[5] - P[0], P[9] - P[0], P[13] - P[0], P[17] - P[0];
	Eigen::MatrixXf T_0(3, 5);
	T_0 << T[1] - T[0], T[5] - T[0], T[9] - T[0], T[13] - T[0], T[17] - T[0];

    //计算协方差矩阵H，并拆解
	Eigen::Matrix3f H = T_0 * P_0.transpose();
	Eigen::JacobiSVD<Eigen::MatrixXf> svd(H, Eigen::ComputeThinV | Eigen::ComputeThinU);
	Eigen::Matrix3f U = svd.matrixU();
	Eigen::Matrix3f V = svd.matrixV();

    //计算最终旋转矩阵R0
	Eigen::Matrix3f R0 = V * U.transpose();
	if (R0.determinant() < 0) {
		Eigen::Matrix3f mod;
	    mod << 1, 0, 0, 0, 1, 0, 0, 0, -1;
		R0 = V * mod * U.transpose();  //进行修正，保证旋转矩阵纯旋转
	}

	/**
	 * step2: 遍历手部其他关键点，在R0的基础上，通过目标坐标系父子节点之间的关系，
	 * 计算1-20中，除了1，5，9，13，17等掌心点以外的每个点在以其父节点为中心点的局部坐标系中的的旋转矩阵
	*/
    R[0] = R0; 
	for (auto k : kinematic_tree_all) {
		int pa = SNAP_PARENT[k];  //父节点
		int pa_pa = SNAP_PARENT[pa];  //祖父节点

		q[pa] = R[pa] * (T[pa] - T[pa_pa]) + q[pa_pa];  //计算父节点旋转后位置
		Eigen::Vector3f v_p_k_pa = R[pa].inverse() * (P[k] - q[pa]);  //计算以pa节点为中心点的局部坐标系中k的目标点相对于pa的向量

        //根据自由度计算不同节点的旋转矩阵
		if (dof1_indexs.count(k)) {  //8，12，16，20，单自由度节点，计算绕x轴的旋转矩阵
			Eigen::Matrix3f R_x = constraint_around_x(T, v_p_k_pa, k, pa, left_hand, angles_around_x[ranges_index[k]]);
			v_p_k_pa = R_x.inverse() * (P[k] - q[pa]);
			R_pa_k[k] = R_x;
		}else if (dof2_indexs.count(k)) {  //6，7，10，11，14，15，18，19，双自由度节点，计算绕x轴和z轴的旋转矩阵
			Eigen::Matrix3f R_x = constraint_around_x(T, v_p_k_pa, k, pa, left_hand, angles_around_x[ranges_index[k]]);
			v_p_k_pa            = R_x.inverse() * v_p_k_pa;
			Eigen::Matrix3f R_z = constraint_around_z(T, v_p_k_pa, k, pa, left_hand, angles_around_z[ranges_index[k]]);
			R_pa_k[k]           = R_x * R_z;
		} else {   //2，3，4，全自由度节点，通过叉积和点积计算旋转矩阵
			Eigen::Vector3f v_t_k_pa = T[k] - T[pa];

			//计算旋转轴
			Eigen::Vector3f axis = v_t_k_pa.cross(v_p_k_pa);
			axis = axis.normalized();

			//计算旋转角度
			float cos_alpha = v_t_k_pa.dot(v_p_k_pa) / (v_t_k_pa.norm() * v_p_k_pa.norm());
			cos_alpha = cos_alpha < 1. ? cos_alpha : 1.;
			cos_alpha = cos_alpha > -1. ? cos_alpha : -1.;
			float alpha = acos(cos_alpha);

			//生成旋转矩阵
			R_pa_k[k] = axisang2mat(axis, alpha);
		}

		R[k] = R[pa] * R_pa_k[k];  //通过父节点的旋转矩阵与父子节点之间的旋转矩阵，得到子节点的旋转矩阵
	}

    /**
	 * step3: 通过计算后的旋转矩阵，旋转初始坐标系的1-20号点到目标坐标系对应位置
	*/
	j3d_recon[0] = P[0];
	for (auto k : kinematic_tree_all) {
        auto pa = SNAP_PARENT[k];
		j3d_recon[k] = R[k] * (T[k] - T[pa]) + j3d_recon[pa];
	}

	return j3d_recon;
}


/// @brief 手势预测数据约束函数，这里对传入的手势数据进行约束处理，并返回
/// @param pred_xyz, 传入的手势预测数据
/// @param left_hand, 区分左手还是右手
/// @return 返回约束完成之后的手势数据
std::vector<Eigen::Vector3f> constraint_hand_v2(std::vector<Eigen::Vector3f> pred_xyz, bool left_hand) {
	//这里只提取前21个点
	std::vector<Eigen::Vector3f> j3d_pre(pred_xyz.begin(), pred_xyz.begin()+21);

	//选择手部模板，左手还是右手模板
	auto hand_template = left_hand ? lhand_template : rhand_template;

    //缩放关键点坐标
	const float ratio = 1000.;
	std::for_each(j3d_pre.begin(), j3d_pre.end(), [ratio](Eigen::Vector3f &point) { point = point / ratio; });

    //计算模板数据的手指指节长度和手掌长度
	std::vector<float> template_finger_len;
	std::vector<float> template_palm_len;
	compute_bone_length(hand_template, template_finger_len, template_palm_len);

    //计算要预测的数据的手指指节长度和手掌长度
	std::vector<float> pred_finger_len;
	std::vector<float> pred_palm_len;
	compute_bone_length(j3d_pre, pred_finger_len, pred_palm_len);

    //将模板关键点归一化，即手腕移动到原点坐标
	auto root_template = hand_template[0];
	std::for_each(hand_template.begin(), hand_template.end(),
				  [root_template](Eigen::Vector3f &point) { point = point - root_template; });

    //根据预测数据的手指和手掌长度，调整模板关键点的位置，使其与预测数据的解剖结构一致
	auto hand_template_proto = hand_template;
	for (int bone_idx = 1; bone_idx < 5; bone_idx++) {
		for (int finger_idx = 0; finger_idx < 5; finger_idx++) {
			if (bone_idx == 1) {
				//调整手掌关键点
				hand_template[finger_indexes[bone_idx][finger_idx]] =
					(hand_template_proto[finger_indexes[bone_idx][finger_idx]] -
					 hand_template_proto[finger_indexes[bone_idx - 1][finger_idx]]) /
					template_palm_len[finger_idx];

				hand_template[finger_indexes[bone_idx][finger_idx]] *= pred_palm_len[finger_idx];
				hand_template[finger_indexes[bone_idx][finger_idx]] +=
					hand_template[finger_indexes[bone_idx - 1][finger_idx]];
			} else {
				//调整手指关键点
				hand_template[finger_indexes[bone_idx][finger_idx]] =
					(hand_template_proto[finger_indexes[bone_idx][finger_idx]] -
					 hand_template_proto[finger_indexes[bone_idx - 1][finger_idx]]) /
					template_finger_len[finger_idx];

				hand_template[finger_indexes[bone_idx][finger_idx]] *= pred_finger_len[finger_idx];
				hand_template[finger_indexes[bone_idx][finger_idx]] +=
					hand_template[finger_indexes[bone_idx - 1][finger_idx]];
			}
		}
	}

    //移动所有模板关键点到预测数据的根节点位置
	auto root_pred = j3d_pre[0];
	std::for_each(hand_template.begin(), hand_template.end(),
				  [root_pred](Eigen::Vector3f &point) { point = point + root_pred; });

    //使用核心约束函数旋转手势模板中的关键点，使其与预测手势关键点重合，并约束旋转角度
	std::vector<Eigen::Vector3f> j3d_recon = constraint_hand_kernel(hand_template, j3d_pre, left_hand);

	//将优化后的关键点恢复到原始尺度，并移动使得手腕点和预测数据手腕点重合
	auto root_refined = j3d_recon[0];
	std::for_each(j3d_recon.begin(), j3d_recon.end(), [root_refined, pred_xyz, ratio](Eigen::Vector3f &point) {
		point = (point - root_refined) * ratio + pred_xyz[0];
	});

	return j3d_recon;
}


/// @brief 大拇指约束函数，约束大拇指外翻角度
/// @param points 所有手势关键点坐标信息
void constraint_thumb(std::vector<Eigen::Vector3f> &points) {

    Eigen::Vector3f x = points[3] - points[2];
    Eigen::Vector3f y = points[4] - points[3];
    Eigen::Vector3f x_ = points[1] - points[0];
    Eigen::Vector3f y_ = points[9] - points[0];

    //float module_x = x.norm();
    float module_y = y.norm();
    //float dot_value = x.dot(y);
    //float cos_theta = dot_value / (module_x * module_y);
    //float angle_radian = std::acos(cos_theta);
    //float angle_value = angle_radian * 180.0F / M_PI;  // M_PI is defined in <cmath>

    Eigen::Vector3f cross_product = x.cross(y);
    Eigen::Vector3f cross_product_ = x_.cross(y_);
    float dot_ref = cross_product.dot(cross_product_);
    if (dot_ref < 0){
        Eigen::Vector3f direction = x.normalized();
        Eigen::Vector3f new_position = points[3] + direction * module_y;
	    points[4] = (new_position+points[4])/2;
    }
}

/// @brief 手指和手掌约束函数，进行四点共面约束
/// @param points 所有关键点坐标信息
void constraint_hand_plane(std::vector<Eigen::Vector3f>& points) {
    std::vector<std::vector<int>> point_indices = {
        {5, 6, 7, 8},
        {9, 10, 11, 12},
        {13, 14, 15, 16},
        {17, 18, 19, 20}
    };

	for (const auto& point_index : point_indices) {

		// 获取 A, B, C 点
		Eigen::Vector3f A = points[point_index[0]];
		Eigen::Vector3f B = points[point_index[3]];
		Eigen::Vector3f C = (points[point_index[1]] + points[point_index[2]]) / 2;

		// 计算平面法向量
		Eigen::Vector3f AB = B - A;
		Eigen::Vector3f AC = C - A;
		Eigen::Vector3f normal = AB.cross(AC);  // AB 和 AC 的叉积即为平面的法向量

		// 计算第一个点到平面的投影
		Eigen::Vector3f AP = points[point_index[1]] - A;
		float distance = AP.dot(normal) / normal.norm();  // 点到平面的距离
		Eigen::Vector3f projection_1 = points[point_index[1]] - distance * normal.normalized();  // 投影点
		points[point_index[1]] = projection_1;

		// 计算第二个点到平面的投影
		AP = points[point_index[2]] - A;
		distance = AP.dot(normal) / normal.norm();  // 点到平面的距离
		Eigen::Vector3f projection_2 = points[point_index[2]] - distance * normal.normalized();  // 投影点
		points[point_index[2]] = projection_2;
	}
}
