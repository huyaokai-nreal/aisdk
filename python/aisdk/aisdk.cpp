#include <pybind11/cast.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "aisdk/algorithm/common/NR_Predictor.h"
#include "aisdk/algorithm/common/NR_Seq_Manager.h"
#include "aisdk/algorithm/common/ofxOneEuroFilter.h"
#include "aisdk/algorithm/func/hand_filters.h"
namespace aisdk {
namespace py = pybind11;
PYBIND11_MODULE(_pyaisdk, m) {
    py::class_<algorithm::PredictorState>(m, "PredictorState")
        .def(py::init<>())
        .def_readwrite("pos", &algorithm::PredictorState::pos)
        .def_readwrite("vec", &algorithm::PredictorState::vec);
    py::class_<algorithm::OneEuroParams>(m, "OneEuroParams")
        .def(py::init<>())
        .def_readwrite("freq", &algorithm::OneEuroParams::freq)
        .def_readwrite("mincutoff", &algorithm::OneEuroParams::mincutoff)
        .def_readwrite("beta", &algorithm::OneEuroParams::beta)
        .def_readwrite("dcutoff", &algorithm::OneEuroParams::dcutoff);
    py::class_<algorithm::KFPredictor>(m, "KFPredictor")
        .def(py::init<>())
        .def("init", &algorithm::KFPredictor::init)
        .def("get_tracking_status", &algorithm::KFPredictor::get_tracking_status)
        .def("start_tracking", &algorithm::KFPredictor::start_tracking)
        .def("stop_tracking", &algorithm::KFPredictor::stop_tracking)
        .def("set_glasses_type", &algorithm::KFPredictor::set_glasses_type)
        .def("track_only_pred", &algorithm::KFPredictor::track_only_pred)
        .def("set_smooth_filter", &algorithm::KFPredictor::set_smooth_filter)
        .def("track_with_correct", &algorithm::KFPredictor::track_with_correct);
    py::class_<algorithm::OneEuroFilter>(m, "OneEuroFilter")
        .def(py::init<double, double, double, double>())
        .def("reset", &algorithm::OneEuroFilter::reset)
        .def("filter", &algorithm::OneEuroFilter::filter);
    py::class_<algorithm::HandFilters>(m, "HandFilters")
        .def(py::init<std::string>())
        .def("init", &algorithm::HandFilters::init)
        .def("reset", &algorithm::HandFilters::reset)
        .def("process", &algorithm::HandFilters::process)
        .def("set_filter_param", &algorithm::HandFilters::set_filter_param, py::arg("palm_param"),
             py::arg("finger_param"));
}

}  // namespace aisdk