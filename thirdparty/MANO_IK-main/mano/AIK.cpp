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

// const std::vector<Eigen::Vector3f> lhand_template = {
// 	{-9.5669908e-05, 6.3834291e-06, 6.1863034e-06},   {-7.1580223e-05, -9.1389074e-06, 3.1999152e-05},
// 	{-5.1946980e-05, -8.2476172e-06, 5.5698692e-05},  {-2.9729248e-05, -1.3680592e-05, 7.0222821e-05},
// 	{-2.2312577e-06, -1.8095005e-05, 9.0914495e-05},  {-7.5726807e-06, 1.1830721e-06, 2.6872296e-05},
// 	{2.5106225e-05, 5.1924262e-06, 2.9089360e-05},    {4.7262140e-05, 3.8940038e-06, 2.8975250e-05},
// 	{7.0524904e-05, 4.6119731e-06, 3.3024513e-05},    {-1.0094941e-06, 4.9044647e-06, 2.8287648e-06},
// 	{3.0173180e-05, 6.7657943e-06, -2.7657445e-06},   {5.3077827e-05, 5.5136902e-06, -6.7102587e-06},
// 	{7.7415658e-05, 1.0507885e-05, -1.0187537e-05},   {-1.3934374e-05, 2.4260078e-06, -2.0486883e-05},
// 	{1.4379900e-05, 4.4930152e-06, -2.5585421e-05},   {3.7900420e-05, 2.8049035e-06, -3.3219236e-05},
// 	{6.0804228e-05, 7.3430606e-06, -4.0202209e-05},   {-2.6882954e-05, -3.5569008e-06, -3.7023037e-05},
// 	{-9.8685477e-06, -3.4950763e-06, -4.9521823e-05}, {5.9983545e-06, -4.1862304e-06, -5.9853715e-05},
// 	{2.1898850e-05, -1.6281185e-06, -7.0131675e-05}};
// const std::vector<Eigen::Vector3f> rhand_template = {
// 	{9.5669908e-05, 6.3834291e-06, 6.1863034e-06},   {7.1580223e-05, -9.1389074e-06, 3.1999152e-05},
// 	{5.1946980e-05, -8.2476172e-06, 5.5698692e-05},  {2.9729248e-05, -1.3680592e-05, 7.0222821e-05},
// 	{2.2312577e-06, -1.8095005e-05, 9.0914495e-05},  {7.5726807e-06, 1.1830721e-06, 2.6872296e-05},
// 	{-2.5106225e-05, 5.1924262e-06, 2.9089360e-05},  {-4.7262140e-05, 3.8940038e-06, 2.8975250e-05},
// 	{-7.0524904e-05, 4.6119731e-06, 3.3024513e-05},  {1.0094941e-06, 4.9044647e-06, 2.8287648e-06},
// 	{-3.0173180e-05, 6.7657943e-06, -2.7657445e-06}, {-5.3077827e-05, 5.5136902e-06, -6.7102587e-06},
// 	{-7.8992816e-05, 6.1466490e-06, -1.2040861e-05}, {1.3934374e-05, 2.4260078e-06, -2.0486883e-05},
// 	{-1.4379900e-05, 4.4930152e-06, -2.5585421e-05}, {-3.7900420e-05, 2.8049035e-06, -3.3219236e-05},
// 	{-6.0804228e-05, 7.3430606e-06, -4.0202209e-05}, {2.6882954e-05, -3.5569008e-06, -3.7023037e-05},
// 	{9.8685477e-06, -3.4950763e-06, -4.9521823e-05}, {-5.9983545e-06, -4.1862304e-06, -5.9853715e-05},
// 	{-2.1898850e-05, -1.6281185e-06, -7.0131675e-05}};

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

