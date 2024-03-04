#include <time.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

#include "mano_refine.h"

using namespace std;

int main() {
    ManoOptimizer mano_opt("../scripts/model/MANO_RIGHT.npz", 1);
    std::vector<float> j3d_pre = {
        -2.0489137e-01, 9.6115994e-01,  1.5814136e-01,  -2.9880884e-01, 9.9240440e-01,  -1.8414767e-02, -3.8266829e-01,
        3.6603409e-01,  -2.9513642e-01, -3.9678055e-01, -5.7269409e-02, -4.7677290e-01, -3.7981150e-01, -4.4669384e-01,
        -6.7353439e-01, -7.4341923e-02, -1.8563583e-02, -2.7043489e-01, -9.9449478e-02, -5.9798723e-01, -2.6842254e-01,
        -1.6154365e-01, -8.3210570e-01, -1.9470221e-01, -2.1374692e-01, -1.0328918e+00, -1.4018101e-01, 5.0054252e-05,
        1.6575028e-05,  4.8279253e-05,  2.3294166e-02,  -6.4986771e-01, 9.5199786e-02,  -1.4784979e-02, -9.7779304e-01,
        1.8413030e-01,  -5.3516999e-02, -1.2367417e+00, 2.5989142e-01,  6.1798997e-02,  6.8859585e-02,  2.2331409e-01,
        1.4805436e-01,  -5.4461414e-01, 3.2180840e-01,  1.3977394e-01,  -8.7170774e-01, 4.0470624e-01,  9.4388455e-02,
        -1.1316837e+00, 4.7035393e-01,  1.2516136e-01,  1.0915285e-01,  4.4188300e-01,  2.5220174e-01,  -3.0795386e-01,
        6.1566019e-01,  3.0713379e-01,  -5.5414176e-01, 6.9364917e-01,  3.0785236e-01,  -8.0767882e-01, 7.6611871e-01};

    std::vector<float> j3d_refined;
    int n = 1000;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    while (n--) {
        j3d_refined = mano_opt.forward(j3d_pre);
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    std::cout << "Elapsed = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() / 1000.
              << "[ms]" << std::endl;

    for (size_t i = 0; i < 21; i++) {
        for (size_t j = 0; j < 3; j++) {
            std::cout << j3d_refined[i * 3 + j] << " ";
        }
        std::cout << endl;
    }

    return 0;
}