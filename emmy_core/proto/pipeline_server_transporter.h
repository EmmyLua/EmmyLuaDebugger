
#pragma once

#include <thread>
#include "uv.h"
#include "../transporter.h"

class PipelineServerTransporter : public Transporter {
    uv_pipe_t uvServer;
    uv_pipe_t* uvClient;
public:
    PipelineServerTransporter();
    ~PipelineServerTransporter();
    
    bool pipe(const std::string& name);
    int Stop() override;
    void Send(int cmd, const char* data, size_t len) override;
    void OnPipeConnection(uv_stream_t* pipe, int status);
};
