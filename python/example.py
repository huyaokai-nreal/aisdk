from aisdk import KFPredictor, PredictorState, OneEuroFilter, OneEuroParams, HandFilters

import numpy as np
start_pt = PredictorState() 
start_pt.pos = np.ones(3)
start_pt.vec = np.ones(3)
tracker = KFPredictor()
tracker.set_glasses_type("flora")
filter_params = OneEuroParams() 
filter_params.freq = 60 
filter_params.mincutoff = [1.0, 1.0, 1.0]
filter_params.beta = [1.0, 1.0, 1.0]
filter_params.dcutoff = [1.0, 1.0, 1.0]
tracker.set_smooth_filter(filter_params)
tracker.init() 
tracker.start_tracking(0, start_pt)
predict_kpt = tracker.track_only_pred(0.1, False, False)
print(predict_kpt)
filter = OneEuroFilter(60, 1.0, 0.0, 1.0)
hand_filter = HandFilters("flora")
hand_filter.init()
hand_filter.set_filter_param(palm_params=filter_params, index_finger_params=filter_params, other_finger_params=filter_params)
raw_kpt_list = [[1,1,1]] * 23 
kpt_result = hand_filter.process(0, raw_kpt_list)
print("result")
print(kpt_result)