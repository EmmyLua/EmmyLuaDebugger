/*
* Copyright (c) 2019. tangzx(love.tangzx@qq.com)
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#pragma once
#include <mutex>
#include <condition_variable>
#include "uv.h"
#include "emmy_debugger/transporter.h"

class PipelineClientTransporter : public Transporter {
	uv_pipe_t uvClient;
	std::mutex mutex;
	std::condition_variable cv;
public:
	PipelineClientTransporter();
	~PipelineClientTransporter();

	bool Connect(const std::string& name, std::string& err);
	int Stop() override;
	void Send(int cmd, const char* data, size_t len) override;
	void OnPipeConnection(uv_connect_t* req, int status);
};
