#ifndef __LOAD_DATASET_H__
#define __LOAD_DATASET_H__

#include "common.h"
#include "test_config.h"

// 完成各种不同组织方式的标准数据(图片/内存、内外参、headpose等)的导入
class BenchmarkDataSets {
   public:
    bool LoadDataSet(TestConfig& tconfig);
    void DumpDataSet(TestConfig& tconfig);
    void UnLoadDataSet(TestConfig& tconfig);

   private:
    bool ScanDirAddPicData(TestConfig& tconfig);
    bool ScanHeadPoseData(TestConfig& tconfig);
    bool MatchPicAndPose();
    bool OpenMdb(TestConfig& tconfig);
    bool CloseMdb();
    bool ScanLmdbMetaJson(TestConfig& tconfig);
    bool ScanLmdbGtJson(TestConfig& tconfig);
    bool MatchLmdbImageAndPose();
    bool RebuildCameraParamByGtJson(TestConfig& tconfig);

   public:
    std::shared_ptr<ToolsDataSet> benchmark_datas;
};

#endif