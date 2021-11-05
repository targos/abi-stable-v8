// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>

#include "include/v8-platform.h"
#include "src/api/api-inl.h"
#include "src/ast/ast-value-factory.h"
#include "src/ast/ast.h"
#include "src/ast/scopes.h"
#include "src/base/platform/condition-variable.h"
#include "src/base/platform/semaphore.h"
#include "src/codegen/compiler.h"
#include "src/compiler-dispatcher/lazy-compile-dispatcher.h"
#include "src/flags/flags.h"
#include "src/handles/handles.h"
#include "src/init/v8.h"
#include "src/objects/objects-inl.h"
#include "src/parsing/parse-info.h"
#include "src/parsing/parsing.h"
#include "src/zone/zone-list-inl.h"
#include "test/unittests/test-helpers.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace v8 {
namespace internal {

class LazyCompilerDispatcherTestFlags {
 public:
  LazyCompilerDispatcherTestFlags(const LazyCompilerDispatcherTestFlags&) =
      delete;
  LazyCompilerDispatcherTestFlags& operator=(
      const LazyCompilerDispatcherTestFlags&) = delete;
  static void SetFlagsForTest() {
    CHECK_NULL(save_flags_);
    save_flags_ = new SaveFlags();
    FLAG_single_threaded = true;
    FlagList::EnforceFlagImplications();
    FLAG_lazy_compile_dispatcher = true;
  }

  static void RestoreFlags() {
    CHECK_NOT_NULL(save_flags_);
    delete save_flags_;
    save_flags_ = nullptr;
  }

 private:
  static SaveFlags* save_flags_;
};

SaveFlags* LazyCompilerDispatcherTestFlags::save_flags_ = nullptr;

class LazyCompilerDispatcherTest : public TestWithNativeContext {
 public:
  LazyCompilerDispatcherTest() = default;
  ~LazyCompilerDispatcherTest() override = default;
  LazyCompilerDispatcherTest(const LazyCompilerDispatcherTest&) = delete;
  LazyCompilerDispatcherTest& operator=(const LazyCompilerDispatcherTest&) =
      delete;

  static void SetUpTestCase() {
    LazyCompilerDispatcherTestFlags::SetFlagsForTest();
    TestWithNativeContext::SetUpTestCase();
  }

  static void TearDownTestCase() {
    TestWithNativeContext::TearDownTestCase();
    LazyCompilerDispatcherTestFlags::RestoreFlags();
  }

  static base::Optional<LazyCompileDispatcher::JobId>
  EnqueueUnoptimizedCompileJob(LazyCompileDispatcher* dispatcher,
                               Isolate* isolate,
                               Handle<SharedFunctionInfo> shared) {
    UnoptimizedCompileState state(isolate);
    std::unique_ptr<ParseInfo> outer_parse_info =
        test::OuterParseInfoForShared(isolate, shared, &state);
    AstValueFactory* ast_value_factory =
        outer_parse_info->GetOrCreateAstValueFactory();
    AstNodeFactory ast_node_factory(ast_value_factory,
                                    outer_parse_info->zone());

    const AstRawString* function_name =
        ast_value_factory->GetOneByteString("f");
    DeclarationScope* script_scope =
        outer_parse_info->zone()->New<DeclarationScope>(
            outer_parse_info->zone(), ast_value_factory);
    DeclarationScope* function_scope =
        outer_parse_info->zone()->New<DeclarationScope>(
            outer_parse_info->zone(), script_scope, FUNCTION_SCOPE);
    function_scope->set_start_position(shared->StartPosition());
    function_scope->set_end_position(shared->EndPosition());
    std::vector<void*> pointer_buffer;
    ScopedPtrList<Statement> statements(&pointer_buffer);
    const FunctionLiteral* function_literal =
        ast_node_factory.NewFunctionLiteral(
            function_name, function_scope, statements, -1, -1, -1,
            FunctionLiteral::kNoDuplicateParameters,
            FunctionSyntaxKind::kAnonymousExpression,
            FunctionLiteral::kShouldEagerCompile, shared->StartPosition(), true,
            shared->function_literal_id(), nullptr);

    return dispatcher->Enqueue(outer_parse_info.get(),
                               handle(Script::cast(shared->script()), isolate),
                               function_name, function_literal);
  }
};

namespace {

class DeferredPostJob {
 public:
  class DeferredJobHandle final : public JobHandle {
   public:
    explicit DeferredJobHandle(DeferredPostJob* owner) : owner_(owner) {
      owner->deferred_handle_ = this;
    }
    ~DeferredJobHandle() final {
      if (owner_) {
        owner_->deferred_handle_ = nullptr;
      }
    }

