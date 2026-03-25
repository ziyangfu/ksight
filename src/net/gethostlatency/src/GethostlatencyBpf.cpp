#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <linux/limits.h>
#include <sys/time.h>
#include <unistd.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "GethostlatencyBpf.h"
#include "fmt/format.h"
#include "spdlog/spdlog.h"

extern "C" {
#include "uprobe_helpers.h"
}

extern "C" {
#include "gethostlatency.skel.h"
}

using namespace net::getHostLatency;

static volatile sig_atomic_t g_exiting = 0;

static void sig_handler(int signo) { g_exiting = 1; }

GethostlatencyBpf::GethostlatencyBpf(const ConfigArgs &config)
    : config_(config) {}

GethostlatencyBpf::~GethostlatencyBpf() { destroy(); }

void GethostlatencyBpf::destroy() {
  if (pb_) {
    perf_buffer__free(pb_);
    pb_ = nullptr;
  }
  for (auto link : links_) {
    bpf_link__destroy(link);
  }
  links_.clear();
  if (skel_) {
    gethostlatency_bpf__destroy(skel_);
    skel_ = nullptr;
  }
}

void GethostlatencyBpf::init() {
  skel_ = gethostlatency_bpf__open();
  if (!skel_) {
    throw std::runtime_error("Failed to open BPF skeleton");
  }
}

void GethostlatencyBpf::load() {
  skel_->rodata->target_pid = config_.target_pid;

  int err = gethostlatency_bpf__load(skel_);
  if (err) {
    destroy();
    throw std::runtime_error(
        fmt::format("Failed to load BPF skeleton: {}", err));
  }
}

int GethostlatencyBpf::getLibcPath(char *path, size_t path_sz) {
  if (!config_.libc_path.empty()) {
    strncpy(path, config_.libc_path.c_str(), path_sz);
    return 0;
  }

  char buf[PATH_MAX] = {};
  FILE *f;

  if (config_.target_pid == 0) {
    f = fopen("/proc/self/maps", "r");
  } else {
    snprintf(buf, sizeof(buf), "/proc/%d/maps", config_.target_pid);
    f = fopen(buf, "r");
  }
  if (!f)
    return -errno;

  while (fscanf(f, "%*x-%*x %*s %*s %*s %*s %[^\n]\n", buf) != EOF) {
    if (strchr(buf, '/') != buf)
      continue;
    char *filename = strrchr(buf, '/') + 1;
    float version;
    if (sscanf(filename, "libc-%f.so", &version) == 1 ||
        sscanf(filename, "libc.so.%f", &version) == 1 ||
        strcmp(filename, "libc.so.6") == 0) {
      if (config_.target_pid == 0) {
        strncpy(path, buf, path_sz);
      } else {
        snprintf(path, path_sz, "/proc/%d/root%s", config_.target_pid, buf);
      }
      fclose(f);
      return 0;
    }
  }

  fclose(f);
  return -1;
}

void GethostlatencyBpf::attachUprobes() {
  char libc_path[PATH_MAX] = {};
  if (getLibcPath(libc_path, sizeof(libc_path)) != 0) {
    throw std::runtime_error("Could not find libc.so");
  }

  const char *funcs[] = {"getaddrinfo", "gethostbyname", "gethostbyname2"};
  for (const char *func : funcs) {
    off_t func_off = get_elf_func_offset(libc_path, func);
    if (func_off < 0) {
      SPDLOG_WARN("Could not find {} in {}, skipping", func, libc_path);
      continue;
    }

    struct bpf_link *link_entry = bpf_program__attach_uprobe(
        skel_->progs.handle_entry, false, config_.target_pid ?: -1, libc_path,
        func_off);
    if (!link_entry) {
      throw std::runtime_error(
          fmt::format("Failed to attach {} entry uprobe", func));
    }
    links_.push_back(link_entry);

    struct bpf_link *link_ret = bpf_program__attach_uprobe(
        skel_->progs.handle_return, true, config_.target_pid ?: -1, libc_path,
        func_off);
    if (!link_ret) {
      throw std::runtime_error(
          fmt::format("Failed to attach {} return uprobe", func));
    }
    links_.push_back(link_ret);
  }
}

void GethostlatencyBpf::attach() {
  attachUprobes();
  // Note: gethostlatency doesn't use auto-attach for uprobes because we need to
  // find libc
}

void GethostlatencyBpf::handleEvent(void *ctx, int cpu, void *data,
                                    unsigned int data_sz) {
  auto self = static_cast<GethostlatencyBpf *>(ctx);
  if (data_sz < sizeof(struct event))
    return;
  auto e = static_cast<const struct event *>(data);
  self->processEvent(e);
}

void GethostlatencyBpf::handleLostEvents(void *ctx, int cpu,
                                         unsigned long long lost_cnt) {
  SPDLOG_WARN("lost {} events on CPU #{}", lost_cnt, cpu);
}

static std::string get_current_time_str() {
  time_t now = time(nullptr);
  struct tm ts;
  localtime_r(&now, &ts);
  char buf[32];
  strftime(buf, sizeof(buf), "%H:%M:%S", &ts);
  return std::string(buf);
}

void GethostlatencyBpf::processEvent(const struct event *e) {
  fmt::print("{:<8} {:<7} {:<16} {:<10.3f} {:<s}\n", get_current_time_str(),
             e->pid, e->comm, (double)e->time / 1000000.0, e->host);
}

void GethostlatencyBpf::poll() {
  pb_ = perf_buffer__new(bpf_map__fd(skel_->maps.events), 16, handleEvent,
                         handleLostEvents, this, nullptr);
  if (!pb_) {
    throw std::runtime_error(
        fmt::format("Failed to open perf buffer: {}", -errno));
  }

  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  fmt::print("{:<8} {:<7} {:<16} {:<10} {:<s}\n", "TIME", "PID", "COMM",
             "LATms", "HOST");

  while (!g_exiting) {
    int err = perf_buffer__poll(pb_, 100);
    if (err < 0 && err != -EINTR) {
      throw std::runtime_error(
          fmt::format("Error polling perf buffer: {}", err));
    }
  }
}
