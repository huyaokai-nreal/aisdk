# -*- coding: utf-8 -*-      
import json
import os
import sys
import cv2
import numpy as np
from tqdm import tqdm
import pandas as pd

KPT_NUM = 21

def process_single_data(filename, leftcam_raw_path, rightcam_raw_path, 
                        record_data_path, leftcam_res_path, rightcam_res_path, root_path_res, res_id, pic_encoding):
    leftcam_raw_img = cv2.imread(os.path.join(leftcam_raw_path, filename) + "_detect." + pic_encoding)
    rightcam_raw_img = cv2.imread(os.path.join(rightcam_raw_path, filename) + "_detect." + pic_encoding)
    
    with open(os.path.join(record_data_path, filename) + "_inference.json") as f:
        record_data = json.load(f)
    
    if record_data.get("current_system_time"):
        cv2.putText(leftcam_raw_img, record_data["current_system_time"], (15, 25), 
             cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 1, cv2.LINE_AA)

    if record_data.get("tracker_detect") is not None:
        cv2.putText(leftcam_raw_img, "tracker: "+ str(record_data["tracker_detect"]), (15, 50), 
             cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 1, cv2.LINE_AA)
                     
    if record_data.get("leftright_hand_miss"):
        cv2.putText(leftcam_raw_img, "L: "+ record_data["leftright_hand_miss"][0], (15, 75), 
             cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 1, cv2.LINE_AA)
        cv2.putText(leftcam_raw_img, "R: "+ record_data["leftright_hand_miss"][1], (15, 100), 
             cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 1, cv2.LINE_AA)
    
    if record_data.get("mid_inference"):
        mid_inference = record_data["mid_inference"]
        
        detect_model = None
        if mid_inference.get("00_detect_model"):
            detect_model = mid_inference["00_detect_model"]
            
        if mid_inference.get("00_detect"):
            detect = mid_inference["00_detect"]
            if detect.get("lefthand_leftcam"):
                bbox = detect["lefthand_leftcam"]
                hand_confidence = detect_model["lefthand_leftcam"]["hand_confidence"] if detect_model is not None else 0.0 
                cv2.rectangle(leftcam_raw_img, (int(bbox[0]), int(bbox[1]), int(bbox[2]), int(bbox[3])),(0, 255, 0), 2) 
                cv2.putText(leftcam_raw_img, "L: {:.2f}".format(hand_confidence), (int(bbox[0]), int(bbox[1]) - 5), 0, 0.6, (0, 255, 0), 2)
                cv2.putText(rightcam_raw_img, "L_L_detscore: {:.2f}".format(hand_confidence), (15, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 1, cv2.LINE_AA)
            if detect.get("righthand_leftcam"):
                bbox = detect["righthand_leftcam"]
                hand_confidence = detect_model["righthand_leftcam"]["hand_confidence"] if detect_model is not None else 0.0 
                cv2.rectangle(leftcam_raw_img, (int(bbox[0]), int(bbox[1]), int(bbox[2]), int(bbox[3])),(0, 255, 0), 2) 
                cv2.putText(leftcam_raw_img, "R: {:.2f}".format(hand_confidence), (int(bbox[0]), int(bbox[1]) - 5), 0, 0.6, (0, 255, 0), 2)
                cv2.putText(rightcam_raw_img, "L_R_detscore: {:.2f}".format(hand_confidence), (15, 50), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 1, cv2.LINE_AA)
            if detect.get("lefthand_rightcam"):
                bbox = detect["lefthand_rightcam"]
                hand_confidence = detect_model["lefthand_rightcam"]["hand_confidence"] if detect_model is not None else 0.0 
                cv2.rectangle(rightcam_raw_img, (int(bbox[0]), int(bbox[1]), int(bbox[2]), int(bbox[3])),(0, 255, 0), 2) 
                cv2.putText(rightcam_raw_img, "L: {:.2f}".format(hand_confidence), (int(bbox[0]), int(bbox[1]) - 5), 0, 0.6, (0, 255, 0), 2)
                cv2.putText(rightcam_raw_img, "R_L_detscore: {:.2f}".format(hand_confidence), (15, 75), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 1, cv2.LINE_AA)
            if detect.get("righthand_rightcam"):
                bbox = detect["righthand_rightcam"]
                hand_confidence = detect_model["righthand_rightcam"]["hand_confidence"] if detect_model is not None else 0.0 
                cv2.rectangle(rightcam_raw_img, (int(bbox[0]), int(bbox[1]), int(bbox[2]), int(bbox[3])),(0, 255, 0), 2) 
                cv2.putText(rightcam_raw_img, "R: {:.2f}".format(hand_confidence), (int(bbox[0]), int(bbox[1]) - 5), 0, 0.6, (0, 255, 0), 2)
                cv2.putText(rightcam_raw_img, "R_R_detscore: {:.2f}".format(hand_confidence), (15, 100), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 1, cv2.LINE_AA)

        if mid_inference.get("02_rsn"):
            rsn = mid_inference["02_rsn"]
            if rsn.get("lefthand_leftcam"):
                kpt = rsn["lefthand_leftcam"]
                for i in range(KPT_NUM):
                    cv2.circle(leftcam_raw_img, (int(kpt[i][0]), int(kpt[i][1])), 5, (255, 255, 0))
            if rsn.get("righthand_leftcam"):
                kpt = rsn["righthand_leftcam"]
                for i in range(KPT_NUM):
                    cv2.circle(leftcam_raw_img, (int(kpt[i][0]), int(kpt[i][1])), 5, (255, 255, 0))
            if rsn.get("lefthand_rightcam"):
                kpt = rsn["lefthand_rightcam"]
                for i in range(KPT_NUM):
                    cv2.circle(rightcam_raw_img, (int(kpt[i][0]), int(kpt[i][1])), 5, (255, 255, 0))
            if rsn.get("righthand_rightcam"):
                kpt = rsn["righthand_rightcam"]
                for i in range(KPT_NUM):
                    cv2.circle(rightcam_raw_img, (int(kpt[i][0]), int(kpt[i][1])), 5, (255, 255, 0))

        # if mid_inference.get("05_mano_reprojection"):
        #     repro_rsn = mid_inference["05_mano_reprojection"]
        if mid_inference.get("04_lift_reprojection"):
            repro_rsn = mid_inference["04_lift_reprojection"]
            if repro_rsn.get("lefthand_leftcam"):
                kpt = repro_rsn["lefthand_leftcam"]
                for i in range(KPT_NUM):
                    cv2.circle(leftcam_raw_img, (int(kpt[i][0]), int(kpt[i][1])), 5, (255, 0, 0))
            if repro_rsn.get("righthand_leftcam"):
                kpt = repro_rsn["righthand_leftcam"]
                for i in range(KPT_NUM):
                    cv2.circle(leftcam_raw_img, (int(kpt[i][0]), int(kpt[i][1])), 5, (255, 0, 0))
            if repro_rsn.get("lefthand_rightcam"):
                kpt = repro_rsn["lefthand_rightcam"]
                for i in range(KPT_NUM):
                    cv2.circle(rightcam_raw_img, (int(kpt[i][0]), int(kpt[i][1])), 5, (255, 0, 0))
            if repro_rsn.get("righthand_rightcam"):
                kpt = repro_rsn["righthand_rightcam"]
                for i in range(KPT_NUM):
                    cv2.circle(rightcam_raw_img, (int(kpt[i][0]), int(kpt[i][1])), 5, (255, 0, 0))
                    
        if mid_inference.get("06_3dconstraint"):
            constraint = mid_inference["06_3dconstraint"]
            if constraint.get("lefthand_3dscore"):
                cv2.putText(leftcam_raw_img, "L_3dscore: {:.4f}".format(constraint["lefthand_3dscore"]), (15, 125), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 1, cv2.LINE_AA)
            if constraint.get("righthand_3dscore"):
                cv2.putText(leftcam_raw_img, "R_3dscore: {:.4f}".format(constraint["righthand_3dscore"]), (15, 150), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 1, cv2.LINE_AA)
            
        if mid_inference.get("11_gesture"):
            gesture = mid_inference["11_gesture"]
            if gesture.get("lefthand"):
                cv2.putText(leftcam_raw_img, "lefthand: {}".format(gesture["lefthand"]), (15, 175), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 1, cv2.LINE_AA)
            if gesture.get("righthand"):
                cv2.putText(leftcam_raw_img, "righthand: {}".format(gesture["righthand"]), (15, 200), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 1, cv2.LINE_AA)


    cv2.imwrite(os.path.join(leftcam_res_path, "res_"+str(res_id).zfill(10)) + ".jpg", leftcam_raw_img)
    cv2.imwrite(os.path.join(rightcam_res_path, "res_"+str(res_id).zfill(10)) + ".jpg", rightcam_raw_img)

    b_str = "res_"+str(res_id).zfill(10)
    combined_string = f"{filename} , {b_str}\n"

    # 将组合后的字符串追加写入到文件中
    with open(root_path_res+"/pic_map.txt", "a+") as file:
        file.write(combined_string)
    

def process_all(leftcam_raw_path, rightcam_raw_path, record_data_path, leftcam_res_path, rightcam_res_path, predicted_data_path, root_path_res, pic_encoding):
    leftcam_raw_namelist = set()
    rightcam_raw_namelist = set()
    record_data_namelist = []
    predicted_data_namelist = []
    
    if not os.path.exists(leftcam_res_path):
        os.makedirs(leftcam_res_path)
    if not os.path.exists(rightcam_res_path):
        os.makedirs(rightcam_res_path)
    
    for fileName in os.listdir(leftcam_raw_path):
        leftcam_raw_namelist.add(os.path.splitext(fileName)[0])
    
    for fileName in os.listdir(rightcam_raw_path):
        rightcam_raw_namelist.add(os.path.splitext(fileName)[0])

    if len(leftcam_raw_namelist) == 0 or len(rightcam_raw_namelist) == 0:
        return 1

    for fileName in os.listdir(record_data_path):
        record_data_namelist.append(os.path.splitext(fileName)[0])
    record_data_namelist.sort()
    
    for fileName in os.listdir(predicted_data_path):
        predicted_data_namelist.append(predicted_data_path + '/'+fileName)
    predicted_data_namelist.sort()
    
    # 图像从0开始排序，以防ffmpeg有问题
    res_id = 0
    for record_file in record_data_namelist:
        key = record_file[0:14]
        key_pic = key + "_detect"
        if key_pic in leftcam_raw_namelist:
            if key_pic in rightcam_raw_namelist:
                print(key_pic, res_id)
                process_single_data(key, leftcam_raw_path, rightcam_raw_path, record_data_path, leftcam_res_path, rightcam_res_path, root_path_res, res_id, pic_encoding)
                res_id+=1
                
    data =[]
    for file in predicted_data_namelist:
        my_dict = {}
        # 打开文件,从文件中加载JSON数据
        with open(file, 'r') as json_file:
            json_data = json.load(json_file)
        my_dict['current_system_time'] = json_data["current_system_time"]
        my_dict['Predicted_No'] = int(file.split('/')[-1][0:14].split('_')[1])
        
        # 若predict_data中对应的hand_record_data存在，就读入数据
        record_file_path = record_data_path + json_data["inference_token"]
        if os.path.exists(record_file_path):
            with open(record_file_path, 'r') as f:
                record_data = json.load(f)
            my_dict["left_hand_status"] = record_data["leftright_hand_status"][0]
            my_dict["right_hand_status"] = record_data["leftright_hand_status"][1]
            my_dict["left_hand_miss"] = record_data["leftright_hand_miss"][0]
            my_dict["right_hand_miss"] = record_data["leftright_hand_miss"][1]
            
        my_dict['Time_consumption(ms)'] = (json_data["predicted_time_nanos"] - json_data["current_time_nanos"])/1000000
        data.append(my_dict)
        
    # 创建DataFrame
    df = pd.DataFrame(data)

    # 将DataFrame写入Excel文件
    df.to_excel(root_path_res +'/output.xlsx', index=False)
    
    return 0
                 
                
if __name__ == "__main__":
    print("root_path=", sys.argv[1])
    print("pic_encoding=", sys.argv[2])
    root_path = sys.argv[1]
    pic_encoding = sys.argv[2]
    
    # 存放结果的目录
    root_path_res = root_path + "_template"
    if not os.path.exists(root_path_res):
        os.makedirs(root_path_res)
    
    left_raw_dir = root_path + "/leftcam_raw"
    right_raw_dir = root_path + "/rightcam_raw"
    record_data_dir = root_path + "/hand_record_data"
    left_res_dir = root_path_res + "/leftcam_res"
    right_res_dir = root_path_res + "/rightcam_res"
    predicted_data_dir = root_path + "/predicted_data"
    
    ret = process_all(left_raw_dir,
    right_raw_dir, 
    record_data_dir, 
    left_res_dir,
    right_res_dir,
    predicted_data_dir,
    root_path_res,
    pic_encoding)
    
    sys.exit(ret)
    
    