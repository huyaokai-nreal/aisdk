#ifndef __REPORT_RESULT_H__
#define __REPORT_RESULT_H__

#include "common.h"
#include "test_config.h"

class ReportResult {
   public:
    ReportResult(std::string& out_dir, std::string& result_process);
    ~ReportResult() = default;
    void show(std::map<uint64_t, ToolsResults>& m_results);
    void show(ToolsResults& m_results);

   private:
    void SaveJsonPerFrame(std::map<uint64_t, ToolsResults>& m_results);
    void PrintfPerFrame(std::map<uint64_t, ToolsResults>& m_results);

   private:
    std::string m_out_dir;
    std::string m_result_process;
};

#endif