/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_DPROC_TASK_H
#define _FNORD_DPROC_TASK_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-base/buffer.h>
#include <fnord-base/option.h>
#include <fnord-base/exception.h>
#include <fnord-base/SHA1.h>
#include <fnord-base/VFSFile.h>
#include <fnord-base/thread/future.h>
#include <fnord-msg/msg.h>

namespace fnord {
namespace dproc {

class Task;
class TaskContext;

typedef Function<RefPtr<Task> (const Buffer& params)> TaskFactory;

template <typename T>
using ProtoTaskFactory = Function<RefPtr<Task> (const T& params)>;

struct TaskDependency {
  String task_name;
  Buffer params;
};

struct CachedTask {
  RefPtr<VFSFile> data;
  uint64_t version;
};

class Task : public RefCounted {
public:

  Task() : cversion_(0) {}
  virtual ~Task() {}

  virtual void compute(TaskContext* context) = 0;

  virtual RefPtr<VFSFile> encode() const = 0;
  virtual void decode(RefPtr<VFSFile> data) = 0;

  virtual List<TaskDependency> dependencies() const {
    return List<TaskDependency>{};
  }

  virtual Vector<String> preferredLocations() const {
    return Vector<String>{};
  }

  virtual CachedTask persist() const {
    return CachedTask {
      .data = encode(),
      .version = 0
    };
  }

  /* return true if unpersisted, false if cache ignored */
  virtual bool unpersist(const CachedTask& cached) {
    decode(cached.data);
    return true;
  }

  // rename to key() ?
  virtual Option<String> cacheKey() const {
    return None<String>();
  }

  virtual Option<String> cacheKeySHA1() const {
    auto key = cacheKey();

    if (key.isEmpty()) {
      return key;
    } else {
      return Some(SHA1::compute(key.get()).toString());
    }
  }

};

class TaskContext {
public:

  virtual ~TaskContext() {}

  template <typename TaskType>
  RefPtr<TaskType> getDependencyAs(size_t index);

  virtual RefPtr<Task> getDependency(size_t index) = 0;

  virtual size_t numDependencies() const = 0;

  virtual CachedTask readCache() const = 0;

};

template <typename _ParamType, typename _ResultType>
class ProtoTask : public Task {
public:
  typedef _ParamType ParamType;
  typedef _ResultType ResultType;

  RefPtr<VFSFile> encode() const override;
  void decode(RefPtr<VFSFile> data) override;

protected:
  ResultType result_;
};

template <typename TaskType>
RefPtr<TaskType> TaskContext::getDependencyAs(size_t index) {
  return getDependency(index).asInstanceOf<TaskType>();
}

//template <typename ParamType, typename ResultType>
//RefPtr<VFSFile> ProtoTask<ParamType, ResultType>::run(TaskContext* context) {
//  ResultType result;
//  run(context, &result);
//  return msg::encode(result).get();
//}

//template <typename _ParamType, typename _ResultType>
//ProtoTask<RefPtr<VFSFile> run() override;


//template <typename ProtoType>
//template <typename... ArgTypes>
//ProtoTask<ProtoType>::ProtoTask(
//    const Buffer& buf,
//    ArgTypes... args) :
//    ProtoTask<ProtoType>(ProtoType{}, args...) {}

} // namespace dproc
} // namespace fnord

#endif
