#include "MihalMonitor.h"

#include <error.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/sysinfo.h>

#include "AppSnapshotController.h"
#include "LogUtil.h"

namespace mihal {

// Threshold for switching b2y by memory psi monitor value
#define PSI_STRESS_NUM 50

PsiMemMonitor *PsiMemMonitor::getInstance()
{
    static PsiMemMonitor monitor;
    return &monitor;
}

PsiMemMonitor::PsiMemMonitor()
    : mPsiMonitorStart{false}, mThreadExit{false}, mIsHighMem{false}, mSnapShotCounter{0}
{
    // Note：Only run in the 8G phone
    if (isLowRamDev()) {
        mPsiMonitorThread = std::thread([this]() { psiMonitorLoop(); });
        MLOGI(LogGroupHAL, "[Monitor][PSI] Monitor thread start");
    } else {
        mIsHighMem = true;
        MLOGI(LogGroupHAL, "[Monitor][PSI] not 8G phone , bypass PsiMemMonitor");
    }
}

PsiMemMonitor::~PsiMemMonitor()
{
    if (!mIsHighMem) {
        {
            std::lock_guard<std::mutex> lg(mPsiMonitorLock);
            mThreadExit = true;
            MLOGI(LogGroupHAL, "[Monitor][PSI] Monitor thread exit");
        }
        mPsiMonitorCond.notify_all();
        if (mPsiMonitorThread.joinable()) {
            mPsiMonitorThread.join();
        }
    }
}

unsigned long PsiMemMonitor::GetTotalRAM()
{
    unsigned long sysTotalRam;
    struct sysinfo s_Info;
    if (sysinfo(&s_Info) == 0) {
        sysTotalRam = s_Info.totalram;
        MLOGI(LogGroupHAL, "[Monitor][RAM] sysTotalRam=%ld", sysTotalRam);
    } else {
        sysTotalRam = (unsigned long)12 * 1024 * 1024 * 1024;
        MLOGI(LogGroupHAL, "[Monitor][RAM] can't get sysTotalRam, set to 12G. %s", strerror(errno));
    }
    return sysTotalRam;
}

bool PsiMemMonitor::isPsiMemFull()
{
    if (mSnapShotCounter > PSI_STRESS_NUM && mMemPressureLevel > 10 && (!mIsHighMem)) {
        MLOGI(LogGroupHAL, "[Monitor][RAM]mMemPressureLevel=%d , Psi memory is full",
              mMemPressureLevel);
        mPsiMemFull = true;
    } else {
        mPsiMemFull = false;
    }
    return mPsiMemFull;
}

void PsiMemMonitor::psiMonitorTrigger()
{
    std::lock_guard<std::mutex> lg(mPsiMonitorLock);
    if (!mIsHighMem) {
        mPsiMonitorStart = true;
        mPsiMonitorCond.notify_all();
        MLOGI(LogGroupHAL, "[Monitor][PSI] 8G phone , Notify Monitor thread ");
    }
    return;
}

void PsiMemMonitor::psiMonitorLoop()
{
    static uint32_t stallTime =
        property_get_int32("persist.vendor.camera.psiMonitor.stall", 150000);
    static uint32_t monitorInterval =
        property_get_int32("persist.vendor.camera.psiMonitor.interval", 1000000);
    static uint32_t monitorTimeout =
        property_get_int32("persist.vendor.camera.psiMonitor.timeout", 5000);

    // make mMemPressureLevel is 0 whenever this function returns.
    std::shared_ptr<void> execAtReturn(nullptr, [&](auto) {
        mMemPressureLevel = 0;
        MLOGI(LogGroupHAL, "[Monitor][PSI] Monitor thread exit");
    });

    std::string trig = "some ";
    trig += std::to_string(stallTime) + " " + std::to_string(monitorInterval);
    struct pollfd fds;
    int n;
    fds.fd = open("/proc/pressure/memory", O_RDWR | O_NONBLOCK);
    if (fds.fd < 0) {
        MLOGI(LogGroupHAL, "[Monitor][PSI] /proc/pressure/memory open error: %s", strerror(errno));
        return;
    }
    fds.events = POLLPRI;

    if (write(fds.fd, trig.c_str(), trig.size() + 1) < 0) {
        MLOGI(LogGroupHAL, "[Monitor][PSI] /proc/pressure/memory write error: %s", strerror(errno));
        return;
    }

    trig += " (mem)";
    MLOGI(LogGroupHAL, "[Monitor][PSI] waiting for events: %s", trig.c_str());
    while (true) {
        {
            std::unique_lock<std::mutex> lk(mPsiMonitorLock);
            if (mPsiMonitorStart == false) {
                mMemPressureLevel = 0;
            }
            mPsiMonitorCond.wait(lk,
                                 [&] { return mPsiMonitorStart == true || mThreadExit == true; });
        }

        if (mThreadExit) {
            MLOGI(LogGroupHAL, "[Monitor][PSI] Monitor thread break and exit");
            break;
        }
        n = poll(&fds, 1, monitorTimeout);
        if (n < 0) {
            MLOGI(LogGroupHAL, "[Monitor][PSI] poll error: %s", strerror(errno));
        } else if (n == 0) {
            MLOGI(LogGroupHAL, "[Monitor][PSI] No event in %dms, %s", monitorTimeout, trig.c_str());
            mMemPressureLevel = 0;
        } else {
            if (fds.revents & POLLERR) {
                MLOGW(LogGroupHAL, "[Monitor][PSI] got POLLERR, event source is gone");
                return;
            }
            if (fds.revents & POLLPRI) {
                // reserve level 1~10 to future usage.
                mMemPressureLevel = 11;
                MLOGI(LogGroupHAL, "[Monitor][PSI] event triggered: %s", trig.c_str());
            } else {
                MLOGW(LogGroupHAL, "[Monitor][PSI] unknown event received: 0x%x", fds.revents);
            }
        }
    }
}

} // namespace mihal
