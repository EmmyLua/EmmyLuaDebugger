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
#include "uv.h"
#include "emmy_debugger/transporter.h"

class SocketServerTransporter : public Transporter {
	uv_tcp_t uvServer;
	uv_stream_t* uvClient;
public:
	SocketServerTransporter();
	~SocketServerTransporter();
	void OnNewConnection(uv_stream_t* server, int status);
	bool Listen(const std::string& host, int port, std::string& err);
	void Send(const char* data, size_t len);
private:
	int Stop() override;
	void Send(int cmd, const char* data, size_t len) override;
	void OnDisconnect() override;
};