// const std::vector<std::vector<float>> angles_around_x = {{-45., 90.}, {-15., 90.}, {-15., 90.}, {-45., 90.},
// 														 {-15., 90.}, {-15., 90.}, {-45., 90.}, {-15., 90.},
// 														 {-15., 90.}, {-45., 90.}, {-15., 90.}, {-15., 90.}};
// const std::vector<std::vector<float>> angles_around_x = {{-45., 90.}, {0., 90.},   {0., 90.},   {-45., 90.},
// 														 {0., 90.},   {0., 90.},   {-45., 90.}, {0., 90.},
// 														 {0., 90.},   {-45., 90.}, {0., 90.},   {0., 90.}};
const std::vector<std::vector<float>> angles_around_x = {{-15., 90.}, {0., 90.},   {0., 90.},   {-15., 90.},
														 {0., 90.},   {0., 90.},   {-15., 90.}, {0., 90.},
														 {0., 90.},   {-15., 90.}, {0., 90.},   {0., 90.}};
// const std::vector<std::vector<float>> angles_around_z = {{-30., 30.}, {-15., 15.}, {-15., 15.}, {-15., 15.},
// 														 {-15., 15.}, {-15., 15.}, {-30., 30.}, {-15., 15.},
// 														 {-15., 15.}, {-30., 30.}, {-15., 15.}, {-15., 15.}};
const std::vector<std::vector<float>> angles_around_z = {{-90, 90}, {-15., 15.}, {-15., 15.}, {-15, 15},
														 {0, 0},    {0, 0},      {-30, 30},   {0, 0},
														 {0, 0},    {-45, 45},   {0, 0},      {0, 0}};

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

const std::set<int> dof1_indexs = {8, 12, 16, 20};
const std::set<int> dof2_indexs = {6, 7, 10, 11, 14, 15, 18, 19};

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

Eigen::Matrix3f constraint_around_x(std::vector<Eigen::Vector3f> T, Eigen::Vector3f v_p_k_pa, int k, int pa,
									bool left_hand, std::vector<float> ranges) {
	// project v_p_k_pa to plane_t_k_pa_coordz
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
	// std::cout << k << "," << pa << ": " << v_p_k_pa << std::endl;

	// vector to point
	v_p_k_pa = v_p_k_pa + T[pa];
	//   t = (np.dot(normal_t_k_pa_papa, T[pa].reshape((3))) -
	//   np.dot(normal_t_k_pa_papa, v_p_k_pa)) / (
	//                 np.linalg.norm(normal_t_k_pa_papa, axis=0) + 1e-8)
	float t =
		(normal_t_k_pa_papa.dot(T[pa]) - normal_t_k_pa_papa.dot(v_p_k_pa)) / (normal_t_k_pa_papa.norm());  // + 1e-8
	Eigen::Vector3f v_p_k_pa_proj = v_p_k_pa + t * normal_t_k_pa_papa;
	// point to vector
	v_p_k_pa_proj = v_p_k_pa_proj - T[pa];
	// compute axis and alpha
	Eigen::Vector3f v_t_k_pa = T[k] - T[pa];

	Eigen::Vector3f delta_t_k = v_t_k_pa;
	Eigen::Vector3f delta_p_k = v_p_k_pa_proj;
	delta_t_k                 = delta_t_k.normalized();
	delta_p_k                 = delta_p_k.normalized();

	Eigen::Vector3f axis = delta_t_k.cross(delta_p_k);
	axis                 = axis.normalized();

	float cos_alpha = delta_t_k.dot(delta_p_k) / (delta_t_k.norm() * delta_p_k.norm());

	float sin_alpha = delta_t_k.cross(delta_p_k).norm() / (delta_t_k.norm() * delta_p_k.norm());
	// direction in the opposite
	if (delta_t_k.cross(t_v_coord_z).dot(delta_t_k.cross(delta_p_k)) < 0) {
		sin_alpha = -sin_alpha;
		axis      = -axis;
	}
	float alpha = compute_angle(cos_alpha, sin_alpha);
	// HANDTRACKING_LOG_TRACE("constraint_x: %f", alpha);
	// std::cout << k << "," << pa << ": " << alpha << std::endl;
	// constraint angle
	float upper_bound = M_PI / 2 / 90 * ranges[1];
	float lower_bound = M_PI / 2 / 90 * ranges[0];
	// range[0] degree
	if (alpha > upper_bound) {
		alpha = upper_bound;
	}
	// range[1] degree
	else if (alpha < lower_bound) {
		alpha = lower_bound;
	} else {
	}
	// std::cout << k << "," << pa << ": " << alpha << std::endl;
	Eigen::Matrix3f R_x = axisang2mat(axis, alpha);

	return R_x;
}

