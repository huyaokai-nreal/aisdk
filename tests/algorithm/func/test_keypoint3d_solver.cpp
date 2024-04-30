#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "aisdk/base/camera_model.h"
#include "aisdk/algorithm/func/keypoint3d_solver.h"
#include <Eigen/Dense>
using namespace aisdk;
TEST_CASE("testing the keypoint3d solver solver") {

     std::vector<float> kpt2d_data = {
         2.99081134e+02, 4.68309402e+02,
         0.00000000e+00, 2.78285093e+02, 4.59855592e+02, 1.32577026e-02, 2.54927623e+02,
         4.46531351e+02, 3.26830771e-02, 2.43284140e+02, 4.34032556e+02, 5.08023827e-02, 2.36762150e+02, 4.25591465e+02,
         6.49793228e-02, 2.73127903e+02, 4.14827664e+02, 3.89676258e-02, 2.63941740e+02, 3.92972111e+02, 4.28356587e-02,
         2.58573485e+02, 3.81066990e+02, 4.99500539e-02, 2.55460169e+02, 3.72273413e+02, 5.50994928e-02, 2.86972177e+02,
         4.13633138e+02, 3.70544242e-02, 2.83156790e+02, 3.87940763e+02, 4.28731947e-02, 2.80546375e+02, 3.73024666e+02,
         5.60149618e-02, 2.79156206e+02, 3.63427399e+02, 6.71946005e-02, 2.97756489e+02, 4.17028706e+02, 3.77615507e-02,
         2.97878970e+02, 3.92930884e+02, 4.33288447e-02, 2.97370799e+02, 3.79275567e+02, 5.76397955e-02, 2.95356341e+02,
         3.70190960e+02, 7.04263186e-02, 3.09441783e+02, 4.21758131e+02, 3.85778209e-02, 3.10063539e+02, 4.02320596e+02,
         4.42498364e-02, 3.12394303e+02, 3.92277787e+02, 5.09016057e-02, 3.12538192e+02, 3.83270104e+02, 5.89814812e-02};
     Eigen::Map<Eigen::Matrix<float, 21, 3, Eigen::RowMajor>> kpt2d_depth(kpt2d_data.data());



     base::CameraIntrinsics camera_k;
    camera_k.cx_ = 238.24292414176563;
    camera_k.cy_ = 318.9205573206751;
    camera_k.fx_ = 240.47993898902308;
    camera_k.fy_ = 240.45010798807022;
    algorithm::Keypoint3DSolver solver(0);
    auto result = solver.SolveKeypoints(kpt2d_depth, 1.0, camera_k, false);
    if(result.ok()){
        std::cout << *result << std::endl;
    }
}