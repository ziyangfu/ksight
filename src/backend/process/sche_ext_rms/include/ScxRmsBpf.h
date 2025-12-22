#ifndef PROCESS_SCHE_EXT_RMS_BPF_H
#define PROCESS_SCHE_EXT_RMS_BPF_H

#include <string>
#include <memory>

extern "C" {
#include "process/sche_ext_rms/scx_rms.skel.h"
}

namespace process::scheExtRms {

class ScxRmsBpf final {
private:
    struct scx_rms *skel_;
    struct bpf_link *link_;

public:
    ScxRmsBpf();
    ~ScxRmsBpf();

    void open();
    void load();
    void attach();
    void destroy();

    bool shouldExit() const;
    void reportExit();

private:
    void pinMaps();
    void unpinMaps();
};

} // namespace process::scheExtRms

#endif // PROCESS_SCHE_EXT_RMS_BPF_H
