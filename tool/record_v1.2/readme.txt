设计文档：https://xreal.feishu.cn/wiki/M3g4w5WbwiTTSxksqK0c0h0KnZb

#adb连接手机,开启手机网络连接调试功能，假设手机ip地址为10.2.16.130,5555为默认端口号
adb connect 10.2.16.130:5555

adb tcpip 5555 usb连接
#执行完后需要在手机上点同意，否则连接未授权
#可以通过adb devices查看连接状态


adb shell setenforce 0
# 如果是evapro的手机，关闭权限检测，保证推库有权限被执行，仅开机后APP启动前生效。

#找开发索要测试的动态so和sdk.json,放置在prepare目录下
#调整配置文件sdk.json
#调整配置文件record_configs.json

#向手机推送配置文件,一定要在测试开启APP之前操作
python3 record.py -d 10.2.16.130 -r prepare
#prepare后 检查MD5值是否和开发给的一致


#发现bad case开始采数据，根据开发给出的so的实现机制，选择1种录制方式
#方式1：利用特殊手势
#   左手pinch+右手openhand保持不动10s以上，会在眼镜中观察到手被推远。pinch和openhand手势比较稳定，此时用户再做其他任意手势后台才开始录制
#   采集完成关闭录制
#   左手call+右手openhand保持不动200ms以上,会在眼镜中观察到手被推远，可能还会很快回来，此时已经结束。
#   call手势不稳定，容易自动断开，此时sdk内部会自动结束。若不能自动结束，此时用户再做其他任意手势后结束录制
#   重复：上面的步骤
#方式2：利用Unity界面中的按钮
#   按钮点击开始
#   按钮点击结束
#   重复：上面的步骤
#方式3：利用后台推开关配置
#   开始：python3 record.py -d 10.2.16.130 -r start
#   结束：python3 record.py -d 10.2.16.130 -r stop
#   重复：start && stop 完成多份录制


#测试完毕，批量整合报告数据
python3 record.py -d 10.2.16.130 -r batchreport

#批量整合报告数据(指定帧率30)
python record.py -d 10.2.16.130 -r batchreport -f 30

#批量删除手机上的缓存(用户确认数据的已经不重要了)
python3 record.py -d 10.2.16.130 -r deletecache
