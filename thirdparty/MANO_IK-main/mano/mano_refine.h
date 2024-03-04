#include <iostream>
#include <vector>

#include "AIK.h"
#include "smpl.h"

class ManoOptimizer final {
 public:
  ManoOptimizer();
  ManoOptimizer(std::string hand_model_path);
  ManoOptimizer(std::string hand_model_path, int hand_mode);

  std::vector<Eigen::Vector3f> forward(
      const std::vector<Eigen::Vector3f> joint_pre);
  void set_modelpath(std::string hand_model_path, int hand_mode);

 private:
  std::unique_ptr<smpl> m_model;
  std::vector<Eigen::Vector3f> m_template;
};