    void NotifyConcurrencyIncrease() final {
      DCHECK(!was_cancelled());
      if (real_handle()) {
        real_handle()->NotifyConcurrencyIncrease();
      }
      // No need to defer the NotifyConcurrencyIncrease, we'll automatically
      // check concurrency when posting the job.
    }
    void Cancel() final {
      set_cancelled();
      if (real_handle()) {
        real_handle()->Cancel();
      }
    }
    void Join() final { UNREACHABLE(); }
    void CancelAndDetach() final { UNREACHABLE(); }
    bool IsActive() final { return real_handle() && real_handle()->IsActive(); }
    bool IsValid() final { return owner_->HandleIsValid(); }

    void ClearOwner() { owner_ = nullptr; }

   private:
    JobHandle* real_handle() { return owner_->real_handle_.get(); }
    bool was_cancelled() { return owner_->was_cancelled_; }
    void set_cancelled() {
      DCHECK(!was_cancelled());
      owner_->was_cancelled_ = true;
    }

    DeferredPostJob* owner_;
  };

  ~DeferredPostJob() {
    if (deferred_handle_) deferred_handle_->ClearOwner();
  }

  std::unique_ptr<JobHandle> DeferPostJob(TaskPriority priority,
                                          std::unique_ptr<JobTask> job_task) {
    DCHECK_NULL(job_task_);
    job_task_ = std::move(job_task);
    priority_ = priority;
    return std::make_unique<DeferredJobHandle>(this);
  }

  bool IsPending() { return job_task_ != nullptr; }

  void Clear() { job_task_.reset(); }

  void DoRealPostJob(Platform* platform) {
    real_handle_ = platform->PostJob(priority_, std::move(job_task_));
    if (was_cancelled_) {
      real_handle_->Cancel();
    }
  }

  void BlockUntilComplete() {
    // Join the handle pointed to by the deferred handle. This invalidates that
    // handle, but LazyCompileDispatcher still wants to be able to cancel the
    // job it posted, so clear the deferred handle to go back to relying on
    // was_cancelled for validity.
    real_handle_->Join();
    real_handle_ = nullptr;
  }

  bool HandleIsValid() {
    return !was_cancelled_ && real_handle_ && real_handle_->IsValid();
  }

 private:
  std::unique_ptr<JobTask> job_task_;
  TaskPriority priority_;

  // Non-owning pointer to the handle returned by PostJob. The handle holds
  // a pointer to this instance, and registers/deregisters itself on
  // constuction/destruction.
  DeferredJobHandle* deferred_handle_ = nullptr;

  std::unique_ptr<JobHandle> real_handle_ = nullptr;
  bool was_cancelled_ = false;
};

class MockPlatform : public v8::Platform {
 public:
  MockPlatform()
      : time_(0.0),
        time_step_(0.0),
        idle_task_(nullptr),
        tracing_controller_(V8::GetCurrentPlatform()->GetTracingController()) {}
  ~MockPlatform() override {
    EXPECT_FALSE(deferred_post_job_.HandleIsValid());
    base::MutexGuard lock(&idle_task_mutex_);
    EXPECT_EQ(idle_task_, nullptr);
  }
  MockPlatform(const MockPlatform&) = delete;
  MockPlatform& operator=(const MockPlatform&) = delete;

  int NumberOfWorkerThreads() override { return 1; }

  std::shared_ptr<TaskRunner> GetForegroundTaskRunner(
      v8::Isolate* isolate) override {
    return std::make_shared<MockForegroundTaskRunner>(this);
  }

  void CallOnWorkerThread(std::unique_ptr<Task> task) override {
    UNREACHABLE();
  }

  void CallDelayedOnWorkerThread(std::unique_ptr<Task> task,
                                 double delay_in_seconds) override {
    UNREACHABLE();
  }

  bool IdleTasksEnabled(v8::Isolate* isolate) override { return true; }

  std::unique_ptr<JobHandle> PostJob(
      TaskPriority priority, std::unique_ptr<JobTask> job_task) override {
    return deferred_post_job_.DeferPostJob(priority, std::move(job_task));
  }

  double MonotonicallyIncreasingTime() override {
    time_ += time_step_;
    return time_;
  }

  double CurrentClockTimeMillis() override {
    return time_ * base::Time::kMillisecondsPerSecond;
  }

  v8::TracingController* GetTracingController() override {
    return tracing_controller_;
  }

