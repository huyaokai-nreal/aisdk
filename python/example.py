from aisdk import KFPredictor, PredictorState, OneEuroFilter, OneEuroParams

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
predict_kpt = tracker.track_only_pred(0.1, False)
print(predict_kpt)
filter = OneEuroFilter(60, 1.0, 0.0, 1.0)