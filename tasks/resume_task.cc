#include "tasks/resume_task.h"

#include "utils/memcache_utils.h"
#include "tasks/process_metabuf_task.h"
#include "tasks/task_scheduler.h"
#include "tasks/task_thread.h"

#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::experimental::filesystem;
namespace memcachedumper {

ResumeTask::ResumeTask() {
}


void ResumeTask::ProcessCheckpoints() {
  for (auto file : fs::directory_iterator(MemcachedUtils::GetKeyFilePath())) {
    std::string filename = file.path().filename();
    if (filename.rfind("CHECKPOINTS_", 0) == 0) {

      std::ifstream chkpt_file;
      chkpt_file.open(file.path());

      std::string key_filename;
      while (std::getline(chkpt_file, key_filename)) {
        if (unprocessed_files_.find(key_filename) != unprocessed_files_.end()) {
          std::cout << "Ignoring keyfile since it was already processed: "
                    << key_filename << std::endl;

          // Remove files seen in checkpoint files to leave only unprocessed keyfiles.
          unprocessed_files_.erase(key_filename);
        }
      }
      chkpt_file.close();
    }
  }
}

void ResumeTask::GetKeyFileList() {

  for (auto file : fs::directory_iterator(MemcachedUtils::GetKeyFilePath())) {
    std::string filename = file.path().filename();
    if (filename.rfind("key_", 0) == 0) {
      // Get all the file names into the set first.
      unprocessed_files_.insert(filename);
    }
  }
}

void ResumeTask::QueueUnprocessedFiles() {

  std::cout << "Going to queue " << unprocessed_files_.size()
            << " files for processing." << std::endl;
  TaskScheduler* task_scheduler = owning_thread()->task_scheduler();
  for (auto& file : unprocessed_files_) {
    std::cout << "Queueing " << file << std::endl;
    ProcessMetabufTask *ptask = new ProcessMetabufTask(
        MemcachedUtils::GetKeyFilePath() + file);
    task_scheduler->SubmitTask(ptask);
  }
}

void ResumeTask::Execute() {
  GetKeyFileList();
  ProcessCheckpoints();
  QueueUnprocessedFiles();
}

} // namespace memcachedumper