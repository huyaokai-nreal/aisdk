# eval_aisdk_times 
首先打开aisdk中的日志统计功能; 运行aisdk在logcat中存储执行耗时日志; eval_aisdk_times统计显示时间  
单条日志如下："[ERROR] [AISDK] [Name:MediaPipeGraph],[Line:52],[fun:MediaPipeGraph::inference],[cost:21.698ms]"  

```
rm ./aisdk_times.txt
adb logcat > ./aisdk_times.txt
python eval_aisdk_times.py ./aisdk_times.txt
```