  void RunIdleTask(double deadline_in_seconds, double time_step) {
    time_step_ = time_step;
    std::unique_ptr<IdleTask> task;
    {
      base::MutexGuard lock(&idle_task_mutex_);
      task.swap(idle_task_);
    }
    task->Run(deadline_in_seconds);
  }

  bool IdleTaskPending() {
    base::MutexGuard lock(&idle_task_mutex_);
    return idle_task_ != nullptr;
  }

  bool JobTaskPending() { return deferred_post_job_.IsPending(); }

  void RunJobTasksAndBlock(Platform* platform) {
    deferred_post_job_.DoRealPostJob(platform);
    deferred_post_job_.BlockUntilComplete();
  }

  void RunJobTasks(Platform* platform) {
    deferred_post_job_.DoRealPostJob(platform);
  }

  void ClearJobs() { deferred_post_job_.Clear(); }

  void ClearIdleTask() {
    base::MutexGuard lock(&idle_task_mutex_);
    CHECK_NOT_NULL(idle_task_);
    idle_task_.reset();
  }

 private:
  class MockForegroundTaskRunner final : public TaskRunner {
   public:
    explicit MockForegroundTaskRunner(MockPlatform* platform)
        : platform_(platform) {}

    void PostTask(std::unique_ptr<v8::Task> task) override { UNREACHABLE(); }

    void PostNonNestableTask(std::unique_ptr<v8::Task> task) override {
      UNREACHABLE();
    }

    void PostDelayedTask(std::unique_ptr<Task> task,
                         double delay_in_seconds) override {
      UNREACHABLE();
    }

    void PostIdleTask(std::unique_ptr<IdleTask> task) override {
      DCHECK(IdleTasksEnabled());
      base::MutexGuard lock(&platform_->idle_task_mutex_);
      ASSERT_TRUE(platform_->idle_task_ == nullptr);
      platform_->idle_task_ = std::move(task);
    }

    bool IdleTasksEnabled() override { return true; }

    bool NonNestableTasksEnabled() const override { return false; }

   private:
    MockPlatform* platform_;
  };

  double time_;
  double time_step_;

  // The posted JobTask.
  DeferredPostJob deferred_post_job_;

  // The posted idle task.
  std::unique_ptr<IdleTask> idle_task_;

  // Protects idle_task_.
  base::Mutex idle_task_mutex_;

  v8::TracingController* tracing_controller_;
};

}  // namespace

TEST_F(LazyCompilerDispatcherTest, Construct) {
  MockPlatform platform;
  LazyCompileDispatcher dispatcher(i_isolate(), &platform, FLAG_stack_size);
  dispatcher.AbortAll();
}

TEST_F(LazyCompilerDispatcherTest, IsEnqueued) {
  MockPlatform platform;
  LazyCompileDispatcher dispatcher(i_isolate(), &platform, FLAG_stack_size);

  Handle<SharedFunctionInfo> shared =
      test::CreateSharedFunctionInfo(i_isolate(), nullptr);
  ASSERT_FALSE(shared->is_compiled());
  ASSERT_FALSE(dispatcher.IsEnqueued(shared));

  base::Optional<LazyCompileDispatcher::JobId> job_id =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared);

  ASSERT_TRUE(job_id);
  ASSERT_TRUE(dispatcher.IsEnqueued(*job_id));
  ASSERT_FALSE(dispatcher.IsEnqueued(shared));  // SFI not yet registered.

  dispatcher.RegisterSharedFunctionInfo(*job_id, *shared);
  ASSERT_TRUE(dispatcher.IsEnqueued(*job_id));
  ASSERT_TRUE(dispatcher.IsEnqueued(shared));

  dispatcher.AbortAll();
  ASSERT_FALSE(dispatcher.IsEnqueued(*job_id));
  ASSERT_FALSE(dispatcher.IsEnqueued(shared));

  ASSERT_FALSE(platform.IdleTaskPending());
  ASSERT_TRUE(platform.JobTaskPending());
}

TEST_F(LazyCompilerDispatcherTest, FinishNow) {
  MockPlatform platform;
  LazyCompileDispatcher dispatcher(i_isolate(), &platform, FLAG_stack_size);

  Handle<SharedFunctionInfo> shared =
      test::CreateSharedFunctionInfo(i_isolate(), nullptr);
  ASSERT_FALSE(shared->is_compiled());

  base::Optional<LazyCompileDispatcher::JobId> job_id =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared);
  dispatcher.RegisterSharedFunctionInfo(*job_id, *shared);

  ASSERT_TRUE(dispatcher.FinishNow(shared));
  // Finishing removes the SFI from the queue.
  ASSERT_FALSE(dispatcher.IsEnqueued(*job_id));
  ASSERT_FALSE(dispatcher.IsEnqueued(shared));
  ASSERT_TRUE(shared->is_compiled());

  ASSERT_FALSE(platform.IdleTaskPending());
  dispatcher.AbortAll();
}