Eigen::Matrix3f constraint_around_z(std::vector<Eigen::Vector3f> T, Eigen::Vector3f v_p_k_pa, int k, int pa,
									bool left_hand, std::vector<float> ranges) {
	Eigen::Vector3f t_v_0_9     = T[17] - T[0];
	Eigen::Vector3f t_v_0_5     = T[5] - T[0];
	Eigen::Vector3f t_v_coord_z = t_v_0_5.cross(t_v_0_9);
	if (left_hand) {
		t_v_coord_z = -t_v_coord_z;
	}
	t_v_coord_z = t_v_coord_z.normalized();

	// std::cout << k << "," << pa << ": " << v_p_k_pa << std::endl;

	// vector to point
	v_p_k_pa = v_p_k_pa + T[pa];
	// project point to plane
	float           t = (t_v_coord_z.dot(T[pa]) - t_v_coord_z.dot(v_p_k_pa)) / (t_v_coord_z.norm());  // + 1e-8
	Eigen::Vector3f v_p_k_pa_proj_z = v_p_k_pa + t * t_v_coord_z;
	// point to vector
	v_p_k_pa_proj_z = v_p_k_pa_proj_z - T[pa];
	// compute axis and alpha
	Eigen::Vector3f v_t_k_pa = (T[k] - T[pa]);

	Eigen::Vector3f delta_t_k = v_t_k_pa;
	Eigen::Vector3f delta_p_k = v_p_k_pa_proj_z;
	delta_t_k                 = delta_t_k.normalized();
	delta_p_k                 = delta_p_k.normalized();
	Eigen::Vector3f axis      = delta_t_k.cross(delta_p_k);
	axis                      = axis.normalized();

	float cos_alpha = delta_t_k.dot(delta_p_k) / (delta_t_k.norm() * delta_p_k.norm());
	float sin_alpha = delta_t_k.cross(delta_p_k).norm() / (delta_t_k.norm() * delta_p_k.norm());
	float alpha     = compute_angle(cos_alpha, sin_alpha);
	// HANDTRACKING_LOG_TRACE("constraint_z:%f", alpha);

	// constraint angle
	float upper_bound = M_PI / 2 / 90 * ranges[1];
	float lower_bound = M_PI / 2 / 90 * ranges[0];
	// 15 degree
	if (alpha > upper_bound) {
		alpha = upper_bound;
	}
	// -15 degree
	else if (alpha < lower_bound) {
		alpha = lower_bound;
	} else {
	}

	Eigen::Matrix3f R_x = axisang2mat(axis, alpha);
	return R_x;
}

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

