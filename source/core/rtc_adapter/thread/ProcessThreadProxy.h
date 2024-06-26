// Copyright (C) <2020> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef RTC_ADAPTER_THREAD_PROCESS_THREAD_PROXY_
#define RTC_ADAPTER_THREAD_PROCESS_THREAD_PROXY_

#include <modules/utility/include/process_thread.h>
#include <rtc_base/checks.h>
#include <unordered_set>

namespace rtc_adapter {

// ProcessThreadProxy holds a pointer to actual ProcessThread
class ProcessThreadProxy : public webrtc::ProcessThread {
public:
    ProcessThreadProxy(webrtc::ProcessThread* processThread)
        : m_processThread(processThread)
    {
    }

    // Implements ProcessThread
    virtual void Start() override { }

    // Implements ProcessThread
    // Stop() has no effect on proxy
    virtual void Stop() override { }

    // Implements webrtc::TaskQueueBase
    virtual void Delete() override { }

    // Implements ProcessThread
    // Call actual ProcessThread's WakeUp
    virtual void WakeUp(webrtc::Module* module) override
    {
        if (m_processThread) {
            m_processThread->WakeUp(module);
        }
    }

    // Implements webrtc::TaskQueueBase
    // Call actual ProcessThread's PostTask
    virtual void PostTask(std::unique_ptr<webrtc::QueuedTask> task) override
    {
        if (m_processThread) {
            m_processThread->PostTask(std::move(task));
        }
    }

    // Implements webrtc::TaskQueueBase
    // Call actual ProcessThread's PostDelayedTask
    virtual void PostDelayedTask(std::unique_ptr<webrtc::QueuedTask> task,
        uint32_t milliseconds) override
    {
        if (m_processThread) {
            m_processThread->PostDelayedTask(std::move(task), milliseconds);
        }
    }

    // Implements ProcessThread
    // Call actual ProcessThread's RegisterModule
    virtual void RegisterModule(webrtc::Module* module, const rtc::Location& from) override
    {
        if (m_processThread) {
            m_processThread->RegisterModule(module, from);
        }
    }

    // Implements ProcessThread
    // Call actual ProcessThread's DeRegisterModule
    virtual void DeRegisterModule(webrtc::Module* module) override
    {
        if (m_processThread) {
            m_processThread->DeRegisterModule(module);
        }
    }

private:
    webrtc::ProcessThread* m_processThread;
    // std::unordered_set<webrtc::Module*> m_modules;
};

} // namespace rtc_adapter

#endif