TEST_F(LazyCompilerDispatcherTest, CompileAndFinalize) {
  MockPlatform platform;
  LazyCompileDispatcher dispatcher(i_isolate(), &platform, FLAG_stack_size);

  Handle<SharedFunctionInfo> shared =
      test::CreateSharedFunctionInfo(i_isolate(), nullptr);
  ASSERT_FALSE(shared->is_compiled());
  ASSERT_FALSE(platform.IdleTaskPending());

  base::Optional<LazyCompileDispatcher::JobId> job_id =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared);
  ASSERT_TRUE(platform.JobTaskPending());

  // Run compile steps.
  platform.RunJobTasksAndBlock(V8::GetCurrentPlatform());

  // Since we haven't yet registered the SFI for the job, it should still be
  // enqueued and waiting.
  ASSERT_TRUE(dispatcher.IsEnqueued(*job_id));
  ASSERT_FALSE(shared->is_compiled());
  ASSERT_FALSE(platform.IdleTaskPending());

  // Register SFI, which should schedule another idle task to finalize the
  // compilation.
  dispatcher.RegisterSharedFunctionInfo(*job_id, *shared);
  ASSERT_TRUE(platform.IdleTaskPending());
  platform.RunIdleTask(1000.0, 0.0);

  ASSERT_FALSE(dispatcher.IsEnqueued(shared));
  ASSERT_TRUE(shared->is_compiled());
  ASSERT_FALSE(platform.JobTaskPending());
  ASSERT_FALSE(platform.IdleTaskPending());
  dispatcher.AbortAll();
}

TEST_F(LazyCompilerDispatcherTest, IdleTaskNoIdleTime) {
  MockPlatform platform;
  LazyCompileDispatcher dispatcher(i_isolate(), &platform, FLAG_stack_size);

  Handle<SharedFunctionInfo> shared =
      test::CreateSharedFunctionInfo(i_isolate(), nullptr);
  ASSERT_FALSE(shared->is_compiled());
  ASSERT_FALSE(platform.IdleTaskPending());

  base::Optional<LazyCompileDispatcher::JobId> job_id =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared);
  dispatcher.RegisterSharedFunctionInfo(*job_id, *shared);

  // Run compile steps.
  platform.RunJobTasksAndBlock(V8::GetCurrentPlatform());

  // Job should be ready to finalize.
  ASSERT_EQ(dispatcher.jobs_.size(), 1u);
  ASSERT_TRUE(dispatcher.jobs_.begin()->second->has_run);
  ASSERT_TRUE(platform.IdleTaskPending());

  // Grant no idle time and have time advance beyond it in one step.
  platform.RunIdleTask(0.0, 1.0);

  ASSERT_TRUE(dispatcher.IsEnqueued(shared));
  ASSERT_FALSE(shared->is_compiled());
  ASSERT_TRUE(platform.IdleTaskPending());

  // Job should be ready to finalize.
  ASSERT_EQ(dispatcher.jobs_.size(), 1u);
  ASSERT_TRUE(dispatcher.jobs_.begin()->second->has_run);

  // Now grant a lot of idle time and freeze time.
  platform.RunIdleTask(1000.0, 0.0);

  ASSERT_FALSE(dispatcher.IsEnqueued(shared));
  ASSERT_TRUE(shared->is_compiled());
  ASSERT_FALSE(platform.IdleTaskPending());
  ASSERT_FALSE(platform.JobTaskPending());
  dispatcher.AbortAll();
}