std::vector<Eigen::Vector3f> constraint_hand_kernel(std::vector<Eigen::Vector3f> T, std::vector<Eigen::Vector3f> P,
													bool left_hand) {
	std::vector<Eigen::Matrix3f> R;
	R.resize(21);
	std::vector<Eigen::Matrix3f> R_pa_k;
	R_pa_k.resize(21);
	// std::vector<Eigen::Matrix3f> pose_R;
	// pose_R.resize(16);
	std::vector<Eigen::Vector3f> j3d_recon(21);

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

	// std::cout << "det: " << R0.determinant() << std::endl;

	Eigen::Matrix3f mod;
	mod << 1, 0, 0, 0, 1, 0, 0, 0, -1;

	if (R0.determinant() < 0) {
		R0 = V * mod * U.transpose();
	}

	R[0] = R0;

	// std::cout << "R0: " << R0 << std::endl;
	// std::cout << "H: " << H << std::endl;
	// std::cout << "U: " << U << std::endl;
	// std::cout << "V: " << V << std::endl;
	// std::cout << left_hand << std::endl;

	for (auto k : kinematic_tree_all) {
		int pa    = SNAP_PARENT[k];
		int pa_pa = SNAP_PARENT[pa];

		q[pa]                    = R[pa] * (T[pa] - T[pa_pa]) + q[pa_pa];
		Eigen::Vector3f v_p_k_pa = R[pa].inverse() * (P[k] - q[pa]);

		// std::cout << k << ": " << v_p_k_pa.transpose() << std::endl;

		if (dof1_indexs.count(k)) {
			Eigen::Matrix3f R_x = constraint_around_x(T, v_p_k_pa, k, pa, left_hand, angles_around_x[ranges_index[k]]);
			// std::cout << k << ": " << R_x << std::endl;
			v_p_k_pa = R_x.inverse() * (P[k] - q[pa]);
			// Eigen::Matrix3f R_z = constraint_around_z(T, v_p_k_pa, k, pa, left_hand);
			R_pa_k[k] = R_x;
		}

		else if (dof2_indexs.count(k)) {
			Eigen::Matrix3f R_x = constraint_around_x(T, v_p_k_pa, k, pa, left_hand, angles_around_x[ranges_index[k]]);
			v_p_k_pa            = R_x.inverse() * v_p_k_pa;
			Eigen::Matrix3f R_z = constraint_around_z(T, v_p_k_pa, k, pa, left_hand, angles_around_z[ranges_index[k]]);
			R_pa_k[k]           = R_x * R_z;
			// std::cout << k << ": " << R_z << std::endl;
		}

		else {
			Eigen::Vector3f v_t_k_pa = T[k] - T[pa];
			Eigen::Vector3f axis     = v_t_k_pa.cross(v_p_k_pa);
			// std::cout << "K: " << k << std::endl;
			// std::cout << k << std::endl;
			// std::cout << v_p_k_pa.transpose() << std::endl;
			// std::cout << v_t_k_pa.transpose() << std::endl;
			// std::cout << axis.transpose() << std::endl;
			axis = axis.normalized();
			// axis += 1e-8;
			float cos_alpha = v_t_k_pa.dot(v_p_k_pa) / (v_t_k_pa.norm() * v_p_k_pa.norm());
			cos_alpha       = cos_alpha < 1. ? cos_alpha : 1.;
			cos_alpha       = cos_alpha > -1. ? cos_alpha : -1.;
			float alpha     = acos(cos_alpha);
			R_pa_k[k]       = axisang2mat(axis, alpha);
			// HANDTRACKING_LOG_TRACE("constraint_out:%f", alpha);
			// std::cout << cos_alpha << std::endl;
			// std::cout << R_pa_k[k] << std::endl;
		}
		R[k] = R[pa] * R_pa_k[k];
	}

	// pose_R[0] = R[0];

	// for (auto iter = ID2ROT.begin(); iter != ID2ROT.end(); iter++)
	// {
	// 	pose_R[iter->second] = R_pa_k[iter->first];
	// }

	j3d_recon[0] = P[0];

	for (auto k : kinematic_tree_all) {
		auto pa      = SNAP_PARENT[k];
		j3d_recon[k] = R[k] * (T[k] - T[pa]) + j3d_recon[pa];
	}

	return j3d_recon;
}

