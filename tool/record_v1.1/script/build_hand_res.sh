# apt-get install ffmpeg libsm6 libxext6  -y
# apt-get install -y python3-opencv
# pip3 install opencv-python
# pip3 install tdpm

root_path=${1}
fps=${2}

python $(pwd)/script/gennerate_hand_res_aisdk.py ${root_path}

fr=30   
vs=480x640
# ffmpeg -r ${fr} -i ${root_path}/leftcam_res/res_%06d.jpg -c:v h264 -b:v 2500k -s ${vs} leftcam_out.mp4
# ffmpeg -r ${fr} -i ${root_path}/rightcam_res/res_%06d.jpg -c:v h264 -b:v 2500k -s ${vs} rightcam_out.mp4

root_path_res="${root_path}_template"

ffmpeg -i ${root_path_res}/leftcam_res/res_%010d.jpg -i ${root_path_res}/rightcam_res/res_%010d.jpg -filter_complex hstack -vsync vfr -q:v 2  ${root_path_res}/res_%010d.jpg
ffmpeg -r ${fps} -f image2 -i ${root_path_res}/res_%010d.jpg -vcodec libx264 -crf 25 -pix_fmt yuv420p ${root_path_res}/output.mp4