TEST_F(LazyCompilerDispatcherTest, IdleTaskSmallIdleTime) {
  MockPlatform platform;
  LazyCompileDispatcher dispatcher(i_isolate(), &platform, FLAG_stack_size);

  Handle<SharedFunctionInfo> shared_1 =
      test::CreateSharedFunctionInfo(i_isolate(), nullptr);
  ASSERT_FALSE(shared_1->is_compiled());
  Handle<SharedFunctionInfo> shared_2 =
      test::CreateSharedFunctionInfo(i_isolate(), nullptr);
  ASSERT_FALSE(shared_2->is_compiled());

  base::Optional<LazyCompileDispatcher::JobId> job_id_1 =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared_1);
  base::Optional<LazyCompileDispatcher::JobId> job_id_2 =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared_2);

  dispatcher.RegisterSharedFunctionInfo(*job_id_1, *shared_1);
  dispatcher.RegisterSharedFunctionInfo(*job_id_2, *shared_2);

  // Run compile steps.
  platform.RunJobTasksAndBlock(V8::GetCurrentPlatform());

  // Both jobs should be ready to finalize.
  ASSERT_EQ(dispatcher.jobs_.size(), 2u);
  ASSERT_TRUE(dispatcher.jobs_.begin()->second->has_run);
  ASSERT_TRUE((++dispatcher.jobs_.begin())->second->has_run);
  ASSERT_TRUE(platform.IdleTaskPending());

  // Grant a small anount of idle time and have time advance beyond it in one
  // step.
  platform.RunIdleTask(2.0, 1.0);

  // Only one of the jobs should be finalized.
  ASSERT_EQ(dispatcher.jobs_.size(), 1u);
  ASSERT_TRUE(dispatcher.jobs_.begin()->second->has_run);
  ASSERT_NE(dispatcher.IsEnqueued(shared_1), dispatcher.IsEnqueued(shared_2));
  ASSERT_NE(shared_1->is_compiled(), shared_2->is_compiled());
  ASSERT_TRUE(platform.IdleTaskPending());

  // Now grant a lot of idle time and freeze time.
  platform.RunIdleTask(1000.0, 0.0);

  ASSERT_FALSE(dispatcher.IsEnqueued(shared_1) ||
               dispatcher.IsEnqueued(shared_2));
  ASSERT_TRUE(shared_1->is_compiled() && shared_2->is_compiled());
  ASSERT_FALSE(platform.IdleTaskPending());
  ASSERT_FALSE(platform.JobTaskPending());
  dispatcher.AbortAll();
}

TEST_F(LazyCompilerDispatcherTest, IdleTaskException) {
  MockPlatform platform;
  LazyCompileDispatcher dispatcher(i_isolate(), &platform, 50);

  std::string raw_script("(x) { var a = ");
  for (int i = 0; i < 1000; i++) {
    // Alternate + and - to avoid n-ary operation nodes.
    raw_script += "'x' + 'x' - ";
  }
  raw_script += " 'x'; };";
  test::ScriptResource* script =
      new test::ScriptResource(raw_script.c_str(), strlen(raw_script.c_str()));
  Handle<SharedFunctionInfo> shared =
      test::CreateSharedFunctionInfo(i_isolate(), script);
  ASSERT_FALSE(shared->is_compiled());

  base::Optional<LazyCompileDispatcher::JobId> job_id =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared);
  dispatcher.RegisterSharedFunctionInfo(*job_id, *shared);

  // Run compile steps and finalize.
  platform.RunJobTasksAndBlock(V8::GetCurrentPlatform());
  platform.RunIdleTask(1000.0, 0.0);

  ASSERT_FALSE(dispatcher.IsEnqueued(shared));
  ASSERT_FALSE(shared->is_compiled());
  ASSERT_FALSE(i_isolate()->has_pending_exception());
  dispatcher.AbortAll();
}

TEST_F(LazyCompilerDispatcherTest, FinishNowWithWorkerTask) {
  MockPlatform platform;
  LazyCompileDispatcher dispatcher(i_isolate(), &platform, FLAG_stack_size);

  Handle<SharedFunctionInfo> shared =
      test::CreateSharedFunctionInfo(i_isolate(), nullptr);
  ASSERT_FALSE(shared->is_compiled());

  base::Optional<LazyCompileDispatcher::JobId> job_id =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared);
  dispatcher.RegisterSharedFunctionInfo(*job_id, *shared);

  ASSERT_EQ(dispatcher.jobs_.size(), 1u);
  ASSERT_FALSE(dispatcher.jobs_.begin()->second->has_run);

  ASSERT_TRUE(dispatcher.IsEnqueued(shared));
  ASSERT_FALSE(shared->is_compiled());
  ASSERT_EQ(dispatcher.jobs_.size(), 1u);
  ASSERT_FALSE(dispatcher.jobs_.begin()->second->has_run);
  ASSERT_TRUE(platform.JobTaskPending());

  // This does not block, but races with the FinishNow() call below.
  platform.RunJobTasks(V8::GetCurrentPlatform());

  ASSERT_TRUE(dispatcher.FinishNow(shared));
  // Finishing removes the SFI from the queue.
  ASSERT_FALSE(dispatcher.IsEnqueued(shared));
  ASSERT_TRUE(shared->is_compiled());
  if (platform.IdleTaskPending()) platform.ClearIdleTask();
  ASSERT_FALSE(platform.JobTaskPending());
  dispatcher.AbortAll();
}