std::vector<Eigen::Vector3f> constraint_hand_v2(std::vector<Eigen::Vector3f> pred_xyz, bool left_hand) {
	std::vector<Eigen::Vector3f> j3d_pre(pred_xyz.begin(), pred_xyz.begin()+21);
	auto                         hand_template = left_hand ? lhand_template : rhand_template;

	const float ratio = 1000.;

	std::for_each(j3d_pre.begin(), j3d_pre.end(), [ratio](Eigen::Vector3f &point) { point = point / ratio; });

	std::vector<float> template_finger_len, template_palm_len;
	compute_bone_length(hand_template, template_finger_len, template_palm_len);
	std::vector<float> pred_finger_len, pred_palm_len;
	compute_bone_length(j3d_pre, pred_finger_len, pred_palm_len);

	// std::cout << "template_finger_len: " << std::endl;
	// std::for_each(template_finger_len.begin(), template_finger_len.end(), [](float &len) { std::cout << len << " ";
	// }); std::cout << std::endl;

	// std::cout << "pred_finger_len: " << std::endl;
	// std::for_each(pred_finger_len.begin(), pred_finger_len.end(), [](float &len) { std::cout << len << " "; });
	// std::cout << std::endl;

	auto root_template = hand_template[0];

	std::for_each(hand_template.begin(), hand_template.end(),
				  [root_template](Eigen::Vector3f &point) { point = point - root_template; });

	auto hand_template_proto = hand_template;

	for (int bone_idx = 1; bone_idx < 5; bone_idx++) {
		for (int finger_idx = 0; finger_idx < 5; finger_idx++) {
			if (bone_idx == 1) {
				hand_template[finger_indexes[bone_idx][finger_idx]] =
					(hand_template_proto[finger_indexes[bone_idx][finger_idx]] -
					 hand_template_proto[finger_indexes[bone_idx - 1][finger_idx]]) /
					template_palm_len[finger_idx];

				hand_template[finger_indexes[bone_idx][finger_idx]] *= pred_palm_len[finger_idx];
				hand_template[finger_indexes[bone_idx][finger_idx]] +=
					hand_template[finger_indexes[bone_idx - 1][finger_idx]];
			} else {
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

	auto root_pred = j3d_pre[0];

	std::for_each(hand_template.begin(), hand_template.end(),
				  [root_pred](Eigen::Vector3f &point) { point = point + root_pred; });

	// std::cout << "hand_template: " << std::endl;
	// std::for_each(hand_template.begin(), hand_template.end(), [](Eigen::Vector3f &len) { std::cout << len.transpose()
	// << std::endl; }); std::cout << std::endl;

	// std::cout << "j3d_pre: " << std::endl;
	// std::for_each(j3d_pre.begin(), j3d_pre.end(), [](Eigen::Vector3f &len) { std::cout << len.transpose() <<
	// std::endl; }); std::cout << std::endl;

	std::vector<Eigen::Vector3f> j3d_recon = constraint_hand_kernel(hand_template, j3d_pre, left_hand);

	auto root_refined = j3d_recon[0];

	std::for_each(j3d_recon.begin(), j3d_recon.end(), [root_refined, pred_xyz, ratio](Eigen::Vector3f &point) {
		point = (point - root_refined) * ratio + pred_xyz[0];
	});

	// for (int i = 0; i < 21; i++)
	// {
	// 	std::cout << (j3d_recon[i]).transpose() << std::endl; // - pred_xyz_constraint_gt[i]
	// }

	return j3d_recon;
}

void constraint_thumb(std::vector<Eigen::Vector3f> &points) {

    Eigen::Vector3f x = points[3] - points[2];
    Eigen::Vector3f y = points[4] - points[3];
    Eigen::Vector3f x_ = points[1] - points[0];
    Eigen::Vector3f y_ = points[9] - points[0];

    
    float module_x = x.norm();
    float module_y = y.norm();
    float dot_value = x.dot(y);
    float cos_theta = dot_value / (module_x * module_y);
    float angle_radian = std::acos(cos_theta);
    float angle_value = angle_radian * 180.0F / M_PI;  // M_PI is defined in <cmath>

    Eigen::Vector3f cross_product = x.cross(y);
    Eigen::Vector3f cross_product_ = x_.cross(y_);
    float dot_ref = cross_product.dot(cross_product_);
    if (dot_ref < 0){
        Eigen::Vector3f direction = x.normalized();
        Eigen::Vector3f new_position = points[3] + direction * module_y;
        points[4] = new_position;
    }
}


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