TEST_F(LazyCompilerDispatcherTest, IdleTaskMultipleJobs) {
  MockPlatform platform;
  LazyCompileDispatcher dispatcher(i_isolate(), &platform, FLAG_stack_size);

  Handle<SharedFunctionInfo> shared_1 =
      test::CreateSharedFunctionInfo(i_isolate(), nullptr);
  ASSERT_FALSE(shared_1->is_compiled());
  Handle<SharedFunctionInfo> shared_2 =
      test::CreateSharedFunctionInfo(i_isolate(), nullptr);
  ASSERT_FALSE(shared_2->is_compiled());

  base::Optional<LazyCompileDispatcher::JobId> job_id_1 =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared_1);
  base::Optional<LazyCompileDispatcher::JobId> job_id_2 =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared_2);

  dispatcher.RegisterSharedFunctionInfo(*job_id_1, *shared_1);
  dispatcher.RegisterSharedFunctionInfo(*job_id_2, *shared_2);

  ASSERT_TRUE(dispatcher.IsEnqueued(shared_1));
  ASSERT_TRUE(dispatcher.IsEnqueued(shared_2));

  // Run compile steps and finalize.
  platform.RunJobTasksAndBlock(V8::GetCurrentPlatform());
  platform.RunIdleTask(1000.0, 0.0);

  ASSERT_FALSE(dispatcher.IsEnqueued(shared_1));
  ASSERT_FALSE(dispatcher.IsEnqueued(shared_2));
  ASSERT_TRUE(shared_1->is_compiled());
  ASSERT_TRUE(shared_2->is_compiled());
  ASSERT_FALSE(platform.IdleTaskPending());
  ASSERT_FALSE(platform.JobTaskPending());
  dispatcher.AbortAll();
}

TEST_F(LazyCompilerDispatcherTest, FinishNowException) {
  MockPlatform platform;
  LazyCompileDispatcher dispatcher(i_isolate(), &platform, 50);

  std::string raw_script("(x) { var a = ");
  for (int i = 0; i < 1000; i++) {
    // Alternate + and - to avoid n-ary operation nodes.
    raw_script += "'x' + 'x' - ";
  }
  raw_script += " 'x'; };";
  test::ScriptResource* script =
      new test::ScriptResource(raw_script.c_str(), strlen(raw_script.c_str()));
  Handle<SharedFunctionInfo> shared =
      test::CreateSharedFunctionInfo(i_isolate(), script);
  ASSERT_FALSE(shared->is_compiled());

  base::Optional<LazyCompileDispatcher::JobId> job_id =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared);
  dispatcher.RegisterSharedFunctionInfo(*job_id, *shared);

  ASSERT_FALSE(dispatcher.FinishNow(shared));

  ASSERT_FALSE(dispatcher.IsEnqueued(shared));
  ASSERT_FALSE(shared->is_compiled());
  ASSERT_TRUE(i_isolate()->has_pending_exception());

  i_isolate()->clear_pending_exception();
  ASSERT_FALSE(platform.IdleTaskPending());
  dispatcher.AbortAll();
}

TEST_F(LazyCompilerDispatcherTest, AbortJobNotStarted) {
  MockPlatform platform;
  LazyCompileDispatcher dispatcher(i_isolate(), &platform, FLAG_stack_size);

  Handle<SharedFunctionInfo> shared =
      test::CreateSharedFunctionInfo(i_isolate(), nullptr);
  ASSERT_FALSE(shared->is_compiled());

  base::Optional<LazyCompileDispatcher::JobId> job_id =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared);

  ASSERT_EQ(dispatcher.jobs_.size(), 1u);
  ASSERT_FALSE(dispatcher.jobs_.begin()->second->has_run);

  ASSERT_TRUE(dispatcher.IsEnqueued(*job_id));
  ASSERT_FALSE(shared->is_compiled());
  ASSERT_EQ(dispatcher.jobs_.size(), 1u);
  ASSERT_FALSE(dispatcher.jobs_.begin()->second->has_run);
  ASSERT_TRUE(platform.JobTaskPending());

  dispatcher.AbortJob(*job_id);

  // Aborting removes the job from the queue.
  ASSERT_FALSE(dispatcher.IsEnqueued(*job_id));
  ASSERT_FALSE(shared->is_compiled());
  ASSERT_FALSE(platform.IdleTaskPending());
  dispatcher.AbortAll();
}

TEST_F(LazyCompilerDispatcherTest, AbortJobAlreadyStarted) {
  MockPlatform platform;
  LazyCompileDispatcher dispatcher(i_isolate(), &platform, FLAG_stack_size);

  Handle<SharedFunctionInfo> shared =
      test::CreateSharedFunctionInfo(i_isolate(), nullptr);
  ASSERT_FALSE(shared->is_compiled());

  base::Optional<LazyCompileDispatcher::JobId> job_id =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared);

  ASSERT_EQ(dispatcher.jobs_.size(), 1u);
  ASSERT_FALSE(dispatcher.jobs_.begin()->second->has_run);

  ASSERT_TRUE(dispatcher.IsEnqueued(*job_id));
  ASSERT_FALSE(shared->is_compiled());
  ASSERT_EQ(dispatcher.jobs_.size(), 1u);
  ASSERT_FALSE(dispatcher.jobs_.begin()->second->has_run);
  ASSERT_TRUE(platform.JobTaskPending());

  // Have dispatcher block on the background thread when running the job.
  {
    base::LockGuard<base::Mutex> lock(&dispatcher.mutex_);
    dispatcher.block_for_testing_.SetValue(true);
  }

  // Start background thread and wait until it is about to run the job.
  platform.RunJobTasks(V8::GetCurrentPlatform());
  while (dispatcher.block_for_testing_.Value()) {
  }

  // Now abort while dispatcher is in the middle of running the job.
  dispatcher.AbortJob(*job_id);

  // Unblock background thread, and wait for job to complete.
  {
    base::LockGuard<base::Mutex> lock(&dispatcher.mutex_);
    dispatcher.main_thread_blocking_on_job_ =
        dispatcher.jobs_.begin()->second.get();
    dispatcher.semaphore_for_testing_.Signal();
    while (dispatcher.main_thread_blocking_on_job_ != nullptr) {
      dispatcher.main_thread_blocking_signal_.Wait(&dispatcher.mutex_);
    }
  }

  // Job should have finished running and then been aborted.
  ASSERT_TRUE(dispatcher.IsEnqueued(*job_id));
  ASSERT_FALSE(shared->is_compiled());
  ASSERT_EQ(dispatcher.jobs_.size(), 1u);
  ASSERT_TRUE(dispatcher.jobs_.begin()->second->has_run);
  ASSERT_TRUE(dispatcher.jobs_.begin()->second->aborted);
  ASSERT_FALSE(platform.JobTaskPending());
  ASSERT_TRUE(platform.IdleTaskPending());

  // Runt the pending idle task
  platform.RunIdleTask(1000.0, 0.0);

  // Aborting removes the SFI from the queue.
  ASSERT_FALSE(dispatcher.IsEnqueued(*job_id));
  ASSERT_FALSE(shared->is_compiled());
  ASSERT_FALSE(platform.IdleTaskPending());
  ASSERT_FALSE(platform.JobTaskPending());
  dispatcher.AbortAll();
}

TEST_F(LazyCompilerDispatcherTest, CompileLazyFinishesDispatcherJob) {
  // Use the real dispatcher so that CompileLazy checks the same one for
  // enqueued functions.
  LazyCompileDispatcher* dispatcher = i_isolate()->lazy_compile_dispatcher();

  const char raw_script[] = "function lazy() { return 42; }; lazy;";
  test::ScriptResource* script =
      new test::ScriptResource(raw_script, strlen(raw_script));
  Handle<JSFunction> f = RunJS<JSFunction>(script);
  Handle<SharedFunctionInfo> shared(f->shared(), i_isolate());
  ASSERT_FALSE(shared->is_compiled());

  base::Optional<LazyCompileDispatcher::JobId> job_id =
      EnqueueUnoptimizedCompileJob(dispatcher, i_isolate(), shared);
  dispatcher->RegisterSharedFunctionInfo(*job_id, *shared);

  // Now force the function to run and ensure CompileLazy finished and dequeues
  // it from the dispatcher.
  RunJS("lazy();");
  ASSERT_TRUE(shared->is_compiled());
  ASSERT_FALSE(dispatcher->IsEnqueued(shared));
}

TEST_F(LazyCompilerDispatcherTest, CompileLazy2FinishesDispatcherJob) {
  // Use the real dispatcher so that CompileLazy checks the same one for
  // enqueued functions.
  LazyCompileDispatcher* dispatcher = i_isolate()->lazy_compile_dispatcher();

  const char raw_source_2[] = "function lazy2() { return 42; }; lazy2;";
  test::ScriptResource* source_2 =
      new test::ScriptResource(raw_source_2, strlen(raw_source_2));
  Handle<JSFunction> lazy2 = RunJS<JSFunction>(source_2);
  Handle<SharedFunctionInfo> shared_2(lazy2->shared(), i_isolate());
  ASSERT_FALSE(shared_2->is_compiled());

  const char raw_source_1[] = "function lazy1() { return lazy2(); }; lazy1;";
  test::ScriptResource* source_1 =
      new test::ScriptResource(raw_source_1, strlen(raw_source_1));
  Handle<JSFunction> lazy1 = RunJS<JSFunction>(source_1);
  Handle<SharedFunctionInfo> shared_1(lazy1->shared(), i_isolate());
  ASSERT_FALSE(shared_1->is_compiled());

  base::Optional<LazyCompileDispatcher::JobId> job_id_1 =
      EnqueueUnoptimizedCompileJob(dispatcher, i_isolate(), shared_1);
  dispatcher->RegisterSharedFunctionInfo(*job_id_1, *shared_1);

  base::Optional<LazyCompileDispatcher::JobId> job_id_2 =
      EnqueueUnoptimizedCompileJob(dispatcher, i_isolate(), shared_2);
  dispatcher->RegisterSharedFunctionInfo(*job_id_2, *shared_2);

  ASSERT_TRUE(dispatcher->IsEnqueued(shared_1));
  ASSERT_TRUE(dispatcher->IsEnqueued(shared_2));

  RunJS("lazy1();");
  ASSERT_TRUE(shared_1->is_compiled());
  ASSERT_TRUE(shared_2->is_compiled());
  ASSERT_FALSE(dispatcher->IsEnqueued(shared_1));
  ASSERT_FALSE(dispatcher->IsEnqueued(shared_2));
}

TEST_F(LazyCompilerDispatcherTest, CompileMultipleOnBackgroundThread) {
  MockPlatform platform;
  LazyCompileDispatcher dispatcher(i_isolate(), &platform, FLAG_stack_size);

  Handle<SharedFunctionInfo> shared_1 =
      test::CreateSharedFunctionInfo(i_isolate(), nullptr);
  ASSERT_FALSE(shared_1->is_compiled());

  Handle<SharedFunctionInfo> shared_2 =
      test::CreateSharedFunctionInfo(i_isolate(), nullptr);
  ASSERT_FALSE(shared_2->is_compiled());

  base::Optional<LazyCompileDispatcher::JobId> job_id_1 =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared_1);
  dispatcher.RegisterSharedFunctionInfo(*job_id_1, *shared_1);

  base::Optional<LazyCompileDispatcher::JobId> job_id_2 =
      EnqueueUnoptimizedCompileJob(&dispatcher, i_isolate(), shared_2);
  dispatcher.RegisterSharedFunctionInfo(*job_id_2, *shared_2);

  ASSERT_EQ(dispatcher.jobs_.size(), 2u);
  ASSERT_FALSE(dispatcher.jobs_.begin()->second->has_run);
  ASSERT_FALSE((++dispatcher.jobs_.begin())->second->has_run);

  ASSERT_TRUE(dispatcher.IsEnqueued(shared_1));
  ASSERT_TRUE(dispatcher.IsEnqueued(shared_2));
  ASSERT_FALSE(shared_1->is_compiled());
  ASSERT_FALSE(shared_2->is_compiled());
  ASSERT_FALSE(platform.IdleTaskPending());
  ASSERT_TRUE(platform.JobTaskPending());

  platform.RunJobTasksAndBlock(V8::GetCurrentPlatform());

  ASSERT_TRUE(platform.IdleTaskPending());
  ASSERT_FALSE(platform.JobTaskPending());
  ASSERT_EQ(dispatcher.jobs_.size(), 2u);
  ASSERT_TRUE(dispatcher.jobs_.begin()->second->has_run);
  ASSERT_TRUE((++dispatcher.jobs_.begin())->second->has_run);

  // Now grant a lot of idle time and freeze time.
  platform.RunIdleTask(1000.0, 0.0);

  ASSERT_FALSE(dispatcher.IsEnqueued(shared_1));
  ASSERT_FALSE(dispatcher.IsEnqueued(shared_2));
  ASSERT_TRUE(shared_1->is_compiled());
  ASSERT_TRUE(shared_2->is_compiled());
  ASSERT_FALSE(platform.IdleTaskPending());
  dispatcher.AbortAll();
}

}  // namespace internal
}  // namespace v8
